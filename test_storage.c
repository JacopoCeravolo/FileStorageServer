#include <stdio.h>
#include <errno.h>

#include "storage.h"

int main(int argc, char const *argv[])
{
    storage_t *storage = storage_create(1024, 3);
    if (storage == NULL) {
        printf("storage_create failed: %s\n", strerror(errno));
    }

    FILE *f;
    file_t *f1, *f2, *f3;
    char buffer[MAX_BUFFER];

    f = fopen("storage_log.txt", "w+");
    f1 = malloc(sizeof(file_t));
    f2 = malloc(sizeof(file_t));
    f3 = malloc(sizeof(file_t));

    f1->contents = malloc(256);
    f2->contents = malloc(256);
    f3->contents = malloc(256);

    /* File 1 */
    strcpy(f1->path, "file1");
    strcpy(buffer, "Contents of file 1");
    f1->size = strlen(buffer) + 1;
    memcpy(f1->contents, buffer, strlen(buffer) + 1);
    if (storage_add_file(storage, f1) != 0) {
        printf("storage_add_file failed: %s\n", strerror(errno));
    }
    printf("file1 added\n");

    /* File 2 */
    strcpy(f2->path, "file2");
    strcpy(buffer, "Contents of file 2");
    f2->size = strlen(buffer) + 1;
    memcpy(f2->contents, buffer, strlen(buffer) + 1);
    if (storage_add_file(storage, f2) != 0) {
        printf("storage_add_file failed: %s\n", strerror(errno));
    }
    printf("file2 added\n");

    /* File 3 */
    strcpy(f3->path, "file3");
    strcpy(buffer, "Contents of file 3");
    f3->size = strlen(buffer) + 1;
    memcpy(f3->contents, buffer, strlen(buffer) + 1);
    if (storage_add_file(storage, f3) != 0) {
        printf("storage_add_file failed: %s\n", strerror(errno));
    }
    printf("file3 added\n");

    fprintf(f, "\nBEFORE\n");
    storage_dump(storage, f);

    printf("attempting to read file1\n");
    file_t *tmp = storage_get_file(storage, "file1");
    if (tmp == NULL) {
        printf("storage_get_file failed: %s\n", strerror(errno));
    } else {
        print_file(NULL, (void*)tmp, stdout);
    }

    printf("attempting to read file5, expecting error\n");
    tmp = storage_get_file(storage, "file5");
    if (tmp == NULL) {
        printf("storage_get_file failed: %s\n", strerror(errno));
    } else {
        print_file(NULL, (void*)tmp, stdout);
    }

    printf("modifying file1\n");
    memcpy(f1->contents, buffer, strlen(buffer) + 1);
    if (storage_add_file(storage, f1) != 0) {
        printf("storage_add_file failed: %s\n", strerror(errno));
    }
    printf("modified file1\n");

    fprintf(f, "\nAFTER\n");

    storage_dump(storage, f);
    storage_destroy(storage);
    fclose(f);
    return 0;
}
