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

    // Compare dates and times
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
            if (ent->d_type == DT_REG) { // If it's a regular file
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

    // Open current directory
    dir = opendir(folderPath);
    if (dir == NULL) {
        syslog(LOG_PERROR,"Unable to open directory");
        exit(EXIT_FAILURE);
    }

    int numFiles = count_files(folderPath);
    struct FileInfo files[numFiles];
    int fileCount = 0;

    // Read file names and parse them
    while ((ent = readdir(dir)) != NULL) {
        // Filter out directories
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

    // Sort files by date and time
    qsort(files, fileCount, sizeof(struct FileInfo), compareFileInfo);

    for (int i = 0; i < fileCount; ++i) {
        sortedFiles[i] = files[i];
        syslog(LOG_NOTICE, "file sorted: %s index: %d\n", sortedFiles[i].fileName, i);
    }
}

void sort_Allfiles(char *folderPaths[], int numFolders, struct FileInfo sortedFiles[]) {
    int totalFiles = 0;
    
    // Iterate over each folder path
    for (int folderIndex = 0; folderIndex < numFolders; folderIndex++) {
        // Check if the folder path exists
        if (access(folderPaths[folderIndex], F_OK) != 0) {
            syslog(LOG_WARNING, "Folder does not exist: %s", folderPaths[folderIndex]);
            continue; // Move to the next folder if the folder does not exist
        }

        DIR *dir;
        struct dirent *ent;

        // Open current directory
        dir = opendir(folderPaths[folderIndex]);
        if (dir == NULL) {
            syslog(LOG_PERROR, "Unable to open directory %s", folderPaths[folderIndex]);
            continue; // Move to the next folder if unable to open this one
        }

        // Count the number of files in the folder
        int numFiles = count_files(folderPaths[folderIndex]);
        struct FileInfo files[numFiles];
        int fileCount = 0;

        // Read file names and parse them
        while ((ent = readdir(dir)) != NULL) {
            // Filter out directories
            struct stat st;
            char filepath[512];
            sprintf(filepath, "./%s", ent->d_name);
            if (stat(filepath, &st) != 0) {
                syslog(LOG_NOTICE, "file: %s\n", filepath);
                if (strcmp(ent->d_name, "owner") != 0) {
                    parseFileName(ent->d_name, &files[fileCount]);
                    fileCount++;
                }
            }
        }

        closedir(dir);

        // Add files from this folder to the total list
        for (int i = 0; i < fileCount; ++i) {
            sortedFiles[totalFiles++] = files[i];
            // syslog(LOG_NOTICE, "%s index: %d\n", sortedFiles[totalFiles - 1].fileName, totalFiles - 1);
        }
    }

    // Sort the total list of files
    qsort(sortedFiles, totalFiles, sizeof(struct FileInfo), compareFileInfo);

    // syslog(LOG_NOTICE, "tamanho: %d'\n'", totalFiles);

    // for(int i = 0; i<totalFiles; i++){
    //     syslog(LOG_NOTICE, "%s\n", sortedFiles[i].fileName);
    // }
}

int count_Allfiles(char *folderPaths[], int numFolders) {
    int totalFiles = 0;
    
    // Iterate over each folder path
    for (int folderIndex = 0; folderIndex < numFolders; folderIndex++) {
        // Check if the folder path exists
        if (access(folderPaths[folderIndex], F_OK) != 0) {
            syslog(LOG_WARNING, "Folder does not exist: %s", folderPaths[folderIndex]);
            continue; // Move to the next folder if the folder does not exist
        }
        else{
            syslog(LOG_NOTICE, "Folder does exist: %s", folderPaths[folderIndex]);
        }

        DIR *dir;

        // Open current directory
        dir = opendir(folderPaths[folderIndex]);
        if (dir == NULL) {
            syslog(LOG_PERROR, "Unable to open directory %s", folderPaths[folderIndex]);
            continue; // Move to the next folder if unable to open this one
        } else {
            syslog(LOG_NOTICE, "Folder opened: %s", folderPaths[folderIndex]);
        }

        // Count the number of files in the folder
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

    // __uid_t uid = pw->pw_uid;

    int ngroups = 0;

    // this call is just to get the correct ngroups
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

    // here we actually get the groups
    if (getgrouplist(username, pw->pw_gid, groups, &ngroups) == -1) {
        if (errno != 0) {
            syslog(LOG_ERR, "getgrouplist error: %m");
            free(groups);
            exit(EXIT_FAILURE);
        }
    }

    // allocate memory for group names
    *groupNames = (char**)malloc(ngroups * sizeof(char*));
    if (*groupNames == NULL) {
        syslog(LOG_ERR, "malloc error: %m");
        free(groups);
        exit(EXIT_FAILURE);
    }

    // example to print the groups name
    for (int i = 0; i < ngroups; i++) {
        struct group* gr = getgrgid(groups[i]);
        if(gr == NULL) {
            syslog(LOG_ERR, "getgrgid error: %m");
            continue;
        }

        char* groupName;
        if (strcmp(gr->gr_name, username) != 0) {
            // Group name is different from username
            size_t len = strlen(gFolderPath) + strlen(gr->gr_name) + 2; // 2 for '/' and '\0'
            groupName = (char*)malloc(len);
            if (groupName == NULL) {
                syslog(LOG_ERR, "malloc error: %m");
                continue;
            }
            snprintf(groupName, len, "%s/%s", gFolderPath, gr->gr_name);
        } else {
            // Group name is the same as username
            size_t len = strlen(uFolderPath) + strlen(gr->gr_name) + 2; // 2 for '/' and '\0'
            groupName = (char*)malloc(len);
            if (groupName == NULL) {
                syslog(LOG_ERR, "malloc error: %m");
                continue;
            }
            snprintf(groupName, len, "%s/%s", uFolderPath, gr->gr_name);
        }
        // syslog(LOG_NOTICE,"nome do grupo: %s'\n'", groupName);
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


void escreverLista(struct FileInfo sortedFiles[], int numFiles, char *user, char msg[]) {
    int offset = 0;
    offset += snprintf(msg + offset, MAX_CHAR_ARRAY_LENGTH - offset,
                        "Index | From |      Received      | Status | is Reply | Size of Message |  Via  |\n");
    for (int i = 0; i < numFiles; i++) {
        // syslog(LOG_NOTICE, "%s\n", sortedFiles[i].fileName);
        offset += snprintf(msg + offset, MAX_CHAR_ARRAY_LENGTH - offset,
                           "   %d   | %s | %02d-%02d-%04d %02d:%02d:%02d | %s | %s | %d |  %s  |\n",
                           i+1, sortedFiles[i].nameSender, sortedFiles[i].day, sortedFiles[i].month, sortedFiles[i].year,
                           sortedFiles[i].hour, sortedFiles[i].minute, sortedFiles[i].second,
                           (sortedFiles[i].read == 1) ? "Read" : "Not Read",
                           (sortedFiles[i].isReply != 0) ? "Yes" : "No",
                           sortedFiles[i].tam, strcmp(sortedFiles[i].name, user) ? sortedFiles[i].name : "DM");
    }
}