
#ifndef _FILELS_H_
#define _FILELS_H_
#define PATH "saved_files"
struct file_data {
    int size;
    void *data;
};

extern struct file_data *file_load(char *filename);
extern void file_free(struct file_data *filedata);
extern void save_file(char* path, struct file_data* filedata);

#endif