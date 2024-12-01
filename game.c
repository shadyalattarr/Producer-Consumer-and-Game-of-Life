#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GRID_SIZE 20
#define NUM_THREADS 4
#define GENERATIONS 32

int grid[GRID_SIZE][GRID_SIZE];
pthread_barrier_t barrier;


void print_grid() {
    system("clear"); 
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 1) {
                printf("# ");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000); 
}

// Function to compute next generation of Game of Life
void* compute_next_gen(void* arg) {
  
     /* 
    write your code here 
    */

}


void initialize_grid(int grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;  // Set every cell to 0 (dead)
        }
    }
    }
void initialize_patterns(int grid[GRID_SIZE][GRID_SIZE]) {
    
    initialize_grid(grid);

    // Initialize Still Life (Square) at top-left
    grid[1][1] = 1;
    grid[1][2] = 1;
    grid[2][1] = 1;
    grid[2][2] = 1;

    // Initialize Oscillator (Blinker) in the middle
    grid[5][6] = 1;
    grid[6][6] = 1;
    grid[7][6] = 1;

    // Initialize Spaceship (Glider) in the bottom-right
    grid[10][10] = 1;
    grid[11][11] = 1;
    grid[12][9] = 1;
    grid[12][10] = 1;
    grid[12][11] = 1;
}

int main() {
    initialize_patterns(grid) ;
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    /* 
    write your code here 
    */

    pthread_barrier_destroy(&barrier);
    return 0;
}
