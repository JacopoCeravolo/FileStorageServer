#include "client_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>

#include "api/fileserver_api.h"
#include "utils/linked_list.h"
#include "utils/utilities.h"

#define SOCKET_PATH "/tmp/LSO_socket.sk"

bool VERBOSE;
char *socket_path = NULL;

int 
get_files_from_directory(char *source_dir, int *how_many, list_t *files_list){
	
    DIR *dir;
	struct dirent *file;
	char *relative_pathname;
	char *absolute_pathname;
	
	dir = opendir(source_dir);
	if( dir == NULL ) return -1;
	
	errno = 0;

	while( (file = readdir(dir)) != NULL && *how_many != 0){
		
		if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0){
			continue;
        }
       
        relative_pathname = calloc(strlen(source_dir) + strlen(file->d_name) + 2, sizeof(char));
        if( relative_pathname == NULL ) return -1;
        strcat(relative_pathname,source_dir);
        strcat(relative_pathname,"/");
        strcat(relative_pathname,file->d_name);
        
        if( file->d_type != DT_DIR ){ 
        
        	absolute_pathname = realpath(relative_pathname, NULL);
        	if( absolute_pathname == NULL ) return -1;
        	
        	if( list_insert_tail(files_list, absolute_pathname) != 0 ){
        		return -1;
        	}
        	if( *how_many != -1 ) *how_many -= 1;
        }
        else{ 
        	if( get_files_from_directory(relative_pathname, how_many, files_list) == -1 ) return -1;
        }
        free(relative_pathname);
	}
	if( errno != 0 ) return -1;
	
	free(dir);
	return 0;
}


int main(int argc, char * const argv[])
{
    if (argc < 2) {
        fprintf(stderr, "client program requires at least one argument, rerun with -h for usage\n");
        exit(EXIT_FAILURE);
    }

    list_t *action_list;
    action_list = list_create(NULL, free_action, print_action);
    if (action_list == NULL) {
        fprintf(stderr, "request list could not be initialized, exiting\n");
        exit(EXIT_FAILURE);
    }

    int res;
    if ((res = parse_options_in_list(argc, argv, action_list)) != 0) {
        if (res == 1) { // option -h found
            print_help_msg();
            list_destroy(action_list);
            exit(EXIT_SUCCESS);
        }
        fprintf(stderr, "some options have not been parsed correctly\n");
    }

    if (socket_path == NULL) {
        socket_path = malloc(strlen(SOCKET_PATH) + 1);
        strcpy(socket_path, SOCKET_PATH);
    }

    if (openConnection(socket_path, 0, (struct timespec){0,0}) != 0) {
        api_perror("openConnection(%s) failed", socket_path);
        list_destroy(action_list);
        exit(EXIT_FAILURE);
    }

    // if (VERBOSE) {
    //     printf("%-10s %-65s %-10s %-20s\n", "OPT_TYPE", "FILE", "N_BYTES", "RESULT");
    // }

    while (!list_is_empty(action_list)) {

        action_t *current_action = (action_t*) list_remove_head(action_list);
    
        // print_action(current_action, stdout);

        execute_action(current_action);

        free_action(current_action);
    }

    closeConnection(socket_path);
    list_destroy(action_list);
    return 0;
}

int
parse_options_in_list(int n_options, char * const option_vector[], list_t *action_list)
{
    int opt, socket_set = 0;

    while ((opt = getopt(n_options, option_vector, CLIENT_OPTIONS)) != -1) {

        action_t    *new_action = malloc(sizeof(action_t));

        switch (opt) {

            case 'h': free(new_action); return 1;

            case 'f': {
                if (socket_set == 0) {
                  if (strlen(optarg) + 1 > MAX_PATH) {
                        fprintf(stderr, "socket path too long");
                        // set err
                        break;
                    }
                    socket_path = malloc(strlen(optarg) + 1);
                    strcpy(socket_path, optarg);
                    socket_set = 1;
                } else {
                    fprintf(stderr, "socket path already set, it won't be overwritten\n");
                }
                break;
            }

            case 'p': {
                if (VERBOSE) { fprintf(stderr, "printing already enabled\n"); }
                else { VERBOSE = true; }
                break;
            }

            case 't': {
                action_t *prev_action = (action_t*)list_remove_tail(action_list);
                prev_action->wait_time = strtol(optarg, NULL, 10);
                list_insert_tail(action_list, prev_action);
                break;
            }

            case 'a': {
                new_action->code = APPEND;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'W': {
                new_action->code = WRITE;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'w': {
                new_action->code = WRITE_DIR;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'D': {
                action_t *write_action = (action_t*)list_remove_tail(action_list);
                if ((write_action->code == WRITE) || (write_action->code == WRITE_DIR)
                    || (write_action->code == APPEND)) {
                    if (write_action->directory == NULL) {
                        write_action->directory = malloc(strlen(optarg) + 1);
                        strcpy(write_action->directory, optarg);
                    }
                } else {
                    fprintf(stderr, "option -D needs to be paired with write operation\n");
                }

                list_insert_tail(action_list, write_action);
                break;
            }

            case 'r': {
                new_action->code = READ;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'R': {
                new_action->code = READ_N;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'd': {
                action_t *read_action = (action_t*)list_remove_tail(action_list);
                if ((read_action->code == READ) || (read_action->code == READ_N)) {
                    if (read_action->directory == NULL) {
                        read_action->directory = malloc(strlen(optarg) + 1);
                        strcpy(read_action->directory, optarg);
                    }
                } else {
                    fprintf(stderr, "option -d needs to be paired with read operation\n");
                }

                list_insert_tail(action_list, read_action);
                break;
            }

            case 'l': {
                new_action->code = LOCK;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'u': {
                new_action->code = UNLOCK;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'c': {
                new_action->code = REMOVE;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    fprintf(stderr, "-w arguments too long\n");
                    break;
                }

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }
        }
    }
    return 0;
}

void
execute_action(action_t *action)
{
    if (action == NULL) return;

    switch (action->code) {
        
        case WRITE: {

            char *dirname = NULL;
            if (action->directory != NULL) { // must be change to full path
                dirname = malloc(strlen(action->directory) + 1);
                strcpy(dirname, action->directory);
            }
            
            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            char *absolute_path = realpath(file_path, NULL);

            
            while (file_path != NULL) {

                writeFile(absolute_path, action->directory);
                if (VERBOSE) print_result();

                
                /* if (writeFile(absolute_path, action->directory) != 0) {
                    if (VERBOSE) api_perror("writeFile(%s, %s) failed", absolute_path, dirname);
                } else {
                    if (VERBOSE) {
                        if (VERBOSE) print_result();
                    }
                } */

                free(absolute_path);
                absolute_path = NULL;

                file_path = strtok(NULL, ",");
                if (file_path != NULL) absolute_path = realpath(file_path, NULL);
                // get absolute path and verify

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case APPEND: {
            
            char *dirname = NULL;
            if (action->directory != NULL) { // must be change to full path
                dirname = malloc(strlen(action->directory) + 1);
                strcpy(dirname, action->directory);
            }
            
            char *file_path = strtok(action->arguments, ",");
            char *source_path = strtok(NULL, ",");
            // get absolute path and verify
            char *absolute_path = realpath(source_path, NULL);
            char *path_in_server = realpath(file_path, NULL);

            FILE *file_ptr;

            file_ptr = fopen(absolute_path, "rb");
            if ( file_ptr == NULL ) {
                errno = EIO;
                perror("fopen()");
                break;
            }

            struct stat st;
            size_t file_size;
            if(stat(absolute_path, &st) == 0){
                file_size = st.st_size;
            } else {
                fclose(file_ptr);
                errno = EIO;
                perror("stat()");
                return -1;
            }

        
            void* file_data;
            file_data = malloc(file_size);
            if ( file_data == NULL ) {
                fclose(file_ptr);
                errno = ENOMEM;
                return -1;
            }

            if ( fread(file_data, 1, file_size, file_ptr) < file_size ) {
                if ( ferror(file_ptr) ) {
                    fclose(file_ptr);
                    if ( file_data != NULL ) free(file_data);
                    errno = EIO;
                    perror("fread()");
                    return -1;
                }
            }

            fclose(file_ptr);

            appendToFile(path_in_server, file_data, file_size, dirname);
            print_result();
            
            free(absolute_path);
            free(path_in_server);

            if (action->wait_time != 0) msleep(action->wait_time);

            break;
        }

        case WRITE_DIR: {
            
            char *dirname = strtok(action->arguments, ",");
            char *tmp = strtok(NULL, " ");
            int how_many = atoi(tmp);

            list_t *files_list = list_create(string_compare, free, NULL);

            if (get_files_from_directory(dirname, &how_many, files_list) != 0) break;

            while (!list_is_empty(files_list)) {
                char *path = (char*)list_remove_head(files_list);

                writeFile(path, action->directory);
                if (VERBOSE) print_result();

                /* if (writeFile(path, action->directory) != 0) {
                    if (VERBOSE) api_perror("writeFile(%s, %s) failed", path, action->directory);
                } else {
                    if (VERBOSE) {
                        if (VERBOSE) print_result();
                    }
                } */

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            list_destroy(files_list);
            break;
        }

        case READ: {

            char *dirname = NULL;
            if (action->directory != NULL) { // must be change to full path
                dirname = malloc(strlen(action->directory) + 1);
                strcpy(dirname, action->directory);
                if (mkdir_p(dirname) == -1) {free(dirname); dirname = NULL;}
            }
            
            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            char *absolute_path = realpath(file_path, NULL);
            
            while (file_path != NULL) {

                void *read_buffer;
                size_t buffer_size;

                readFile(absolute_path, &read_buffer, &buffer_size);
                if (VERBOSE) print_result();
                
                /* if (readFile(file_path, &read_buffer, &buffer_size) != 0) {
                    if (VERBOSE) api_perror("readFile(%s, %s) failed", file_path, dirname);
                } else {
                    if (VERBOSE) {
                        if (VERBOSE) print_result();
                    }
                } */

                if (dirname != NULL) {
                    write_file_in_directory(dirname, file_path, buffer_size, read_buffer);
                }

                free(read_buffer);

                free(absolute_path);
                absolute_path = NULL;

                file_path = strtok(NULL, ",");
                if (file_path != NULL) absolute_path = realpath(file_path, NULL);
                // get absolute path and verify

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case READ_N: {

            int N = atoi(action->arguments);

            readNFiles(N, action->directory);
            if (VERBOSE) print_result();

            /* if (readNFiles(N, action->directory) != 0) {
                if (VERBOSE) api_perror("readNFiles(%d, %s) failed", N, action->directory);
            } else {
                if (VERBOSE) api_perror("readNFiles(%d, %s)", N, action->directory);
            } */
            break;
        }

        case LOCK: {

            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            char *absolute_path = realpath(file_path, NULL);
            
            while (file_path != NULL) {
                
                lockFile(absolute_path);
                if (VERBOSE) print_result();

                /* if (lockFile(file_path) != 0) {
                    if (VERBOSE) api_perror("lockFile(%s) failed", file_path);
                } else {
                    if (VERBOSE) {
                        if (VERBOSE) print_result();
                    }
                } */

                free(absolute_path);
                absolute_path = NULL;

                file_path = strtok(NULL, ",");
                if (file_path != NULL) absolute_path = realpath(file_path, NULL);
                // get absolute path and verify

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case UNLOCK: {

            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            char *absolute_path = realpath(file_path, NULL);
            
            while (file_path != NULL) {

                unlockFile(absolute_path);
                if (VERBOSE) print_result();
                
                /* if (unlockFile(file_path) != 0) {
                    if (VERBOSE) api_perror("unlockFile(%s) failed", file_path);
                } else {
                    if (VERBOSE) {
                        if (VERBOSE) print_result();
                    }
                } */

                free(absolute_path);
                absolute_path = NULL;

                file_path = strtok(NULL, ",");
                if (file_path != NULL) absolute_path = realpath(file_path, NULL);
                // get absolute path and verify

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case REMOVE: {
            
            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            char *absolute_path = realpath(file_path, NULL);
            
            while (file_path != NULL) {

                removeFile(absolute_path);
                if (VERBOSE) print_result();
                
                /* if (removeFile(file_path) != 0) {
                    if (VERBOSE) api_perror("removeFile(%s) failed", file_path);
                } else {
                    if (VERBOSE) {
                        if (VERBOSE) print_result();
                    }
                } */

                free(absolute_path);
                absolute_path = NULL;

                file_path = strtok(NULL, ",");
                if (file_path != NULL) absolute_path = realpath(file_path, NULL);
                // get absolute path and verify

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }
    }
}

int
read_file_in_directory(const char *file_path, const char *dirname, void* read_buffer, size_t buffer_size)
{
    return 0;
}

void
print_help_msg()
{
    printf("\nThe client program allows to interact with the File Storage Server to send, receive, edit and delete files.\n" \
                                        "It behaves differently according to the options passed through the command line.\n" \
                                        "\nCurrently included options are:\n\n" \
                                        "    -h                        Prints this message\n" \
                                        "    -p                        Enables printing of infomation for each operation in the format:\n" \
                                        "                              OPT_TYPE      FILE      RESULT      N_BYTES\n" \
                                        "    -f sockname               Specifes the name of AF_UNIX socket to connect to\n" \
                                        "    -w dirname[,n_files]      Sends the contents of directory dirname to File Storage Server.\n" \
                                        "                              All subdirectories are visited recursively sending up to n_files.\n" \
                                        "                              If n_files=0 or n_files is unspecifed, all contents are sent to File Storage Server\n" \
                                        "    -W file1[,file2...]       Sends files specifed as arguments separated by commas to File Storage Server\n" \
                                        "    -D dirname                Specifies the directory dirname where to write files expelled by File Storage Server\n" \
                                        "                              in case of capacity misses. Option -D should be coupled with -w or -W, otherwise an error\n" \
                                        "                              message is print and all expelled files are trashed. If not specified all expelled files are trashed\n" \
                                        "    -r file1[,file2...]       Reads files specifed as arguments separated by commas from File Storage Server\n" \
                                        "    -R [n_files]              Reads n_files files from File Storage Server. If n=0 or unspecified reads all files in File Storage Server\n" \
                                        "    -d dirname                Specifies the directory dirname where to write files read from File Strage Server.\n" \
                                        "                              Option -d should be coupled with -r or -R, otherwise an error message is print and read files are not saved\n" \
                                        "    -t time                   Times in milleseconds to wait in between requests to File Storage Server\n" \
                                        "    -l file1[,file2...]       List of files to acquire mutual exclusion on\n" \
                                        "    -u file1[,file2...]       List of files to release mutual exclusion on\n" \
                                        "    -c file1[,file2...]       List of files to delete from File Storage Server\n");
}

