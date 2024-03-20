#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define MAX_ELEMENTS 1000

int **generateMatrix(int number_of_lines, int number_columns)
{
    int **matrix_generated;
    matrix_generated = (int **)malloc(number_of_lines * sizeof(int *));
    for (int i = 0; i < number_of_lines; i++)
    {
        matrix_generated[i] = (int *)malloc(number_columns * sizeof(int));
        for (int j = 0; j < number_columns; j++)
        {
            matrix_generated[i][j] = rand() % 10;
        }
    }
    return matrix_generated;
}

void displayMatrix(int **matrix, int number_of_lines, int number_of_columns)
{
    // printf("Matriz: \n");
    for (int i = 0; i < number_of_lines; i++)
    {
        for (int j = 0; j < number_of_columns; j++)
        {
            printf("%d\t", matrix[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    int id, np;

    MPI_Status st;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    if (id == 0)
    {
        int number_of_lines = 5;
        int number_of_columns = 5;

        // printf("Digite o número de linhas da matriz: ");
        // scanf("%d", &number_of_lines);
        // printf("Digite o número de colunas da matriz: ");
        // scanf("%d", &number_of_columns);

        int **initial_matrix = generateMatrix(number_of_lines, number_of_columns);
        displayMatrix(initial_matrix, number_of_lines, number_of_columns);
    }

    MPI_Finalize();
}