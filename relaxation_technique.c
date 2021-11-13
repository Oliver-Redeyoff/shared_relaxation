#include <stdio.h>
#include <stdlib.h>
#include "relaxation_technique.h"

double* makeMatrix(int size) {

    // allocate memory for new matrix of given size
    double* matrix = malloc(size*size*sizeof(double));

    // put initial values in matrix
    for (int i=0 ; i<size ; i++) {
        for (int j=0 ; j<size ; j++){

            // populate with 1.0 if left or top edge, else with 0.0
            if (i==0 || j==0){
                matrix[i*size + j] = 1.0;
            } 
            else if (i == size-1 || j == size-1) {
                matrix[i*size + j] = 0.0;
            }
            else {
                matrix[i*size + j] = 0.0;
            }

        }
    }

    return matrix;
}

BLOCK* makeBlocks(double* matrix, int size, int thread_count) {

    BLOCK* blocks = malloc(thread_count*sizeof(BLOCK));
    
    // get editable part of matrix (size-2), and divides it into blocks which are
    // groups of n>=1 rows of the matrix.
    // there are two cases, either size%thread_count==0 in which case we can divide 
    // the number of rows equally inbetween the blocks, or size%thread_count>0 in
    // which case there will be thread_count-1 equally sized blocks and one block 
    // which contains size%thread_count rows

    int editable_matrix_size = size-2;
    
    int remainder = editable_matrix_size%thread_count;

    int equal_block_size = (editable_matrix_size-remainder)/thread_count;
    int equal_block_count = remainder==0 ? thread_count : thread_count-1;

    for(int i=0 ; i<equal_block_count ; i++) {
        BLOCK new_block;
        new_block.start_row = equal_block_size*i+1;
        new_block.end_row = equal_block_size*(i+1);

        blocks[i] = new_block;
    }

    if (remainder > 0) {
        BLOCK new_block;
        new_block.start_row = equal_block_size*equal_block_count+1;
        new_block.end_row = editable_matrix_size;

        blocks[thread_count-1] = new_block;
    }

    return blocks;

}

double getSuroundingAverage(int index, double* matrix, int size) {
    double top_value = matrix[index - size];
    double right_value = matrix[index + 1];
    double bottom_value = matrix[index + size];
    double left_value = matrix[index - 1];

    // printf("top value : %f\n", top_value);
    // printf("right value : %f\n", right_value);
    // printf("bottom value : %f\n", bottom_value);
    // printf("left value : %f\n", left_value);
    // printf("\n");

    return (top_value + right_value + bottom_value + left_value)/4;
}

void processBlock(BLOCK* block, double* matrix, int size) {
    int start_index = block->start_row * size;
    int end_index = (block->end_row+1) * size;

    double* new_values = malloc((end_index-start_index)*sizeof(double));

    for(int m_i=start_index ; m_i<end_index ; m_i++) {

        // get index for block new values
        int b_i = m_i-start_index;

        // keep any edge value as is
        if (b_i%size != 0 && (b_i+1)%size != 0) {
            new_values[b_i] = getSuroundingAverage(m_i, matrix, size);
        } else {
            new_values[b_i] = matrix[m_i];
        }

    }

    block->new_values = new_values;

}

void updateMatrix(double* matrix, int size, BLOCK* blocks, int block_count) {
    
    for (int i=0 ; i<block_count ; i++) {
        int start_index = blocks[i].start_row * size;
        int end_index = (blocks[i].end_row+1) * size;

        for(int m_i=start_index ; m_i<end_index ; m_i++) {
        
            // get index for block new values
            int b_i = m_i-start_index;
            
            // map block new_values to matrix values
            matrix[m_i] = blocks[i].new_values[b_i];

        }
    }

}

void printMatrix(double* matrix, int size) {
    for (int i=0 ; i<size ; i++) {
        printf("\n");
        for (int j=0 ; j<size ; j++){
            printf("%f, ", matrix[i*size + j]);
        }
    }
    printf("\n\n");
}

void printMatrixBlocks(double* matrix, int size, BLOCK* blocks, int block_count) {
    for (int i=0 ; i<size ; i++) {
        printf("\n");

        int is_start = 0;
        int is_end = 0;
        for(int q=0 ; q<block_count ; q++) {
            if(blocks[q].start_row == i) {
                is_start = 1;
                printf("\033[0;32m s%d \033[0m", q);
            }
            if(blocks[q].end_row == i) {
                is_end = 1;
                printf("\033[0;31m e%d \033[0m", q);
            }
        }
        if (is_start == 0) {
            printf("    ");
        }
        if (is_end == 0) {
            printf("    ");
        }

        for (int j=0 ; j<size ; j++){
            printf("%f, ", matrix[i*size + j]);
            if (j>10) {
                printf("...");
                break;
            }
        }
    }
    printf("\n\n");
}

void printBlocks(BLOCK* blocks, int block_count, int size) {
    for (int i=0 ; i<block_count ; i++) {
        printf("block %d:\n", i);
        for (int j=0 ; j<(blocks[i].end_row+1-blocks[i].start_row) * size ; j++){
            if(j%size==0) {
                printf("\n");
            }
            printf("%f, ", blocks[i].new_values[j]);
        }
        printf("\n");
    }
}

int main() {
    system("clear");

    int matrix_size = 20;
    int thread_count = 2;
    int decimal_precision = 4;

    // instantiate empty matrix using size
    double* matrix = makeMatrix(matrix_size);
    //printMatrix(matrix, matrix_size);

    // divide matrix into chuncks depending on number of threads
    BLOCK* blocks = makeBlocks(matrix, matrix_size, thread_count);
    printMatrixBlocks(matrix, matrix_size, blocks, thread_count);

    // need to make the following loop until the matrix is unchanged to a certain precision

    // iterate through the blocks and calculate the new values
    // each block should be processed on a different thread when I know how to do that
    for (int i=0 ; i<thread_count ; i++) {
        processBlock(&blocks[i], matrix, matrix_size);
    }

    updateMatrix(matrix, matrix_size, blocks, thread_count);

    printMatrixBlocks(matrix, matrix_size, blocks, thread_count);

    return 0;
}