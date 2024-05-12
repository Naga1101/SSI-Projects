#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <grp.h>

#include "../include/struct.h"
#include "../include/files_struct.h"
#include "../include/message_commands.h"

#define MAX_CHAR_ARRAY_LENGTH 512

/*
* Popula a sruct FileInfo com base no nome do ficheiro
*/
void parseFileName(char *filename, struct FileInfo *info) {
    strncpy(info->fileName, filename, 128);
    sscanf(filename, "%d;%[^;];%[^;];%02d-%02d-%04d|%02d:%02d:%02d;%d;%d;%d;%d", &info->id, info->name, info->nameSender, &info->day, &info->month, &info->year, &info->hour, &info->minute, &info->second, &info->tam, &info->read, &info->nReplys, &info->isReply);
}

int count_files(const char *folderPath) {
    int count = 0;
    DIR *dir;
    struct dirent *entry;
    struct stat st;

    dir = opendir(folderPath);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }


    while ((entry = readdir(dir)) != NULL) {
        char filePath[300];
        if (strcmp(entry->d_name, "owner") != 0) {
            snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
            if (stat(filePath, &st) == 0 && S_ISREG(st.st_mode)) {
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

int compareFileInfo(const void *a, const void *b) {
    struct FileInfo *fileA = (struct FileInfo *)a;
    struct FileInfo *fileB = (struct FileInfo *)b;

    if (fileA->read != fileB->read)
        return fileA->read - fileB->read;

    if (fileA->year != fileB->year)
        return fileA->year - fileB->year;
    if (fileA->month != fileB->month)
        return fileA->month - fileB->month;
    if (fileA->day != fileB->day)
        return fileA->day - fileB->day;
    if (fileA->hour != fileB->hour)
        return fileA->hour - fileB->hour;
    if (fileA->minute != fileB->minute)
        return fileA->minute - fileB->minute;
    if (fileA->second != fileB->second)
        return fileA->second - fileB->second;

    return strcmp(fileA->name, fileB->name);
}

int getHighestID(const char *userFolderPath) {
    DIR *dir;
    struct dirent *ent;
    int highestID = 0;

    if ((dir = opendir(userFolderPath)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                char *filename = ent->d_name;
                char *token = strtok(filename, ";");
                if (token != NULL) {
                    int id = atoi(token);
                    if (id > highestID) {
                        highestID = id;
                    }
                }
            }
        }
        closedir(dir);
    } else {
        perror("Error opening directory");
        return -1;
    }
    return highestID;
}

void sort_files(char *folderPath, struct FileInfo sortedFiles[]){
    DIR *dir;
    struct dirent *ent;

    dir = opendir(folderPath);
    if (dir == NULL) {
        syslog(LOG_PERROR,"Unable to open directory");
        exit(EXIT_FAILURE);
    }

    int numFiles = count_files(folderPath);
    struct FileInfo files[numFiles];
    int fileCount = 0;

    while ((ent = readdir(dir)) != NULL) {
        struct stat st;
        char filepath[512];
        sprintf(filepath, "./%s", ent->d_name);
        if (stat(filepath, &st) != 0) {
            syslog(LOG_NOTICE, "file: %s\n", filepath);
            parseFileName(ent->d_name, &files[fileCount]);
            fileCount++;
        }
    }

    closedir(dir);

    // Sort for tempo
    qsort(files, fileCount, sizeof(struct FileInfo), compareFileInfo);

    for (int i = 0; i < fileCount; ++i) {
        sortedFiles[i] = files[i];
        syslog(LOG_NOTICE, "file sorted: %s index: %d\n", sortedFiles[i].fileName, i);
    }
}

void sort_Allfiles(char *folderPaths[], int numFolders, struct FileInfo sortedFiles[], char *user) {
    int totalFiles = 0;
    
    for (int folderIndex = 0; folderIndex < numFolders; folderIndex++) {
        if (access(folderPaths[folderIndex], F_OK) != 0) {
            syslog(LOG_WARNING, "Folder does not exist: %s", folderPaths[folderIndex]);
            continue;
        }

        DIR *dir;
        struct dirent *ent;

        dir = opendir(folderPaths[folderIndex]);
        if (dir == NULL) {
            syslog(LOG_PERROR, "Unable to open directory %s", folderPaths[folderIndex]);
            continue; // se nao consegue abrir skip
        }

        // Conta o numero de ficheiros no folder
        int numFiles = count_files(folderPaths[folderIndex]);
        struct FileInfo files[numFiles];
        int fileCount = 0;

        // Ler o nome dos ficheiros e parsing
        while ((ent = readdir(dir)) != NULL) {
            struct stat st;
            char filepath[512];

            sprintf(filepath, "./%s", ent->d_name);
            if (stat(filepath, &st) != 0) {
                syslog(LOG_NOTICE, "file: %s\n", filepath);
                if (strcmp(ent->d_name, "owner") != 0) {
                    parseFileName(ent->d_name, &files[fileCount]);
                    if(strcmp(user, files[fileCount].name) != 0){
                        sprintf(filepath, "%s/%s", folderPaths[folderIndex], ent->d_name);
                        files[fileCount].read = checkIfRead(filepath, user, files[fileCount].tam, files[fileCount].read);
                        syslog(LOG_NOTICE, "read result %d", files[fileCount].read );
                    }
                    fileCount++;
                }
            }
        }

        closedir(dir);

        for (int i = 0; i < fileCount; ++i) {
            sortedFiles[totalFiles++] = files[i];
        }
    }

    qsort(sortedFiles, totalFiles, sizeof(struct FileInfo), compareFileInfo);
}

int count_Allfiles(char *folderPaths[], int numFolders) {
    int totalFiles = 0;
    
    // Iterarar as pastas
    for (int folderIndex = 0; folderIndex < numFolders; folderIndex++) {
        // Verifica se o folder existe
        if (access(folderPaths[folderIndex], F_OK) != 0) {
            syslog(LOG_WARNING, "Folder does not exist: %s", folderPaths[folderIndex]);
            continue;
        }
        else{
            syslog(LOG_NOTICE, "Folder does exist: %s", folderPaths[folderIndex]);
        }

        DIR *dir;

        dir = opendir(folderPaths[folderIndex]);
        if (dir == NULL) {
            syslog(LOG_PERROR, "Unable to open directory %s", folderPaths[folderIndex]);
            continue;
        } else {
            syslog(LOG_NOTICE, "Folder opened: %s", folderPaths[folderIndex]);
        }

        // numero de ficheiros
        totalFiles += count_files(folderPaths[folderIndex]);

        closedir(dir);
    }

    return totalFiles;
}

int getUserGroups(char* uFolderPath, char* gFolderPath, char* username, char*** groupNames) {
    struct passwd* pw = getpwnam(username);
    if(pw == NULL){
        syslog(LOG_ERR, "getpwnam error: %m");
        exit(EXIT_FAILURE);
    }

    int ngroups = 0;

    if (getgrouplist(username, pw->pw_gid, NULL, &ngroups) == -1) {
        if (errno != 0) {
            syslog(LOG_ERR, "getgrouplist error: %m");
            exit(EXIT_FAILURE);
        }
    }
    __gid_t* groups = (gid_t*)malloc(ngroups * sizeof(gid_t));
    if (groups == NULL) {
        syslog(LOG_ERR, "malloc error: %m");
        exit(EXIT_FAILURE);
    }

    // obter os grupos
    if (getgrouplist(username, pw->pw_gid, groups, &ngroups) == -1) {
        if (errno != 0) {
            syslog(LOG_ERR, "getgrouplist error: %m");
            free(groups);
            exit(EXIT_FAILURE);
        }
    }

    *groupNames = (char**)malloc(ngroups * sizeof(char*));
    if (*groupNames == NULL) {
        syslog(LOG_ERR, "malloc error: %m");
        free(groups);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < ngroups; i++) {
        struct group* gr = getgrgid(groups[i]);
        if(gr == NULL) {
            syslog(LOG_ERR, "getgrgid error: %m");
            continue;
        }

        char* groupName;
        if (strcmp(gr->gr_name, username) != 0) {
            // Nome do grupo é diferente do username
            size_t len = strlen(gFolderPath) + strlen(gr->gr_name) + 2; // 2 for '/' and '\0'
            groupName = (char*)malloc(len);
            if (groupName == NULL) {
                syslog(LOG_ERR, "malloc error: %m");
                continue;
            }
            snprintf(groupName, len, "%s/%s", gFolderPath, gr->gr_name);
        } else {
            // Nome do grupo é igual do username
            size_t len = strlen(uFolderPath) + strlen(gr->gr_name) + 2; // 2 for '/' and '\0'
            groupName = (char*)malloc(len);
            if (groupName == NULL) {
                syslog(LOG_ERR, "malloc error: %m");
                continue;
            }
            snprintf(groupName, len, "%s/%s", uFolderPath, gr->gr_name);
        }
        (*groupNames)[i] = groupName;
    }

    free(groups);

    return ngroups;
}

void generate_timestamp(char *timestamp) {
    time_t now = time(NULL);
    struct tm *tm_info;
    tm_info = localtime(&now);
    strftime(timestamp, 20, "%d-%m-%Y|%H:%M:%S", tm_info);
}


char* escreverLista(struct FileInfo sortedFiles[], int numFiles, int flagAll, char *user) {
    int offset = 0;
    int capacity = 2048;
    char *msg = malloc(capacity);
    if (!msg) return NULL;

    offset += snprintf(msg + offset, capacity - offset, 
                       "Index | From |      Received      | Status | is Reply | Size of Message |  Via  |\n");

    for (int i = 0; i < numFiles; i++) {
        if (flagAll == 0 && sortedFiles[i].read == 1) {
            break;
        }

        int needed = snprintf(NULL, 0, 
            "   %d   | %s | %02d-%02d-%04d %02d:%02d:%02d | %s | %s | %d |  %s  |\n",
            i + 1, sortedFiles[i].nameSender, sortedFiles[i].day, sortedFiles[i].month, sortedFiles[i].year,
            sortedFiles[i].hour, sortedFiles[i].minute, sortedFiles[i].second,
            (sortedFiles[i].read == 1) ? "Read" : "Not Read",
            (sortedFiles[i].isReply != 0) ? "Yes" : "No",
            sortedFiles[i].tam, strcmp(sortedFiles[i].name, user) ? sortedFiles[i].name : "DM");

        if (offset + needed >= capacity) {
            capacity += needed + 512;
            char *new_msg = realloc(msg, capacity);
            if (!new_msg) {
                free(msg);
                return NULL;
            }
            msg = new_msg;
        }

        offset += snprintf(msg + offset, capacity - offset, 
            "   %d   | %s | %02d-%02d-%04d %02d:%02d:%02d | %s | %s | %d |  %s  |\n",
            i + 1, sortedFiles[i].nameSender, sortedFiles[i].day, sortedFiles[i].month, sortedFiles[i].year,
            sortedFiles[i].hour, sortedFiles[i].minute, sortedFiles[i].second,
            (sortedFiles[i].read == 1) ? "Read" : "Not Read",
            (sortedFiles[i].isReply != 0) ? "Yes" : "No",
            sortedFiles[i].tam, strcmp(sortedFiles[i].name, user) ? sortedFiles[i].name : "DM");
    }

    return msg;
}

char* selectDestino(char** foldersWAccess, int numFolders, const char* dest) {
    for (int i = 0; i < numFolders; i++) {
        syslog(LOG_NOTICE, "path: %s", foldersWAccess[i]);
        if (strstr(foldersWAccess[i], dest) != NULL) {
            syslog(LOG_NOTICE, "Encontrei: %s", foldersWAccess[i]);
            return foldersWAccess[i];
        }
    }
    return NULL;
}

int checkIfRead(char *file, char *username, int start, int end) {



    syslog(LOG_ERR, "File %s", file);
    syslog(LOG_NOTICE, "user %s", username);
    syslog(LOG_NOTICE, "start %d", start);
    syslog(LOG_NOTICE, "end %d", end);


    int fd = open(file, O_RDONLY);
    if (fd == -1) {
        syslog(LOG_ERR, "Error opening file: %m");
        return -1; 
    }

    if(end == 0){
        return 0;
    }

    if (lseek(fd, start, SEEK_SET) == -1) {
        syslog(LOG_ERR, "Error seeking file: %m");
        close(fd);
        return -1; 
    }

    char *buffer = (char *)malloc(end + 1); 
    if (buffer == NULL) {
        syslog(LOG_ERR, "Memory allocation error");
        close(fd);
        return -1; 
    }

    ssize_t bytesRead = read(fd, buffer, end);
    if (bytesRead == -1) {
        syslog(LOG_ERR, "Error reading file: %m");
        free(buffer);
        close(fd);
        return -1; 
    }
    buffer[bytesRead] = '\0'; 

    syslog(LOG_NOTICE, "buffer: %s", buffer);

    int found = 0;
    char *token = strtok(buffer, ";");
    while (token != NULL) {
        if (strcmp(token, username) == 0) {
            found = 1;
            break;
        }
        token = strtok(NULL, ";");
    }

    free(buffer);
    close(fd);
    return found; // Return 1 if found - 0 otherwise
}