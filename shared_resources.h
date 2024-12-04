#define NUM_SEMS 3              // Number of semaphores - mutex - empty slots - full slots
//#define BUFFER_SIZE 10          // Number of buffer slots
//#define ITEM_SIZE sizeof(Item) // Size of each item
//#define SHM_SIZE (BUFFER_SIZE * ITEM_SIZE) // Total shared memory size
#define FTOK_PATH "sharedfile"
#define FTOK_PROJ_ID 'X'    // Project ID for ftok()
