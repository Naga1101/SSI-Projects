#ifndef STRUCTS_H
#define STRUCTS_H

#define FIFO "/tmp/concordia_fifo"
#define MSG_SIZE 512
#define COMMAND_SIZE 16
#define usersize 16

typedef enum{
    MENSAGEM,
    GRUPO,
    USER
} FLAG;

typedef struct {
    FLAG flag;
    char command[17];
    char user[16];
    char dest[16];
    char msg[MSG_SIZE];
    int all_mid;
    int pid;
} ConcordiaRequest;

#endif /* STRUCTS_H */

// syslog(LOG_NOTICE, "Handler da flag Command: %d\n", request.flag);
// syslog(LOG_NOTICE, "Handler da flag Command: %s\n", request.command);
// syslog(LOG_NOTICE, "Handler da flag User: %s\n", request.user);
// syslog(LOG_NOTICE, "Handler da flag Dest: %s\n", request.dest);
// syslog(LOG_NOTICE, "Handler da flag Msg: %s\n", request.msg);
// syslog(LOG_NOTICE, "Handler da flag All_Mid: %d\n", request.all_mid);