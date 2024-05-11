#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "./struct.h"

void handle_user_message(ConcordiaRequest request, char* usersFolderName, char* groupsFolderPath);
void handle_group_message(ConcordiaRequest request, char* groupsFolderName);
void handle_user_command(ConcordiaRequest request, char* usersFolderName);

#endif 