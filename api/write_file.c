#include "fileserver_api.h"

int
writeFile(const char* pathname, const char* dirname){
    
    printf("> [CLIENT] writing [%s]\n", pathname);
    FILE *file_ptr;
    file_ptr = fopen(pathname, "rb");
    if (file_ptr == NULL) {
        errno = EIO;
        return -1;
    }

       
    fseek(file_ptr, 0, SEEK_END);
    size_t file_size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);
    if (file_size == -1) {
        fclose(file_ptr);
        return -1;
    }
        
    void* file_data;
    file_data = malloc(file_size);
    if (file_data == NULL) {
        errno = ENOMEM;
        fclose(file_ptr);
    }

    if (fread(file_data, 1, file_size, file_ptr) < file_size) {
        if (ferror(file_ptr)) {
            fclose(file_ptr);
            if (file_data != NULL) free(file_data);
        }
    }

    fclose(file_ptr);
    
    send_request(socket_fd, WRITE_FILE, pathname, file_size, file_data);
    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);

    return 0;
}