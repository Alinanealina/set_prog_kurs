#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#define M 3
const int N = 1024;
int n, s[M];
pthread_mutex_t st_mutex;
int add_sock(int sock)
{
    char* m = "Отказано в доступе.";
    pthread_mutex_lock(&st_mutex);
    if (n == M)
    {
        if (send(sock, (const char*)m, strlen(m), 0) < 0)
            perror(" Не удалось отправить.");
        else
        {
            printf(" Много клиентов.\n");
            fflush(stdout);
        }
        pthread_mutex_unlock(&st_mutex);
        return 0;
    }
    s[n++] = sock;
    pthread_mutex_unlock(&st_mutex);
    return 1;
}

void del_sock(int sock)
{
    int i, j;
    pthread_mutex_lock(&st_mutex);
    for (i = 0, j = 0; j < n; i++, j++)
    {
        if (s[i] == sock)
            j++;
        s[i] = s[j];
    }
    n--;
    close(sock);
    pthread_mutex_unlock(&st_mutex);
}

void msg(int sock)
{
    char buf[N], m[N + 12];
    int i, len;
    while (1)
    {
        bzero(buf, N);
        len = recv(sock, buf, N, 0);
        if (len < 0)
        {
            perror(" Плохое получение потоком.");
            return;
        }
        else if (len == 0)
        {
            printf(" Пользователь %d ушел.\n", sock);
            fflush(stdout);
            break;
        }
        else
        {
            buf[len] = '\0';
            sprintf(m, " Сокет %d: ", sock);
            strcat(m, buf);
            pthread_mutex_lock(&st_mutex);
            for (i = 0; i < n; i++)
            {
                if ((s[i] != sock) && (send(s[i], (const char*)m, strlen(m), 0) < 0))
                {
                    perror(" Не удалось отправить.\n");
                    return;
                }
            }
            pthread_mutex_unlock(&st_mutex);
        }
    }
    del_sock(sock);
}

int main()
{
    int sock_s, sock_c, len;
    struct sockaddr_in servAddr;
    pthread_t th;
    pthread_attr_t ta;
    if ((sock_s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror(" Сокет не открылся.");
        return 1;
    }
    bzero(&servAddr, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = 0;
    if (bind(sock_s, (const struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
    {
        perror(" Сервер не удалось связать.");
        return 1;
    }
    len = sizeof(servAddr);
    if (getsockname(sock_s, (struct sockaddr*)&servAddr, &len))
    {
        perror(" Проблема с getsockname.");
        return 1;
    }
    printf(" Номер порта сервера: %d\n", ntohs(servAddr.sin_port));
    listen(sock_s, M);

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&st_mutex, 0);

    while (1)
    {
        if ((sock_c = accept(sock_s, 0, 0)) < 0)
        {
            perror(" Проблема с сокетом клиента.");
            return 1;
        }
        if (!add_sock(sock_c))
            continue;
        printf(" Socket для клиента: %d\n", sock_c);
        fflush(stdout);
        if (pthread_create(&th, &ta, (void*)msg, (void*)sock_c) < 0)
        {
            perror(" Проблема с потоком.");
            return 1;
        }
    }
    return 0;
}