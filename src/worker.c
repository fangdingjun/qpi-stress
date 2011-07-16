/*
 * Filename: worker.c
 * Author: fangdingjun@gmail.com
 * License: GPLv2
 * Date: 2011-7-10
 *
 */
#include <stdio.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#define fill_mem(p,d,s) memset(p,d,s)
#include "qpi.h"

int read_mem(char *p, int data, size_t s);

int worker(int num)
{

    char *local, *remote;
    //int id;
    int k;

    fprintf(stderr, "\tbind process %d  to cpu %ld\n", num,
            cpu_num - num - 1);
    if (bind_to_cpu(cpu_num - num - 1) < 0) {
        //(*total_e)++;
        kill(0, SIGUSR1);
        exit(-1);
    }
    local = (char *)create_shm(key[num], shm_size);
    if (local == NULL) {
        kill(0, SIGUSR1);
        exit(-1);
    }
    k = (num % 2 == 0 ? num + 1 : num - 1);

    remote = (char *)wait_remote_shm(key[k], shm_size);
    if (remote == NULL) {
        kill(0, SIGUSR1);
        exit(-1);
    }
    while (1) {
        sem_wait(sem[num]);
        fill_mem(local, data, shm_size);
        sem_wait(write_lock);
        (*total_w)++;
        sem_post(write_lock);
        sem_post(sem[num]);
        sem_wait(sem[k]);
        if (read_mem(remote, data, shm_size) < 0) {
            sem_post(sem[k]);
            (*total_e)++;
            kill(0, SIGUSR1);
            exit(-1);
        }
        sem_wait(read_lock);
        (*total_r)++;
        sem_post(read_lock);
        sem_post(sem[k]);
    }
}

int read_mem(char *p, int data, size_t s)
{
    int buf_size = 4 << 20;	// 4M
    char buf[buf_size];
    int i;
    int loop;
    int remain = 0;
    char *p1;
    memset(buf, data, buf_size);
    loop = s / buf_size;
    if (s % buf_size != 0) {
        remain = s % buf_size;
    }

    p1 = p;
    for (i = 0; i < loop; i++) {
        if (memcmp(p1, buf, buf_size)) {	// data not equal
            return -1;
        }
        p1 += buf_size;
    }
    if (remain) {
        if (memcmp(p1, buf, remain)) {
            return -1;
        }
    }
    return 0;
}

int do_fork(int m)
{
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        //kill(0,SIGUSR1);
        return -1;
        //sleep(1);
    } else if (pid == 0) {	// child
        worker(m);
        exit(0);
    }
    //parent
    return 0;
}

