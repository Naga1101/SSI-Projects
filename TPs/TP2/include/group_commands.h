#ifndef GROUP_COMMANDS_H
#define GROUP_COMMANDS_H

// #define bin "/home/rui/Desktop/2324-G31/TPs/TP2/bin"
#define bin "/home/nuno/SSI/2324-G31/TPs/TP2/bin"

#include <grp.h>

void create_group (char *user, char *group, char* dirPath, int pid);
void remove_group(char *user, char *group, char *groupFolderPath, int pid);
void add_user_to_group(char *user, char *grupo, char *user_to_add, char *groupsFolderName, int pid);
void remove_user_from_group(char *user, char *group, char *user_to_remove, char *groupsFolderName, int pid);
void listar_membros_grupo(char *user, char *dest, char *groupsFolderName, int pid);
void exec_setfacl(char *path, char *group);
int remove_user_from_system_group(const char *user_to_remove, const char *group);
int user_in_group(char *user, struct group *grp);

#endif