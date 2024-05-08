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

// Função para obter o nome do usuário atual
char* obter_usuario_atual() {
    char* usuario = getenv("USER");
    return usuario;
}


void enviar_msg_grupo(char *grupo, char *msg, ConcordiaRequest *request) {
    printf("Enviando mensagem ao grupo %s\n", grupo);

    snprintf(request->command, COMMAND_SIZE, "enviar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);
    snprintf(request->msg, MSG_SIZE, "%s", msg);

    send_to_deamon(request);
}

void criar_grupo(char *nome, ConcordiaRequest *request) {
    printf("Criando grupo %s\n", nome);

    snprintf(request->command, COMMAND_SIZE, "criar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", nome);

    send_to_deamon(request);
}

void remover_grupo(char *grupo, ConcordiaRequest *request) {
    printf("Removendo grupo %s\n", grupo);

    snprintf(request->command, COMMAND_SIZE, "remover");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);

    send_to_deamon(request);
}

void listar_membros(char *grupo, ConcordiaRequest *request) {
    printf("Listando membros do grupo %s\n", grupo);

    snprintf(request->command, COMMAND_SIZE, "listar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);

    send_to_deamon(request);
}

void adicionar_usuario(char *grupo, char *uid, ConcordiaRequest *request) {
    printf("Adicionando usuário %s ao grupo %s\n", uid, grupo);

    snprintf(request->command, COMMAND_SIZE, "adicionar");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", grupo);
    snprintf(request->msg, MSG_SIZE, "%s", uid);

    send_to_deamon(request);
}

void remover_usuario(char *uid, ConcordiaRequest *request) {
    printf("Removendo usuário %s\n", uid);

    snprintf(request->command, COMMAND_SIZE, "remover");
    snprintf(request->user, usersize, "%s", obter_usuario_atual());
    snprintf(request->dest, usersize, "%s", uid);

    send_to_deamon(request);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [opções]\n", argv[0]);
        return EXIT_FAILURE;
    }

    ConcordiaRequest *request = malloc(sizeof(ConcordiaRequest));
    request->flag = GRUPO;

    if (strcmp(argv[1], "criar") == 0 && argc == 3) {
        criar_grupo(argv[2], request);
    } else if (strcmp(argv[1], "remover") == 0) {
        remover_grupo(argv[2], request);
    } else if (strcmp(argv[1], "listar") == 0) {
        listar_membros(argv[2], request);
    } else if (strcmp(argv[1], "adicionar") == 0 && argc == 3) {
        adicionar_usuario(argv[2], argv[3], request);
    } else if (strcmp(argv[1], "remover-usuario") == 0 && argc == 3) {
        remover_usuario(argv[2], request);
    } else {
        fprintf(stderr, "Comando inválido ou argumentos incorretos.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}