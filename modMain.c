#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double** matrix_maker(int size) {
	double**  matrix = (double **)malloc(size * sizeof(double*));

	if (matrix == NULL) {
		fprintf(stderr, "Memory allocation failed. \n");
		exit(EXIT_FAILURE);
	}

	// generating random numbers.
	srand(time(NULL));


	// allocating memory for columns in each row:
	for (size_t i = 0; i < size; i++ ){
		matrix[i] = (double*)malloc(size * sizeof(double));

		if (matrix[i] == NULL) {
			fprintf(stderr, "Memory allocation failed. \n");

			// freeing the previously allocated memory:
			for (size_t j = 0; j < i; j++)
				free(matrix[j]);

			free(matrix);

			exit(EXIT_FAILURE);
		}

		// if memory allocation is successful, we fill the matrix with random values:
		for (size_t j = 0; j < size; j++)
			matrix[i][j] = (double)rand() / RAND_MAX;

	}

	return matrix;
}

void matrix_printer(double** matrix, int size) {
	for (size_t i = 0; i < size; i++) {
		for (size_t j = 0; j < size; j++) {
			printf("%f\t", matrix[i][j]);
		}

	printf("\n");
	}
}


int main() {
	int size = 5;
	double** matrix = matrix_maker(size);
    //double** matrix = size;

	printf("Generated matrix:\n");
    matrix_printer(matrix, size);

	// freeing the allocated memory after completion of tasks:
	for (size_t i = 0; i < size; i++)
		free(matrix[i]);

	return 0;
}
