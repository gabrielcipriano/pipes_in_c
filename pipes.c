#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define OK 0
#define NO_INPUT 1
#define TOO_LONG 2
#define SIZE_INPUT 2

// Maneira mais segura de adquirir um imput do teclado
static int getLine(char *mensagem, char *buff, size_t size) {
    int ch, extra;

    // Get line with buffer overrun protection.
    if (mensagem != NULL) {
        printf("%s \n", mensagem);
        fflush(stdout);
    }
    if (fgets(buff, size, stdin) == NULL)
        return NO_INPUT;

    // If it was too long, there'll be no newline. In that case, we flush
    // to end of line so that excess doesn't affect the next call.
    if (buff[strlen(buff) - 1] != '\n') {
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? TOO_LONG : OK;
    }

    // Otherwise remove newline and give string back to caller.
    buff[strlen(buff) - 1] = '\0';
    return OK;
}

void closePipe(int *fd) {
    close(fd[0]);
    close(fd[1]);
}

void imprimirArray(int *arr, int size) {
    printf("[");
    for (int i = 0; i < size - 1; i++)
        printf("%d, ", arr[i]);
    printf("%d]\n", arr[size - 1]);
}

int somaArray(int *array, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += array[i];
    }
    return sum;
}

int main(int argc, char *argv[]) {
    int fd[3][2];
    for (int i = 0; i < 3; i++) {
        if (pipe(fd[i]) < 0) {
            return 1;  //error
        }
    }

    int pid1 = fork();
    if (pid1 == -1) {
        return 1;  //error
    }

    if (pid1 == 0) {
        // P1 - primeiro filho
        close(fd[0][1]);
        closePipe(fd[2]);
        close(fd[1][0]);

        int x;
        //lendo X
        if (read(fd[0][0], &x, sizeof(int)) < 0) {
            return 2;
        }
        int n;
        // Lendo tamanho da mensagem
        if (read(fd[0][0], &n, sizeof(int)) < 0) {
            return 2;
        }
        char mensagem[n];
        //lendo mensagem
        if (read(fd[0][0], mensagem, sizeof(char) * n) < 0) {
            return 2;
        }
        printf("%s\n", mensagem);

        //Gerando a lista aleatoria
        int array[10];
        srand(time(NULL));
        n = rand() % 10 + 1;
        for (int i = 0; i < n; i++) {
            array[i] = rand() % x + 1;
        }

        //Enviando o tamanho da lista
        if (write(fd[1][1], &n, sizeof(int)) < 0) {
            return 2;
        }
        //Enviando a lista
        if (write(fd[1][1], array, sizeof(int) * n) < 0) {
            return 2;
        }

        close(fd[1][1]);
        close(fd[0][0]);
        return 0;
    }

    int pid2 = fork();
    if (pid2 == -1) {
        return 1;  //error
    }

    if (pid2 == 0) {
        // P2 - segundo filho
        closePipe(fd[0]);
        close(fd[1][1]);
        close(fd[2][0]);

        int n;
        if (read(fd[1][0], &n, sizeof(int)) < 0) {
            return 3;  //error
        }
        int array[n];

        if (read(fd[1][0], array, sizeof(int) * n) < 0) {
            return 3;  //error
        }
        printf("Array recebida de P1: ");
        imprimirArray(array, n);

        int soma = somaArray(array, n);

        if (write(fd[2][1], &soma, sizeof(int)) < 0) {
            return 3;  //error
        }

        close(fd[1][0]);
        close(fd[2][1]);

        return 0;
    }

    // P0 - Processo pai
    closePipe(fd[1]);
    close(fd[0][0]);
    close(fd[2][1]);

    int x = -1;
    char inteiroString[SIZE_INPUT];
    char *imprimir =
        "Escreva um numero inteiro na faixa de 1 a 5->";

    // Pegando um X entre 1 e 5 do teclado
    while (!(x >= 1 && x <= 5)) {
        if (getLine(imprimir, inteiroString, SIZE_INPUT) == OK)
            x = inteiroString[0] - '0';
    }
    // enviando X ao filho 1
    if (write(fd[0][1], &x, sizeof(int)) < 0) {
        return 1;  //error
    }

    const char *mensagem =
        "Meu filho, crie e envie para o seu irmão \
um array de números inteiros com valores randômicos entre 1 e o \
valor enviado anteriormente. O tamanho do array também deve ser \
randômico, na faixa de 1 a 10 ";
    int n = strlen(mensagem) + 1;
    // Enviando tamanho da mensagem ao filho 1
    if (write(fd[0][1], &n, sizeof(int)) < 0) {
        return 1;  //error
    }
    // Enviando mensagem ao filho 1
    if (write(fd[0][1], mensagem, sizeof(char) * n) < 0) {
        return 1;  //error
    }

    int sum;
    // Lendo a soma do filho 2
    if (read(fd[2][0], &sum, sizeof(int)) < 0) {
        return 1;  //error
    }

    printf("soma: %d \n", sum);

    close(fd[0][1]);
    close(fd[2][0]);

    int pid3 = fork();
    if (pid3 == 0) {
        // P3- terceiro filho
        closePipe(fd[0]);
        closePipe(fd[1]);
        closePipe(fd[2]);
        int filefd = creat("PipePing.txt", 0644);
        dup2(filefd, STDOUT_FILENO);
        close(filefd);
        execlp("ping", "ping", "-c", "5", "ufes.br", NULL);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}