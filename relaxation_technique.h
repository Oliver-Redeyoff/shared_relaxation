typedef struct block {
    int start_index;
    int end_index;
    double* new_values;
} BLOCK;

double* makeMatrix();
BLOCK* makeBlocks();

double getSuroundingAverage(int index);
void processBlock(BLOCK* block);

void printMatrix();
void printMatrixBlocks();
void printBlocks();

void* initWorkerThread(void* vargp);