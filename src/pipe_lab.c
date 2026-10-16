#include "pipe_lab.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int task_one_way(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    fflush(stdout); /* ป้องกัน stdout buffer ถูกก๊อปปี้ไปที่ child */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(fd[1]);
        char buffer[64];
        ssize_t n = read(fd[0], buffer, sizeof(buffer) - 1);
        if (n >= 0) {
            buffer[n] = '\0';
        }
        printf("child received: %s\n", buffer);
        close(fd[0]);
        exit(0);
    }

    close(fd[0]);
    write(fd[1], "ping", 4);
    close(fd[1]);
    waitpid(pid, NULL, 0);
    printf("parent: sent ping\n");
    
    return 0;
}

int task_two_way(void) {
    int p2c[2], c2p[2];
    if (pipe(p2c) == -1 || pipe(c2p) == -1) { perror("pipe"); return 1; }

    fflush(stdout); /* ป้องกัน stdout buffer ถูกก๊อปปี้ไปที่ child */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(p2c[1]);
        close(c2p[0]);

        int value;
        read(p2c[0], &value, sizeof(value));
        
        value *= 2;
        printf("child computed: %d\n", value);
        
        write(c2p[1], &value, sizeof(value));

        close(p2c[0]);
        close(c2p[1]);
        exit(0);
    }

    close(p2c[0]);
    close(c2p[1]);

    int send_val = 21;
    write(p2c[1], &send_val, sizeof(send_val));
    
    int recv_val;
    read(c2p[0], &recv_val, sizeof(recv_val));
    printf("parent received: %d\n", recv_val);
    
    close(p2c[1]);
    close(c2p[0]);
    waitpid(pid, NULL, 0);
    
    return 0;
}

int task_eof(void) {
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    fflush(stdout); /* ป้องกัน stdout buffer ถูกก๊อปปี้ไปที่ child */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(fd[1]);
        
        char buf[16];
        int total_bytes = 0;
        ssize_t bytes_read;
        
        while ((bytes_read = read(fd[0], buf, sizeof(buf))) > 0) {
            total_bytes += bytes_read;
        }
        
        printf("child: got EOF after %d bytes\n", total_bytes);
        close(fd[0]);
        exit(0);
    }

    close(fd[0]);
    write(fd[1], "hello", 5);
    close(fd[1]);
    waitpid(pid, NULL, 0);
    
    return 0;
}

int task_broken_pipe(void) {
    int data_fd[2];
    int sync_fd[2];
    if (pipe(data_fd) == -1 || pipe(sync_fd) == -1) { perror("pipe"); return 1; }

    fflush(stdout); /* ป้องกัน stdout buffer ถูกก๊อปปี้ไปที่ child */
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(data_fd[0]);
        close(data_fd[1]);
        close(sync_fd[0]);

        char ack = '1';
        write(sync_fd[1], &ack, 1);
        close(sync_fd[1]);
        
        exit(0);
    }

    close(sync_fd[1]);

    char dummy;
    read(sync_fd[0], &dummy, 1);
    close(sync_fd[0]);
    close(data_fd[0]);

    signal(SIGPIPE, SIG_IGN);

    char data_byte = 'X';
    ssize_t res = write(data_fd[1], &data_byte, 1);
    
    if (res == -1 && errno == EPIPE) {
        printf("parent: broken pipe detected\n");
    }

    close(data_fd[1]);
    waitpid(pid, NULL, 0);
    
    return 0;
}

int main(void) {
    if (task_one_way() != 0)     { fprintf(stderr, "task_one_way failed\n");     return 1; }
    if (task_two_way() != 0)     { fprintf(stderr, "task_two_way failed\n");     return 1; }
    if (task_eof() != 0)         { fprintf(stderr, "task_eof failed\n");         return 1; }
    if (task_broken_pipe() != 0) { fprintf(stderr, "task_broken_pipe failed\n"); return 1; }
    printf("all tasks done\n");
    return 0;
}