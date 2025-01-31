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

#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Commodity{
    char product_name[11];
    double price;
};
//vector in buffer wont work cuz allocate din heap
struct Buffer {
    int write_index;
    int read_index;
    size_t buffer_size;  // Store the size of the buffer
    Commodity data[];          // Flexible array member
};


void append(Buffer* buffer,double price_gen, char* product_name){
    // Initialize or modify the shared buffer (only one process or thread at a time for safety)
    buffer->data[buffer->write_index].price = price_gen;
    strncpy(buffer->data[buffer->write_index].product_name,product_name,11);
    
    
    //printf("buffersize = %d\n",(int)buffer->buffer_size);
    //pid_t pid = getppid(); // Get the process ID
    // cout << "Producer " << pid << " produced: " 
    //         << buffer->data[buffer->write_index].product_name << " of price "
    //         << buffer->data[buffer->write_index].price << endl;
        
    // assume we sure there is an empty spot
    buffer->write_index = (buffer->write_index + 1) % buffer->buffer_size;
        
        

    // Print buffer state
    // cout << "Buffer state: ";
    // for (int j = 0; j < (int)buffer->buffer_size; ++j) {
    //     if(j % 5 == 0){
    //         cout << endl;
    //     }
    //     cout << buffer->data[j].product_name << " " << buffer->data[j].price << " ";
    // }
    // cout << endl;
}

int create_Or_Get_Semaphore(key_t key, int buffer_size) {
    
    // Try to get the semaphore without creating it
    int semid = semget(key, 3, 0666);
    if (semid == -1) {
        if (errno == ENOENT) {
            // Semaphore does not exist, create it
            //printf("Semaphore does not exist, create it\n");
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

            //printf("Semaphore created and initialized to %d with ID: %d\n", buffer_size, semid);
        } else {
            perror("semget");
            return -1;
        }
    } else {
        //printf("Semaphore already exists with ID: %d\n", semid);
    }

    return semid;  // Return the semaphore ID
}

int get_Commodity_Key(const char *commodity) {
    // Sorted map of commodities to unique keys
    static const map<string, int> commodityKeyMap = {
        {"ALUMINIUM", 0},
        {"COPPER", 1},
        {"COTTON", 2},
        {"CRUDEOIL", 3},
        {"GOLD", 4},
        {"LEAD", 5},
        {"MENTHAOIL", 6},
        {"NATURALGAS", 7},
        {"NICKEL", 8},
        {"SILVER", 9},
        {"ZINC", 10}
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
    // Calculate the total size required for the Buffer, including all commodities
    size_t total_size = sizeof(Buffer) + sizeof(Commodity) * buffer_size;

    // Try to create the shared memory segment
    int shm_id = shmget(key, total_size, IPC_CREAT | IPC_EXCL | 0666);
    if (shm_id < 0) {
        // If already exists, attach to it
        shm_id = shmget(key, total_size, 0666);
        if (shm_id < 0) {
            cerr << "Failed to create or attach to shared memory!" << endl;
            exit(1);
        }
        is_new = false; // Buffer already exists
    } else {
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
        // Initialize each commodity's product_name and price to default values
        for (size_t i = 0; i < buffer_size; ++i) {
            buffer->data[i].product_name[0] = '\0'; // Empty string
            buffer->data[i].price = 0.0;           // Default price
        }
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

    // ---------------------
    // shared memory creation
    // just creating a unique key

    // int product_key = get_Commodity_Key(commodity);
    // printf("PRODUCT_KEY: %d\n",product_key);
    key_t shm_key = ftok("/tmp", 20);
    bool is_new = false;
    
    // int shmid = shmget(shm_key, sizeof(Buffer) + sizeof(double) * buffer_size, IPC_CREAT | 0666);
    // if (shmid == -1) {
    //     cerr << "Shared memory creation failed!" << endl;
    //     return 1;
    // }

    

    
    // ----------------------
    // ----------------------
    // creating the semaphore set for ONE PRODUCT--- for now iog
    
    // creating unique semkey -> 
    // to access semaphore across different processes and such
    // need a semaphore for each PRODUCT?

    key_t SEM_KEY = ftok("/tmp", 22);
    // Create a semaphore set with 3 semaphores
    int semid = create_Or_Get_Semaphore(SEM_KEY,(int)buffer_size); //semget(SEM_KEY, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // // 0 - mutex
    // // 1 - empty slots
    // // 2 - full slots
    // // Initialize semaphore 0 (mutex-access on buffer) to 1 (resource available)
    // if (semctl(semid, 0, SETVAL, 1) == -1) {
    //     perror("semctl failed");
    //     exit(EXIT_FAILURE);
    // }

    // // Initialize semaphore 1 (empty slots) to buffer_size (resource available)
    // if (semctl(semid, 1, SETVAL, buffer_size) == -1) {
    //     perror("semctl failed");
    //     exit(EXIT_FAILURE);
    // }

    // // Initialize semaphore 2 (full slots) to 0 (wait)
    // if (semctl(semid, 2, SETVAL, 0) == -1) {
    //     perror("semctl failed");
    //     exit(EXIT_FAILURE);
    // }

    // ------------------------
    // Seed the generator with a unique value
    unsigned seed = chrono::system_clock::now().time_since_epoch().count() + getpid();  // seed is time + pid
    //printf("Seed: %d\n",seed);
    //printf("PID: %d\n",getpid());
    
    default_random_engine generator(seed); // to ensure each process generates different numbers -> not default seed
    normal_distribution<double> distribution(price_mean,price_sd);
    // first let's try and produce into terminal instead into a prod log file
    //int com_key;
    while(true)
    {
        print_time();
        double price_gen = distribution(generator);

        printf("\033[31m%s: generating a new value %7.2lf\033[0m\n", commodity, price_gen);
        // produce

        print_time();
        printf("\033[31m%s: trying to get mutex on shared buffer\033[0m\n",commodity);
        //semWait(e) -> mt spot?
        sem_Wait(semid,1);
        // Attach the shared memory to the process
        Buffer* buffer = create_Or_Attach_Buffer(shm_key,buffer_size,is_new);
        // if (is_new) {
        //     cout << "Buffer created and initialized." << endl;
        // } else {
        //     cout << "Buffer attached to existing shared memory." << endl;
        // }


        print_time();
        printf("\033[31m%s: placing %7.2lf on shared buffer\033[0m\n",commodity,price_gen);
        // semWait(s) -> mutex
        //cout<<"fee eiih\n";
        sem_Wait(semid,0);
        //cout << "2oli fee eih\n";

        // append
        //cout <<"before going com_key" << commodity << "\n";
        //com_key = get_Commodity_Key(commodity);
        //cout << "com_key we clearrrrrr = " << com_key << "\n";
        append(buffer,price_gen,commodity); // note buffer here is a pointer to buffer
        //cout<<"dount we gget here\n";
        // Detach from shared memory
        shmdt(buffer);

        // semSignal(s) -> mutex
        sem_Signal(semid,0);

        // smeSignal(n) -> a product is ready
        sem_Signal(semid,2);
        
        
        print_time();
        printf("\033[31m%s: sleeping for %d ms\033[0m\n", commodity,wait_interval); 
        printf("\n\n");
        
        usleep(wait_interval * 1000); // usleep uses micro and we got milli
    }
    
    return 0;
}
