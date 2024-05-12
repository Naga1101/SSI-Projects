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

    // printf("command %s\n", request->command);

    close(fd);
    printf("Efetuando o seu pedido.'\n'");
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
char* obter_usuario_atual() {
    char* usuario = getenv("USER");
    return usuario;
}

void criar_grupo(char *nome, ConcordiaRequest *request) {
    snprintf(request->command, COMMAND_SIZE, "criar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", nome);

    send_to_deamon(request);
    printf("Criando grupo %s...\n", nome);

    read_from_daemon();
}

void remover_grupo(char *grupo, ConcordiaRequest *request) {
    snprintf(request->command, COMMAND_SIZE, "remover");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);

    send_to_deamon(request);
    printf("Removendo grupo %s...\n", grupo);

    read_from_daemon();
}

void listar_membros(char *grupo, ConcordiaRequest *request) {
    snprintf(request->command, COMMAND_SIZE, "listar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);

    send_to_deamon(request);
    printf("Listando membros do grupo %s...\n", grupo);

    printf("Group members: '\n'");
    read_from_daemon();
}

void adicionar_usuario(char *grupo, char *uid, ConcordiaRequest *request) {
    snprintf(request->command, COMMAND_SIZE, "adicionar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);
    snprintf(request->msg, MSG_SIZE, "%s", uid);

    send_to_deamon(request);
    printf("Adicionando usuário %s ao grupo %s...\n", uid, grupo);

    read_from_daemon();
}

//TODO verificar casos de falha
void remover_usuario(char *uid, char *grupo, ConcordiaRequest *request) {
    snprintf(request->command, COMMAND_SIZE, "remover-user");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);
    snprintf(request->msg, usersize, "%s", uid);

    send_to_deamon(request);
    printf("Removendo usuário %s do grupo %s...\n", uid, grupo);

    read_from_daemon();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [opções]\n", argv[0]);
        return EXIT_FAILURE;
    }

    ConcordiaRequest *request = malloc(sizeof(ConcordiaRequest));
    request->flag = GRUPO;
    request->pid = getpid();

    if (strcmp(argv[1], "criar") == 0 && argc == 3) {
        criar_grupo(argv[2], request);
    } 
    else if (strcmp(argv[1], "remover") == 0) {
        remover_grupo(argv[2], request);
    } 
    else if (strcmp(argv[1], "listar") == 0) {
        listar_membros(argv[2], request);
    } 
    else if (strcmp(argv[1], "adicionar") == 0 && argc == 4) {
        adicionar_usuario(argv[2], argv[3], request);
    } 
    else if (strcmp(argv[1], "remover-user") == 0 && argc == 4) {
        remover_usuario(argv[2], argv[3], request);
    } 
    else {
        fprintf(stderr, "Comando inválido ou argumentos incorretos.\n");
        return EXIT_FAILURE;
    }
    

    return EXIT_SUCCESS;
}