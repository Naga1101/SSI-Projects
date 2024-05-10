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

int add_user_to_system_group(const char *user_to_add, const char *group) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process: execute usermod command
        syslog(LOG_INFO, "Executing command: usermod -aG %s %s", group, user_to_add);

        execlp("usermod", "usermod", "-aG", group, user_to_add, (char *)NULL);

        // Only reached if execlp fails
        syslog(LOG_ERR, "execlp failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process: wait for child to complete
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
        } else {
            syslog(LOG_ERR, "Child process did not exit normally");
            return -1;
        }
    } else {
        // Fork failed
        syslog(LOG_ERR, "Failed to fork: %s", strerror(errno));
        return -1;
    }
}


int remove_user_from_system_group(const char *user_to_remove, const char *group) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        syslog(LOG_INFO, "Executing command: gpasswd -d %s %s", user_to_remove, group);

        execlp("gpasswd", "gpasswd", "-d", user_to_remove, group, (char *)NULL);
        
        syslog(LOG_ERR, "execlp failed: %s", strerror(errno));
        _exit(EXIT_FAILURE);  // Important to use _exit() in child
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
        } else {
            syslog(LOG_ERR, "Child did not exit normally");
            return -1;
        }
    }
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
        } else {
            syslog(LOG_ERR, "Child did not exit normally");
        }
        return -1;
    }
}

void create_group(char *user, char *group, char* groupFolderPath) {
    char FolderPath[256];
    char ownerFilePath[270];

    snprintf(FolderPath, sizeof(FolderPath), "%s/%s", groupFolderPath, group);

    if (mkdir(FolderPath, 0775) == -1) {
        syslog(LOG_ERR, "Failed to create the directory '%s': %s\n", FolderPath, strerror(errno));
    }


    struct passwd *pw;
    struct group *grp;
    //uid_t userID;
    gid_t groupID;

    grp = getgrnam(group);
    if (!grp) {
        if(create_system_group(group) != 0){
            syslog(LOG_ERR, "Failed to create the group '%s': %s", group, strerror(errno));
            return;
        }
        
        // pegar na info do grupo
        grp = getgrnam(group);
        if (!grp) {
            syslog(LOG_ERR, "Failed to refetch group info for '%s'", group);
            return;
        }
    }
    else{
        syslog(LOG_ERR, "Group already exists");
    }


    groupID = grp->gr_gid;

    pw = getpwnam(user);
    if (!pw) {
        syslog(LOG_ERR, "Failed to find user '%s': %s", user, strerror(errno));
        return;
    }

    //userID = pw->pw_uid;

    if (chown(FolderPath, 0, groupID) == -1) {
        syslog(LOG_ERR, "Failed to change owner or group of '%s': %s", FolderPath, strerror(errno));
        rmdir(FolderPath);
        return;
    }

    // Set the directory permissions (Total para dono | read para group members)
    if (chmod(FolderPath, S_IRWXU | S_IRGRP | S_IXGRP) == -1) {
        syslog(LOG_ERR, "Failed to set permissions on '%s': %s", FolderPath, strerror(errno));
        rmdir(FolderPath);
        return;
    }

    // Create and write the owner file
    snprintf(ownerFilePath, sizeof(ownerFilePath), "%s/owner", FolderPath);
    FILE *ownerFile = fopen(ownerFilePath, "w");
    if (!ownerFile) {
        syslog(LOG_ERR, "Failed to create owner file in '%s': %s", FolderPath, strerror(errno));
        rmdir(FolderPath);
        return;
    }

    if (fprintf(ownerFile, "%s", user) < 0) {
        syslog(LOG_ERR, "Failed to write to owner file in '%s'", FolderPath);
        fclose(ownerFile);
        rmdir(FolderPath);
        return;
    }

    fclose(ownerFile);

    if (chown(ownerFilePath, 0, groupID) == -1) {
        syslog(LOG_ERR, "Failed to change owner or group of owner file '%s': %s", ownerFilePath, strerror(errno));
    }

    if (chmod(ownerFilePath, S_IRWXU | S_IRGRP | S_IXGRP) == -1) {
        syslog(LOG_ERR, "Failed to set permissions on owner file '%s': %s", ownerFilePath, strerror(errno));
    }

    if (add_user_to_system_group(user, group) != 0) {
        syslog(LOG_ERR, "Failed to add user '%s' to group '%s'", user, group);
        return;
    }

    syslog(LOG_NOTICE, "Group '%s' created successfully with owner '%s'", group, user);
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
        } else {
            syslog(LOG_ERR, "Child did not exit normally");
        }
        return -1;
    }
}

static int remove_entry(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int remove_directory(const char *dir_path) {
    return nftw(dir_path, remove_entry, 64, FTW_DEPTH | FTW_PHYS);
}

void remove_group(char *user, char *group, char *groupFolderPath){
    char FolderPath[256];
    char ownerFilePath[270];
    char ownerName[32];

    snprintf(FolderPath, sizeof(FolderPath), "%s/%s", groupFolderPath, group);
    snprintf(ownerFilePath, sizeof(ownerFilePath), "%s/owner", FolderPath);

    FILE *ownerFile = fopen(ownerFilePath, "r");
    if (!ownerFile) {
        syslog(LOG_ERR, "Failed to open owner file in '%s': %s", FolderPath, strerror(errno));
        return;
    }

    if (fgets(ownerName, sizeof(ownerName), ownerFile) == NULL) {
        syslog(LOG_ERR, "Failed to read owner file in '%s'", FolderPath);
        fclose(ownerFile);
        return;
    }

    fclose(ownerFile);

    ownerName[strcspn(ownerName, "\n")] = 0;

    if (strcmp(user, ownerName) != 0) {
        syslog(LOG_ERR, "Unauthorized attempt to remove group '%s' by user '%s'", group, user);
        return;
    }
    else{
        syslog(LOG_NOTICE, "Owner verified, deleting group: %s\n", group);
    }

    if(remove_directory(FolderPath) != 0){
        syslog(LOG_ERR, "Error removing group directory: %s", strerror(errno));
    }

    if (delete_system_group(group) != 0) {
        syslog(LOG_ERR, "Failed to delete system group '%s'", group);
        return;
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

void add_user_to_group(char *user, char *group, char *user_to_add, char *groupsFolderName) {
    char group_folder_path[256];
    snprintf(group_folder_path, sizeof(group_folder_path), "%s/%s", groupsFolderName, group);

    if (!is_owner(group_folder_path, user)) {
        syslog(LOG_ERR, "Unauthorized attempt to modify group '%s' by user '%s'", group, user);
        return;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "usermod -aG %s %s", group, user_to_add);

    /*int status = system(cmd);
    if (status != 0) {
        syslog(LOG_ERR, "Failed to add user '%s' to group '%s': %s", user_to_add, group, strerror(errno));
    } else {
        syslog(LOG_NOTICE, "User '%s' added to group '%s' successfully", user_to_add, group);
    }*/

    if (add_user_to_system_group(user_to_add, group) != 0) {
        syslog(LOG_ERR, "Failed to add user '%s' to group '%s'", user, group);
        return;
    }
}

void remove_user_from_group(char *user, char *group, char *user_to_remove, char *groupsFolderName) {
    char group_folder_path[256];
    snprintf(group_folder_path, sizeof(group_folder_path), "%s/%s", groupsFolderName, group);

    if (!is_owner(group_folder_path, user)) {
        syslog(LOG_ERR, "Unauthorized attempt to modify group '%s' by user '%s'", group, user);
        return;
    }

    if (remove_user_from_system_group(user_to_remove, group) != 0) {
        syslog(LOG_ERR, "Failed to remove user '%s' from group '%s'", user_to_remove, group);
        return;
    } else {
        syslog(LOG_NOTICE, "User '%s' removed from group '%s' successfully", user_to_remove, group);
    }
}

void returnListToClient(int pid, char *message){
    char fifoName[55];
    sprintf(fifoName, "/home/rui/Desktop/2324-G31/TPs/TP2/bin/fifo_%d", pid);

    syslog(LOG_NOTICE, "fifoname: %s", fifoName);

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1){
        syslog(LOG_ERR, "Error opening return fifo: %s \n", strerror(errno));
    }

    syslog(LOG_NOTICE, "%s", message);

    // enviar lista
    if (write(fd, message, strlen(message)) == -1) {
        perror("Error writing to FIFO");
    }
    close(fd);
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
        syslog(LOG_ERR, "Error getting group %s info", group);
        return;
    }

    int boolean;
    // verificar se o user pertencer ao grupo
    for (int i = 0; grp->gr_mem[i] != NULL; i++) {
        if (strcmp(grp->gr_mem[i], user) == 0) {
            boolean = 1;
            break;
        }
    }

    //se nao pertencer
    if(!boolean){
        syslog(LOG_ERR, "User %s does not belong to the group %s", user, group);
        return;
    }

    // isto verificava se o grupo primario do user da match ao grupo requested mas nao sei se é necessário
    /*
    if (pwd->pw_gid == grp->gr_gid) {
        boolean = 1;
    }
    */

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

int user_in_group(const char *user, const struct group *grp) {
    if (!grp) return 0;
    for (int i = 0; grp->gr_mem[i] != NULL; i++) {
        if (strcmp(grp->gr_mem[i], user) == 0) {
            return 1; // user found in group
        }
    }
    return 0;
}

void enviar_mensagem_grupo(char *user, char *group, char *msg, char *groupsFolderName) {
    char group_folder_path[256];
    snprintf(group_folder_path, sizeof(group_folder_path), "%s/%s", groupsFolderName, group);

    struct group *grp = getgrnam(group);
    if (!grp) {
        syslog(LOG_ERR, "Group %s not found", group);
        return;
    }

    if (!user_in_group(user, grp)) {
        syslog(LOG_ERR, "User %s does not belong to the group %s", user, group);
        return;
    }

    struct stat st;
    if (stat(group_folder_path, &st) == -1) {
        syslog(LOG_ERR, "Folder does not exist: %s", group_folder_path);
        return;
    }

    char timestamp[20];
    generate_timestamp(timestamp);

    int tam = strlen(msg);
    int id = getHighestID(group_folder_path);
    char fileName[512];
    snprintf(fileName, sizeof(fileName), "%s/%d;%s;%s;%s;%d;0;0;0;0.txt", group_folder_path, id + 1, group, user, timestamp, tam);

    int file = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file < 0) {
        syslog(LOG_ERR, "Error opening file '%s': %s", fileName, strerror(errno));
        return;
    }

    if (write(file, msg, tam) != tam) {
        syslog(LOG_ERR, "Error writting the message in file'%s'", fileName);
    }

    close(file);

    // Change the group of the file to match the group folder
    if (chown(fileName, 0, grp->gr_gid) == -1) {
        syslog(LOG_ERR, "Failed to change group of the file '%s': %s", fileName, strerror(errno));
    }

    // Set appropriate permissions for the file (read and write for owner, read for group)
    if (chmod(fileName, S_IRUSR | S_IWUSR | S_IRGRP) == -1) {
        syslog(LOG_ERR, "Failed to set permissions on the file '%s': %s", fileName, strerror(errno));
    }


    syslog(LOG_NOTICE, "Message written to group %s in file: %s", group, fileName);
}


void ler_mensagem_grupo(ConcordiaRequest request, char* folderPath) {
    int i = request.all_mid;
    syslog(LOG_NOTICE, "Entrei ler grupo: %s\n", request.dest);

    char groupFolderPath[120]; 
    snprintf(groupFolderPath, sizeof(groupFolderPath), "%s/%s", folderPath, request.dest);

    struct stat st;
    if (stat(groupFolderPath, &st) == -1) {
        syslog(LOG_ERR, "Group folder does not exist: %s\n", groupFolderPath);
        exit(EXIT_FAILURE);
    }

    int numFiles = count_files(groupFolderPath);
    struct FileInfo sortedFiles[numFiles];
    sort_files(groupFolderPath, sortedFiles);

    if (sortedFiles == NULL) {
        syslog(LOG_ERR, "Error sorting files.\n");
        exit(EXIT_FAILURE);
    }

    int tam = sortedFiles[i].tam;

    char fileName[256];
    snprintf(fileName, sizeof(fileName), "%s/%s", groupFolderPath, sortedFiles[i].fileName);
    syslog(LOG_NOTICE, "Group file: %s tam: %d\n", fileName, tam);

    int file = open(fileName, O_RDONLY);
    if (file < 0) {
        if (errno == ENOENT) {
            syslog(LOG_ERR, "File '%s' does not exist.\n", fileName);
        } else if (errno == EACCES) {
            syslog(LOG_ERR, "Permission denied to open file '%s'.\n", fileName);
        } else {
            syslog(LOG_ERR, "Error opening file '%s': %s\n", fileName, strerror(errno));
        }
        exit(EXIT_FAILURE);
    }

    char msg[tam + 1];  // Allocating extra byte for null terminator
    msg[tam] = '\0';

    read(file, msg, tam);

    close(file);

    syslog(LOG_NOTICE, "Group message read: %s\n", msg);

    if (sortedFiles[i].read == 0) {
        char updateName[380];
        snprintf(updateName, sizeof(updateName), "%s/%s;%s;%02d-%02d-%04d|%02d:%02d:%02d;%02d;1;%d;%d.txt", 
                 groupFolderPath, sortedFiles[i].name, sortedFiles[i].nameSender, sortedFiles[i].day, 
                 sortedFiles[i].month, sortedFiles[i].year, sortedFiles[i].hour, sortedFiles[i].minute, 
                 sortedFiles[i].second, sortedFiles[i].tam, sortedFiles[i].nReplys, sortedFiles[i].isReply);
        rename(fileName, updateName);
    }

    returnListToClient(request.pid, msg);
}

void responder_mensagem_grupo(ConcordiaRequest request, char* groupsFolderPath){
    syslog(LOG_NOTICE, "Entrei responder grupo: %s\n", request.dest);

    char groupFolderPath[360];
    snprintf(groupFolderPath, sizeof(groupFolderPath), "%s/%s", groupsFolderPath, request.dest);

    struct stat st;
    if (stat(groupFolderPath, &st) == -1) {
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

    // Update the existing file to mark as replied
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


void remove_group_message(ConcordiaRequest request, char* folderPath){
    int i = request.all_mid;
    syslog(LOG_NOTICE, "Entrei remover: %s\n", request.dest);


    char GroupFolderPath[100];
    snprintf(GroupFolderPath, sizeof(GroupFolderPath), "%s/%s", folderPath, request.dest); // aqui
    // snprintf(userFolderPath, sizeof(userFolderPath), "/home/nuno/teste");

    if(!is_owner(GroupFolderPath, request.user)){
        syslog(LOG_ERR, "Acesso nao autorizado, remove cancelado\n");
        return;
    }

    struct stat st;
    if (stat(GroupFolderPath, &st) == -1) {
        syslog(LOG_ERR, "Folder doesnt exist: %s\n", GroupFolderPath);
        exit(EXIT_FAILURE);
    }

    int numFiles = count_files(GroupFolderPath);
    struct FileInfo sortedFiles[numFiles];
    sort_files(GroupFolderPath, sortedFiles);

    if (sortedFiles == NULL) {
        syslog(LOG_ERR, "Error sorting files.\n");
        exit(EXIT_FAILURE);
    }

    char fileRemove[264];
    snprintf(fileRemove, sizeof(fileRemove), "%s/%s", GroupFolderPath, sortedFiles[i].fileName);
    if (remove(fileRemove) == 0) {
        syslog(LOG_NOTICE, "File '%s' has been successfully removed.\n", fileRemove);
    } else {
        syslog(LOG_PERROR, "Error removing file %s\n", fileRemove);
    }
}