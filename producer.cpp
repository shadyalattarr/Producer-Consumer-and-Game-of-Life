#include <iostream>
#include <string>
#include <cstdlib> // For atof()
#include <time.h>
#include <ctime>
#include <unistd.h>
#include <random>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <cstring>


using namespace::std;

//vector in buffer wont work cuz allocate din heap
struct Buffer {
    int write_index;
    int read_index;
    size_t buffer_size;  // Store the size of the buffer
    double data[];          // Flexible array member
};

Buffer* createOrAttachBuffer(key_t key, size_t buffer_size, bool& is_new) {
    size_t total_size = sizeof(Buffer) + sizeof(double) * buffer_size;

    // Try to create the shared memory segment
    int shm_id = shmget(key, total_size, IPC_CREAT | IPC_EXCL | 0666);
    // Remove the shared memory segment 
    // IPC_EXCL make shmget fail if shared memory already exists, and return -ve to shmid


    if (shm_id < 0) {
        // If it already exists, attach to it
        shm_id = shmget(key, total_size, 0666);
        printf("shmid: %d\n",shm_id);
        if (shm_id < 0) {
            printf("ERROR?\n");
            std::cerr << "Failed to create or attach to shared memory!" << std::endl;
            exit(1);
        }
        is_new = false; // Buffer already exists
    } else {
        is_new = true; // This process created the buffer
    }

    // Attach to the shared memory
    Buffer* buffer = (Buffer*)shmat(shm_id, nullptr, 0);
    if (buffer == (void*)-1) {
        std::cerr << "Failed to attach to shared memory!" << std::endl;
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


void print_time()
{
    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, &tp) == -1) {
        cerr << "Problem with retrieving time.\n";
        exit(3);
    }

    time_t timestamp = time(NULL);
    struct tm datetime = *localtime(&timestamp);
    char output[50];
    long milliseconds = tp.tv_nsec / 1000000; // turn ns to ms

    // Format the date and time string
    strftime(output, 50, "%m/%d/%Y %H:%M:%S", &datetime);

    // Print using printf
    printf("[%s.%03ld] ", output, milliseconds); // 0 padding
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

int main(int argc, char *argv[]) {
    if (argc != 6) { // Check for the correct number of arguments
        cerr << "Usage: ./producer <commodity> <value1> <mean> <interval> <buffer_size>\n";
        return 1;
    }

    char commodity[11];
    strncpy(commodity, argv[1], sizeof(commodity) - 1); // First argument (e.g., NATURALGAS)
    commodity[sizeof(commodity) - 1] = '\0'; // Ensure null-termination       
    
    double price_mean = atof(argv[2]);         // Convert second argument to double
    double price_sd = atof(argv[3]);           // Convert third argument to double
    int wait_interval = atoi(argv[4]);     // Convert fourth argument to int
    int buffer_size = atoi(argv[5]);  // Convert fifth argument to int

    // Output the parsed values
    // cout << "argv[0] :" << argv[0] << "\n";
    // cout << "Commodity: " << commodity << "\n";
    // cout << "Price mean: $ " << price_mean << "\n";
    // cout << "Price standard deviation: $ " << price_sd << "\n";
    // cout << "Interval: " << wait_interval << " ms\n";
    // cout << "Buffer size: " << buffer_size << "\n";
    
    // ---------------------
    // shared memory creation
    // just creating a unique key

    int key;
    if (strcmp(commodity, "GOLD") == 0) {
        key = 1;  // Project ID for GOLD
    } else if (strcmp(commodity, "ALUM") == 0) {
        key =  2;  // Project ID for ALUM
    }
    key_t shm_key = ftok("/tmp", key);
    bool is_new = false;
    
    // int shmid = shmget(shm_key, sizeof(Buffer) + sizeof(double) * buffer_size, IPC_CREAT | 0666);
    // if (shmid == -1) {
    //     std::cerr << "Shared memory creation failed!" << std::endl;
    //     return 1;
    // }

    

    
    // ----------------------
    // ----------------------
    // creating the semaphore set for ONE PRODUCT--- for now iog
    
    // creating unique semkey -> 
    // to access semaphore across different processes and such
    key_t SEM_KEY = ftok("semkey", 55);
    // Create a semaphore set with 3 semaphores
    int semid = semget(SEM_KEY, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

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

    // ------------------------

    default_random_engine generator;
    normal_distribution<double> distribution(price_mean,price_sd);
    // first let's try and produce into terminal instead into a prod log file
    while(true)
    {
        print_time();
        double price_gen = distribution(generator);
        printf("%s: generating a new value %f\n", commodity, price_gen);
        // produce

        print_time();
        printf("%s: trying to get mutex on shared buffer\n",commodity);
        //semWait(e) -> mt spot?
        //sem_Wait(semid,1);
        // Attach the shared memory to the process
        Buffer* buffer = createOrAttachBuffer(shm_key,buffer_size,is_new);
        if (is_new) {
            std::cout << "Buffer created and initialized." << std::endl;
        } else {
            std::cout << "Buffer attached to existing shared memory." << std::endl;
        }


        print_time();
        printf("%s: placing %f on shared buffer\n",commodity,price_gen);
        // semWait(s) -> mutex
        //sem_Wait(semid,0);

        // append
        // Initialize or modify the shared buffer (only one process or thread at a time for safety)
        buffer->data[buffer->write_index] = price_gen;
        
        printf("buffersize = %d\n",(int)buffer->buffer_size);
        pid_t pid = getppid(); // Get the process ID
        std::cout << "Producer " << pid << " produced: " 
                  << buffer->data[buffer->write_index] << std::endl;
        
        // assume we sure there is an empty spot
        buffer->write_index = (buffer->write_index + 1) % buffer->buffer_size;
        
        

        // Print buffer state
        std::cout << "Buffer state: ";
        for (int j = 0; j < (int)buffer->buffer_size; ++j) {
            std::cout << buffer->data[j] << " ";
        }
        std::cout << std::endl;

        // Detach from shared memory
        shmdt(buffer);

        // semSignal(s) -> mutex
        //sem_Signal(semid,0);

        // smeSignal(n) -> a product is ready
        //sem_Signal(semid,2);
        
        
        print_time();
        printf("%s: sleeping for %d ms\n", commodity,wait_interval); 
        
        usleep(wait_interval * 1000); // usleep uses micro and we got milli
    }
    
    return 0;
}
