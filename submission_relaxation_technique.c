/**
* Parallel relaxation technique
* Oliver Redeyoff
*
* HOW TO COMPILE :
* gcc subsission_relaxation_technique.c -lm -lpthread
*
* HOW TO RUN :
* ./a.out <matrix size> <number of threads> <precision decimal number>
*
* Example :
* ./a.out 500 4 3
* this runs the program with a matrix size of 500, 4 threads and a precision to the 3rd decimal point (0.001)
*
**/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

typedef struct block {
    int start_index;
    int end_index;
    double* new_values;
} BLOCK;

// declare global variables
int thread_count;
double decimal_value;
int matrix_size;

double* matrix;
BLOCK* blocks;

int value_change_flag;

pthread_barrier_t barrier_1;
pthread_barrier_t barrier_2;

// Returns array of doubles of length matrix_size^2
double* makeMatrix() {
    // allocate memory for new matrix of given size
    double* matrix = malloc(matrix_size*matrix_size*sizeof(double));

    for (int i=0 ; i<matrix_size ; i++) {
        for (int j=0 ; j<matrix_size ; j++){

            // populate with 1.0 if left or top edge, else with 0.0
            if (i==0 || j==0){
                matrix[i*matrix_size + j] = 1.0;
            } else {
                matrix[i*matrix_size + j] = 0.0;
            }

        }
    }

    return matrix;
}

// Returns thread_count number of blocks which each contain a start_index, an
// end_index and an array of doubles to store the new values that will be computed
// between those indexes. No blocks overlap and they cover all the mutable cells of 
// array
BLOCK* makeBlocks() {
    BLOCK* blocks = malloc(thread_count*sizeof(BLOCK));

    int mutatable_indexes_count = matrix_size*matrix_size - matrix_size*2;

    int equal_block_size = ceil((double)mutatable_indexes_count/(double)thread_count);
    int last_block_size = mutatable_indexes_count%equal_block_size;
    int equal_block_count = (mutatable_indexes_count-last_block_size) / equal_block_size;

    // create equal sized blocks
    for(int i=0 ; i<equal_block_count ; i++) {
        BLOCK new_block;
        new_block.start_index = matrix_size + equal_block_size*i;
        new_block.end_index = matrix_size + equal_block_size*(i+1) - 1;

        double* new_values = malloc((new_block.end_index-new_block.start_index)*sizeof(double));
        new_block.new_values = new_values;

        blocks[i] = new_block;
    }

    // create one final block if it was impossible to have only equally sized ones
    if(last_block_size != 0) {
        BLOCK new_block;
        new_block.start_index = matrix_size + mutatable_indexes_count - last_block_size;
        new_block.end_index = matrix_size*matrix_size - matrix_size-1;

        double* new_values = malloc((new_block.end_index-new_block.start_index)*sizeof(double));
        new_block.new_values = new_values;

        blocks[thread_count-1] = new_block;
    }

    return blocks;
}

// Returns the average of the four cells surrounding a cell at a given index
double getSuroundingAverage(int index) {
    double top_value = matrix[index - matrix_size];
    double right_value = matrix[index + 1];
    double bottom_value = matrix[index + matrix_size];
    double left_value = matrix[index - 1];

    return (top_value + right_value + bottom_value + left_value)/4;
}

// Performs relaxation on range of indexes of matrix defined in the given block
void processBlock(BLOCK* block) {
    int start_index = block->start_index;
    int end_index = block->end_index;

    for(int m_i=start_index ; m_i<=end_index ; m_i++) {

        // get index for block new values
        int b_i = m_i-start_index;

        // keep any edge value as is
        if (m_i%matrix_size != 0 && (m_i+1)%matrix_size != 0) {
            double new_value = getSuroundingAverage(m_i);
            double diff = new_value - block->new_values[b_i];
            if (diff > decimal_value) {
                value_change_flag = 1;
            }
            block->new_values[b_i] = new_value;
        } else {
            block->new_values[b_i] = matrix[m_i];
        }

    }

}

// Updates matrix with values stored in each block's new_value array
void updateMatrix() {
    for (int i=0 ; i<thread_count ; i++) {
        int start_index = blocks[i].start_index;
        int end_index = blocks[i].end_index;

        for(int m_i=start_index ; m_i<=end_index ; m_i++) {
        
            // get index for block new values
            int b_i = m_i-start_index;
            
            // map block new_values to matrix values
            if (m_i%matrix_size != 0 && (m_i+1)%matrix_size != 0) {
                matrix[m_i] = blocks[i].new_values[b_i];
            }

        }
    }
}

// Prints out matrix as table, and highlights each block
void printMatrix() {
    for (int i=0 ; i<matrix_size ; i++) {
        printf("\n");
        for (int j=0 ; j<matrix_size ; j++){
            printf("%f, ", matrix[i*matrix_size + j]);
        }
    }
    printf("\n");
}

// Entry point for worker thread
void* initWorkerThread(void* vargp) {
    BLOCK* block = (BLOCK*)vargp;

    // worker thread loop
    while (1) {
        // perform relaxation on given block
        processBlock(block);

        // wait to synchronise with main and other work threads at barrier 1
        pthread_barrier_wait(&barrier_1);

        // wait to synchronise with main and other work threads at barrier 2
        pthread_barrier_wait(&barrier_2);
    }
}

int main(int argc, char **argv) {

    // set global variables to values passed as arguments
    if (argc != 4) {
        printf("Incorrect number of arguments\n");
        return 1;
    }
    matrix_size = atoi(argv[1]);
    thread_count = atoi(argv[2]);
    int decimal_precision = atoi(argv[3]);
    decimal_value = pow(0.1, decimal_precision);

    pthread_t threads[thread_count];

    // instantiate matrix
    matrix = makeMatrix();

    // instantiate blocks
    blocks = makeBlocks();

    // initialise barriers
    pthread_barrier_init(&barrier_1, NULL, thread_count+1);
    pthread_barrier_init(&barrier_2, NULL, thread_count+1);

    // set initial value of value_change_flag
    value_change_flag = 0;

    // create threads
    for (int i=0 ; i<thread_count ; i++) {
        pthread_create(&threads[i], NULL, initWorkerThread, (void*)&blocks[i]);
    }

    // main thread loop
    while (1) {

        // wait to synchronise with the worker threads at barrier 1
        pthread_barrier_wait(&barrier_1);

        // check if no value has been changed to a certain precision, 
        // if so end program, if not reset the value_change_flag to 0
        if (value_change_flag == 0) {
            break;
        } else {
            value_change_flag = 0;
        }

        // update matrix with the new values contained in the temporary arrays
        updateMatrix();

        // wait to synchronise with worker threads at barrier 2
        pthread_barrier_wait(&barrier_2);

    }

    // output matrix
    printMatrix();

    return 0;
}