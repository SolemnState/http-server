#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "net.h"

#define BACKLOG 10	 // Число подключений, которое может обработать сервер

/**
 * Получение IP адреса (IPv4 или IPv6)
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * Возвращает главный прослушиваемый сокет
 *
 * Возвращает -1, если ошибка
 */
int get_listener_socket(char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;
    int rv;

    // Тут мы ищем подходящий нам интерфейс локальной сети
    // и находим тот, который подходит нам:
    // IPv4 или IPv6 (AF_UNSPEC)
    // TCP (SOCK_STREAM)
    // И пользуется адресом на нашем компьютере (AI_PASSIVE).

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // Итерируемся по списку доступных интерфейсов
    // и пытаемся создать новый сокет на каждом.
    // Выход из цикла, если получилось создать сокет.
    for(p = servinfo; p != NULL; p = p->ai_next) {

        // Попытка создания сокета
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
                continue;
        }

        // SO_REUSEADDR предотвращает ошибку типа адрес уже используется
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt");
            close(sockfd);
            freeaddrinfo(servinfo);
            return -2;
        }

        // Проверка на возможность привязать сокет к нашему локальному адресу
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        // Выход из цикла, если привязали сокет
        break;
    }

    freeaddrinfo(servinfo);

    // Если p == NULL, то мы не смогли выйти из цикла => у нас нет сокета
    if (p == NULL)  {
        fprintf(stderr, "webserver: failed to find local address\n");
        return -3;
    }

    // Ждем новых подключений
    if (listen(sockfd, BACKLOG) == -1) {
        close(sockfd);
        return -4;
    }

    return sockfd;
}