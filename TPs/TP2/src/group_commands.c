#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <string.h> 
#include <pwd.h>
#include <grp.h> 

#include "../include/struct.h"
#include "../include/files_struct.h"
#include "../include/message_commands.h"
#include "../include/utils.h"


/**
 * Adiciona um user ao grupo do sistema linux
*/
int add_user_to_system_group(const char *user_to_add, const char *group) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        syslog(LOG_INFO, "Executing command: usermod -aG %s %s", group, user_to_add);

        execlp("usermod", "usermod", "-aG", group, user_to_add, (char *)NULL);

        syslog(LOG_ERR, "execlp failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (waitpid(pid, &status, 0) == -1) {
            syslog(LOG_ERR, "Failed to waitpid: %s", strerror(errno));
            return -1;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                syslog(LOG_NOTICE, "User '%s' successfully added to group '%s'", user_to_add, group);
                return 0;
            } else {
                syslog(LOG_ERR, "usermod command failed with exit status %d", exit_status);
                return -1;
            }
        }
    }
    return 0;
}


int remove_user_from_system_group(const char *user_to_remove, const char *group) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        syslog(LOG_INFO, "Executing command: gpasswd -d %s %s", user_to_remove, group);

        execlp("gpasswd", "gpasswd", "-d", user_to_remove, group, (char *)NULL);
        
        syslog(LOG_ERR, "execlp failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else {
        if (waitpid(pid, &status, 0) == -1) {
            syslog(LOG_ERR, "Failed to waitpid: %s", strerror(errno));
            return -1;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                return 0;
            } else {
                syslog(LOG_ERR, "gpasswd command failed with exit status %d", exit_status);
                return -1;
            }
        }
    }
    return 0;
}


int create_system_group(const char *group) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        syslog(LOG_INFO, "Executing command: groupadd %s", group);

        execlp("groupadd", "groupadd", group, (char *)NULL);
        
        syslog(LOG_ERR, "execlp failed: %s", strerror(errno));
        _exit(EXIT_FAILURE); 
    } else {
        
        if (waitpid(pid, &status, 0) == -1) {
            syslog(LOG_ERR, "Failed to waitpid: %s", strerror(errno));
            return -1;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                return 0;
            } else {
                syslog(LOG_ERR, "groupadd command failed with exit status %d", exit_status);
            }
        }
        return -1;
    }
}

/**
 * Utilizada para atribuir as ACLS ao que for passado como path
*/
void exec_setfacl(char *path, char *group) {
    pid_t pid;
    int status;

    

    // Start a new process to execute setfacl
    pid = fork();
    if (pid == -1) {
        syslog(LOG_ERR, "Failed to fork: %s", strerror(errno));
        return;
    }

    if (pid == 0) { // Child process
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "g:rx,o::0");
        snprintf(cmd, sizeof(cmd), "g:%s:rx,o::0", group);
        execlp("setfacl", "setfacl", "-m", cmd, path, (char *)NULL);

        // If execlp returns, it must have failed
        syslog(LOG_ERR, "execlp failed to execute setfacl: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else { // Parent process
        // Wait for the child to complete and check status
        if (waitpid(pid, &status, 0) == -1) {
            syslog(LOG_ERR, "Failed to wait for child process: %s", strerror(errno));
        } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            syslog(LOG_ERR, "setfacl command failed with status %d", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            syslog(LOG_ERR, "setfacl command killed by signal %d", WTERMSIG(status));
        }
    }
}


/*
* Função que cria o folder para o grupo, adiciona também o user que criou ao grupo do folder e coloca o seu nome no ficheiro owner para o indentificar como criador
*/
void create_group(char *user, char *group, char* groupFolderPath, int pid) {
    char FolderPath[256];
    char ownerFilePath[270];
    snprintf(FolderPath, sizeof(FolderPath), "%s/%s", groupFolderPath, group);

    if (mkdir(FolderPath, 0750) == -1) {
        char *msg = "Group already exists";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "Failed to create the directory '%s': %s", FolderPath, strerror(errno));
        return;
    }

    if (create_system_group(group) != 0) {
        syslog(LOG_ERR, "Failed to create the group '%s': %s", group, strerror(errno));
        rmdir(FolderPath);
        return;
    }

    struct group *grp = getgrnam(group);
    if (!grp) {
        syslog(LOG_ERR, "Failed to fetch group info for '%s'", group);
        rmdir(FolderPath);
        return;
    }

    gid_t groupID = grp->gr_gid;
    struct passwd *pw = getpwnam(user);
    if (!pw) {
        syslog(LOG_ERR, "Failed to find user '%s'", user);
        rmdir(FolderPath);
        return;
    }

    if (chown(FolderPath, 0, groupID) == -1) {
        syslog(LOG_ERR, "Failed to change owner or group of '%s': %s", FolderPath, strerror(errno));
        rmdir(FolderPath);
        return;
    }

    exec_setfacl(FolderPath, group);

    snprintf(ownerFilePath, sizeof(ownerFilePath), "%s/owner", FolderPath);
    FILE *ownerFile = fopen(ownerFilePath, "w");
    if (!ownerFile) {
        syslog(LOG_ERR, "Failed to create owner file in '%s': %s", FolderPath, strerror(errno));
        rmdir(FolderPath);
        return;
    }

    if (fprintf(ownerFile, "%s", user) < 0) {
        syslog(LOG_ERR, "Failed to registor owner in file '%s'", FolderPath);
        fclose(ownerFile);
        rmdir(FolderPath);
        return;
    }

    fclose(ownerFile);

    if (chown(ownerFilePath, 0, groupID) == -1) {
        syslog(LOG_ERR, "Failed to change owner or group of owner file '%s': %s", ownerFilePath, strerror(errno));
    }

    if (chmod(ownerFilePath, S_IRWXU) == -1) {
        syslog(LOG_ERR, "Failed to set permissions on owner file '%s': %s", ownerFilePath, strerror(errno));
    }

    if (add_user_to_system_group(user, group) != 0) {
        char *msg = "Couldnt find user in the system";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "Failed to add user '%s' to group '%s'", user, group);
        return;
    }

    exec_setfacl(ownerFilePath, group);

    syslog(LOG_NOTICE, "Group '%s' created successfully with owner '%s'", group, user);

    char confirmation[50];
    snprintf(confirmation, sizeof(confirmation), "Criado o grupo %s com sucesso", group);

    returnListToClient(pid, confirmation);
}

int delete_system_group(const char *group) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        syslog(LOG_INFO, "Executing command: groupdel %s", group);
        execlp("groupdel", "groupdel", group, (char *)NULL);
        syslog(LOG_ERR, "execlp failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else {
        if (waitpid(pid, &status, 0) == -1) {
            syslog(LOG_ERR, "Failed to waitpid: %s", strerror(errno));
            return -1;
        }
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                return 0;
            } else {
                syslog(LOG_ERR, "groupdel command failed with exit status %d", exit_status);
            }
        }
        return -1;
    }
}


int is_owner(const char *group_folder_path, const char *user) {
    char owner_file_path[256];
    snprintf(owner_file_path, sizeof(owner_file_path), "%s/owner", group_folder_path);

    FILE *file = fopen(owner_file_path, "r");
    if (!file) {
        syslog(LOG_ERR, "Failed to open owner file in '%s': %s", group_folder_path, strerror(errno));
        return 0;
    }

    char owner_name[100];
    if (fgets(owner_name, sizeof(owner_name), file) == NULL) {
        syslog(LOG_ERR, "Failed to read owner file in '%s'", group_folder_path);
        fclose(file);
        return 0;
    }

    fclose(file);
    owner_name[strcspn(owner_name, "\n")] = 0;

    return strcmp(owner_name, user) == 0;
}


void remove_group(char *user, char *group, char *groupFolderPath, int pid) {
    char folderPath[256];
    snprintf(folderPath, sizeof(folderPath), "%s/%s", groupFolderPath, group);

    if (!is_owner(folderPath, user)) {
        syslog(LOG_ERR, "Unauthorized attempt to remove group '%s' by user '%s'", group, user);
        char *msg = "Unauthorized attempt to remove group";
        returnListToClient(pid, msg);
        return;
    } else {
        syslog(LOG_NOTICE, "Owner verified, deleting group: %s\n", group);
    }

    if (remove_directory(folderPath) != 0) {
        syslog(LOG_ERR, "Error removing group directory '%s': %s", folderPath, strerror(errno));
        return;
    }

    if (delete_system_group(group) != 0) {
        syslog(LOG_ERR, "Failed to delete system group '%s'", group);
        return;
    }

    char confirmation[50];
    snprintf(confirmation, sizeof(confirmation), "O grupo %s foi removido com sucesso", group);

    returnListToClient(pid, confirmation);
}

void add_user_to_group(char *user, char *group, char *user_to_add, char *groupsFolderName, int pid) {
    char group_folder_path[256];
    snprintf(group_folder_path, sizeof(group_folder_path), "%s/%s", groupsFolderName, group);

    if (!is_owner(group_folder_path, user)) {
        syslog(LOG_ERR, "Unauthorized attempt to modify group '%s' by user '%s'", group, user);
        char *msg = "Unauthorized attempt to remove group";
        returnListToClient(pid, msg);
        return;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "usermod -aG %s %s", group, user_to_add);

    if (add_user_to_system_group(user_to_add, group) != 0) {
        char *msg = "Couldnt find user in the system";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "Failed to add user '%s' to group '%s'", user, group);
        return;
    }

    char confirmation[50];
    snprintf(confirmation, sizeof(confirmation), "O user %s foi adicionado do grupo %s com sucesso", user, group);

    returnListToClient(pid, confirmation);
}

void remove_user_from_group(char *user, char *group, char *user_to_remove, char *groupsFolderName, int pid) {
    char group_folder_path[256];
    snprintf(group_folder_path, sizeof(group_folder_path), "%s/%s", groupsFolderName, group);

    if (!is_owner(group_folder_path, user)) {
        syslog(LOG_ERR, "Unauthorized attempt to modify group '%s' by user '%s'", group, user);
        char *msg = "Unauthorized attempt to remove group";
        returnListToClient(pid, msg);
        return;
    }

    if (remove_user_from_system_group(user_to_remove, group) != 0) {
        syslog(LOG_ERR, "Failed to remove user '%s' from group '%s'", user_to_remove, group);
        return;
    } else {
        syslog(LOG_NOTICE, "User '%s' removed from group '%s' successfully", user_to_remove, group);
    }

    char confirmation[50];
    snprintf(confirmation, sizeof(confirmation), "O user %s foi removido do grupo %s com sucesso", user, group);

    returnListToClient(pid, confirmation);
}

int user_in_group(char *user, struct group *grp) {
    if (!grp) return 0;
    for (int i = 0; grp->gr_mem[i] != NULL; i++) {
        if (strcmp(grp->gr_mem[i], user) == 0) {
            return 1; // user pertence
        }
    }
    return 0;
}

void listar_membros_grupo(char *user, char *group, char *groupsFolderName, int pid){
    char group_folder_path[256];
    snprintf(group_folder_path, sizeof(group_folder_path), "%s/%s", groupsFolderName, group);

    struct group *grp;
    struct passwd *pwd;

    pwd = getpwnam(user);
    if(!pwd) {
        syslog(LOG_ERR, "Error getting user %s info", user);
        return;
    }

    grp = getgrnam(group);
    if(!pwd) {
        char *msg = "Group doenst exist";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "Error getting group %s info", group);
        return;
    }

    int boolean;
    // verificar se o user pertencer ao grupo
    boolean = user_in_group(user, grp);

    if(boolean == 0){
        char *msg = "Unauthorized attempt to list group";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "User %s does not belong to the group %s", user, group);
        return;
    }

    char buffer[512];
    buffer[0] = '\0';
    for (int i = 0; grp->gr_mem[i] != NULL; i++) {
        strcat(buffer, grp->gr_mem[i]);
        strcat(buffer, " | ");
    }
    int len = strlen(buffer);
    buffer[len - 2] = '\0';

    syslog(LOG_NOTICE, "Group members are: %s", buffer);
    returnListToClient(pid, buffer);
}

void responder_mensagem_grupo(ConcordiaRequest request, char* groupsFolderPath){
    syslog(LOG_NOTICE, "Entrei responder grupo: %s\n", request.dest);

    char groupFolderPath[360];
    snprintf(groupFolderPath, sizeof(groupFolderPath), "%s/%s", groupsFolderPath, request.dest);

    struct stat st;
    if (stat(groupFolderPath, &st) == -1) {
        char *msg = "Group doesnt exist";
        returnListToClient(request.pid, msg);
        syslog(LOG_ERR, "Folder doesn't exist: %s\n", groupFolderPath);
        exit(EXIT_FAILURE);
    }

    int numFiles = count_files(groupFolderPath);
    struct FileInfo sortedFiles[numFiles];
    sort_files(groupFolderPath, sortedFiles);

    if (sortedFiles == NULL) {
        syslog(LOG_ERR, "Error sorting files.\n");
        exit(EXIT_FAILURE);
    }

    char timestamp[20];
    generate_timestamp(timestamp);

    int tam = strlen(request.msg);
    int id = getHighestID(groupFolderPath);

    char fileName[460];
    snprintf(fileName, sizeof(fileName), "%s/%d;%s;%s;%s;%d;0;0;%d.txt", groupFolderPath, id+1, request.dest, request.user, timestamp, tam, sortedFiles[request.all_mid].id);

    int file = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file < 0) {
        syslog(LOG_ERR, "Error opening file '%s': %s", fileName, strerror(errno));
        exit(EXIT_FAILURE);
    }

    char msg[513];
    strncpy(msg, request.msg, 512);
    msg[tam] = '\0';
    write(file, msg, tam);

    close(file);
    
    syslog(LOG_NOTICE, "Reply written to group %s in file: %s\n", request.dest, fileName);

    exec_setfacl(fileName, request.dest);

    // Update para marcar como read;
    char updateName[630];
    char repliedFile[630];
    snprintf(repliedFile, sizeof(repliedFile), "%s/%s", groupFolderPath, sortedFiles[request.all_mid].fileName);
    snprintf(updateName, sizeof(updateName), "%s/%d;%s;%s;%02d-%02d-%04d|%02d:%02d:%02d;%d;%d;%d;%d.txt",
             groupFolderPath, sortedFiles[request.all_mid].id, sortedFiles[request.all_mid].name, sortedFiles[request.all_mid].nameSender, 
             sortedFiles[request.all_mid].day, sortedFiles[request.all_mid].month, sortedFiles[request.all_mid].year, sortedFiles[request.all_mid].hour, 
             sortedFiles[request.all_mid].minute, sortedFiles[request.all_mid].second, sortedFiles[request.all_mid].tam, sortedFiles[request.all_mid].read, 
             sortedFiles[request.all_mid].nReplys + 1, sortedFiles[request.all_mid].isReply);

    rename(repliedFile, updateName);
    syslog(LOG_NOTICE, "Updated name from %s to file: %s\n", sortedFiles[request.all_mid].fileName, updateName);
}