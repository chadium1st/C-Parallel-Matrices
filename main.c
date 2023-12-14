#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define MAX_SIZE 1000

// MATRIX GENERATOR:
// generates the matrix with random values while checking for memory leaks and exceptions.
double **matrix_maker(int size)
{
	double **matrix = (double **)malloc(size * sizeof(double *));

	if (matrix == NULL)
	{
		fprintf(stderr, "Memory allocation failed. \n");
		exit(EXIT_FAILURE);
	}

	// generating random numbers.
	srand(time(NULL));

	// allocating memory for columns in each row:
	for (size_t i = 0; i < size; i++)
	{
		matrix[i] = (double *)malloc(size * sizeof(double));

		if (matrix[i] == NULL)
		{
			fprintf(stderr, "Memory allocation failed. \n");

			// freeing the previously allocated memory:
			for (size_t j = 0; j < i; j++)
				free(matrix[j]);

			free(matrix);

			exit(EXIT_FAILURE);
		}

		// if memory allocation is successful, we fill the matrix with random values:
		for (size_t j = 0; j < size; j++)
			matrix[i][j] = (double)(rand() % 1000) / 1000.0;
		// scaling the generated values to have a maximum of 3 digits after decimal for consistency.
	}

	return matrix;
}

// MATRIX PRINTER:
// prints the generated matrix.
void matrix_printer(double **matrix, int size)
{
	for (size_t i = 0; i < size; i++)
	{
		for (size_t j = 0; j < size; j++)
		{
			printf("%.3f\t", matrix[i][j]);
		}

		printf("\n");
	}
}

// SEQUENTIAL MULTIPLIER:
// multiplication using sequential algorithm.
double matrix_multiplication_seq(double **m1, double **m2, int size)
{
	double ans = 0.0;
	for (size_t i = 0; i < size; i++)
		for (size_t j = 0; j < size; j++)
			for (size_t k = 0; k < size; k++)
				ans += m1[i][k] * m2[k][j];

	return ((double)((int)(ans * 1000)) / 1000.0);
}

// INTERFACE FOR EASE OF USE:
// structure containing arguements required by the parallel process.
typedef struct
{
	int start_row;
	int end_row;
	double** m1;
	double** m2;
	int size;
	double** result;
} matrix_multiply_args;

// THREAD FUNCTION:
// function that multiplies rows, each thread uses this function 
void* multiply_rows(void* m_args) {
    matrix_multiply_args* args = (matrix_multiply_args*)m_args;
    for (int i = args->start_row; i < args->end_row; i++)
        for (int j = 0; j < args->size; j++)
            for (int k = 0; k < args->size; k++) {
                args->result[i][j] += args->m1[i][k] * args->m2[k][j];
            }

    pthread_exit(NULL);
}

// PARALLEL MULTIPLIER:
// multiplication using pthreads.
double matrix_multiplication_par(double **m1, double **m2, int size)
{
	int num_threads = 4;
	double** result = (double**)malloc(size* sizeof(double*));
	for (int i = 0; i < size; i++)
		result[i] = (double*)malloc(size* sizeof(double));

	pthread_t threads[num_threads];
	matrix_multiply_args args[num_threads];

	// dividing the work among the threads.
	// dividing the work evenly, if not possible, we try to make it as efficient as possible.
	int thread_rows = size / num_threads;
	int remaining_rows = size % num_threads;
	int start_row = 0;

	for (int i = 0; i < num_threads; i++) {
		args[i].start_row = start_row;
		args[i].end_row = start_row + thread_rows + ( i < remaining_rows ? 1 : 0 );
		// ternary: if i is less than remaining_rows, i is 1 else its 0.

		args[i].m1 = m1;
		args[i].m2 = m2;
		args[i].size = size;
		args[i].result = result;

		pthread_create(&threads[i], NULL, multiply_rows, (void*)&args[i]);
		start_row = args[i].end_row;
	}

	// wait for all threads to finish
	for (int i = 0; i < num_threads; i++)
		pthread_join(threads[i], NULL);

	return args[num_threads - 1].result[size - 1][size - 1];
}

// MAIN:
// driver code.
int main()
{
	int size = 0, num_threads = 0;
	printf("Enter the matrix size: ");
	scanf("%i", &size);

	if (size > MAX_SIZE)
	{
		fprintf(stderr, "Matrix size exceeds maximum allowed size.\n");
		exit(EXIT_FAILURE);
	}

	double **matrix = matrix_maker(size);
	double **matrix2 = matrix_maker(size);

	printf("Generated matrix:\n");
	matrix_printer(matrix, size);
	printf("\n\n");
	matrix_printer(matrix2, size);

	printf("\n\n");
	printf("Matrix Multiplication (Sequential):");
	printf("%.3f\n", matrix_multiplication_seq(matrix, matrix2, size));

	printf("\n\n");
	printf("Matrix Multiplication (Parallel):");
	printf("%.3f\n", matrix_multiplication_par(matrix, matrix2, size));

	// freeing the allocated memory after completion of tasks:
	for (size_t i = 0; i < size; i++)
	{
		free(matrix[i]);
		free(matrix2[i]);
	}

	return 0;
}