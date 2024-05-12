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
#include <grp.h>
#include <dirent.h>

#include "../include/struct.h"
#include "../include/user_commands.h"
#include "../include/utils.h"
#include "../include/group_commands.h"

void activate_user(char* user, char* folderPath, int pid){
    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, user);

    struct passwd *pwd = getpwnam(user);
    if (pwd == NULL) {
        syslog(LOG_ERR, "Failed to get UID for user: %s\n", user);
        exit(EXIT_FAILURE);
    }

    uid_t uid = pwd->pw_uid;

    if (mkdir(userFolderPath, 0750) == -1) {
        char *msg = "user already registerd";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "Failed to create the directory: %s and the path is: %s\n", strerror(errno), userFolderPath);
        exit(EXIT_FAILURE);
    }

    if (chown(userFolderPath, 0, uid) == -1) {
        syslog(LOG_ERR, "Failed to set ownership of the directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (chmod(userFolderPath, 0750) == -1) {
        syslog(LOG_ERR, "Failed to change permissions of the directory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    syslog(LOG_NOTICE, "Folder created successfully on path: %s\n", userFolderPath);

    char confirmation[128];
    snprintf(confirmation, sizeof(confirmation), "Adicionado utilizador %s com sucesso!", user);

    returnListToClient(pid, confirmation);
}

int verify_user(char *path, uid_t uid, int pid) {
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        char *msg = "User not registerd";
        returnListToClient(pid, msg);
        syslog(LOG_ERR, "Failed to get path stats: %s\n", strerror(errno));
        return -1;
    }

    // verificar se tem read perms
    if ((statbuf.st_mode & S_IRGRP)) {
        return 1;
    }

    return 0;
}


void deactivate_user(char *user, char *folderPath, int pid, char *groupsFolderName) {
    char userFolderPath[100];
    snprintf(userFolderPath, sizeof(userFolderPath), "%s/%s", folderPath, user);

    struct passwd *pwd = getpwnam(user);
    if (pwd == NULL) {
        syslog(LOG_ERR, "Failed to retrieve user info: %s\n", user);
        exit(EXIT_FAILURE);
    }

    uid_t uid = pwd->pw_uid;

    // tenho de ver isto
    if (verify_user(userFolderPath, uid, pid) != 1) {
        syslog(LOG_ERR, "User does not have sufficient permissions to delete the directory: %s\n", userFolderPath);
        exit(EXIT_FAILURE);
    }

    if (remove_directory(userFolderPath) != 0) {
        syslog(LOG_ERR, "Failed to remove the directory: %s\n", userFolderPath);
        exit(EXIT_FAILURE);
    }


    DIR *dir = opendir(groupsFolderName);
    if (dir == NULL) {
        syslog(LOG_ERR, "Failed to open directory: %s\n", groupsFolderName);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char *groupName = entry->d_name;
            if (strcmp(groupName, ".") != 0 && strcmp(groupName, "..") != 0) {
                struct group *grp = getgrnam(groupName);
                if (grp && user_in_group(user, grp)) {
                    if (remove_user_from_system_group(user, groupName) != 0) {
                        syslog(LOG_ERR, "Failed to remove user %s from group %s\n", user, groupName);
                        continue;
                    }
                    syslog(LOG_NOTICE, "User %s removed from group %s successfully.\n", user, groupName);
                }
            }
        }
    }

    closedir(dir);

    syslog(LOG_NOTICE, "User directory removed successfully: %s\n", userFolderPath);

    char confirmation[128];
    snprintf(confirmation, sizeof(confirmation), "Removido utilizador %s com sucesso!", user);

    returnListToClient(pid, confirmation);
}