#include <iostream>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <cstring> // For memset

struct Buffer {
    int write_index;
    int read_index;
    size_t buffer_size;  // Store the size of the buffer
    int data[];          // Flexible array member
};

// Allocate the buffer dynamically (for shared memory)
Buffer* createBuffer(size_t buffer_size) {
    size_t total_size = sizeof(Buffer) + sizeof(int) * buffer_size;
    Buffer* buffer = (Buffer*)malloc(total_size);
    if (!buffer) return nullptr;

    buffer->write_index = 0;
    buffer->read_index = 0;
    buffer->buffer_size = buffer_size;
    memset(buffer->data, 0, sizeof(int) * buffer_size);  // Optional initialization

    return buffer;
}

int main() {
    // Create shared memory segment for Buffer
    size_t buffer_size = 10;
    int shmid = shmget(IPC_PRIVATE, sizeof(Buffer) + sizeof(int) * buffer_size, IPC_CREAT | 0666);
    if (shmid == -1) {
        std::cerr << "Shared memory creation failed!" << std::endl;
        return 1;
    }

    // Attach the shared memory to the process
    Buffer* buffer = (Buffer*)shmat(shmid, nullptr, 0);
    if (buffer == (Buffer*)-1) {
        std::cerr << "Shared memory attachment failed!" << std::endl;
        return 1;
    }

    // Initialize or modify the shared buffer (only one process or thread at a time for safety)
    buffer->write_index = 1;
    buffer->read_index = 0;
    buffer->data[0] = 42;  // Set some data

    // Use the shared buffer
    std::cout << "Write Index: " << buffer->write_index << std::endl;
    std::cout << "Read Index: " << buffer->read_index << std::endl;
    std::cout << "Data at index 0: " << buffer->data[0] << std::endl;

    // Detach from shared memory
    shmdt(buffer);

    // Optionally, remove shared memory (not necessary during normal operation, but good for cleanup)
    shmctl(shmid, IPC_RMID, nullptr);

    return 0;
}
