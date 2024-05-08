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
    printf("Mensagem enviada com sucesso.'\n'");

}

void enviar_mensagem(char *dest, char *msg, ConcordiaRequest *request) {
    request->flag = MENSAGEM;
    strncpy(request->command,"enviar",10);
    strncpy(request->dest,dest,16);
    strncpy(request->msg,msg,512);
    
    printf("flag: %d, comando: %s, dest: %s, msg: %s", request->flag, request->command, request->dest, request->msg);

    send_to_deamon(request);    
    printf("Enviando mensagem para %s: %s\n", dest, msg);
}

void listar_mensagens(int all, ConcordiaRequest *request) {
    request->flag =  MENSAGEM;
    strncpy(request->command,"listar",10);
    request->all_mid = all;

    send_to_deamon(request);
    printf("Listando mensagens%s\n", all ? " (todas)" : "");
}

void ler_mensagem(int mid, ConcordiaRequest *request) {
    request->flag = MENSAGEM;
    strncpy(request->command,"ler",10);
    request->all_mid = mid;

    send_to_deamon(request);
    printf("Lendo mensagem %d\n", mid);
}

void responder_mensagem(int mid, char *msg, ConcordiaRequest *request) {
    request->flag = MENSAGEM;
    strncpy(request->command,"responder",10);
    request->all_mid = mid;
    strncpy(request->msg,msg,512);

    send_to_deamon(request);
    printf("Respondendo à mensagem %d: %s\n", mid, msg);
}

void remover_mensagem(int mid, ConcordiaRequest *request) {
    request->flag = MENSAGEM;
    strncpy(request->command,"remover",10);
    request->all_mid = mid;

    send_to_deamon(request);
    printf("Removendo mensagem %d\n", mid);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [opções]\n", argv[0]);
        return EXIT_FAILURE;
    }

    ConcordiaRequest *request = malloc(sizeof(ConcordiaRequest));
    if (!request) {
        perror("Falha ao alocar memória para o request");
        return EXIT_FAILURE;
    }

    request->flag = 0;
    request->command[0] = '\0';
    request->user[0] = '\0';
    request->dest[0] = '\0';
    request->msg[0] = '\0';
    request->all_mid = 0;

    if (strcmp(argv[1], "enviar") == 0 && argc == 4) {
        enviar_mensagem(argv[2], argv[3], request);
    } 
    else if (strcmp(argv[1], "listar") == 0) {
        int all = (argc == 3 && strcmp(argv[2], "-a") == 0) ? 1 : 0;
        listar_mensagens(all, request);
    } 
    else if (strcmp(argv[1], "ler") == 0 && argc == 3) {
        ler_mensagem(atoi(argv[2]), request);
    } 
    else if (strcmp(argv[1], "responder") == 0 && argc == 4) {
        responder_mensagem(atoi(argv[2]), argv[3], request);
    } 
    else if (strcmp(argv[1], "remover") == 0 && argc == 3) {
        remover_mensagem(atoi(argv[2]), request);
    } 
    else {
        fprintf(stderr, "Comando inválido ou argumentos incorretos.\n");
        return EXIT_FAILURE;
    }

    free(request);

    return EXIT_SUCCESS;
}