#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/struct.h"

void criar_grupo(const char *nome);
void remover_grupo();
void listar_membros();
void adicionar_usuario(const char *uid);
void remover_usuario(const char *uid);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [opções]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "criar") == 0 && argc == 3) {
        criar_grupo(argv[2]);
    } else if (strcmp(argv[1], "remover") == 0) {
        remover_grupo();
    } else if (strcmp(argv[1], "listar") == 0) {
        listar_membros();
    } else if (strcmp(argv[1], "adicionar") == 0 && argc == 3) {
        adicionar_usuario(argv[2]);
    } else if (strcmp(argv[1], "remover-usuario") == 0 && argc == 3) {
        remover_usuario(argv[2]);
    } else {
        fprintf(stderr, "Comando inválido ou argumentos incorretos.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void criar_grupo(const char *nome) {
    printf("Criando grupo %s\n", nome);
    // Implementar funcionalidade
}

void remover_grupo() {
    printf("Removendo grupo\n");
    // Implementar funcionalidade
}

void listar_membros() {
    printf("Listando membros do grupo\n");
    // Implementar funcionalidade
}

void adicionar_usuario(const char *uid) {
    printf("Adicionando usuário %s ao grupo\n", uid);
    // Implementar funcionalidade
}

void remover_usuario(const char *uid) {
    printf("Removendo usuário %s do grupo\n", uid);
    // Implementar funcionalidade
}
