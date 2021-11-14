#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "relaxation_technique.h"

// declare global variables to store matrix and blocks
int thread_count;
int decimal_precision;
int decimal_precision_flag;
int matrix_size;
double* matrix;
BLOCK* blocks;

pthread_barrier_t barrier;

double* makeMatrix() {

    // allocate memory for new matrix of given size
    double* matrix = malloc(matrix_size*matrix_size*sizeof(double));

    // put initial values in matrix
    for (int i=0 ; i<matrix_size ; i++) {
        for (int j=0 ; j<matrix_size ; j++){

            // populate with 1.0 if left or top edge, else with 0.0
            if (i==0 || j==0){
                matrix[i*matrix_size + j] = 1.0;
            } 
            else if (i == matrix_size-1 || j == matrix_size-1) {
                matrix[i*matrix_size + j] = 0.0;
            }
            else {
                matrix[i*matrix_size + j] = 0.0;
            }

        }
    }

    return matrix;
}

BLOCK* makeBlocks() {

    BLOCK* blocks = malloc(thread_count*sizeof(BLOCK));

    int mutatable_indexes_count = matrix_size*matrix_size - matrix_size*2;

    int equal_block_size = ceil((double)mutatable_indexes_count/(double)thread_count);
    int last_block_size = mutatable_indexes_count%equal_block_size;
    int equal_block_count = (mutatable_indexes_count-last_block_size) / equal_block_size;

    // printf("equal block size: %d\n", equal_block_size);
    // printf("last block size: %d\n", last_block_size);
    // printf("equal block count: %d\n", equal_block_count);

    for(int i=0 ; i<equal_block_count ; i++) {
        BLOCK new_block;
        new_block.start_index = matrix_size + equal_block_size*i;
        new_block.end_index = matrix_size + equal_block_size*(i+1) - 1;

        blocks[i] = new_block;
    }

    if(last_block_size != 0) {
        BLOCK new_block;
        new_block.start_index = matrix_size + mutatable_indexes_count - last_block_size;
        new_block.end_index = matrix_size*matrix_size - matrix_size-1;

        blocks[thread_count-1] = new_block;
    }

    return blocks;

}

double getSuroundingAverage(int index) {
    double top_value = matrix[index - matrix_size];
    double right_value = matrix[index + 1];
    double bottom_value = matrix[index + matrix_size];
    double left_value = matrix[index - 1];

    // printf("top value : %f\n", top_value);
    // printf("right value : %f\n", right_value);
    // printf("bottom value : %f\n", bottom_value);
    // printf("left value : %f\n", left_value);
    // printf("\n");

    return (top_value + right_value + bottom_value + left_value)/4;
}

void processBlock(BLOCK* block) {
    int start_index = block->start_index;
    int end_index = block->end_index;

    double* new_values = malloc((end_index-start_index)*sizeof(double));

    for(int m_i=start_index ; m_i<=end_index ; m_i++) {

        // get index for block new values
        int b_i = m_i-start_index;
        //printf("processing index %d\n", m_i);

        // keep any edge value as is
        if (m_i%matrix_size != 0 && (m_i+1)%matrix_size != 0) {
            //printf("Processing index %d\n", m_i);
            double new_value = getSuroundingAverage(m_i);
            new_values[b_i] = getSuroundingAverage(m_i);
        } else {
            //printf("Ignoring index %d\n", m_i);
            new_values[b_i] = matrix[m_i];
        }

    }

    block->new_values = new_values;

}

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

void printMatrix() {
    for (int i=0 ; i<matrix_size ; i++) {
        printf("\n");
        for (int j=0 ; j<matrix_size ; j++){
            printf("%f, ", matrix[i*matrix_size + j]);
        }
    }
    printf("\n\n");
}

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
            printf("%f\033[0m, ", matrix[i*matrix_size + j]);
        }
    }
    printf("\n\n");

}

void printBlocks() {
    printf("\n\n");
    for (int i=0 ; i<thread_count ; i++) {
        printf("Block %d:\n", i);
        printf("    \033[0;32mStart index :\033[0m %d\n", blocks[i].start_index);
        printf("    \033[0;31mEnd index :\033[0m %d\n", blocks[i].end_index);
        printf("\n\n");
    }
}

void *blockProcessorThread(void *vargp)
{
    BLOCK* block = (BLOCK*)vargp;
    usleep(1000);
    processBlock(block);
    usleep(1000);
    return NULL;
}

int main() {
    system("clear");

    matrix_size = 10;
    decimal_precision = 4;
    thread_count = 3;
    pthread_t threads[thread_count];

    // instantiate empty matrix using size
    matrix = makeMatrix();
    blocks = makeBlocks();

    // this flag will be set to 0 if any thread sees that a value differs from the last value they computed
    decimal_precision_flag = 0;

    for(int j=0 ; j<1 ; j++) {

        //system("clear");
        //printMatrixBlocks();

        for(int i=0 ; i<thread_count ; i++) {
            pthread_create(&threads[i], NULL, blockProcessorThread, (void *)&blocks[i]);
        }
        for(int i=0 ; i<thread_count ; i++) {
            pthread_join(threads[i], NULL);
        }

        updateMatrix();

    }

    return 0;
}