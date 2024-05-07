#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>

//grep -a 'concordia_daemon' /var/log/syslog
// ps -ef 

#include "../include/structs.h"

static void skeleton_daemon() {
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler here
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the second parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close(x);
    }

    /* Open the log file */
    openlog("concordia_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Concordia daemon started successfully.");
}

void process_incoming_messages(int fd) {
    ConcordiaRequest request;
    int bytes_read;

    bytes_read = read(fd, &request, sizeof(ConcordiaRequest));
    if(bytes_read < 0){
        syslog(LOG_ERR, "Erro na leitura de requests\n");
    }
    
    switch (request.flag)
    {
    case MENSAGEM:
        syslog(LOG_NOTICE, "MENSAGEM");
        break;

    case GRUPO:
        syslog(LOG_NOTICE, "GRUPO");
        break;

    case USER:
        syslog(LOG_NOTICE, "USER");
        break;
    
    default:
        break;
    }
}

int main()
{
    skeleton_daemon();

    int fd;

    //apagar o fifo criado previamento
    unlink(FIFO);

    // criar o fifo
    if(mkfifo(FIFO, 0666) != 0) {
        syslog(LOG_ERR, "Failed to create deamon fifo");
        exit(EXIT_FAILURE);
    } 

    // abrir o fifo em leitura para 
    fd = open(FIFO, O_RDONLY);
    if (fd == -1){
        syslog(LOG_ERR, "Failed to open deamon fifo");
    }

    while (1) {
        process_incoming_messages(fd);
    }

    syslog(LOG_NOTICE, "Concordia daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
