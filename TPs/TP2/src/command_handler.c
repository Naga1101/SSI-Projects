#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include "../include/command_handler.h"
#include "../include/message_commands.h"
#include "../include/group_commands.h"
#include "../include/user_commands.h"

void handle_user_message(ConcordiaRequest request, char* usersFolderName, char* groupsFolderPath){
    syslog(LOG_NOTICE, "Handler da flag Command: %s\n", request.command);
    
    if (strcmp(request.command, "enviar") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag Dest: %s\n", request.dest);
        //syslog(LOG_NOTICE, "Handler da flag Msg: %s\n", request.msg);
        enviar_message(request, usersFolderName, groupsFolderPath);

    } else if (strcmp(request.command, "listar") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag All_Mid: %d\n", request.all_mid);
        listar_message(request, usersFolderName, groupsFolderPath);

    } else if (strcmp(request.command, "ler") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag All_Mid: %d\n", request.all_mid);
        ler_message(request, usersFolderName, groupsFolderPath);

    } else if (strcmp(request.command, "responder") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag All_Mid: %d\n", request.all_mid);
        //syslog(LOG_NOTICE, "Handler da flag Msg: %s\n", request.msg);
        responder_message(request, usersFolderName, groupsFolderPath);

    } else if (strcmp(request.command, "remover") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag User: %s\n", request.user);
        //syslog(LOG_NOTICE, "Handler da flag All_Mid: %d\n", request.all_mid);
        remover_message(request, usersFolderName, groupsFolderPath);

    } else {
        printf("Unknown command: %s\n", request.command);
    }
}

void handle_group_message(ConcordiaRequest request, char* groupsFolderName){
    syslog(LOG_NOTICE, "Handler da flag Command: %d\n", request.flag);
    if (strcmp(request.command, "criar") == 0) {
        create_group(request.user, request.dest, groupsFolderName, request.pid);

    } else if (strcmp(request.command, "remover") == 0) {
        remove_group(request.user, request.dest, groupsFolderName, request.pid);

    } else if (strcmp(request.command, "listar") == 0) {
        listar_membros_grupo(request.user, request.dest, groupsFolderName, request.pid);

    } else if (strcmp(request.command, "adicionar") == 0) {
        add_user_to_group(request.user, request.dest, request.msg, groupsFolderName, request.pid);

    } else if (strcmp(request.command, "remover-user") == 0) {
        remove_user_from_group(request.user, request.dest, request.msg, groupsFolderName, request.pid);

    } else {
        printf("Unknown command: %s\n", request.command);
    }
}

void handle_user_command(ConcordiaRequest request, char* usersFolderName, char *groupsFolderName){
    syslog(LOG_NOTICE, "Handler da flag Command: %d\n", request.flag);

    if (strcmp(request.command, "ativar") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag User: %s\n", request.user);
        activate_user(request.user, usersFolderName, request.pid);

    } else if (strcmp(request.command, "desativar") == 0) {
        //syslog(LOG_NOTICE, "Handler da flag User: %s\n", request.user);
        deactivate_user(request.user, usersFolderName, request.pid, groupsFolderName);

    } else {
        printf("Unknown command: %s\n", request.command);
    }
}