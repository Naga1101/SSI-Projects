#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

#include "../include/struct.h"
#include "../include/user_commands.h"

void activate_user(char* user, char* folderPath){
    char userFolderPath[35];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, user);

    if (mkdir(userFolderPath, 0700) == -1) {
        syslog(LOG_ERR, "Failed to create the directory: %s e %s\n", strerror(errno), userFolderPath);
        exit(EXIT_FAILURE);
    }

    if (execlp("chown", "chown", user, userFolderPath, NULL) == -1) {
        syslog(LOG_ERR, "Failed to change ownership of the directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}