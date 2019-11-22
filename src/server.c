/**
 * server.c
 * 
 * Тестировать с помощью curl:
 * 
 *    curl -D - http://localhost:3490/
 *    curl -D - http://localhost:3490/d20
 *    curl -D - http://localhost:3490/index.html
 * 
 * Можно тестировать с помощью браузера:
 *    http://localhost:3490/index.html
 *    http://localhost:3490/example.jpg
 * 
 * POST:
 * 
 *    curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/hello.txt
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include "net.h"
#include "file.h"
#include "mime.h"


#define PORT "3490"  // Порт, к которому будет подлючен пользователь

#define SERVER_FILES "./serverfiles"
#define SERVER_DATA "./server_data"

/**
 * Отправить HTTP ответ (GET)
 *
 * header:       "HTTP/1.1 404 NOT FOUND" или "HTTP/1.1 200 OK", и т.д.
 * content_type: "text/plain", и т.д.
 * body:         отправляемые данные.
 * 
 * Возвращает значение функции send().
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 262144;
    char response[max_response_size];
    time_t rawtime;
    struct tm *info;
    rawtime = time(NULL);
    info = localtime(&rawtime);
    // Формирование ответа
    int response_length =
    sprintf(response,
            "%s\n"
            "Date: %s"
            "Connection: close\n"
            "Content-Length: %d\n"
            "Content-Type: %s\n"
            "\n",
              header, asctime(info), content_length, content_type);
    memcpy(response + response_length, body, content_length);
    // Отправка
    response_length += content_length;
    int rv = send(fd, response, response_length, 0);

    if (rv < 0) {
        perror("send");
    }

    return rv;
}

/**
 * Отправить ответ вида text/plain
 */
void get_d20(int fd)
{
    // Сгенерировать рандомное число от 1 до 20
    srand(time(NULL) + getpid());

    char str[8];

    int random = rand() % 20 + 1;
    int length = sprintf(str, "%d\n", random);
    // Отправить ответ
    send_response(fd, "HTTP/1.1 200 OK", "text/plain", str, length);
}

/**
 * Отправить ответ 404
 */
void resp_404(int fd)
{
    char filepath[4096];
    struct file_data *filedata; 
    char *mime_type;

    // Сформировать путь к файлу 404.html
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_FILES);
    // Загрузить файл в память
    filedata = file_load(filepath);

    if (filedata == NULL) {
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }
    // Определить его MIME тип
    mime_type = mime_type_get(filepath);
    // Отправить ответ
    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

/**
 * Прочитать файл с диска (При GET запросе)
 */
void get_file(int fd, char *request_path) {
    char filepath[65536];
    struct file_data *filedata;
    char *mime_type;
    // Сформировать путь к файлу
    snprintf(filepath, sizeof(filepath), "%s%s", SERVER_DATA, request_path);
    // Загрузить файл в память
    filedata = file_load(filepath);

    if (filedata == NULL) { // Если файл не найден, то отправить index.html
        snprintf(filepath, sizeof filepath, "%s%s/index.html", SERVER_DATA,
                request_path);
    filedata = file_load(filepath);

    if (filedata == NULL) {
        /*
            Если index.html не найдет,
            то отправить 404.html
        */
        resp_404(fd);
        return;
    }
}
    // Определить MIME тип файла
    mime_type = mime_type_get(filepath);
    // Отправить файл
    send_response(fd, "HTTP/1.1 200 OK", mime_type, filedata->data,
                filedata->size);

    file_free(filedata);
}

/**
 * Поиск окончания заголовка HTTP
 * 
 * Символами Перевода строк в HTTP могут быть: \r\n , \n или \r 
 */
char *find_start_of_body(char *header)
{
    char *start;

    if ((start = strstr(header, "\r\n\r\n")) != NULL) {
        return start + 3;
    } else if ((start = strstr(header, "\n\n")) != NULL) {
    return start + 2;
    } else if ((start = strstr(header, "\r\r")) != NULL) {
    return start + 2;
    } else {
    return start;
  }
}

/**
 * Обработка HTTP запроса и отправка ответа
 */
void handle_http_request(int fd)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];
    char request_type[8];       // GET или POST
    char request_path[1024];    // /index.html и тд.
    char request_protocol[128]; // HTTP/1.1
    char* ptr;
    char *mime_type;
    // Получение сообщения от пользователя
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0) {
        perror("recv");
        return;
    }
    
    request[bytes_recvd] = '\0';
    // Ищем конец заголовка
    ptr = find_start_of_body(request);
    char* body = ptr + 1;

    sscanf(request, "%s %s %s", request_type, request_path, request_protocol);
    printf(" Request: %s %s %s", request_type, request_path, request_protocol);

    if (strcmp(request_type, "GET") == 0) // GET запрос
    {
        if (strcmp(request_path, "/d20") == 0)
            get_d20(fd);
        else 
            get_file(fd, request_path);
    }
    else if (strcmp(request_type, "POST") == 0) // POST запрос
    {
        struct file_data* filedata;
        mime_type = mime_type_get(request_path);
        /* Определяем конец файла и записываем
            в структуру его размер и данные 
        */ 
        char* pos = strchr(body, '\0');
        int num = pos - body;
        filedata->size = num;
        printf("\n %d \n", filedata->size);
        filedata->data = body;
        // Сохраняем файл на диск
        save_file(request_path, filedata);
        char response_body[128];
        int length = sprintf(response_body, "{\"status\": \"%s\"}\n", "ok");
        // Отправляем серверу ответ
        send_response(fd, "HTTP/1.1 200 OK", "application/json", response_body,
                length);
    }
    else
    {
        fprintf(stderr, "Unknown request type \"%s\"\n", request_type);
        return;
    }
}

/**
 * Main
 */
int main(void)
{
    int newfd; 
    struct sockaddr_storage their_addr; // Информация о подключенном адресе
    char s[INET6_ADDRSTRLEN];


    // Получаем прослушиваемый сокет
    int listenfd = get_listener_socket(PORT);

    if (listenfd < 0) {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);

    // Главный цикл, который принимает входящие подключения и
    //  отвечает на запросы. После этого ждет новых подключений.
    
    while(1) {
        socklen_t sin_size = sizeof their_addr;

        // Ожидание нового подключения
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1) {
            perror("accept");
            continue;
        }

        // Вывод сообщения о том, что состоялось подключение
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        // newfd - новый сокет для нового поключения 
        // Обрабатывыем HTTP запрос

        handle_http_request(newfd);

        close(newfd);
    }

    return 0;
}