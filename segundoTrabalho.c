
#include <math.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LINES 1000
#define MAX_COLUMNS 1000

#define CONTROLLER_PROCESS 0

#define LINE_INDEX_TAG 2
#define CURRENT_LINE_TAG 3
#define NEXT_LINE_TAG 4
#define DONE_ELEMENT_TAG 5
#define DONE_LINE_TAG 6
#define LINES_PER_PROCESS_TAG 7

typedef struct {
  bool recvd;
  int value;
} cached_element_t;

typedef struct {
  int line_index;
  int *current_line;
  int *next_line;
  cached_element_t *top_line;
} line_data_t;

typedef struct {
  int number_of_lines;
  int number_of_columns;
  int process_id;
  int process_count;
  int lines_to_process;
} process_data_t;

int create_tag(int tag, int line_index, int column_index) {
  return (tag) << 20 | (line_index << 10) | column_index;
}

void validador(int **matriz, int linhas, int colunas) {
  for (int i = 0; i < linhas; i++) {
    for (int j = 0; j < colunas; j++) {
      int soma = 0;
      int contador = 0;

      // Condições para verificar se existe um elemento ao redor do elemento
      // sendo processado
      const bool existe_a_direita = j + 1 < colunas;
      const bool existe_a_esquerda = j - 1 >= 0;
      const bool existe_a_baixo = i + 1 < linhas;
      const bool existe_a_cima = i - 1 >= 0;

      if (existe_a_direita) {
        soma += matriz[i][j + 1];
        contador++;
      }

      if (existe_a_esquerda) {
        soma += matriz[i][j - 1];
        contador++;
      }

      if (existe_a_baixo) {
        soma += matriz[i + 1][j];
        contador++;

        if (existe_a_direita) {
          soma += matriz[i + 1][j + 1];
          contador++;
        }

        if (existe_a_esquerda) {
          soma += matriz[i + 1][j - 1];
          contador++;
        }
      }

      if (existe_a_cima) {
        soma += matriz[i - 1][j];
        contador++;

        if (existe_a_direita) {
          soma += matriz[i - 1][j + 1];
          contador++;
        }

        if (existe_a_esquerda) {
          soma += matriz[i - 1][j - 1];
          contador++;
        }
      }

      // Calcula a média e substitui o valor atual pelo valor da média
      int media = floor((float)soma / contador);

      matriz[i][j] = media;
    }
  }
}

int **generate_matrix(int number_of_lines, int number_columns) {
  int **matrix_generated;
  matrix_generated = (int **)malloc(number_of_lines * sizeof(int *));
  for (int i = 0; i < number_of_lines; i++) {
    matrix_generated[i] = (int *)malloc(number_columns * sizeof(int));
    for (int j = 0; j < number_columns; j++) {
      matrix_generated[i][j] = rand() % 10;
    }
  }
  return matrix_generated;
}

void display_matrix(int **matrix, int number_of_lines, int number_of_columns) {
  printf("Matriz: \n");
  for (int i = 0; i < number_of_lines; i++) {
    for (int j = 0; j < number_of_columns; j++) {
      printf("%d ", matrix[i][j]);
    }

    printf("\n");
  }

  printf("\n");
}

int receive_or_get_item(process_data_t *data, line_data_t *line, int from,
                        int i) {
  if (line->top_line[i].recvd) {
    return line->top_line[i].value;
  }

  int tag = create_tag(DONE_ELEMENT_TAG, line->line_index - 1, i);

  printf("[NODE-%d] Esperando elemento M[%d][%d] de 'NODE-%d' com TAG=%x\n",
         data->process_id, line->line_index - 1, i, from, tag);

  MPI_Recv(&line->top_line[i].value, 1, MPI_INT, from, tag, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);

  line->top_line[i].recvd = true;

  printf("[NODE-%d] Recebido elemento M[%d][%d]=%d de 'NODE-%d'\n",
         data->process_id, line->line_index - 1, i, line->top_line[i].value,
         from);

  return line->top_line[i].value;
}

int process_element(process_data_t *data, line_data_t *line, int i) {
  int soma = 0;
  int contador = 0;

  int prev_process_id = (data->process_id - 1) % data->process_count;

  if (data->process_id == CONTROLLER_PROCESS) {
    prev_process_id = data->process_count - 1;
  }

  printf("[NODE-%d] Processando elemento M[%d][%d]=%d\n", data->process_id,
         line->line_index, i, line->current_line[i]);

  // Condições para verificar se existe um elemento ao redor do elemento
  // sendo processado
  const bool existe_a_direita = i + 1 < data->number_of_columns;
  const bool existe_a_esquerda = i - 1 >= 0;
  const bool existe_a_baixo = line->line_index + 1 < data->number_of_lines;
  const bool existe_a_cima = line->line_index - 1 >= 0;

  if (existe_a_direita) {
    soma += line->current_line[i + 1];
    contador++;
  }

  if (existe_a_esquerda) {
    soma += line->current_line[i - 1];
    contador++;
  }

  if (existe_a_baixo) {
    soma += line->next_line[i];
    contador++;

    if (existe_a_direita) {
      soma += line->next_line[i + 1];
      contador++;
    }

    if (existe_a_esquerda) {
      soma += line->next_line[i - 1];
      contador++;
    }
  }

  if (existe_a_cima) {
    if (existe_a_esquerda) {
      int element = receive_or_get_item(data, line, prev_process_id, i - 1);

      soma += element;
      contador++;
    }

    int element = receive_or_get_item(data, line, prev_process_id, i);

    soma += element;
    contador++;

    if (existe_a_direita) {
      int element = receive_or_get_item(data, line, prev_process_id, i + 1);

      soma += element;
      contador++;
    }
  }

  return floor((float)soma / contador);
}

void control(int np, int number_of_lines, int number_of_columns) {
  int **matrix;
  int **matrix_backup;

  int next_process_id = 1;

  matrix = generate_matrix(number_of_lines, number_of_columns);

  display_matrix(matrix, number_of_lines, number_of_columns);

  // cria uma cópia da matriz original para validação
  matrix_backup = (int **)malloc(number_of_lines * sizeof(int *));
  for (int i = 0; i < number_of_lines; i++) {
    matrix_backup[i] = (int *)malloc(number_of_columns * sizeof(int));
    for (int j = 0; j < number_of_columns; j++) {
      matrix_backup[i][j] = matrix[i][j];
    }
  }

  MPI_Bcast(&number_of_lines, 1, MPI_INT, CONTROLLER_PROCESS, MPI_COMM_WORLD);
  MPI_Bcast(&number_of_columns, 1, MPI_INT, CONTROLLER_PROCESS, MPI_COMM_WORLD);

  printf("[NODE-0] Número de linhas %d\n", number_of_lines);
  printf("[NODE-0] Número de colunas %d\n", number_of_columns);

  int lines_per_process = number_of_lines / np;
  int remaining_lines = number_of_lines % np;

  printf("[NODE-0] Linhas para cada processo %d\n", lines_per_process);
  printf("[NODE-0] Linhas restantes %d\n", remaining_lines);

  process_data_t data;
  data.process_id = CONTROLLER_PROCESS;
  data.process_count = np;
  data.number_of_lines = number_of_lines;
  data.number_of_columns = number_of_columns;
  data.lines_to_process = lines_per_process;

  for (int i = 1; i < np; i++) {
    MPI_Send(&lines_per_process, 1, MPI_INT, i, LINES_PER_PROCESS_TAG,
             MPI_COMM_WORLD);

    for (int j = 0; j < lines_per_process; j++) {
      int line_index = i + j * np;
      printf("[NODE-0] Enviando linha %d para 'NODE-%d'\n", line_index, i);
      int *current_line = matrix[line_index];

      MPI_Send(&line_index, 1, MPI_INT, i, LINE_INDEX_TAG, MPI_COMM_WORLD);
      MPI_Send(current_line, number_of_columns, MPI_INT, i, CURRENT_LINE_TAG,
               MPI_COMM_WORLD);

      if (line_index + 1 < number_of_lines) {
        int *next_line = matrix[line_index + 1];

        MPI_Send(next_line, number_of_columns, MPI_INT, i, NEXT_LINE_TAG,
                 MPI_COMM_WORLD);
      }
    }
  }

  line_data_t lines[data.lines_to_process];

  for (int i = 0; i < lines_per_process; i++) {
    lines[i].line_index = i + i * (np - 1);
    printf("\033[31;1;4m[NODE-0] Usuarei %d, com idx=%d\033[0m\n", i,
           lines[i].line_index);
    lines[i].current_line = matrix[lines[i].line_index];
    lines[i].top_line = (cached_element_t *)malloc(number_of_columns *
                                                   sizeof(cached_element_t));

    if (lines[i].line_index + 1 < number_of_lines) {
      lines[i].next_line = matrix[lines[i].line_index + 1];
    } else {
      lines[i].next_line = NULL;
    }

    for (int j = 0; j < number_of_columns; j++) {
      lines[i].top_line[j].recvd = false;
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  for (int i = 0; i < lines_per_process; i++) {
    for (int j = 0; j < number_of_columns; j++) {
      int result = process_element(&data, &lines[i], j);
      matrix[lines[i].line_index][j] = result;

      int tag = create_tag(DONE_ELEMENT_TAG, lines[i].line_index, j);

      printf(
          "[NODE-0] Enviando elemento processado M[%d][%d]=%d para 'NODE-%d' "
          "com "
          "TAG=%x\n",
          i, j, result, next_process_id, tag);

      MPI_Send(&result, 1, MPI_INT, next_process_id, tag, MPI_COMM_WORLD);
    }
    printf("[NODE-0] CONCLUIDO LINHA %d\n", lines[i].line_index);

    // Esperar a proxima linha completar

    if (i > 0) {
      int prev_completed_line = lines[i - 1].line_index;

      for (int j = 1; j < np; j++) {
        int done_line[number_of_columns];

        int tag = create_tag(DONE_LINE_TAG, prev_completed_line + j, 0);

        printf(
            "\033[31;1;4m[NODE-0] Esperando linha %d de 'NODE-%d' com "
            "TAG=%x\033[0m\n",
            prev_completed_line + j, j, tag);

        MPI_Recv(done_line, number_of_columns, MPI_INT, j, tag, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        printf("[NODE-0] Recebido linha %d de 'NODE-%d'\n",
               prev_completed_line + j, j);

        for (int k = 0; k < number_of_columns; k++) {
          matrix[prev_completed_line + j][k] = done_line[k];
        }
      }
    }
  }

  line_data_t *last_processed = &lines[lines_per_process - 1];

  if (last_processed->line_index < number_of_lines) {
    int last_line_index = last_processed->line_index;

    for (int j = 1; j < np; j++) {
      int done_line[number_of_columns];

      int tag = create_tag(DONE_LINE_TAG, last_line_index + j, 0);

      printf(
          "\033[31;1;4m[NODE-0] Esperando linha %d de 'NODE-%d' com "
          "TAG=%x\033[0m\n",
          last_line_index + j, j, tag);

      MPI_Recv(done_line, number_of_columns, MPI_INT, j, tag, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

      printf("[NODE-0] Recebido linha %d de 'NODE-%d'\n", last_line_index + j,
             j);

      for (int k = 0; k < number_of_columns; k++) {
        matrix[last_line_index + j][k] = done_line[k];
      }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  printf("[NODE-0] Matriz final\n");

  display_matrix(matrix, number_of_lines, number_of_columns);

  validador(matrix_backup, number_of_lines, number_of_columns);

  for (int i = 0; i < number_of_lines; i++) {
    for (int j = 0; j < number_of_columns; j++) {
      if (matrix[i][j] != matrix_backup[i][j]) {
        printf(
            "\033[31;1;4m[NODE-0] Matrizes diferentes na posição [%d][%d] "
            "original=%d, final=%d\033[0m\n",
            i, j, matrix_backup[i][j], matrix[i][j]);
        MPI_Finalize();
        exit(1);
      }
    }
  }

  printf(
      "\033[32;1;4m[NODE-0] Matriz resultado validada com sucesso!\033[0m\n");
}

void node(int id, int np) {
  int next_process_id = (id + 1) % np;

  process_data_t data;
  data.process_id = id;
  data.process_count = np;

  MPI_Bcast(&data.number_of_lines, 1, MPI_INT, CONTROLLER_PROCESS,
            MPI_COMM_WORLD);
  MPI_Bcast(&data.number_of_columns, 1, MPI_INT, CONTROLLER_PROCESS,
            MPI_COMM_WORLD);

  printf("[NODE-%d] Recebido número de linhas %d\n", id, data.number_of_lines);
  printf("[NODE-%d] Recebido número de colunas %d\n", id,
         data.number_of_columns);

  MPI_Recv(&data.lines_to_process, 1, MPI_INT, CONTROLLER_PROCESS,
           LINES_PER_PROCESS_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  printf("[NODE-%d] Recebido número de linhas para processar %d\n", id,
         data.lines_to_process);

  line_data_t lines[data.lines_to_process];

  for (int i = 0; i < data.lines_to_process; i++) {
    lines[i].current_line = (int *)malloc(data.number_of_columns * sizeof(int));
    lines[i].top_line = (cached_element_t *)malloc(data.number_of_columns *
                                                   sizeof(cached_element_t));

    for (int j = 0; j < data.number_of_columns; j++) {
      lines[i].top_line[j].recvd = false;
    }

    MPI_Recv(&lines[i].line_index, 1, MPI_INT, CONTROLLER_PROCESS,
             LINE_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Recv(lines[i].current_line, data.number_of_columns, MPI_INT,
             CONTROLLER_PROCESS, CURRENT_LINE_TAG, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);

    if (lines[i].line_index + 1 < data.number_of_lines) {
      lines[i].next_line = (int *)malloc(data.number_of_columns * sizeof(int));

      MPI_Recv(lines[i].next_line, data.number_of_columns, MPI_INT,
               CONTROLLER_PROCESS, NEXT_LINE_TAG, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    } else {
      lines[i].next_line = NULL;
    }

    printf("[NODE-%d] Recebido linha %d\n", id, lines[i].line_index);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  int done_line[data.number_of_columns];

  for (int i = 0; i < data.lines_to_process; i++) {
    for (int j = 0; j < data.number_of_columns; j++) {
      done_line[j] = process_element(&data, &lines[i], j);
      lines[i].current_line[j] = done_line[j];
      int tag = create_tag(DONE_ELEMENT_TAG, lines[i].line_index, j);

      printf(
          "[NODE-%d] Enviando elemento processado M[%d][%d]=%d para "
          "'NODE-%d' "
          "com "
          "TAG=%x\n",
          id, lines[i].line_index, j, done_line[j], next_process_id, tag);

      MPI_Send(&done_line[j], 1, MPI_INT, next_process_id, tag, MPI_COMM_WORLD);
    }
    printf("[NODE-%d] CONCLUIDO LINHA %d\n", id, lines[i].line_index);

    int tag = create_tag(DONE_LINE_TAG, lines[i].line_index, 0);

    printf(
        "\033[0;32m[NODE-%d] Enviando linha processada %d para "
        "'NODE-%d' com TAG=%x\033[0m\n",
        id, lines[i].line_index, CONTROLLER_PROCESS, tag);

    MPI_Send(done_line, data.number_of_columns, MPI_INT, CONTROLLER_PROCESS,
             tag, MPI_COMM_WORLD);
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char *argv[]) {
  int id, np;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Comm_size(MPI_COMM_WORLD, &np);

  if (argc < 3) {
    if (id == CONTROLLER_PROCESS) {
      printf(
          "Número de argumentos inválido! forneça linhas e colunas na linha de "
          "comando!\n");
      printf("mpirun -np <proc> <programa> <linhas> <colunas>\n");
    }

    MPI_Finalize();
    return 1;
  }

  int linhas = atoi(argv[1]);
  int colunas = atoi(argv[2]);

  if (linhas > MAX_LINES || colunas > MAX_COLUMNS) {
    if (id == CONTROLLER_PROCESS) {
      printf("Número de linhas ou colunas excede o limite máximo!\n");
    }
    MPI_Finalize();
    return 1;
  }

  if (linhas % np != 0) {
    if (id == CONTROLLER_PROCESS) {
      printf("Número de linhas deve ser divisível pelo número de processos!\n");
    }
    MPI_Finalize();
    return 1;
  }

  if (id == CONTROLLER_PROCESS) {
    printf("[NODE-0] Linhas: %d, Colunas: %d\n", linhas, colunas);
    control(np, linhas, colunas);
  } else {
    node(id, np);
  }

  MPI_Finalize();
  return 0;
}
