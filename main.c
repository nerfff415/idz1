#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define BUF_SIZE 5000

int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];     // буфер для чтения данных из файла
    char rev_buf[BUF_SIZE]; // буфер для изменения порядка следования слов
    char out_buf[BUF_SIZE]; // буфер для вывода данных в файл
    int fd[2];              // дескрипторы неименованного канала
    int fd2[2];             // дескрипторы второго неименованного канала
    pid_t pid1, pid2;       // идентификаторы процессов
    int in_fd, out_fd;      // дескрипторы входного и выходного файлов
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        exit(1);
    }
    if ((in_fd = open(argv[1], O_RDONLY)) == -1)
    {
        perror("open input_file");
        exit(1);
    }
    if ((out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
    {
        perror("open output_file");
        exit(1);
    }
    if (pipe(fd) == -1)
    {
        perror("pipe");
        exit(1);
    }

    // создание второго неименованного канала
    if (pipe(fd2) == -1)
    {
        perror("pipe");
        exit(1);
    }

    // создание первого процесса
    if ((pid1 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid1 == 0)
    {
        // процесс чтения данных из файла и записи в первый неименованный канал
        close(fd[0]);
        while ((read(in_fd, buf, BUF_SIZE)) > 0)
        {
            write(fd[1], buf, strlen(buf));
        }
        close(fd[1]);
        exit(0);
    }
    else
    {
        // закрытие write end первого неименованного канала
        close(fd[1]);
    }

    // создание второго процесса
    if ((pid2 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid2 == 0)
    {
        // процесс изменения порядка следования слов в ASCII-строке символов
        close(fd[1]);
        close(fd2[0]);
        int n;
        int i = 0;
        char *token;
        char *rest = buf;
        while ((n = read(fd[0], buf, BUF_SIZE)) > 0)
        {
            buf[n] = '\0';
            int i = 0;
            char *tokens[BUF_SIZE];
            int num_tokens = 0;
            while ((token = strtok_r(rest, " ", &rest)))
            {
                tokens[num_tokens++] = token;
            }
            rev_buf[0] = '\0';                        // clear the output buffer before adding tokens
            for (int k = num_tokens - 1; k >= 0; k--) // loop over tokens in reverse order and add them to output buffer
            {
                strcat(rev_buf, tokens[k]); // add token to output buffer
                strcat(rev_buf, " ");       // add space after token
            }
            rev_buf[strlen(rev_buf) - 1] = '\0'; // remove the extra space added after the last token
            write(fd2[1], rev_buf, strlen(rev_buf));
        }
        close(fd[0]);
        close(fd2[1]);
        exit(0);
    }
    // процесс вывода данных в файл
    close(fd2[1]);
    pid_t pid3;
    if ((pid3 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid3 == 0)
    {
        // процесс вывода данных в заданный файл
        close(fd2[1]);
        while ((read(fd2[0], out_buf, BUF_SIZE)) > 0)
        {
            write(out_fd, out_buf, strlen(out_buf));
        }
        close(fd2[0]);
        exit(0);
    }
    else
    {
        // закрытие ненужных дескрипторов в родительском процессе
        close(fd[0]);
        close(fd2[0]);
        close(fd2[1]);
        close(out_fd);
        wait(NULL);
        wait(NULL);
        wait(NULL);
    }

    // ожидание завершения процессов
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // закрытие файловых дескрипторов
    close(in_fd);
    close(out_fd);

    return 0;
}