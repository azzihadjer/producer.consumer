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

void consumer() {
    for (int item = 0; item < NUM_ITEMS; item++) {
        printf("[Consumer] Waiting for full slot\n");

        P(FULL);  
        printf("[Consumer] Found full slot\n");

        P(MUTEX);  
        item = shm_ptr->buffer[shm_ptr->out];
        printf("[Consumer] Consumed item %d from position %d\n", item, shm_ptr->out);

        shm_ptr->out = (shm_ptr->out + 1) % BUFFER_SIZE;
        V(MUTEX);  

        V(EMPTY);  
        printf("[Consumer] Signaled empty slot for item %d\n", item);

        sleep(1);  
    }

    printf("[Consumer] Consumption complete\n");
}

int main() {
    key_t key = ftok("shmfile", 65);  

    shm_id = shmget(key, sizeof(SharedBuffer), 0666); 
       if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    shm_ptr = (SharedBuffer *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (SharedBuffer *)-1) {
        perror("shmat failed");
        exit(1);
    }

    semid = semget(key, 3, 0666);  
    if (semid == -1) {
        perror("semget failed");
        exit(1);
    }

    consumer(); 

    return 0;
}
