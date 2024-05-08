#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "./include/struct.h"
#include "./message_commands.c"
#include "./group_commands.c"
#include "./user_commands.c"

void handle_user_message(ConcordiaRequest request, char* usersFolderName){
    if (strcmp(request.command, "enviar") == 0) {
        printf("Handling enviar command\n");

    } else if (strcmp(request.command, "listar") == 0) {
        printf("Handling listar command\n");

    } else if (strcmp(request.command, "ler") == 0) {
        printf("Handling ler command\n");

    } else if (strcmp(request.command, "responder") == 0) {
        printf("Handling responder command\n");

    } else if (strcmp(request.command, "remover") == 0) {
        printf("Handling remover command\n");

    } else {
        printf("Unknown command: %s\n", request.command);
    }
}

void handle_group_message(ConcordiaRequest request, char* groupsFolderName){
    if (strcmp(request.command, "criar") == 0) {
        printf("Handling criar command\n");

    } else if (strcmp(request.command, "remover") == 0) {
        printf("Handling remover command\n");

    } else if (strcmp(request.command, "listar") == 0) {
        printf("Handling listar command\n");

    } else if (strcmp(request.command, "adicionar") == 0) {
        printf("Handling adicionar command\n");

    } else if (strcmp(request.command, "remover-usuario") == 0) {
        printf("Handling remover-usuario command\n");

    } else {
        printf("Unknown command: %s\n", request.command);
    }
}

void handle_user_command(ConcordiaRequest request){
    if (strcmp(request.command, "ativar") == 0) {
        printf("Handling ativar command\n");

    } else if (strcmp(request.command, "desativar") == 0) {
        printf("Handling desativar command\n");

    } else {
        printf("Unknown command: %s\n", request.command);
    }
}