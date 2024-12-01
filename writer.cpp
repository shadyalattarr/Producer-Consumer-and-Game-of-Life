#include <iostream>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
using namespace std;

int main()
{
    // ftok to generate unique key
    // file to key
    // project id = 65
    // shared memory as in RAM
    // just creating a unique key
    key_t key = ftok("shmfile", 65);

    // shmget returns an identifier in shmid
    // 1024 is size of the shared memory segment in Bytes
    // 0666 | IPC_CREAT : permissions and create if doesnt exist
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    // shmat to attach to shared memory
    // first 0 is let system choose address -> null ptr
    // second 0 is default read/write mode
    char* str = (char*)shmat(shmid, (void*)0, 0);

    cout << "Write Data : ";
    // read up to 1024 bytes from 
    cin.getline(str, 1024);

    cout << "Data written in memory: " << str << endl;

    // detach from shared memory
    shmdt(str);

    return 0;
}