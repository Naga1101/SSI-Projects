#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/struct.h"


void send_to_deamon(ConcordiaRequest *request){
    int fd = open(FIFO, O_WRONLY);
    if (fd == -1){
        perror("Erro ao abrir o pipe para requests");
        exit(EXIT_FAILURE);
    }


    if(write(fd, request, sizeof(ConcordiaRequest)) == -1){
        perror("Falha ao enviar o request");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    printf("Menesagem enviada com sucesso\n");

}

void ativar_usuario(ConcordiaRequest *request) {
    printf("Ativando usuário\n");

    request->flag = USER;

    send_to_deamon(request);
}

void desativar_usuario(ConcordiaRequest *request) {
    printf("Desativando usuário\n");

    request->flag = USER;
    send_to_deamon(request);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ConcordiaRequest *request = malloc(sizeof(ConcordiaRequest));

    if (strcmp(argv[1], "ativar") == 0) {
        ativar_usuario(request);
    } 
    else if (strcmp(argv[1], "desativar") == 0) {
        desativar_usuario(request);
    } 
    else {
        fprintf(stderr, "Comando inválido.\n");
        return EXIT_FAILURE;
    }

    free(request);

    return EXIT_SUCCESS;
}