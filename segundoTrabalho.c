#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

int **gerarMatriz(int number_of_lines, int number_columns)
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