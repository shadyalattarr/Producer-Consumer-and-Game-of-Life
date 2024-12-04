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
#include <stdio.h>
#include <cstring>

using namespace::std;

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
    int wait_interval = std::atoi(argv[4]);     // Convert fourth argument to int
    int buffer_size = std::atoi(argv[5]);  // Convert fifth argument to int

    // Output the parsed values
    //cout << "argv[0] :" << argv[0] << "\n";
    cout << "Commodity: " << commodity << "\n";
    cout << "Price mean: $ " << price_mean << "\n";
    cout << "Price standard deviation: $ " << price_sd << "\n";
    cout << "Interval: " << wait_interval << " ms\n";
    cout << "Buffer size: " << buffer_size << "\n";
    
    // creating the semaphore set for ONE PRODUCT--- for now iog
    
    // creating unique semkey -> 
    // to access semaphore across different processes and such
    key_t SEM_KEY = ftok("shmfile", 65);
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
        // semWait(e) -> mt spot?
        //sem_Wait(semid,1);

        print_time();
        printf("%s: placing %f on shared buffer\n",commodity,price_gen);
        // semWait(s) -> mutex
        //sem_Wait(semid,0);

        // append

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
