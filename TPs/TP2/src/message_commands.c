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

#include "../include/struct.h"
#include "../include/message_commands.h"

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

void generate_timestamp(char *timestamp) {
    time_t now = time(NULL);
    struct tm *tm_info;
    tm_info = localtime(&now);
    strftime(timestamp, 20, "%d/%m/%Y-%H:%M:%S", tm_info);
}

void enviar_message(ConcordiaRequest request, char* folderPath){
    // syslog(LOG_NOTICE, "Entrei enviar: %s\n", request.dest);
    // char dest[16];
    // strncpy(dest, request.dest, 16);
    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, request.dest);

    struct stat st;
    if (stat(userFolderPath, &st) == -1) {
        syslog(LOG_NOTICE, "Folder doesnt exist: %s\n", userFolderPath);
        exit(EXIT_FAILURE);
    }

    int file_count = count_files(userFolderPath);
    syslog(LOG_NOTICE, "Entrei enviar: %d\n", file_count);


    char timestamp[20];
    generate_timestamp(timestamp);

    char fileName[250];
    snprintf(fileName, sizeof(fileName), "%s/%d-%s-%s.txt", userFolderPath, file_count + 1, request.user, timestamp);
    syslog(LOG_NOTICE, "Entrei enviar: %s\n", fileName);

    int file = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
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

    write(file, request.msg, sizeof(char) * 512);

    close(file);
    
    syslog(LOG_NOTICE, "Message written to %s in file: %s\n", request.dest, fileName);
}