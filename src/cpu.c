/*
 * Filename: cpu.c
 * Author: fangdingjun@gmail.com
 * License: GPLv2
 * Date: 2011-7-10
 *
 */

#include<stdio.h>
#include<unistd.h>
#include<sched.h>
//#include "qpi.h"

int bind_to_cpu(int cpunum)
{
    int num = sysconf(_SC_NPROCESSORS_CONF);
    int i;

    cpu_set_t mask;

    CPU_ZERO(&mask);
    CPU_SET(cpunum, &mask);

    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        perror("set cpu affinity failed");
        return -1;
    }

    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(mask), &mask) == -1) {
        perror("get cpu affinity failed");
        return -1;
    }
    for (i = 0; i < num; i++) {
        if (CPU_ISSET(i, &mask)) {
            break;
        }
    }
    if (i != cpunum) {
        fprintf(stderr,
                "set cpu affinity failed: process not running on cpu %d\n",
                cpunum);
        return -1;
    }
    return 0;
}

