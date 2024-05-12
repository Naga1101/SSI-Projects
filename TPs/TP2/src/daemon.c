#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <pwd.h>

// grep -a 'concordia_daemon' /var/log/syslog
// ps -ef 
// pkill -f ./daemon
// sudo truncate -s 0 /var/log/syslog
// rm -r /usr/share/concordiaINBOX

#include "../include/struct.h"
#include "../include/command_handler.h"

#define mainFolderName "/usr/share/concordiaINBOX"
#define usersFolderName "/usr/share/concordiaINBOX/users"
#define groupsFolderName "/usr/share/concordiaINBOX/groups"

void skeleton_daemon() {
    pid_t pid;

    pid = fork();

    // erro
    if (pid < 0)
        exit(EXIT_FAILURE);

    // pai termina
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // filho session leader
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGHUP, SIG_IGN);


    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    chdir("/");

    /// fechar os FD
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close(x);
    }

    // abrir o ficheiro de logs
    openlog("concordia_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Concordia daemon started successfully.");
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
                syslog(LOG_NOTICE, "MENSAGEM");
                syslog(LOG_NOTICE, "pid que chegou %d", request.pid);

                handle_user_message(request, usersFolderName, groupsFolderName);
                break;
            case GRUPO:
                syslog(LOG_NOTICE, "GRUPO");
                                syslog(LOG_NOTICE, "pid que chegou %d", request.pid);

                syslog(LOG_NOTICE, "Command: %s\n", request.command);
                syslog(LOG_NOTICE, "user: %s\n", request.user);
                syslog(LOG_NOTICE, "dest: %s\n", request.dest);
                handle_group_message(request, groupsFolderName);
                break;
            case USER:
                syslog(LOG_NOTICE, "USER");
                                syslog(LOG_NOTICE, "pid que chegou %d", request.pid);

                syslog(LOG_NOTICE, "Command: %s\n", request.command);
                handle_user_command(request, usersFolderName);
                break;
            default:
                break;
            }
        }
        exit(EXIT_SUCCESS);
    }
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("Daemon terminando...\n");
        exit(EXIT_SUCCESS);
    }
}

int main() {
    // Configuração do tratamento de sinais
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Inicializa o daemon
    skeleton_daemon();

    int fd;

    // apaga por causa dos testes
    unlink(FIFO);

    //criar o fifo
    if(mkfifo(FIFO, 0666) != 0){
        perror("Error creating fifo: ");
        exit(EXIT_FAILURE);
    }

    // criação das directorias que vão armazenar as mensagens
    if (mkdir(mainFolderName, 0755) == 0) {
        syslog(LOG_NOTICE, "Folder created successfully.\n");
    } else {
        syslog(LOG_NOTICE, "Folder not created successfully.\n");
    }

    if (mkdir(usersFolderName, 0755) == 0) {
        syslog(LOG_NOTICE, "Folder created successfully.\n");
    } else {
        syslog(LOG_NOTICE, "Folder not created successfully.\n");
    }

    if (mkdir(groupsFolderName, 0755) == 0) {
        syslog(LOG_NOTICE, "Folder created successfully.\n");
    } else {
        syslog(LOG_NOTICE, "Folder not created successfully.\n");
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
        sleep(1);
    }

    exit(EXIT_SUCCESS);
}