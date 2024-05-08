#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "./struct.h"

void handle_user_message(ConcordiaRequest request, char* usersFolderName);
void handle_group_message(ConcordiaRequest request, char* groupsFolderName);
void handle_user_command(ConcordiaRequest request);


#endif /* COMMAND_HANDLER_H */