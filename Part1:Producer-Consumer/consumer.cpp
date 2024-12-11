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
#include <iomanip> // colors?
#include <vector>
 
using namespace::std;

//vector in buffer wont work cuz allocate din heap
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

struct Avg_Buffer{
    double data[5];
    int write_index;
    int avg_buffer_size;
    Avg_Buffer() {
        for (int i = 0; i < 5; ++i) {
            data[i] = 0;
        }
        write_index = 0;
        avg_buffer_size = 5;
    }
};

void printTable(const vector<string>& commodities, 
                const vector<double>& prices, 
                const vector<double>& avgPrices) {
    // Print table header
    cout << "+-----------------+-------------+-------------+" << endl;
    cout << "|    Currency     |    Price    |  AvgPrice   |" << endl;
    cout << "+-----------------+-------------+-------------+" << endl;

    // Print table rows
    for (size_t i = 0; i < commodities.size(); ++i) {
        // Format prices with green/red arrows based on price comparison
        string priceIndicator = (prices[i] > avgPrices[i]) ? "\033[32m↑\033[0m" : "\033[31m↓\033[0m";
        string avgPriceIndicator = (prices[i] > avgPrices[i]) ? "\033[32m↑\033[0m" : "\033[31m↓\033[0m";

        cout << "| " << setw(15) << commodities[i] << " | "
                  << setw(9) << fixed << setprecision(2) << prices[i] << priceIndicator << " | "
                  << setw(9) << fixed << setprecision(2) << avgPrices[i] << avgPriceIndicator << " |"
                  << endl;
    }

    // Print table footer
    cout << "+-----------------+-------------+-------------+" << endl;
}

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

double get_avg_last_5(Avg_Buffer avg_buffer){
    double sum = 0.0;
    int index;
    for (int i = 1;i<=avg_buffer.avg_buffer_size;i++){
        index = (avg_buffer.write_index-i)%5;
        if (index < 0) {
            index = 5 + index; // Translate -1 to 4, -2 to 3, etc.
        }
	    sum+= avg_buffer.data[index];

    }
        
    printf("sum = %f\n",sum);
    return sum / avg_buffer.avg_buffer_size;
}

void insert_avg_buffer(Avg_Buffer * avg_buffer,double element){
    avg_buffer->data[avg_buffer->write_index] = element;
    avg_buffer->write_index =(avg_buffer->write_index+ 1)%5;
}

void take(Buffer* buffer,double* price, string * product_name){
    *price = buffer->data[buffer->read_index].price;
    *product_name = buffer->data[buffer->read_index].product_name;
    //strncpy(product_name,buffer->data[buffer->read_index].product_name,11);
    
    pid_t pid = getppid(); // Get the process ID
    cout << "Consumer " << pid << " consumed: " 
            <<  buffer->data[buffer->read_index].product_name << " with price: " 
            << buffer->data[buffer->read_index].price << endl; 
    
    //just to show in buffer state
    buffer->data[buffer->read_index].price = 0.0;
    strncpy(buffer->data[buffer->read_index].product_name,"\0",11);
    
    //asd
    // incrememnt buffer->read_index
    buffer->read_index = (buffer->read_index+1) % buffer->buffer_size;

    // Print buffer state
    cout << "Buffer state: ";
    for (int j = 0; j < (int)buffer->buffer_size; ++j) {
        cout << buffer->data[j].product_name << " " << buffer->data[j].price << " ";
    }
    cout << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <buffer_size>\n";
        return 1;
    }

    int buffer_size = atoi(argv[1]); // Convert the argument to an integer

    cout << "Buffer size: " << buffer_size << "\n";

    // Avg_Buffer dabuff;
    // insert_avg_buffer(&dabuff,1);
    // insert_avg_buffer(&dabuff,2);
    // insert_avg_buffer(&dabuff,3);
    // insert_avg_buffer(&dabuff,4);
    // insert_avg_buffer(&dabuff,5);
    // insert_avg_buffer(&dabuff,6);

    // // Print dabuffer state
    // cout << "daBuffer state: ";
    // for (int j = 0; j < dabuff.avg_buffer_size; ++j) {
    //     cout << dabuff.data[j] << " ";
    // }
    // cout << endl;


    // //printf("(-1) mod 5 = %d\n",(2-3)%5);
    // printf("write_index= %d\n",dabuff.write_index);
    // //printf("dabuff.data[(dabuff.write_index-3)%%5] = %f\n",dabuff.data[(dabuff.write_index-8)%5]);
    // printf("%f\n",get_avg_last_5(dabuff));


    
    // Example commodities, prices, and average prices
    vector<string> commodities = {"GOLD","SILVER","CRUDEOIL","NATURALGAS","ALUMINIUM",
                                "COPPER","NICKEL","LEAD","ZINC","MENTHAOIL","COTTON"};
    vector<double> prices   =  {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    vector<double> avgPrices = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    
    // int product_key = get_Commodity_Key("GOLD"); // for now do gold only
    // printf("PRODUCT_KEY: %d\n",product_key);
    key_t shm_key = ftok("/tmp", 20);
    bool is_new = false;

    key_t SEM_KEY = ftok("/tmp", 22);
    // Create a semaphore set with 3 semaphores
    int semid = create_Or_Get_Semaphore(SEM_KEY,(int)buffer_size); //semget(SEM_KEY, 3, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    
    double price_taken;
    string product_takem;
    // 0 - mutex       - s
    // 1 - empty slots - e
    // 2 - full slots  - n

    while(true){
        //semWait(n) - is there a product ready for me?
        printf("is there a product ready for me?\n");
        sem_Wait(semid,2);

        //semWait(s)  -- sync to read buffer
        printf("sync to read buffer\n");
        sem_Wait(semid,0);

        //take()
        // Attach the shared memory to the process
        Buffer* buffer = create_Or_Attach_Buffer(shm_key,buffer_size,is_new);
        if (is_new) {
            cout << "Buffer created and initialized." << endl;
        } else {
            cout << "Buffer attached to existing shared memory." << endl;
        }
        take(buffer,&price_taken,&product_takem);
        printf("\033[31mPRICE TAKEN: %f\033[0m\n",price_taken);
        cout << "\033[31mPRoduct TAKEN: \033[0m" << product_takem << endl;
        
        //semSIgnal(s) - done with critical section
        sem_Signal(semid,0);

        //semSignal(e) - product taken so mtspot available
        sem_Signal(semid,1);
        sleep(3);
        //consume
    }



    // Print the table
    //printTable(commodities, prices, avgPrices);

    return 0;
}