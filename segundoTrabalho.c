
#include <math.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG false

// Desabilita cores automaticamente no windows
#if defined(__WIN32) || defined(__WIN64)
#define USE_COLOR false
#else
#define USE_COLOR true
#endif

#define MAX_LINES 1000
#define MAX_COLUMNS 1000

#define CONTROLLER_PROCESS 0

#define LINE_INDEX_TAG 2
#define CURRENT_LINE_TAG 3
#define NEXT_LINE_TAG 4
#define DONE_ELEMENT_TAG 5
#define DONE_LINE_TAG 6
#define LINES_PER_PROCESS_TAG 7

/*
  cached_element_t
  Estrutura de dados para armazenar elementos recebidos de outros processos
  Como o valor do elemento e se ele foi recebido ou não
*/
typedef struct {
  bool recvd;
  int value;
} cached_element_t;

/*
  line_data_t
  Estrutura de dados para armazenar informações sobre a linha
  Como o índice da linha
  A linha em si
  A linha seguinte
  E a linha anterior, que faz um cache dos elementos recebidos do
  processo que a processou
*/
typedef struct {
  int line_index;
  int *current_line;
  int *next_line;
  cached_element_t *top_line;
} line_data_t;

/*
  process_data_t
  Estrutura de dados para armazenar informações sobre o processo
  Como a quantidade de linhas a serem processadas por ele
  O número de linhas e colunas da matriz
  E o id do processo
*/
typedef struct {
  int number_of_lines;
  int number_of_columns;
  int process_id;
  int process_count;
  int lines_to_process;
} process_data_t;

/*
  Macros para imprimir informações e mensagens de debug
  Com cores se USE_COLOR for verdadeiro
*/
#define info(id, f_, ...)                             \
  do {                                                \
    if (USE_COLOR) {                                  \
      int color = (id + 5) % 8;                       \
      printf("\033[0;9%dm[PROCESSO-%d] ", color, id); \
      printf((f_), ##__VA_ARGS__);                    \
      printf("\033[0m\n");                            \
                                                      \
    } else {                                          \
      printf("[PROCESSO-%d] ", id);                   \
      printf((f_), ##__VA_ARGS__);                    \
      printf("\n");                                   \
    }                                                 \
  } while (0)

#define debug(id, f_, ...)                              \
  do {                                                  \
    if (DEBUG) {                                        \
      if (USE_COLOR) {                                  \
        int color = (id + 5) % 8;                       \
        printf("\033[0;9%dm[PROCESSO-%d] ", color, id); \
        printf((f_), ##__VA_ARGS__);                    \
        printf("\033[0m\n");                            \
                                                        \
      } else {                                          \
        printf("[PROCESSO-%d] ", id);                   \
        printf((f_), ##__VA_ARGS__);                    \
        printf("\n");                                   \
      }                                                 \
    }                                                   \
  } while (0)

/*
  create_tag
  Função para criar uma tag para mensagens MPI
  A tag é composta por um identificador de tipo de mensagem
  O índice da linha e o índice da coluna

  Ficam 12 bits para a tag
  10 bits para o índice da linha
  10 bits para o índice da coluna
*/
int create_tag(int tag, int line_index, int column_index) {
  return (tag) << 20 | (line_index << 10) | column_index;
}

/*
  validador
  Função que executa o algoritmo em modo sincrono
  Usada para validadar a matriz calculada pelo algoritmo em MPI
*/
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

/*
  generate_matrix
  Função para gerar uma matriz de números aleatórios
  Recebe o número de linhas e colunas da matriz
  Retorna um ponteiro para a matriz gerada
*/
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

/*
  display_matrix
  Função para exibir a matriz na tela
  Recebe a matriz, o número de linhas e o número de colunas
  Imprime a matriz na tela

  Só é usada pelo processo 0
*/
void display_matrix(int **matrix, int number_of_lines, int number_of_columns) {
  char *matrix_to_print =
      (char *)malloc(number_of_columns * number_of_lines * 6 * sizeof(char));

  for (int i = 0; i < number_of_lines; i++) {
    sprintf(matrix_to_print + strlen(matrix_to_print), "[ ");
    for (int j = 0; j < number_of_columns; j++) {
      sprintf(matrix_to_print + strlen(matrix_to_print), "%d ", matrix[i][j]);
    }

    sprintf(matrix_to_print + strlen(matrix_to_print), "]\n");
  }

  info(CONTROLLER_PROCESS, "\n\n%s", matrix_to_print);

  free(matrix_to_print);
}

/*
  receive_or_get_item
  Função para receber um elemento de outro processo
  Se o elemento já foi recebido, retorna o valor do elemento
  Se não, recebe o elemento do processo e retorna o valor
*/
int receive_or_get_item(process_data_t *data, line_data_t *line, int from,
                        int i) {
  if (line->top_line[i].recvd) {
    return line->top_line[i].value;
  }

  int tag = create_tag(DONE_ELEMENT_TAG, line->line_index - 1, i);

  debug(data->process_id,
        "Esperando elemento M[%d][%d] de 'PROCESSO-%d' com TAG=0x%x",
        line->line_index - 1, i, from, tag);

  MPI_Recv(&line->top_line[i].value, 1, MPI_INT, from, tag, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);

  line->top_line[i].recvd = true;

  debug(data->process_id, "Recebido elemento M[%d][%d]=%d de 'PROCESSO-%d'",
        line->line_index - 1, i, line->top_line[i].value, from);

  return line->top_line[i].value;
}

/*
  process_element
  Função para processar um elemento da matriz
  Recebe a estrutura de dados do processo, a linha e o índice do elemento
  Retorna o valor do elemento processado
*/
int process_element(process_data_t *data, line_data_t *line, int i) {
  int soma = 0;
  int contador = 0;

  int prev_process_id = (data->process_id - 1) % data->process_count;

  if (data->process_id == CONTROLLER_PROCESS) {
    prev_process_id = data->process_count - 1;
  }

  debug(data->process_id, "Processando elemento M[%d][%d]=%d", line->line_index,
        i, line->current_line[i]);

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

/*
  control
  Função do processo 0
  Responsável por coordenar a execução do algoritmo
  Recebe o número de processos, o número de linhas e o número de colunas
  Gera a matriz, envia as linhas para os processos e processa as linhas
  Recebe as linhas processadas e valida a matriz final
*/
void control(int np, int number_of_lines, int number_of_columns) {
  int **matrix;
  int **matrix_backup;

  int next_process_id = 1;

  matrix = generate_matrix(number_of_lines, number_of_columns);

  info(CONTROLLER_PROCESS, "Matriz gerada:");

  display_matrix(matrix, number_of_lines, number_of_columns);

  // cria uma cópia da matriz original para validação
  matrix_backup = (int **)malloc(number_of_lines * sizeof(int *));
  for (int i = 0; i < number_of_lines; i++) {
    matrix_backup[i] = (int *)malloc(number_of_columns * sizeof(int));
    for (int j = 0; j < number_of_columns; j++) {
      matrix_backup[i][j] = matrix[i][j];
    }
  }

  info(CONTROLLER_PROCESS, "Número de linhas %d", number_of_lines);
  info(CONTROLLER_PROCESS, "Número de colunas %d", number_of_columns);

  int lines_per_process = number_of_lines / np;

  info(CONTROLLER_PROCESS, "Linhas para cada processo %d", lines_per_process);

  process_data_t data;
  data.process_id = CONTROLLER_PROCESS;
  data.process_count = np;
  data.number_of_lines = number_of_lines;
  data.number_of_columns = number_of_columns;
  data.lines_to_process = lines_per_process;

  // Envia para cada processo o número de linhas que ele deve processar
  // e as linhas que ele deve processar.
  // De maneira intercalada, para que nenhum processo receba linhas consecutivas
  for (int i = 1; i < np; i++) {
    MPI_Send(&lines_per_process, 1, MPI_INT, i, LINES_PER_PROCESS_TAG,
             MPI_COMM_WORLD);

    for (int j = 0; j < lines_per_process; j++) {
      int line_index = i + j * np;
      info(CONTROLLER_PROCESS, "Enviando linha %d para 'PROCESSO-%d'",
           line_index, i);
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

  // Aloca e inicializa as linhas que o processo 0 é responsável por processar
  line_data_t lines[data.lines_to_process];

  for (int i = 0; i < lines_per_process; i++) {
    lines[i].line_index = i + i * (np - 1);
    debug(CONTROLLER_PROCESS, "Escolhido linha %d", lines[i].line_index);
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

  // Aguarde que todos os processos tenham recebido suas linhas
  MPI_Barrier(MPI_COMM_WORLD);

  // Processa cada linha e envia cada elemento processado para o processo
  // vizinho
  for (int i = 0; i < lines_per_process; i++) {
    for (int j = 0; j < number_of_columns; j++) {
      int result = process_element(&data, &lines[i], j);
      matrix[lines[i].line_index][j] = result;

      int tag = create_tag(DONE_ELEMENT_TAG, lines[i].line_index, j);

      info(CONTROLLER_PROCESS,
           "Concluído elemento M[%d][%d] enviando para "
           "'PROCESSO-%d'",
           lines[i].line_index, j, next_process_id);

      MPI_Send(&result, 1, MPI_INT, next_process_id, tag, MPI_COMM_WORLD);
    }

    info(CONTROLLER_PROCESS, "Concluído linha %d", lines[i].line_index);

    // Ao terminar uma linha, espera todas as linhas anteriores serem
    // processadas e as recebe
    if (i > 0) {
      int prev_completed_line = lines[i - 1].line_index;

      for (int j = 1; j < np; j++) {
        int done_line[number_of_columns];

        int tag = create_tag(DONE_LINE_TAG, prev_completed_line + j, 0);

        debug(CONTROLLER_PROCESS,
              "Esperando linha %d de 'PROCESSO-%d' com "
              "TAG=0x%x",
              prev_completed_line + j, j, tag);

        MPI_Recv(done_line, number_of_columns, MPI_INT, j, tag, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        info(CONTROLLER_PROCESS, "Recebido linha %d de 'PROCESSO-%d'",
             prev_completed_line + j, j);

        // Atualiza a linha na matriz
        for (int k = 0; k < number_of_columns; k++) {
          matrix[prev_completed_line + j][k] = done_line[k];
        }
      }
    }
  }

  // Caso especial onde o processo-0 não process a última linha da matriz
  // Aqui espera as linhas seguintes serem processadas e as recebe
  line_data_t *last_processed = &lines[lines_per_process - 1];

  if (last_processed->line_index < number_of_lines) {
    int last_line_index = last_processed->line_index;

    for (int j = 1; j < np; j++) {
      int done_line[number_of_columns];

      int tag = create_tag(DONE_LINE_TAG, last_line_index + j, 0);

      debug(CONTROLLER_PROCESS,
            "Esperando linha %d de 'PROCESSO-%d' com "
            "TAG=0x%x",
            last_line_index + j, j, tag);

      MPI_Recv(done_line, number_of_columns, MPI_INT, j, tag, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

      info(CONTROLLER_PROCESS, "Recebido linha %d de 'PROCESSO-%d'",
           last_line_index + j, j);

      for (int k = 0; k < number_of_columns; k++) {
        matrix[last_line_index + j][k] = done_line[k];
      }
    }
  }

  // Aguarde que todos os processos tenham processado suas linhas
  MPI_Barrier(MPI_COMM_WORLD);

  info(CONTROLLER_PROCESS, "Matriz final");

  display_matrix(matrix, number_of_lines, number_of_columns);

  validador(matrix_backup, number_of_lines, number_of_columns);

  // Valida a matriz final
  for (int i = 0; i < number_of_lines; i++) {
    for (int j = 0; j < number_of_columns; j++) {
      if (matrix[i][j] != matrix_backup[i][j]) {
        info(CONTROLLER_PROCESS,
             "ERRO! Matrizes diferentes na posição [%d][%d] "
             "original=%d, final=%d",
             i, j, matrix_backup[i][j], matrix[i][j]);

        MPI_Finalize();
        exit(1);
      }
    }
  }

  info(CONTROLLER_PROCESS, "Matriz resultado validada com sucesso!");
}

/*
  node
  Função dos processos coordenados pelo processo 0
  Recebe o id do processo, o número de processos, o número de linhas e o número
  de colunas
  Recebe do processo 0 as linhas a serem processadas, processa as linhas e envia
  os elementos processados para o próximo processo Ao final de cada linha
*/
void node(int id, int np, int number_of_lines, int number_of_columns) {
  int next_process_id = (id + 1) % np;

  process_data_t data;
  data.process_id = id;
  data.process_count = np;
  data.number_of_lines = number_of_lines;
  data.number_of_columns = number_of_columns;

  debug(id, "Recebido número de linhas %d", data.number_of_lines);
  debug(id, "Recebido número de colunas %d", data.number_of_columns);

  MPI_Recv(&data.lines_to_process, 1, MPI_INT, CONTROLLER_PROCESS,
           LINES_PER_PROCESS_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  info(id, "Recebido número de linhas para processar %d",
       data.lines_to_process);

  line_data_t lines[data.lines_to_process];

  // Aloca e recebe do processo-0 cada uma das linhas a serem processadas
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

    info(id, "Recebido linha %d", lines[i].line_index);
  }

  // Aguarde que todos os processos tenham recebido suas linhas
  MPI_Barrier(MPI_COMM_WORLD);

  int done_line[data.number_of_columns];

  // Processa cada linha e envia cada elemento processado para o processo
  // seguinte
  for (int i = 0; i < data.lines_to_process; i++) {
    for (int j = 0; j < data.number_of_columns; j++) {
      done_line[j] = process_element(&data, &lines[i], j);
      lines[i].current_line[j] = done_line[j];
      int tag = create_tag(DONE_ELEMENT_TAG, lines[i].line_index, j);

      info(id,
           "Concluído elemento M[%d][%d] enviando para "
           "'PROCESSO-%d'",
           lines[i].line_index, j, next_process_id);

      MPI_Send(&done_line[j], 1, MPI_INT, next_process_id, tag, MPI_COMM_WORLD);
    }

    // Envia a linha inteira processada para o processo-0

    info(id, "Concluído linha %d", lines[i].line_index);

    int tag = create_tag(DONE_LINE_TAG, lines[i].line_index, 0);

    info(id,
         "Enviando linha processada %d para "
         "'PROCESSO-%d'",
         lines[i].line_index, CONTROLLER_PROCESS);

    MPI_Send(done_line, data.number_of_columns, MPI_INT, CONTROLLER_PROCESS,
             tag, MPI_COMM_WORLD);
  }

  // Aguarde que todos os processos tenham processado suas linhas
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
    control(np, linhas, colunas);
  } else {
    node(id, np, linhas, colunas);
  }

  MPI_Finalize();
  return 0;
}
