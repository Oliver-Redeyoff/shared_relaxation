/**
* Parallel relaxation technique
* Oliver Redeyoff
*
* Strategy:
*
* 1 - the main thread initialises 2 barriers with count set to the number n of 
*     worker threads plus 1 for the main thread, and sets the value_change_flag 
*     to 0. It then generates n worker threads which are each assigned distinct 
*     ranges of the array that they are to operate on. Go to step 2.
*
* 2 - the main thread waits at barrier 1
*   - the worker threads perform a relaxation on their assigned range of the 
*     matrix and store the results in a temporary array, if a single value 
*     differs to a given precision to the value they computed the previous 
*     cycle, set the value_change_flag to 1, then wait at barrier 1
*   - once the main thread and all worker threads are waiting at barrier 1, they 
*     are all released and we go to step 3
*
* 3 - the worker threads wait at barrier 2
*   - the main thread checks if value_change_flag is 0, if it is then end the program 
*     and output the matrix, if not it resets value_change_flag to 0 and updates the 
*     matrix with the new values which are stored in each temporary array
*
**/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include "relaxation_technique.h"

// declare global variables to store matrix and blocks
int thread_count;
int decimal_precision;
int value_change_flag;
int matrix_size;
double* matrix;
BLOCK* blocks;

// Returns array of doubles of length matrix_size^2
double* makeMatrix() {
    // allocate memory for new matrix of given size
    double* matrix = malloc(matrix_size*matrix_size*sizeof(double));

    // put initial values in matrix
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

    for(int i=0 ; i<equal_block_count ; i++) {
        BLOCK new_block;
        new_block.start_index = matrix_size + equal_block_size*i;
        new_block.end_index = matrix_size + equal_block_size*(i+1) - 1;

        double* new_values = malloc((new_block.end_index-new_block.start_index)*sizeof(double));
        new_block.new_values = new_values;

        blocks[i] = new_block;
    }

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

// Returns a double converted to a double with a decimal precision defined by decimal_precision
double withPrecision(double num) {
    return floor(pow(10, decimal_precision)*num) / pow(10, decimal_precision);
}

// Performs relaxation for range indexes of matrix defined in the given block
void processBlock(BLOCK* block) {
    int start_index = block->start_index;
    int end_index = block->end_index;

    for(int m_i=start_index ; m_i<=end_index ; m_i++) {

        // get index for block new values
        int b_i = m_i-start_index;

        // keep any edge value as is
        if (m_i%matrix_size != 0 && (m_i+1)%matrix_size != 0) {
            double new_value = getSuroundingAverage(m_i);
            if (withPrecision(new_value) != withPrecision(block->new_values[b_i])) {
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

// Prints out matrix as table
void printMatrix() {
    for (int i=0 ; i<matrix_size ; i++) {
        printf("\n");
        for (int j=0 ; j<matrix_size ; j++){
            printf("%f, ", withPrecision(matrix[i*matrix_size + j]));
        }
    }
    printf("\n\n");
}

// Prints out matrix as table, and highlights each block
void printMatrixBlocks() {
    char colors[6][20] = {"\033[0;31m", "\033[0;32m", "\033[0;33m", "\033[0;34m", "\033[0;35m", "\033[0;36m"};

    for (int i=0 ; i<matrix_size ; i++) {
        printf("\n");
        for (int j=0 ; j<matrix_size ; j++){
            int index = i*matrix_size + j;

            for(int q=0 ; q<thread_count ; q++) {
                if(index >= blocks[q].start_index && index <= blocks[q].end_index) {
                    printf("%s", colors[q%5]);
                }
            }
            printf("%f\033[0m, ", withPrecision(matrix[i*matrix_size + j]));
        }
    }
    printf("\n\n");
}

// Prints out data of each block
void printBlocks() {
    printf("\n\n");
    for (int i=0 ; i<thread_count ; i++) {
        printf("Block %d:\n", i);
        printf("    \033[0;32mStart index :\033[0m %d\n", blocks[i].start_index);
        printf("    \033[0;31mEnd index :\033[0m %d\n", blocks[i].end_index);
        printf("\n\n");
    }
}

// Entry point for worker thread
void* initWorkerThread(void* vargp) {
    BLOCK* block = (BLOCK*)vargp;

    // // worker thread loop
    // while (1) {
    //     // perform relaxation on given block
    //     processBlock(block);

    //     // wait to synchronise with main and other work threads at barrier 1
    //     pthread_barrier_wait(&barrier_1);

    //     // wait to synchronise with main and other work threads at barrier 2
    //     pthread_barrier_wait(&barrier_2);
    // }

    return NULL;
}

int main(int argc, char **argv) {

    // set global variables to passed values
    if (argc != 4) {
        printf("Too few arguments\n");
        return 1;
    }
    matrix_size = atoi(argv[1]);
    thread_count = atoi(argv[2]);
    decimal_precision = atoi(argv[3]);

    struct timeval start, end;
  
    // start timer
    gettimeofday(&start, NULL);

    // instantiate matrix
    matrix = makeMatrix();
    // instantiate blocks
    blocks = makeBlocks();

    value_change_flag = 0;

    while (1) {

        for (int i=0 ; i<thread_count ; i++) {
            processBlock(&blocks[i]);
        }

        // check if no value has been changed, if so end program, if not
        // reset the value_change_flag to 0
        if (value_change_flag == 0) {
            break;
        } else {
            value_change_flag = 0;
        }

        // update matrix with the new values contained in the temporary arrays
        updateMatrix();

        // system("clear");
        // printMatrixBlocks();
        // usleep(100000);

    }

    // end timer
    gettimeofday(&end, NULL);
  
    // calculate total time taken by the program
    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    
    //system("clear");
    //printMatrixBlocks();
    printf("%d, %d, %d, %f\n", matrix_size, thread_count, decimal_precision, time_taken);

    return 0;
}