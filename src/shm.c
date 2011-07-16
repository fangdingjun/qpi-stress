/*
 * Filename: report.c
 * Author: fangdingjun@gmail.com
 * License: GPLv2
 * Date: 2011-7-10
 *
 */
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "qpi.h"

void *create_shm(key_t k, size_t s)
{
    int id;
    void *p;
    id = shmget(k, s, IPC_CREAT | 0644);
    if (id == -1) {
        perror("shmget");
        return NULL;
    }
    p = shmat(id, NULL, 0);
    if (p == (void *)-1) {
        perror("shmat");
        return NULL;
    }
    return p;
}

int init_one_key(key_t * k, char *name)
{
    FILE *fp = fopen(name, "w");
    fclose(fp);
    *k = ftok(name, 0);
    return 0;
}

int init_shm_key(key_t * k, int num)
{
    int i;
    char fn_r[] = "/dev/shm/qpitestxxxxx";
    char fname[256];
    for (i = 0; i < num; i++) {
        sprintf(fname, "%s%d", fn_r, i);
        FILE *fp = fopen(fname, "w");
        fclose(fp);
        k[i] = ftok(fname, 0);
    }
    return 0;
}

int destroy_shm(key_t * k, int num)
{
    int i;
    int id;
    char fn_r[] = "/dev/shm/qpitestxxxxx";
    char fname[256];
    for (i = 0; i < num; i++) {
        id = shmget(k[i], shm_size, 0644);
        if (id != -1) {
            shmctl(id, IPC_RMID, NULL);
        }
        sprintf(fname, "%s%d", fn_r, i);
        unlink(fname);
    }
    return 0;
}

int destroy_sem(sem_t ** s, int num)
{
    int i;
    char sem_r[] = "semxxxx";
    char sem_n[256];
    for (i = 0; i < num; i++) {
        sem_close(s[i]);
        sprintf(sem_n, "%s%d", sem_r, i);
        sem_unlink(sem_n);
    }
    return 0;
}

int init_sem(sem_t ** s, int num)
{
    int i;
    char sem_r[] = "semxxxx";
    char sem_n[256];
    for (i = 0; i < num; i++) {
        sprintf(sem_n, "%s%d", sem_r, i);
        s[i] = sem_open(sem_n, O_CREAT, 0644, 0);
        if (s[i] == SEM_FAILED) {
            perror("sem_open");
            exit(-1);
        }
        sem_init(s[i], 1, 0);
    }
    return 0;
}

void *wait_remote_shm(key_t k, size_t s)
{
    void *p;
    int id;
    while (1) {		// wait remote to initial share memory
        id = shmget(k, s, 0644);
        if (id == -1) {
            if (errno == ENOENT) {
                usleep(10);
            } else {
                perror("shmget");
                //exit(-1);
                return NULL;
            }
        } else {
            break;
        }
    }

    p = shmat(id, NULL, 0);
    if (p == (void *)-1) {
        perror("shmat");
        //exit(-1);
        return NULL;
    }
    return p;
}

