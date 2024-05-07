#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/structs.h"

void enviar_mensagem(const char *dest, const char *msg);
void listar_mensagens(int all);
void ler_mensagem(int mid);
void responder_mensagem(int mid, const char *msg);
void remover_mensagem(int mid);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [opções]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "enviar") == 0 && argc == 4) {
        enviar_mensagem(argv[2], argv[3]);
    } 
    else if (strcmp(argv[1], "listar") == 0) {
        int all = argc == 3 && strcmp(argv[2], "-a") == 0;
        listar_mensagens(all);
    } 
    else if (strcmp(argv[1], "ler") == 0 && argc == 3) {
        ler_mensagem(atoi(argv[2]));
    } 
    else if (strcmp(argv[1], "responder") == 0 && argc == 4) {
        responder_mensagem(atoi(argv[2]), argv[3]);
    } 
    else if (strcmp(argv[1], "remover") == 0 && argc == 3) {
        remover_mensagem(atoi(argv[2]));
    } 
    else {
        fprintf(stderr, "Comando inválido ou argumentos incorretos.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void enviar_mensagem(const char *dest, const char *msg) {
    printf("Enviando mensagem para %s: %s\n", dest, msg);
    // Implementar funcionalidade
}

void listar_mensagens(int all) {
    printf("Listando mensagens%s\n", all ? " (todas)" : "");
    // Implementar funcionalidade
}

void ler_mensagem(int mid) {
    printf("Lendo mensagem %d\n", mid);
    // Implementar funcionalidade
}

void responder_mensagem(int mid, const char *msg) {
    printf("Respondendo à mensagem %d: %s\n", mid, msg);
    // Implementar funcionalidade
}

void remover_mensagem(int mid) {
    printf("Removendo mensagem %d\n", mid);
    // Implementar funcionalidade
}

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
    printf("Menesagem enviada com sucesso");

}
