/*
 * this is tool to stress cpu's qpi channel on romely platform
 *
 * Filename: main.c
 * Author: fangdingjun@gmail.com
 * License: GPLv2
 * Date: 2011-7-10
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include "qpi.h"


int main(int argc, char **argv)
{
    /* initial global value */
    proc_num = 2;
    data = 0xaa;
    shm_size = 4 << 20;	// 4 MB
    running = 1;
    output = NULL;
    fp = NULL;
    ret = 0;

    int opt_idx;
    unsigned int timeout = 0;
    int delay = 10;
    long page_size;
    long total_page;
    unsigned long req_mem;
    unsigned long avb_mem;
    int c;
    int i;
    key_t k_read;
    key_t k_write;
    key_t k_error;
    //int id_r,id_w;

    struct option opt[] = {
        {"timeout", 1, 0, 't'},
        {"number", 1, 0, 'n'},
        {"data", 1, 0, 'd'},
        {"size", 1, 0, 's'},
        {"help", 0, 0, 'h'},
        {"delay", 1, 0, 'p'},
        {"output", 1, 0, 'o'},
        {0, 0, 0, 0}
    };
    proc_num = cpu_num = sysconf(_SC_NPROCESSORS_CONF);
    // process command line options
    while (1) {
        c = getopt_long(argc, argv, "hn:t:d:s:p:o:", opt, &opt_idx);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'n':
                proc_num = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg) * 60;
                break;
            case 'd':
                //data=atoi(optarg);
                if (!memcmp("0x", optarg, 2)) {
                    sscanf(optarg, "%x", &data);
                } else {
                    data = atoi(optarg);
                }
                break;
            case 's':
                shm_size = atoi(optarg) << 20;
                break;
            case 'p':
                delay = atoi(optarg);
                break;
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage();
                break;
            default:
                usage();
        }
    }
    if (argc > optind) {
        usage();
    }
    // process number error
    page_size = sysconf(_SC_PAGESIZE);
    total_page = sysconf(_SC_PHYS_PAGES);
    req_mem = proc_num * shm_size;
    avb_mem = page_size * total_page;
    //fp=stderr;
    if (output != NULL) {
        fp = fopen(output, "w");
        if (fp == NULL) {
            fprintf(stderr,
                    "WARNING: open %s failed, log to screen\n",
                    output);
            //fp=stderr;
        }
    }
    // 0 for random data
    if (data == 0) {
        srand(time(NULL));
        data = rand() % 0xff;
    }
    //printf("argc %d optind %d\n",argc,optind);
    //return 0;
    fprintf(stdout, "summay:\n"
            "\ttotal processors: %ld\n"
            "\ttotal memory: %ld MB\n"
            "\tstart processes: %d\n"
            "\tdata partern: 0x%02x\n"
            "\truntime: %u seconds\n"
            "\tresult display delay: %d seconds\n"
            "\tper process  share memory size: %lu MB\n\n",
            cpu_num,
            avb_mem >> 20,
            proc_num, data,
            timeout ==
            0 ? (unsigned int)-1 : (unsigned int)timeout, delay,
            shm_size >> 20);

    if (proc_num % 2) {
        fprintf(stderr,
                "ERROR: process number must be 2, 4, 6, 8, ...\n");
        exit(-1);
    }

    if (proc_num > cpu_num) {
        fprintf(stderr,
                "ERROR: only %ld processors core,"
                " but you want to run %d processes\n",
                cpu_num, proc_num);
        exit(-1);
    }

    if (req_mem > avb_mem) {
        fprintf(stderr,
                "ERROR: total available memory is %ld bytes,"
                " but you request %d * %lu = %lu bytes\n",
                avb_mem, proc_num, shm_size, req_mem);
        exit(-1);
    }

    sem_t *l_sem[proc_num];
    sem = l_sem;
    key_t a_key[proc_num];
    key = a_key;
    init_shm_key(key, proc_num);
    init_sem(sem, proc_num);
    init_one_key(&k_read, "/dev/shm/read_counter");
    init_one_key(&k_write, "/dev/shm/write_counter");
    init_one_key(&k_error, "/dev/shm/error_counter");
    total_r = (int *)create_shm(k_read, sizeof(int));
    total_w = (int *)create_shm(k_write, sizeof(int));
    total_e = (int *)create_shm(k_error, sizeof(int));
    if (!(total_r && total_w && total_e)) {
        //exit(-1);
        ret = -1;
        goto out;
    }
    *total_e = 0;
    *total_r = 0;
    *total_w = 0;

    read_lock = sem_open("read", O_CREAT, 0644, 0);
    if (read_lock == SEM_FAILED) {
        perror("sem_open");
        //exit(-1);
        ret = -1;
        goto out;
    }
    sem_init(read_lock, 1, 0);

    write_lock = sem_open("write", O_CREAT, 0644, 0);
    if (write_lock == SEM_FAILED) {
        perror("sem_open");
        //exit(-1);
        ret = -1;
        goto out;
    }
    sem_init(write_lock, 1, 0);

    for (i = 0; i < proc_num; i++) {
        if (do_fork(i) < 0) {	//  fork subprocess
            ret = -1;
            goto out1;
        }
    }

    usleep(100);
    fprintf(stdout, "-------------------------------\n");
    fprintf(stdout, "\nQPI stress staring ...\n\n");
    //sleep(1);

    signal(SIGINT, sig);
    signal(SIGTERM, sig);
    signal(SIGHUP, sig);
    signal(SIGALRM, sig);
    signal(SIGUSR1, sig);
    signal(SIGUSR2, sig);

    // register timer
    if (timeout) {
        alarm(timeout);
    }
    //report status
    report(delay);

out1:
    kill(0, SIGUSR2);	// terminate children process
    sleep(2);

    // check if data check error
    if (*total_e) {
        fprintf(stderr, "data check error: %d\n", *total_e);
        ret = -1;
    }
out:
    // destroy share memory and semaphore
    destroy_shm(key, proc_num);
    destroy_sem(sem, proc_num);

    /* cleaning semaphore */
    sem_close(write_lock);
    sem_close(read_lock);
    sem_unlink("write");
    sem_unlink("read");

    delete_shm_by_name("/dev/shm/read_counter", sizeof(int));
    delete_shm_by_name("/dev/shm/write_counter", sizeof(int));
    delete_shm_by_name("/dev/shm/error_counter", sizeof(int));

    if(fp){
        fclose(fp);
    }
    return ret;
}

int delete_shm_by_name(char *n, size_t s)
{
    key_t k;
    int id;
    k = ftok(n, 0);
    id = shmget(k, s, 0644);
    if (id != -1) {
        shmctl(id, IPC_RMID, NULL);
    }
    unlink(n);
    return 0;
}

void usage()
{
    fprintf(stderr, "Usage: "
            "qpi_stress [options]"
            "\n\t-h, --help\n\t\tprint this help"
            "\n\t-n, --number=n\n\t\t start n processes, default number of cpu cores\n"
            "\t-t, --timeout=n\n\t\t time to run(minute), default 0-run util CTRL+C pressed\n"
            "\t-d, --data=n\n\t\t  data to fill memory, 0 for random, default 0xaa\n"
            "\t-s, --size=n\n\t\t size of share memory(MB), default 4\n"
            "\t-p, --delay=n\n\t\t result display delay(seconds), default 10\n"
            "\t-o, --output=file\n\t\t write result to file, default screen\n"
            "Example:\n\t"
            "qpi_stress -n4 -s10 -t10 -d 0x55"
            "\n   start 4 processes, run 10 minutes, per process requst 10 MB memory, use 0x55 to fill\n");
    exit(-1);
}

void sig(int num)
{
    //printf("get signal %d, cleaning and exit ...\n",num);
    if (num == SIGALRM) {
        fprintf(stderr, "timeout, cleaning and exiting...\n");
    } else if (num == SIGINT) {
        fprintf(stderr, "CTRL+C pressed, aborting...\n");
    } else if (num == SIGUSR1) {
        fprintf(stderr, "error occured, aborting...\n");
        ret = 1;
    } else if (num == SIGUSR2) {
        ;
    } else {
        fprintf(stderr, "got signal %d, exiting...\n", num);
    }
    running = 0;
    //return;
}
