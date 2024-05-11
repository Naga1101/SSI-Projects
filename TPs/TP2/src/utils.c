#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <string.h> 

#include "../include/utils.h"

void returnListToClient(int pid, char *message){
    char fifoName[55];
    sprintf(fifoName, "/home/nuno/SSI/2324-G31/TPs/TP2/bin/fifo_%d", pid);

    syslog(LOG_NOTICE, "fifoname: %s", fifoName);

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1){
        syslog(LOG_ERR, "Error opening return fifo: %s \n", strerror(errno));
    }

    syslog(LOG_NOTICE, "%s", message);

    // enviar lista
    if (write(fd, message, strlen(message)+1) == -1) {
        perror("Error writing to FIFO");
    }
    close(fd);
}

static int remove_entry(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int remove_directory(const char *dir_path) {
    return nftw(dir_path, remove_entry, 64, FTW_DEPTH | FTW_PHYS);
}