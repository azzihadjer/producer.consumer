#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 5  
#define NUM_ITEMS 10   
#define MUTEX 0  //  for mutual exclusion (critical section)
#define EMPTY 1  //  index for counting empty 
#define FULL  2  //  index for counting full 

typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out;
} SharedBuffer;

int semid;  
int shm_id;  
SharedBuffer *shm_ptr; 

struct sembuf sem_op;

void init_semaphores() {
    key_t key = 1234;
    semid = semget(key, 3, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, 1);  
    semctl(semid, 1, SETVAL, BUFFER_SIZE);  
    semctl(semid, 2, SETVAL, 0);  
}

void P(int sem_num) {
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1; 
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
}

void V(int sem_num) {
    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;  
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
}

void producer() {
    for (int item = 0; item < NUM_ITEMS; item++) {
        printf("[Producer] Trying to produce item %d\n", item + 1);

        P(EMPTY);  
        printf("[Producer] Found empty slot for item %d\n", item + 1);

        P(MUTEX); 
        printf("[Producer] Writing item %d to buffer position %d\n", item + 1, shm_ptr->in);

        shm_ptr->buffer[shm_ptr->in] = item + 1;  
        shm_ptr->in = (shm_ptr->in + 1) % BUFFER_SIZE;  

        V(MUTEX);  
        printf("[Producer] Item %d produced and mutex released\n", item + 1);

        V(FULL);  
        printf("[Producer] Signaled full slot for item %d\n", item + 1);

        sleep(1);  
    }

    printf("[Producer] Production complete\n");
}

int main() {
    key_t key = 1234; 

    shm_id = shmget(key, sizeof(SharedBuffer), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    shm_ptr = (SharedBuffer *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (SharedBuffer *)-1) {
        perror("shmat failed");
        exit(1);
    }

    shm_ptr->in = 0;
    shm_ptr->out = 0;

    init_semaphores();
    producer();  

    shmctl(shm_id, IPC_RMID, NULL);  
    semctl(semid, 0, IPC_RMID);  

    return 0;
}
