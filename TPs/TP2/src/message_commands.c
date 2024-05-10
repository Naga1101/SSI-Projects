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
#include "../include/files_struct.h"
#include "../include/message_commands.h"

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
    int id = getHighestID(userFolderPath);
    char fileName[250];
    snprintf(fileName, sizeof(fileName), "%s/%d;%s;%s;%s;%d;0;0;0;0.txt", userFolderPath, id+1, request.dest, request.user,  timestamp, tam);
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

//TODO : adaptar para ver as mensagens de grupos
// verificar no nome do file quem enviou
void ler_message(ConcordiaRequest request, char* folderPath){
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


    char fileName[256];
    snprintf(fileName, sizeof(fileName), "%s/%s", userFolderPath, sortedFiles[i].fileName);
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

    read(file, msg, tam);

    close(file);

    syslog(LOG_NOTICE, "Message read: %s\n", msg);

    if(sortedFiles[i].read == 0){
        char updateName[264];                                                                                          //para onde foi enviado, quem enviou
        snprintf(updateName, sizeof(updateName), "%s/%s;%s;%02d-%02d-%04d|%02d:%02d:%02d;%02d;1;%d;%d.txt", userFolderPath, sortedFiles[i].name, 
        sortedFiles[i].nameSender, sortedFiles[i].day, sortedFiles[i].month, sortedFiles[i].year, sortedFiles[i].hour, sortedFiles[i].minute, 
        sortedFiles[i].second, sortedFiles[i].tam, sortedFiles[i].nReplys, sortedFiles[i].isReply);
        rename(fileName, updateName);
    }
}

//TODO ajustar para responder a grupos tambem visto que tem de descobrir para quem é que é a mensagem atraves do indice
// utilizar o user para ver todos os grupos a que pertence e dps organizar todas as mensagens pela ordem com o sort a partir dai usar o sortedFiles[i].name no lugar do request.dest
void responder_message(ConcordiaRequest request, char* folderPath){
    int i = request.all_mid;
    syslog(LOG_NOTICE, "Entrei responder: %s\n", request.user);

    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, request.user); // aqui
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


    char timestamp[20];
    generate_timestamp(timestamp);

    int tam = strlen(request.msg);
    int id = getHighestID(userFolderPath);

    char fileName[250];
    snprintf(fileName, sizeof(fileName), "%s/%d;%s;%s;%s;%d;0;0;%d.txt", userFolderPath, id+1, sortedFiles[i].nameSender, request.user,  timestamp, tam, sortedFiles[i].id);
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
    
    syslog(LOG_NOTICE, "Reply written to %s in file: %s\n", sortedFiles[i].nameSender, fileName);

    char updateName[264];
    char repliedFile[264];
    snprintf(repliedFile, sizeof(repliedFile), "%s/%s", userFolderPath, sortedFiles[i].fileName);                                                                                 //para onde foi enviado, quem enviou
    snprintf(updateName, sizeof(updateName), "%s/%d;%s;%s;%02d-%02d-%04d|%02d:%02d:%02d;%d;%d;%d;%d.txt", userFolderPath, sortedFiles[i].id, sortedFiles[i].name, sortedFiles[i].nameSender, sortedFiles[i].day, sortedFiles[i].month, sortedFiles[i].year, sortedFiles[i].hour, sortedFiles[i].minute, sortedFiles[i].second, sortedFiles[i].tam, sortedFiles[i].read, sortedFiles[i].nReplys + 1, sortedFiles[i].isReply);
    rename(repliedFile, updateName);
    syslog(LOG_NOTICE, "Updated name from %s to file: %s\n", sortedFiles[i].fileName, updateName);

}

void remover_message(ConcordiaRequest request, char* folderPath){
    int i = request.all_mid;
    syslog(LOG_NOTICE, "Entrei remover: %s\n", request.user);

    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, request.user); // aqui
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

    char fileRemove[264];
    snprintf(fileRemove, sizeof(fileRemove), "%s/%s", userFolderPath, sortedFiles[i].fileName);
    if (remove(fileRemove) == 0) {
        syslog(LOG_NOTICE, "File '%s' has been successfully removed.\n", fileRemove);
    } else {
        syslog(LOG_PERROR, "Error removing file %s\n", fileRemove);
    }
}