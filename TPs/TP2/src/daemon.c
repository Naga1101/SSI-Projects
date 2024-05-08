#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "./include/struct.h"
#include "./command_handler.c"

#define mainFolderName "./messages"
#define usersFolderName "./messages/users"
#define groupsFolderName "./messages/groups"


// Function prototypes
void signal_handler(int sig);
void process_incoming_messages();
void store_message(const char *message);
void deliver_messages();
void create_daemon();

int main() {
    // Configuração do tratamento de sinais
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // criação das directorias que vão armazenar as mensagens
    if (mkdir(mainFolderName, 0777) == 0) {
        printf("Folder created successfully.\n");
    } else {
        printf("Unable to create folder.\n");
    }

    if (mkdir(usersFolderName, 0777) == 0) {
        printf("Folder created successfully.\n");
    } else {
        printf("Unable to create folder.\n");
    }

    if (mkdir(groupsFolderName, 0777) == 0) {
        printf("Folder created successfully.\n");
    } else {
        printf("Unable to create folder.\n");
    }

    // Inicializa o daemon
    create_daemon();

    int fd;

    //apaga por causa dos testes
    unlink(FIFO);

    //criar o fifo
    if(mkfifo(FIFO, 0666) != 0){
        perror("Error creating fifo: ");
        exit(EXIT_FAILURE);
    }

    //abre o fifo para ler os pedidos
    fd = open(FIFO, O_RDONLY);
    if(fd == -1){
        perror("Error opening fifo");
        exit(EXIT_FAILURE);
    }

    // Loop principal do daemon
    while (1) {
        process_incoming_messages(fd);
        deliver_messages();
        sleep(1);  // Substitua por uma espera mais eficiente se necessário
    }

    exit(EXIT_SUCCESS);
}

void create_daemon() {
    pid_t pid;

    // Primeiro fork
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Cria uma nova sessão
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Segundo fork
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Altera o diretório de trabalho
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    // Redefine a máscara de arquivo
    umask(0);

    // Fecha todos os descritores de arquivo
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
}

void process_incoming_messages(int fifo_fd) {
    ConcordiaRequest request;
    int bytes_read;
    pid_t pid;

    // First fork
    pid = fork();

    if (pid == 0) {
        // Read data from FIFO
        bytes_read = read(fifo_fd, &request, sizeof(request));
        if (bytes_read > 0) {
            switch (request.flag)
            {
            case MENSAGEM:
                handle_user_message(request, usersFolderName);
                break;
            case GRUPO:
                handle_group_message(request, groupsFolderName);
                break;
            case USER:
                handle_user_command(request);
                break;
            default:
                break;
            }
        }
        exit(EXIT_SUCCESS);
    }
}

void deliver_messages() {
    // Implemente a lógica de entrega de mensagens
}

void store_message(const char *message) {
    // Implemente a lógica de armazenamento de mensagens baseada no tipo
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("Daemon terminando...\n");
        exit(EXIT_SUCCESS);
    }
}
