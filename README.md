# segundo-trabalho-PC

Segundo trabalho prático desenvolvido na disciplina de Programação Concorrente, do curso de Informática, do Departamento de Informática da Universidade Estadual de Maringá.

O trabalho tinha como especificações:

♦️  Desenvolver uma aplicação multi-thread para processar uma matriz bidimensional contendo valores inteiros numa escala de 0 a 9. A ordem da matriz deve ser fornecida pelo usuário, podendo ser quadrada ou retangular. A matriz deverá ser gerada aleatoriamente e mostrada na tela. Pode-se determinar um tamanho máximo para a matriz na ordem de 1000x1000 elementos.

♦️  A matriz deverá ser percorrida, da esquerda para a direita, de cima para baixo, e cada elemento deverá ser substituído pela média aritmética simples de seus 8 vizinhos (truncada - parte inteira).

♦️  O processo 0 gera a matriz e envia para os demais processos as linhas que cada um irá precisará. O processo 0 processa a linha 0, precisa das linhas 0 e 1. O processo 1 processa a linha 1, precisa das linhas 1 e 2. O processo 2 processa a linha 2, precisa das linhas 2 e 3 e assim sucessivamente.

♦️  Além disso, cada processo precisa dos valores modificados da linha anterior, na medida em que vão ficando pronto. Assim, o processo 1 precisará dos elementos prontos da linha 0, para poder avançar nas modificações da linha 1. O processo 2 precisará dos elementos prontos da linha 1, para poder avançar nas modificações da linha 2. O processo 3 precisará dos elementos prontos da linha 2, para poder avançar nas modificações da linha 3 e assim sucessivamente.

♦️  Somente o processo 0 aloca uma matriz inteira, os demais processo alocam apenas duas linhas da matriz, a que ele irá transformar e a seguinte.

♦️  Cada processo deverá enviar os resultados dos seus cálculos (novos valores para as células) para o processo vizinho seguinte.

♦️  Ao final do processamento da sua respectiva linha, cada processo deverá envia-la de volta para o processo 0.

♦️  O processos 0 imprime a matriz modificada no vídeo

O código deve ser feito em C, utilizando a biblioteca MPI.
