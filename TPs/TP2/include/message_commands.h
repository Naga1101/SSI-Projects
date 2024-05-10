#ifndef MESSAGE_COMMANDS_H
#define MESSAGE_COMMANDS_H

#include "./struct.h"

void enviar_message(ConcordiaRequest request, char* folderPath);
void ler_message(ConcordiaRequest request, char* folderPath);
void responder_message(ConcordiaRequest request, char* folderPath);

#endif /* MESSAGE_COMMANDS_H */