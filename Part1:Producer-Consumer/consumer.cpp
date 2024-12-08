#include <iostream>
#include <string>
#include <cstdlib> // For atof()
#include <time.h>
#include <ctime>
#include <unistd.h> // For getpid()
#include <random>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <cstring>
#include <chrono>
#include <map> // for key
 
using namespace::std;

//vector in buffer wont work cuz allocate din heap
struct Buffer {
    int write_index;
    int read_index;
    size_t buffer_size;  // Store the size of the buffer
    double data[];          // Flexible array member
};


struct avg_buffer{
    double data[5];
    int write_index;
    avg_buffer() {
        for (int i = 0; i < 5; ++i) {
            data[i] = 0;
        }
        write_index = 0;
    }
};

int create_Or_Get_Semaphore(key_t key, int buffer_size) {
    
    // Try to get the semaphore without creating it
    int semid = semget(key, 3, 0666);
    if (semid == -1) {
        if (errno == ENOENT) {
            // Semaphore does not exist, create it
            printf("Semaphore does not exist, create it\n");
            semid = semget(key, 3, IPC_CREAT | 0666);
            if (semid == -1) {
                perror("semget (create)");
                return -1;
            }

            
            // 0 - mutex
            // 1 - empty slots
            // 2 - full slots
            // Initialize semaphore 0 (mutex-access on buffer) to 1 (resource available)
            if (semctl(semid, 0, SETVAL, 1) == -1) {
                perror("semctl failed");
                exit(EXIT_FAILURE);
            }

            // Initialize semaphore 1 (empty slots) to buffer_size (resource available)
            if (semctl(semid, 1, SETVAL, buffer_size) == -1) {
                perror("semctl failed");
                exit(EXIT_FAILURE);
            }

            // Initialize semaphore 2 (full slots) to 0 (wait)
            if (semctl(semid, 2, SETVAL, 0) == -1) {
                perror("semctl failed");
                exit(EXIT_FAILURE);
            }

            printf("Semaphore created and initialized to %d with ID: %d\n", buffer_size, semid);
        } else {
            perror("semget");
            return -1;
        }
    } else {
        printf("Semaphore already exists with ID: %d\n", semid);
    }

    return semid;  // Return the semaphore ID
}

int get_Commodity_Key(const char *commodity) {
    // Map of commodities to unique keys
    static const map<string, int> commodityKeyMap = {
        {"GOLD", 1},
        {"SILVER", 2},
        {"CRUDEOIL", 3},
        {"NATURALGAS", 4},
        {"ALUMINIUM", 5},
        {"COPPER", 6},
        {"NICKEL", 7},
        {"LEAD", 8},
        {"ZINC", 9},
        {"MENTHAOIL", 10},
        {"COTTON", 11}
    };

    // search for the pair you want
    // com_key is an iterator that pts to pair
    auto com_key = commodityKeyMap.find(commodity); // returns pair with the commodity
    if (com_key != commodityKeyMap.end()) {
        return com_key->second; // Return the associated key
    }

    return -1; // Invalid commodity
}


Buffer* create_Or_Attach_Buffer(key_t key, size_t buffer_size, bool& is_new) {
    size_t total_size = sizeof(Buffer) + sizeof(double) * buffer_size;

    // Try to create the shared memory segment
    int shm_id = shmget(key, total_size, IPC_CREAT | IPC_EXCL | 0666);
    // Remove the shared memory segment 
    // IPC_EXCL make shmget fail if shared memory already exists, and return -ve to shmid


    if (shm_id < 0) {
        // if already exists, attach to it
        shm_id = shmget(key, total_size, 0666);
        printf("shmid: %d\n",shm_id);
        if (shm_id < 0) {
            printf("ERROR?\n");
            cerr << "Failed to create or attach to shared memory!" << endl;
            exit(1);
        }
        is_new = false; // Buffer already exists
    } else {
        printf("shmid: %d\n",shm_id);
        is_new = true; // This process created the buffer
    }

    // Attach to shared memory
    Buffer* buffer = (Buffer*)shmat(shm_id, nullptr, 0);
    if (buffer == (void*)-1) {
        cerr << "Failed to attach to shared memory!" << endl;
        exit(1);
    }

    // Initialize only if this process created the buffer
    if (is_new) {
        buffer->write_index = 0;
        buffer->read_index = 0;
        buffer->buffer_size = buffer_size;
        memset(buffer->data, 0, sizeof(double) * buffer_size);
    }

    return buffer;
}



// Function to lock semaphore 
void sem_Wait(int semid,short unsigned int sem_num) {
    struct sembuf sb = {sem_num, -1, 0}; // Decrement semaphore sem_num
    if (semop(semid, &sb, 1) == -1) {
        perror("semop lock failed");
        exit(EXIT_FAILURE);
    }
}

// Function to unlock semaphore (V operation)
void sem_Signal(int semid,short unsigned int sem_num) {
    struct sembuf sb = {sem_num, 1, 0}; // Increment semaphore sem_num
    if (semop(semid, &sb, 1) == -1) {
        perror("semop unlock failed");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <buffer_size>\n";
        return 1;
    }

    int buffer_size = atoi(argv[1]); // Convert the argument to an integer

    cout << "Buffer size: " << buffer_size << "\n";


    return 0;
}