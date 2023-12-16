#define _POSIX_C_SOURCE 200112L

/* using strict C mode disables a few libraries, including something that stops pthread_barrier_t from getting defined.
we use this constant to counter that. */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_SIZE 10000
#define NUM_CORES sysconf(_SC_NPROCESSORS_ONLN)
#define NUM_THREADS NUM_CORES

pthread_barrier_t barrier;


// WRITING THE TIMING RESULTS INTO A FILE:
// function to write the results obtained from sequential and parallel into a file to plot later on.
void write_timing_results(double size, double par, double seq)
{
	FILE *fp = fopen("timing_results.txt", "a");
	if (!fp)
	{
		fprintf(stderr, "Error opening file for writing.\n");
		exit(EXIT_FAILURE);
	}
	else if(fp) 
	{
		fprintf(fp, "%.0f %.6f %.6f\n", size, par, seq);
		fclose(fp);
	}
}

// GET LIVE TIME:
// function to get the current time and then convert it into seconds.
double get_current_time()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

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
		// scaling the generated values to have a maximum of 3 digits after the decimal for consistency.
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
	double start_time = get_current_time();

	double ans = 0.0;
	for (size_t i = 0; i < size; i++)
		for (size_t j = 0; j < size; j++)
			for (size_t k = 0; k < size; k++)
				ans += m1[i][k] * m2[k][j];

	double end_time = get_current_time();

	double result = ((double)((int)(ans * 1000)) / 1000.0);
	return end_time - start_time;
}

// STRUCT FOR THREAD FUNCTION:
// structure containing arguments required by the parallel process.
typedef struct
{
	int start_row;
	int end_row;
	double **m1;
	double **m2;
	int size;
	double **result;
} matrix_multiply_args;

// THREAD FUNCTION:
/* function that multiplies rows, each thread uses this function.
the function was writing to the same result variable again and again, causing a race condition while threading.
so we implemented mutex to counter that.*/
void *multiply_rows(void *m_args)
{
	matrix_multiply_args *args = (matrix_multiply_args *)m_args;

	// initialize a local variable to accumulate the result for a thread
	double **local_result = (double **)malloc(args->size * sizeof(double));

	// allocating memory for the local result var
	for (int i = 0; i < args->size; i++)
		local_result[i] = (double *)malloc(args->size * sizeof(double));

	for (int i = 0; i < args->size; i++)
		for (int j = 0; j < args->size; j++)
			local_result[i][j] = 0.0;

	for (int i = args->start_row; i < args->end_row; i++)
		for (int j = 0; j < args->size; j++)
			for (int k = 0; k < args->size; k++)
				local_result[i][j] += args->m1[i][k] * args->m2[k][j];

	// wait for all threads to finish before proceeding
	pthread_barrier_wait(&barrier);

	// update the global result matrix in a critical section.
	// no need for a lock here due to the barrier synchronization.
	for (int i = 0; i < args->size; i++)
		for (int j = 0; j < args->size; j++)
			args->result[i][j] += local_result[i][j];

	// free the allocated memory after the completion of the process.
	for (int i = 0; i < args->size; i++)
		free(local_result[i]);

	pthread_exit(NULL);
}

// PARALLEL MULTIPLIER:
// multiplication using pthreads.
double matrix_multiplication_par(double **m1, double **m2, int size)
{
	double **result = (double **)malloc(size * sizeof(double *));
	for (int i = 0; i < size; i++)
		result[i] = (double *)malloc(size * sizeof(double));

	double start_time = get_current_time();

	pthread_t threads[NUM_THREADS];
	matrix_multiply_args args[NUM_THREADS];

	// dividing the work among the threads.
	// dividing the work evenly, if not possible, we try to make it as efficient as possible.
	int thread_rows = size / NUM_THREADS;
	int remaining_rows = size % NUM_THREADS;
	int start_row = 0;

	for (int i = 0; i < NUM_THREADS; i++)
	{
		args[i].start_row = start_row;
		args[i].end_row = start_row + thread_rows + (i < remaining_rows ? 1 : 0);
		// ternary: if i is less than remaining_rows, i is 1 else it's 0.

		args[i].m1 = m1;
		args[i].m2 = m2;
		args[i].size = size;
		args[i].result = result;

		pthread_create(&threads[i], NULL, multiply_rows, (void *)&args[i]);
		start_row = args[i].end_row;
	}

	// wait for all threads to finish
	for (int i = 0; i < NUM_THREADS; i++)
		pthread_join(threads[i], NULL);

	double end_time = get_current_time();

	double result_final = args[NUM_THREADS - 1].result[size - 1][size - 1];

	return end_time - start_time;
}

// CALLER:
// calls parallel and seq.
void caller(int set_parallel, double **m1, double **m2, int size)
{
	double parallel_time, sequential_time;
	if (set_parallel == 1)
	{
		printf("Time Elapsed for Matrix Multiplication (Parallel):");
		parallel_time = matrix_multiplication_par(m1, m2, size);
		printf("%.3f\n", parallel_time);
		set_parallel = 0;
		caller(set_parallel, m1, m2, size);
	}
	else if (set_parallel == 0)
	{
		printf("Time Elapsed for Matrix Multiplication (Sequential):");
		sequential_time = matrix_multiplication_seq(m1, m2, size);
		printf("%.3f\n", sequential_time);
		write_timing_results(size, parallel_time, sequential_time);
		return;
	}
}

// MAIN:
// driver code.
int main()
{
	printf("Number of threads: %ld", NUM_THREADS);
	printf("\n");
	int size = 0;
	printf("Enter the matrix size: ");
	scanf("%i", &size);

	if (size > MAX_SIZE)
	{
		fprintf(stderr, "Matrix size exceeds the maximum allowed size.\n");
		exit(EXIT_FAILURE);
	}

	double **matrix = matrix_maker(size);
	double **matrix2 = matrix_maker(size);

	// Initialize the barrier
	if (pthread_barrier_init(&barrier, NULL, NUM_THREADS) != 0)
	{
		fprintf(stderr, "Failed to initialize barrier.\n");
		exit(EXIT_FAILURE);
	}

	// printf("Generated matrix:\n");
	// matrix_printer(matrix, size);
	// printf("\n\n");
	// matrix_printer(matrix2, size);

	caller(1, matrix, matrix2, size);

	// freeing the allocated memory after completion of tasks:
	for (size_t i = 0; i < size; i++)
	{
		free(matrix[i]);
		free(matrix2[i]);
	}

	// Destroy the barrier
	pthread_barrier_destroy(&barrier);

	return 0;
}