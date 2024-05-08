#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./include/struct.h"

void ativar_usuario();
void desativar_usuario();

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "ativar") == 0) {
        ativar_usuario();
    } else if (strcmp(argv[1], "desativar") == 0) {
        desativar_usuario();
    } else {
        fprintf(stderr, "Comando inválido.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void ativar_usuario() {
    printf("Ativando usuário\n");
    // Implementar funcionalidade
}

void desativar_usuario() {
    printf("Desativando usuário\n");
    // Implementar funcionalidade
}
