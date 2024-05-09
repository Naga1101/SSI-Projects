#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>

#include "../include/struct.h"
#include "../include/user_commands.h"

void activate_user(char* user, char* folderPath){
    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, user);

    // syslog(LOG_NOTICE, "User: %s\n", user);
    // syslog(LOG_NOTICE, "User converted to uid: %u\n", uid);
    // syslog(LOG_NOTICE, "Folder path before: %s\n", folderPath);
    // syslog(LOG_NOTICE, "Folder path after: %s\n", userFolderPath);

    struct passwd *pwd = getpwnam(user);
    if (pwd == NULL) {
        syslog(LOG_ERR, "Failed to retrieve UID for user: %s\n", user);
        exit(EXIT_FAILURE);
    }

    uid_t uid = pwd->pw_uid;

    if (mkdir(userFolderPath, 0500) == -1) { // Adjusted permission to grant group write access
        syslog(LOG_ERR, "Failed to create the directory: %s and the path is: %s\n", strerror(errno), userFolderPath);
        exit(EXIT_FAILURE);
    }

    if (chown(userFolderPath, uid, -1) == -1) {
        syslog(LOG_ERR, "Failed to change ownership of the directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Grant group read, write, and execute permissions
    if (chmod(userFolderPath, 0570) == -1) {
        syslog(LOG_ERR, "Failed to change permissions of the directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    syslog(LOG_NOTICE, "Folder created successfully on path: %s\n", userFolderPath);
}