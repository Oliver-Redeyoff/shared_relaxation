typedef struct block {
    int start_row;
    int end_row;
    double* new_values;
} BLOCK;

double* makeMatrix(int size);
BLOCK* makeBlocks(double* matrix, int size, int thread_count);

double getSuroundingAverage(int index, double* matrix, int size);
void processBlock(BLOCK* block, double* matrix, int size);
void updateMatrix(double* matrix, int size, BLOCK* blocks, int block_count);

void printMatrix(double* matrix, int size);
void printMatrixBlocks(double* matrix, int size, BLOCK* blocks, int block_count);
void printBlocks(BLOCK* blocks, int block_count, int size);