#ifndef GROUP_COMMANDS_H
#define GROUP_COMMANDS_H

// #define bin "/home/rui/Desktop/2324-G31/TPs/TP2/bin"
#define bin "/home/nuno/SSI/2324-G31/TPs/TP2/bin"

void create_group (char *user, char *group, char* dirPath);
void remove_group(char *user, char *group, char *groupFolderPath);
void add_user_to_group(char *user, char *grupo, char *user_to_add, char *groupsFolderName);
void remove_user_from_group(char *user, char *group, char *user_to_remove, char *groupsFolderName);
void listar_membros_grupo(char *user, char *dest, char *groupsFolderName, int pid);
void returnListToClient(int pid, char *message);
void enviar_mensagem_grupo(char *user, char *dest, char *msg, char *groupsFolderName);

#endif