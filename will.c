/*
* Sequential implementation of the Relaxation technique
*   Example command ./shared -n 4 -p 1 -s 4 -f ./example.txt
*
*   Arguments
*       -n (int) Number of threads to use
*       -p (int) Precision to use
*       -s (int) Size of one side of array to read in (ie 4x4 -> -s 4)
*       -f (string) File name to read from
*               For file format see example.txt
*       -o (string) File name to output to
*       -g Generate array (do not use files)
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct relaxationData
{
    int bounds[2];
    double *old_values;
    double *new_values;
    int ARRAY_DIMENSIONS_SQRT;
    double PRECISION;
    int result;
    pthread_barrier_t *completed;
    pthread_barrier_t *temp;
} RelaxationData;

double calculate_diff(double first, double second);
void print_values(int dimension, double values[]);
void load_data(char *file_name, double values[], int dimensions);
void generate_data(double *values, int dimensions);
void write_data(char *file_name, double values[], int dimensions);
void start_relaxation_thread(RelaxationData *relaxation_data);

int main(int argc, char *argv[])
{

    int ARRAY_DIMENSIONS, ARRAY_DIMENSIONS_SQRT, NUM_THREADS, GENERATE;
    unsigned long long DATA_SIZE = sizeof(double);
    double PRECISION;
    char *INPUT_FILE_NAME = "", *OUTPUT_FILE_NAME = "";

    // Parse args
    int c;
    while ((c = getopt(argc, argv, "n:p:s:f:o:g")) != -1)
    {
        int success;
        switch (c)
        {
        case 'g':
            GENERATE = 1;
            break;

        case 'p':
            success = sscanf(optarg, "%lf", &PRECISION);
            if (!success)
            {
                printf("ERROR PRECISION could not be determined from '%s'\n", optarg);
                return -1;
            }
            break;

        case 's':
            success = sscanf(optarg, "%d", &ARRAY_DIMENSIONS_SQRT);
            if (!success)
            {
                printf("ERROR ARRAY_DIMENSIONS could not be determined from '%s'\n", optarg);
                return -1;
            }
            ARRAY_DIMENSIONS = ARRAY_DIMENSIONS_SQRT * ARRAY_DIMENSIONS_SQRT;
            DATA_SIZE *= (unsigned)ARRAY_DIMENSIONS;
            break;

        case 'f':
            INPUT_FILE_NAME = optarg;
            break;

        case 'o':
            OUTPUT_FILE_NAME = optarg;
            break;

        case 'n':
            success = sscanf(optarg, "%d", &NUM_THREADS);
            if (!success)
            {
                printf("ERROR NUMBER_OF_THREADS could not be determined from '%s'\n", optarg);
                return -1;
            }
            break;

        case '?':
            printf("Unknown argument '%c'\n", optopt);
            break;
        }
    }

    // Read in file
    double *old_values = malloc(DATA_SIZE);

    if (GENERATE) {
        generate_data(old_values, ARRAY_DIMENSIONS_SQRT);
    } else {
        load_data(INPUT_FILE_NAME, old_values, ARRAY_DIMENSIONS_SQRT);
    }

    double *new_values = malloc(DATA_SIZE);
    memcpy(new_values, old_values, DATA_SIZE);

    // Perform relaxation

    int precision_reached = 0;
    int inner_array_size = (ARRAY_DIMENSIONS_SQRT - 2) * (ARRAY_DIMENSIONS_SQRT - 2);
    int bucket_size = inner_array_size / NUM_THREADS;

    pthread_t threads[NUM_THREADS];
    RelaxationData thread_data[NUM_THREADS];

    pthread_barrier_t completed;
    pthread_barrier_init(&completed, NULL, (unsigned int)NUM_THREADS+1);
    pthread_barrier_t temp;
    pthread_barrier_init(&temp, NULL, (unsigned int)NUM_THREADS+1);

    // Create thread to work on each bound of data
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int lower = i * bucket_size;
        int upper = lower + bucket_size;

        thread_data[i] = (RelaxationData){
            {lower, upper},
            old_values,
            new_values,
            ARRAY_DIMENSIONS_SQRT,
            PRECISION,
            0,
            &completed,
            &temp
        };

        pthread_create(
            &threads[i],
            NULL,
            (void *(*)(void *))start_relaxation_thread,
            // (void (*)(void *))start_relaxation_thread,
            (void *)&thread_data[i]);
    }

    while (!precision_reached)
    {
        // Wait for each thread to finish and get the retval
        pthread_barrier_wait(&completed);
        precision_reached = 1;
        for (int i = 0; i < NUM_THREADS; i++)
        {
            precision_reached &= thread_data[i].result;
        };
        memcpy(
            old_values,
            new_values,
            DATA_SIZE);
        pthread_barrier_wait(&temp);
    }

    // Write results to file
    if (0 && GENERATE) {
        print_values(ARRAY_DIMENSIONS_SQRT, new_values);
    } else {
        write_data(OUTPUT_FILE_NAME, new_values, ARRAY_DIMENSIONS_SQRT);
    }

    free(old_values);
    free(new_values);
    return 0;
}

/* Calculate the absolute difference between two doubles
*   Args:
*       first (double): Number to subtract from
*       second (double): Number to substract
*   
*   Returns:
*       (double): The absolute difference
*/
double calculate_diff(double first, double second)
{
    double diff = first - second;
    if (diff < 0)
    {
        diff *= -1;
    }
    return diff;
}

/* Pretty print the array of values
*   Args:
*       dimension (int): Size of 1 row/column
*       values (double[]): Array to print
*
*/
void print_values(int dimension, double values[])
{
    for (int i = 0; i < dimension; i++)
    {
        for (int j = 0; j < dimension; j++)
        {
            printf("%.10f\t", values[i * dimension + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/* Read data to execute on from the file
*   Args:
*       file_name (char *): File name to read from
*       values (double[]): Array to load data into
*       dimensions (int): Size of each row/column
*
*/
void load_data(char *file_name, double values[], int dimensions)
{
    FILE *file = fopen(file_name, "r");
    int line_num = 0;
    int col_num = 0;

    char c;
    char digits[1080]; // 1077 is maximum number of characters for a double
    int current_digit = 0;
    while ((c = (char)getc(file)) != EOF)
    {
        if (c == ' ' || c == '\n')
        {
            values[line_num * dimensions + col_num] = atof(digits);

            for (int i = 0; i < current_digit; i++)
            {
                digits[i] = 0;
            }
            current_digit = 0;
            if (c == ' ')
            {
                col_num++;
            }
            else
            {
                line_num++;
                col_num = 0;
            }
        }
        else
        {
            digits[current_digit] = (char)c;
            current_digit++;
        }
    }
    fclose(file);
    printf("Loaded data\n");
}

void generate_data(double *values, int dimensions) {
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            if (i == 0 || j == 0){
                values[i * dimensions + j] = 1.0;
            } else {
                values[i * dimensions + j] = 0.0;
            }
        }
    }
}

/* Write results to the file
*   Args:
*       file_name (char *): File name to read from
*       values (double[]): Array to load data into
*       dimensions (int): Size of each row/column
*
*/
void write_data(char *file_name, double values[], int dimensions)
{
    FILE *file = fopen(file_name, "w");

    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            char str[1080];
            sprintf(str, "%f", values[dimensions*i + j]);
            fputs(str, file);
            if (j < dimensions-1) {
                fputc(' ', file);
            }
        }
        fputc('\n', file);
    }
    fclose(file);
    printf("Written data\n");
}

/* Start a thread to perform relaxation calculations on the data
*   Within the given bounds
*   
*   Args:
*       relaxation_data (RelaxationData): Data to operate on, array to put data
            and the bounds to operate on
*/
void start_relaxation_thread(RelaxationData *relaxation_data)
{   
    while (1) {
        int precision_reached = 1;
        for (int i = relaxation_data->bounds[0]; i < relaxation_data->bounds[1]; i++)
        {
            int inner_array_size = relaxation_data->ARRAY_DIMENSIONS_SQRT - 2;
            int row_number = (1 + (i / inner_array_size)) * relaxation_data->ARRAY_DIMENSIONS_SQRT;
            int column_number = i % inner_array_size + 1;
            int position = row_number + column_number;
            double sum_of_values = 0;

            sum_of_values += relaxation_data->old_values[position - relaxation_data->ARRAY_DIMENSIONS_SQRT];
            sum_of_values += relaxation_data->old_values[position + relaxation_data->ARRAY_DIMENSIONS_SQRT];
            sum_of_values += relaxation_data->old_values[position - 1];
            sum_of_values += relaxation_data->old_values[position + 1];

            relaxation_data->new_values[position] = sum_of_values / 4;
            double diff = calculate_diff(
                relaxation_data->old_values[i],
                relaxation_data->new_values[i]);

            precision_reached &= diff < relaxation_data->PRECISION;
        }
        relaxation_data->result = precision_reached;
        pthread_barrier_wait(relaxation_data->completed);
        pthread_barrier_wait(relaxation_data->temp);
    }
}