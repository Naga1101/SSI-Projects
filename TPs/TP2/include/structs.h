#ifndef STRUCTS_H
#define STRUCTS_H

#define FIFO "/tmp/concordia_fifo"

typedef enum{
    MENSAGEM,
    GRUPO,
    USER
} FLAG;

typedef struct {
    FLAG flag;
    char *command;
    char *user;
    char *msg;
} ConcordiaRequest;

#endif /* STRUCTS_H */