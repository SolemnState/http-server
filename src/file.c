#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "file.h"

/**
 * Загружает файл в память и возвращает указатель на данные.
 */
struct file_data *file_load(char *filename)
{
    char *buffer, *p;
    struct stat buf;
    int bytes_read, bytes_remaining, total_bytes = 0;

    // Определение размера файла
    if (stat(filename, &buf) == -1) {
        return NULL;
    }

    // Убедиться, что это файл
    if (!(buf.st_mode & S_IFREG)) {
        return NULL;
    }

    // Открыть файл для чтения
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        return NULL;
    }

    // Выделить нужное количество памяти 
    bytes_remaining = buf.st_size;
    p = buffer = malloc(bytes_remaining);

    if (buffer == NULL) {
        return NULL;
    }

    // Чтение всего файла
    while (bytes_read = fread(p, 1, bytes_remaining, fp), \
            bytes_read != 0 && bytes_remaining > 0) {
        if (bytes_read == -1) {
            free(buffer);
            return NULL;
        }

        bytes_remaining -= bytes_read;
        p += bytes_read;
        total_bytes += bytes_read;
    }

    struct file_data *filedata = malloc(sizeof *filedata);

    if (filedata == NULL) {
        free(buffer);
        return NULL;
    }

    filedata->data = buffer;
    filedata->size = total_bytes;

    return filedata;
}
/**
 * Сохрание файла на диск
 */
void save_file(char* path, struct file_data* filedata)
{
    char new_path[100]= {'\0'};
    int out_fd;
    strcat(new_path,PATH);
    strcat(new_path,path);
    out_fd =open(new_path, O_CREAT|O_RDWR, 0777);
    int nwritten = write(out_fd, filedata->data, filedata->size);
    close(out_fd);
}


void file_free(struct file_data *filedata)
{
    free(filedata->data);
    free(filedata);
}
