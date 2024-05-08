#ifndef STRUCTS_H
#define STRUCTS_H

#define FIFO "/tmp/concordia_fifo"
#define MSG_SIZE 512
#define COMMAND_SIZE 10
#define usersize 16

typedef enum{
    MENSAGEM,
    GRUPO,
    USER
} FLAG;

typedef struct {
    FLAG flag;
    char command[10];
    char user[16];
    char dest[16];
    char msg[MSG_SIZE];
    int all_mid;
} ConcordiaRequest;

#endif /* STRUCTS_H */