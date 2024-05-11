#ifndef MESSAGE_COMMANDS_H
#define MESSAGE_COMMANDS_H

#include "./struct.h"

void enviar_message(ConcordiaRequest request, char* folderPath);
void ler_message(ConcordiaRequest request, char* uFolderPath, char* gFolderPath);
void responder_message(ConcordiaRequest request, char* uFolderPath, char* gFolderPath);
void remover_message(ConcordiaRequest request, char* uFolderPath, char* gFolderPath);
void listar_message(ConcordiaRequest request, char* ufolderPath, char* gfolderPath);

#endif 