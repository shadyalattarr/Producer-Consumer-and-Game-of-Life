#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GRID_SIZE 20
#define NUM_THREADS 4
#define GENERATIONS 32

int grid[GRID_SIZE][GRID_SIZE];
pthread_barrier_t barrier;

void print_grid()
{
    system("clear");
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[i][j] == 1)
            {
                printf("# ");
            }
            else
            {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000);
}

int count_live_neighbors(int row, int col)
{
    int live_neighbors = 0;
    for (int i = row - 1; i <= row + 1; i++)
    {
        for (int j = col - 1; j <= col + 1; j++)
        {
            if (i == row && j == col)
                continue;

            if (i >= 0 && i < GRID_SIZE && j >= 0 && j < GRID_SIZE)
            {
                live_neighbors += grid[i][j];
            }
        }
    }
    return live_neighbors;
}

// Function to compute next generation of Game of Life
void *compute_next_gen(void *arg)
{

    /*
    write your code here
   */
    int thread_id = *((int *)arg);
    int first_row_t = thread_id * (GRID_SIZE / NUM_THREADS);
    int last_row_t = first_row_t + (GRID_SIZE / NUM_THREADS);

    int hold_board[GRID_SIZE][GRID_SIZE];
    for (int i = first_row_t; i < last_row_t; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            int live_neighbors = count_live_neighbors(i, j);
            if (grid[i][j] == 0) // here we will chesk the birth rule. A dead cell (0) will become alive (1) in the next generation if it has exactly 3 live neighbors.
            {
                if (live_neighbors == 3)
                    hold_board[i][j] = 1; // the birth rule applies here
                else
                    hold_board[i][j] = 0; // nothing changes
            }
            else // this means that it is either survival or death (call is originally alive)
            {
                if (live_neighbors < 2 || live_neighbors > 3) // here we will check on the death rule. A live cell (1) will die (become 0) in the next generation if it has fewer than 2 live
                {                                             // neighbors (underpopulation) or more than 3 live neighbors (overpopulation).
                    hold_board[i][j] = 0;                     // the death rule applies here
                }
                else
                    hold_board[i][j] = grid[i][j]; // the survival rule applies here. Note: "the cell stays the same" means that the hold equals the old or equals 1 in this case
            }
        }
    }

    pthread_barrier_wait(&barrier);
    for (int i = first_row_t; i < last_row_t; i++) // normal updating of the grid but we wait until the threads finish
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = hold_board[i][j];
        }
    }
    return NULL;
}

void initialize_grid(int grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = 0; // Set every cell to 0 (dead)
        }
    }
}
void initialize_patterns(int grid[GRID_SIZE][GRID_SIZE])
{

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

void start_game_of_life()
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int start_gen = 0; start_gen < GENERATIONS; start_gen++)
    {
        for (int thread_number = 0; thread_number < NUM_THREADS; thread_number++)
        {
            thread_ids[thread_number] = thread_number;
            pthread_create(&threads[thread_number], NULL, compute_next_gen, (void *)&thread_ids[thread_number]);
        }
        for (int thread_number = 0; thread_number < NUM_THREADS; thread_number++)
        {
            pthread_join(threads[thread_number], NULL);
        }
        print_grid();
    }
    pthread_barrier_destroy(&barrier);
}

int main()
{
    initialize_patterns(grid);
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    /*
    write your code here
    */

    start_game_of_life();
    return 0;
}
