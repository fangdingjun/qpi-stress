#ifndef QPI_H
#define QPI_H
    sem_t *read_lock;
    sem_t *write_lock;
    int proc_num;
    unsigned int data;
    long cpu_num;
    int *total_r;
    int *total_w;
    int *total_e;
    key_t *key;
    sem_t **sem;
    unsigned long shm_size;
    volatile int running;
    char *output;
    FILE *fp;
    void usage();
    int init_shm_key(key_t * k, int num);
    int init_sem(sem_t ** s, int num);
    int init_one_key(key_t * k, char *name);
    int do_fork(int m);
    int worker(int num);
    int destroy_shm(key_t * k, int num);
    int destroy_sem(sem_t ** s, int num);
    int report(int delay);
    int delete_shm_by_name(char *n, size_t s);
    int ret;
    void sig(int num);
    void *create_shm(key_t k, size_t s);
    int bind_to_cpu(int cpunum);
    void *create_shm(key_t k, size_t s);
    void *wait_remote_shm(key_t k, size_t s);
#endif
