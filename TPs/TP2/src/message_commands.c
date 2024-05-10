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

#include "../include/struct.h"
#include "../include/message_commands.h"

struct FileInfo {
    char fileName[128];
    char name[64];
    int day, month, year, hour, minute, second, tam, read;
};

void parseFileName(char *filename, struct FileInfo *info) {
    strncpy(info->fileName, filename, 128);
    sscanf(filename, "%[^;];%02d-%02d-%04d|%02d:%02d:%02d;%d;%d", info->name, &info->day, &info->month, &info->year, &info->hour, &info->minute, &info->second, &info->tam, &info->read);
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
        snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
        if (stat(filePath, &st) == 0 && S_ISREG(st.st_mode)) {
            count++;
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


void generate_timestamp(char *timestamp) {
    time_t now = time(NULL);
    struct tm *tm_info;
    tm_info = localtime(&now);
    strftime(timestamp, 20, "%d-%m-%Y|%H:%M:%S", tm_info);
}

void enviar_message(ConcordiaRequest request, char* folderPath){
    // syslog(LOG_NOTICE, "Entrei enviar: %s\n", request.dest);
    // char dest[16];
    // strncpy(dest, request.dest, 16);
    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, request.dest);
    // snprintf(userFolderPath, sizeof(userFolderPath), "/home/nuno/teste");

    struct stat st;
    if (stat(userFolderPath, &st) == -1) {
        syslog(LOG_ERR, "Folder doesnt exist: %s\n", userFolderPath);
        exit(EXIT_FAILURE);
    }

    char timestamp[20];
    generate_timestamp(timestamp);

    int tam = strlen(request.msg);

    char fileName[250];
    snprintf(fileName, sizeof(fileName), "%s/%s;%s;%d;0.txt", userFolderPath, request.user, timestamp, tam);
    // syslog(LOG_NOTICE, "Entrei enviar: %s\n", fileName);

    int file = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644);
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

    // syslog(LOG_NOTICE, "Tamanho buffer do request: %d\n", tam);
    char msg[513];
    strncpy(msg, request.msg, 512);
    syslog(LOG_NOTICE, "Tamanho buffer: %d\n", tam);
    msg[tam] = '\0';
    write(file, msg, sizeof(char) * tam);

    close(file);
    
    syslog(LOG_NOTICE, "Message written to %s in file: %s\n", request.dest, fileName);
}

void ler_message(ConcordiaRequest request, char* folderPath){
    DIR *dir;
    int i = request.all_mid;
    syslog(LOG_NOTICE, "Entrei ler: %s\n", request.user);
    // char dest[16];
    // strncpy(dest, request.dest, 16);
    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, request.user);
    // snprintf(userFolderPath, sizeof(userFolderPath), "/home/nuno/teste");

    struct stat st;
    if (stat(userFolderPath, &st) == -1) {
        syslog(LOG_ERR, "Folder doesnt exist: %s\n", userFolderPath);
        exit(EXIT_FAILURE);
    }

    int numFiles = count_files(userFolderPath);
    struct FileInfo sortedFiles[numFiles];
    sort_files(userFolderPath, sortedFiles);

    if (sortedFiles == NULL) {
        syslog(LOG_ERR, "Error sorting files.\n");
        exit(EXIT_FAILURE);
    }
    // else{
    //     syslog(LOG_NOTICE, "aqui %d\n", numFiles);
    //     for (int i = 0; i < numFiles; ++i) {
    //        syslog(LOG_NOTICE, "file sorted: %s index: %d tam: %d\n", sortedFiles[i].fileName, i, sortedFiles[i].tam);
    //     }
    // }
    
    int tam = sortedFiles[i].tam;

    dir = opendir(userFolderPath);
    if (dir == NULL) {
        syslog(LOG_PERROR,"Unable to open directory");
        exit(EXIT_FAILURE);
    }

    char fileName[138];
    snprintf(fileName, sizeof(fileName), "./%s", sortedFiles[i].fileName);
    syslog(LOG_NOTICE, "file: %s tam: %d\n", fileName, tam);

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

    char msg[tam];
    msg[tam] = '\0';

    read(file, msg, tam-1);

    close(file);

    closedir(dir);

    syslog(LOG_NOTICE, "Message read: %s\n", msg);

    // passar o nome do file para 1
}