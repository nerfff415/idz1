#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define BUF_SIZE 200

int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];
    char rev_buf[BUF_SIZE];
    char out_buf[BUF_SIZE];
    int in_fd, out_fd;
    pid_t pid1, pid2;
    int fifo_fd1, fifo_fd2;
    char *fifo_name1 = "/tmp/fifo1";
    struct stat st;

    // Создаем FIFO
    if (mkfifo(fifo_name1, 0666) == -1 && errno != EEXIST)
    {
        perror("Ошибка при создании FIFO");
        exit(EXIT_FAILURE);
    }

    // Открываем файл для чтения
    if ((in_fd = open(argv[1], O_RDONLY)) == -1)
    {
        perror("Ошибка при открытии файла для чтения");
        exit(EXIT_FAILURE);
    }

    // Создаем первый процесс
    if ((pid1 = fork()) == -1)
    {
        perror("Ошибка при создании первого процесса");
        exit(EXIT_FAILURE);
    }
    else if (pid1 == 0)
    {
        // Код первого процесса

        // Открываем FIFO для записи
        if ((fifo_fd1 = open(fifo_name1, O_WRONLY)) == -1)
        {
            perror("Ошибка при открытии FIFO для записи");
            exit(EXIT_FAILURE);
        }

        // Читаем данные из файла и записываем их в FIFO
        int bytes_read;
        while ((bytes_read = read(in_fd, buf, BUF_SIZE)) > 0)
        {
            int bytes_written = 0;
            while (bytes_written < bytes_read)
            {
                int res = write(fifo_fd1, buf + bytes_written, bytes_read - bytes_written);
                if (res == -1)
                {
                    perror("Ошибка при записи в FIFO");
                    exit(EXIT_FAILURE);
                }
                bytes_written += res;
            }
        }
        if (bytes_read == -1)
        {
            perror("Ошибка при чтении файла");
            exit(EXIT_FAILURE);
        }

        // Закрываем FIFO и файл
        close(fifo_fd1);
        close(in_fd);

        // Завершаем первый процесс
        exit(EXIT_SUCCESS);
    }

    // Создаем второй процесс
    if ((pid2 = fork()) == -1)
    {
        perror("Ошибка при создании второго процесса");
        exit(EXIT_FAILURE);
    }
    else if (pid2 == 0)
    {
        // Код второго процесса

        // Открываем FIFO для чтения
        if ((fifo_fd2 = open(fifo_name1, O_RDONLY)) == -1)
        {
            perror("Ошибка при открытии FIFO для чтения");
            exit(EXIT_FAILURE);
        }

        // Открываем файл для записи
        if ((out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
        {
            perror("Ошибка при открытии файла для записи");
            exit(EXIT_FAILURE);
        }

        // Обрабатываем данные из FIFO и записываем их в файл
        int bytes_read, bytes_written;
        while ((bytes_read = read(fifo_fd2, buf, BUF_SIZE)) > 0)
        {
            int i = 0;
            char *tokens[BUF_SIZE];
            int num_tokens = 0;
            char *token;
            char *rest = buf;
            while ((token = strtok_r(rest, " ", &rest)))
            {
                tokens[num_tokens++] = token;
            }
            rev_buf[0] = '\0';
            for (int k = num_tokens - 1; k >= 0; k--)
            {
                strcat(rev_buf, tokens[k]);
                strcat(rev_buf, " ");
            }
            rev_buf[strlen(rev_buf) - 1] = '\0';
            bytes_written = write(out_fd, rev_buf, bytes_read);
            if (bytes_written == -1)
            {
                perror("Ошибка при записи в файл");
                exit(EXIT_FAILURE);
            }
        }
        if (bytes_read == -1)
        {
            perror("Ошибка при чтении FIFO");
            exit(EXIT_FAILURE);
        }

        // Закрываем FIFO и файл
        close(fifo_fd2);
        close(out_fd);

        // Завершаем второй процесс
        exit(EXIT_SUCCESS);
    }

    // Ждем завершения обоих процессов
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    // Удаляем FIFO
    if (stat(fifo_name1, &st) == 0)
    {
        if (unlink(fifo_name1) == -1)
        {
            perror("Ошибка при удалении FIFO");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}