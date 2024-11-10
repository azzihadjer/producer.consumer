#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 5  
#define NUM_ITEMS 10   
#define MUTEX 0  
#define EMPTY 1  
#define FULL  2  

typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out;
} SharedBuffer;

int semid;  
int shm_id;  
SharedBuffer *shm_ptr; 

// Initialize semaphores: MUTEX=1, EMPTY=BUFFER_SIZE, FULL=0
void init_semaphores() {
    semctl(semid, MUTEX, SETVAL, 1);   // MUTEX = 1 (binary semaphore)
    semctl(semid, EMPTY, SETVAL, BUFFER_SIZE);  // EMPTY = BUFFER_SIZE
    semctl(semid, FULL, SETVAL, 0);  // FULL = 0
}

int main() {
    key_t key = 1234;

    // Create shared memory segment
    shm_id = shmget(key, sizeof(SharedBuffer), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    shm_ptr = (SharedBuffer *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (SharedBuffer *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared buffer positions
    shm_ptr->in = 0;
    shm_ptr->out = 0;

    // Create semaphores
    semid = semget(key, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize semaphores
    init_semaphores();

    // Fork producer and consumer processes
    pid_t pid = fork();
    if (pid == 0) {
        execl("./producer", "producer", NULL);  // Run producer
        exit(1);
    }

    pid = fork();
    if (pid == 0) {
        execl("./consumer", "consumer", NULL);  // Run consumer
        exit(1);
    }

    // Wait for child processes to finish
    wait(NULL);
    wait(NULL);

    // Clean up shared memory and semaphores
    shmctl(shm_id, IPC_RMID, NULL);  // Remove shared memory
    semctl(semid, 0, IPC_RMID);      // Remove semaphores

    return 0;
}
