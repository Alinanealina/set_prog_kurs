#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
const int N = 1024;
int f = 1;
void rec_msg(int sock)
{
    int len;
    char buf[N];
    while (1)
    {
        if ((len = recv(sock, buf, N, 0)) < 0)
        {
            perror(" Плохое получение потоком.");
            return;
        }
        buf[len] = '\0';
        printf(" %s", buf);
        if (strcmp("Отказано в доступе.", buf) == 0)
        {
            printf(" Нажмите Enter, чтобы выйти.");
            fflush(stdout);
            f = 0;
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    int sock;
    char buf[N];
    struct sockaddr_in servAddr;
    struct hostent* hp, * gethostbyname();
    pthread_t th;
    pthread_attr_t ta;
    if (argc < 3)
    {
        printf(" Ввести IP_сервера порт\n");
        return 1;
    }
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror(" Сокет не открылся.");
        return 1;
    }
    bzero(&servAddr, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    hp = gethostbyname(argv[1]);
    bcopy(hp->h_addr, &servAddr.sin_addr, hp->h_length);
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
    {
        perror(" Не удалось соединиться.\n");
        return 1;
    }

    if (pthread_create(&th, &ta, (void*)rec_msg, (void*)sock) < 0)
    {
        perror(" Проблема с потоком.");
        return 1;
    }
    printf(" Клиент готов.\n");

    while (f)
    {
        bzero(buf, N);
        fgets(buf, N, stdin);
        if (send(sock, (const char*)buf, strlen(buf), 0) < 0)
        {
            perror(" Не удалось отправить.\n");
            return 1;
        }
    }

    close(sock);
    return 0;
}