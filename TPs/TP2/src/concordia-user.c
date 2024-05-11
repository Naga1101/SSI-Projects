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
    printf("Mensagem enviada com sucesso\n");
}

// Função para obter o nome do usuário atual
char *obter_usuario_atual() {
    char *user = getenv("USER");
    return user;
}

void ativar_usuario(ConcordiaRequest *request) {
    printf("Ativando usuário\n");

    request->flag = USER;
    snprintf(request->command, COMMAND_SIZE, "ativar");
    char *user = obter_usuario_atual();
    snprintf(request->user, usersize,"%s", user);
    printf("%s'\n'", user);

    send_to_deamon(request);
}

void desativar_usuario(ConcordiaRequest *request) {
    printf("Desativando usuário\n");

    request->flag = USER;
    snprintf(request->command, COMMAND_SIZE, "desativar");
    char *user = obter_usuario_atual();
    snprintf(request->user, usersize,"%s", user);

    send_to_deamon(request);
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
