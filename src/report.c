/*
 * Filename: report.c
 * Author: fangdingjun@gmail.com
 * License: GPLv2
 * Date: 2011-7-10
 *
 */

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "qpi.h"

#define DIFF(start,end)  ((end.tv_sec-start.tv_sec) * 1000 + (end.tv_usec-start.tv_usec))

int report(int delay)
{
    struct timeval tv, tv1;
    unsigned long tmp_read = 0, tmp_write = 0;
    unsigned long timedelay = 0;
    unsigned long mem_total_r = 0, mem_total_w = 0;
    time_t now;
    struct tm *tt;
    char t_out[100];
    int i;


    if (fp){
        fprintf(fp, "0 Read_perf_MB/s Read_delay_ms "
                "Write_perf_MB/s Write_delay_ms\n");
    }

    // wake up thread 
    for (i = 0; i < proc_num; i++) {
        //printf("sem_post(&shm_sem[%d])\n",i);
        sem_post(sem[i]);
    }

    sem_post(read_lock);
    sem_post(write_lock);
    gettimeofday(&tv, NULL);
    while (running) {
        memcpy(&tv1, &tv, sizeof(struct timeval));
        gettimeofday(&tv, NULL);

        timedelay = DIFF(tv1, tv);
        sem_wait(read_lock);
        tmp_read = *total_r;
        *total_r = 0;
        sem_post(read_lock);

        sem_wait(write_lock);
        tmp_write = *total_w;
        *total_w = 0;
        sem_post(write_lock);

        //printf("timedelay %lu, read = %lu,write=%lu\n",timedelay,tmp_read,tmp_write);

        now = time(NULL);
        tt = localtime(&now);
        if (tmp_read && tmp_write && timedelay) {
            mem_total_r = (tmp_read * shm_size) >> 20;
            mem_total_w = (tmp_write * shm_size) >> 20;
            //printf("%s, read %lu MB," " write %lu MB,"
            //      " per %d seconds\n",
            //ctime(&now),
            //     t_out, mem_total_r, mem_total_w, delay);
            strftime(t_out, sizeof(t_out), "%Y-%m-%d %H:%M:%S", tt);
            if (!fp) {
                fprintf(stdout, "in recent %lu r/w(s):\n"
                        "%s\n"
                        "read: %4.2f MB/s, delay: %4.2f ms, write: %4.2f MB/s, delay: %4.2f ms\n\n",
                        tmp_read + tmp_write,
                        t_out,
                        (float)mem_total_r * 1000 /
                        (float)timedelay,
                        (float)timedelay / (float)tmp_read,
                        (float)mem_total_w * 1000 /
                        (float)timedelay,
                        (float)timedelay / (float)tmp_write);

            } else {
                strftime(t_out, sizeof(t_out), "%m_%d_%H:%M:%S",
                        tt);
                fprintf(fp, "%s %4.2f %4.2f %4.2f %4.2f\n",
                        t_out,
                        (float)mem_total_r * 1000 /
                        (float)timedelay,
                        (float)timedelay / (float)tmp_read,
                        (float)mem_total_w * 1000 /
                        (float)timedelay,
                        (float)timedelay / (float)tmp_write);
            }
        }
        //total_old_r = total_r;
        //total_old_w = total_w;
        // if error, stop it
        if (*total_e) {
            //printf("data check error: %d\n",error);
            kill(0, SIGUSR1);
            break;
        }
        //printf("total=%d\n",total);
        //gettimeofday(&tv1,NULL);
        sleep(delay);
    }
    //fclose(fp);
    return 0;
}

