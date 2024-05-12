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
    printf("Efetuando o seu pedido.\n");
}

void read_from_daemon(){
    char fifoName[128];
    snprintf(fifoName, sizeof(fifoName), "/var/lib/concordia/fifos/fifo_%d", getpid());

    // printf("fifoName %s", fifoName);

    if (mkfifo(fifoName, 0660) == -1) {
        perror("Error creating return FIFO \n");
    }

    int fd2 = open(fifoName, O_RDONLY);
    if(fd2 == -1){
        perror("Error opening FIFO");
        return;
    }

    ssize_t bytes_read;
    char databuffer[MSG_SIZE];

    if((bytes_read = read(fd2, databuffer, sizeof(databuffer))) > 0){
        printf("%s\n", databuffer);
    }

    unlink(fifoName);
}

// Função para obter o nome do usuário atual
char *obter_usuario_atual() {
    char *user = getenv("USER");
    return user;
}

void ativar_usuario(ConcordiaRequest *request) {
    request->flag = USER;
    snprintf(request->command, COMMAND_SIZE, "ativar");
    char *user = obter_usuario_atual();
    snprintf(request->user, usersize,"%s", user);
    // printf("%s'\n'", user);

    send_to_deamon(request);
    printf("Ativando usuário %s...\n", user);

    read_from_daemon();
}

void desativar_usuario(ConcordiaRequest *request) {
    request->flag = USER;
    snprintf(request->command, COMMAND_SIZE, "desativar");
    char *user = obter_usuario_atual();
    snprintf(request->user, usersize,"%s", user);

    send_to_deamon(request);
    printf("Desativando usuário %s\n", user);

    read_from_daemon();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ConcordiaRequest *request = malloc(sizeof(ConcordiaRequest));
    if (!request) {
        perror("Falha ao alocar memória para o request");
        return EXIT_FAILURE;
    }

    request->flag = USER;
    request->pid = getpid();

    if (strcmp(argv[1], "ativar") == 0) {
        ativar_usuario(request);
    } else if (strcmp(argv[1], "desativar") == 0) {
        desativar_usuario(request);
    } else {
        fprintf(stderr, "Comando inválido.\n");
        free(request);
        return EXIT_FAILURE;
    }

    free(request);
    return EXIT_SUCCESS;
}
