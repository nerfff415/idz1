#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUF_SIZE 5000

int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];
    char rev_buf[BUF_SIZE];
    char out_buf[BUF_SIZE];
    int in_fd, out_fd;
    pid_t pid1, pid2, pid3;
    int fifo_fd1, fifo_fd2;
    char *fifo_name1 = "/tmp/fifo1";
    char *fifo_name2 = "/tmp/fifo2";

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

    // create named pipes
    if (mkfifo(fifo_name1, 0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }

    if (mkfifo(fifo_name2, 0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }

    if ((pid1 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid1 == 0)
    {
        close(STDIN_FILENO);
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
        close(STDOUT_FILENO);
        fifo_fd1 = open(fifo_name1, O_WRONLY);
        while (fgets(buf, BUF_SIZE, stdin) != NULL)
        {
            write(fifo_fd1, buf, strlen(buf));
        }
        close(fifo_fd1);
        exit(0);
    }

    if ((pid2 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid2 == 0)
    {
        close(STDIN_FILENO);
        fifo_fd1 = open(fifo_name1, O_RDONLY);
        close(STDOUT_FILENO);
        fifo_fd2 = open(fifo_name2, O_WRONLY);
        while (read(fifo_fd1, buf, BUF_SIZE) > 0)
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
            write(fifo_fd2, rev_buf, strlen(rev_buf));
        }
        close(fifo_fd1);
        close(fifo_fd2);
        exit(0);
    }
    if ((pid3 = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid3 == 0)
    {
        close(STDIN_FILENO);
        fifo_fd2 = open(fifo_name2, O_RDONLY);
        while (read(fifo_fd2, buf, BUF_SIZE) > 0)
        {
            int i = 0;
            while (buf[i] != '\0')
            {
                out_buf[i] = buf[i];
                i++;
            }
            write(out_fd, out_buf, strlen(out_buf)); // запись в файл output.txt
        }
        close(fifo_fd2);
        close(out_fd); // закрытие файлового дескриптора
        exit(0);
    }
    // wait for child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    // cleanup
    unlink(fifo_name1);
    unlink(fifo_name2);
    close(in_fd);
    close(out_fd);

    return 0;
}