#ifndef USER_COMMANDS_H
#define USER_COMMANDS_H

#include "./struct.h"

void activate_user(char* user, char* folderPath, int pid);
void deactivate_user(char *user, char *folderPath, int pid);

#endif 