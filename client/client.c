#include "client_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api/fileserver_api.h"
#include "utils/linked_list.h"
#include "utils/utilities.h"

bool VERBOSE;
char *socket_path = NULL;

int main(int argc, char * const argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Client program requires at least one argument, rerun with -h for usage\n");
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
        socket_path = malloc(strlen(DEFAULT_SOCKET_PATH) + 1);
        strcpy(socket_path, DEFAULT_SOCKET_PATH);
    }

    if (openConnection(socket_path, 0, (struct timespec){0,0}) != 0) {
        fprintf(stderr, "openConnection() failed\n");
        list_destroy(action_list);
        exit(EXIT_FAILURE);
    }

    while (!list_is_empty(action_list)) {

        action_t *current_action = (action_t*) list_remove_head(action_list);
    
        print_action(current_action, stdout);

        execute_action(current_action);

        free_action(current_action);
    }

    closeConnection(DEFAULT_SOCKET_PATH);
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
                else { printf("printing enabled\n"); VERBOSE = true; }
                break;
            }

            case 't': {
                action_t *prev_action = (action_t*)list_remove_tail(action_list);
                prev_action->wait_time = strtol(optarg, NULL, 10);
                list_insert_tail(action_list, prev_action);
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
                if ((write_action->code == WRITE) || (write_action->code == WRITE_DIR)) {
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
            
            while (file_path != NULL) {
                
                if (openFile(file_path, O_CREATE|O_LOCK) != 0) {
                    fprintf(stderr, "openFile(%s) failed\n", file_path);
                }

                if (writeFile(file_path, dirname) != 0) {
                    fprintf(stderr, "writeFile(%s, %s) failed\n", file_path, dirname);
                }

                file_path = strtok(NULL, ",");
                // get absolute path and verify
            }

            break;
        }

        case WRITE_DIR: {
            break;
        }

        case READ: {

            char *dirname = NULL;
            if (action->directory != NULL) { // must be change to full path
                dirname = malloc(strlen(action->directory) + 1);
                strcpy(dirname, action->directory);
            }
            
            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            
            while (file_path != NULL) {

                void *read_buffer;
                size_t buffer_size;
                
                if (readFile(file_path, &read_buffer, &buffer_size) != 0) {
                    fprintf(stderr, "readFile(%s) failed\n", file_path);
                }

                if (dirname != NULL) {
                    read_file_in_directory(file_path, dirname, read_buffer, buffer_size);
                }

                free(read_buffer);

                file_path = strtok(NULL, ",");
                // get absolute path and verify
            }

            break;
        }

        case READ_N: {
            break;
        }

        case LOCK: {

            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            
            while (file_path != NULL) {
                
                if (lockFile(file_path) != 0) {
                    fprintf(stderr, "lockFile(%s) failed\n", file_path);
                }

                file_path = strtok(NULL, ",");
                // get absolute path and verify
            }

            break;
        }

        case UNLOCK: {

            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            
            while (file_path != NULL) {
                
                if (unlockFile(file_path) != 0) {
                    fprintf(stderr, "unlockFile(%s) failed\n", file_path);
                }

                file_path = strtok(NULL, ",");
                // get absolute path and verify
            }

            break;
        }

        case REMOVE: {
            
            char *file_path = strtok(action->arguments, ",");
            // get absolute path and verify
            
            while (file_path != NULL) {
                
                if (removeFile(file_path) != 0) {
                    fprintf(stderr, "removeFile(%s) failed\n", file_path);
                }

                file_path = strtok(NULL, ",");
                // get absolute path and verify
            }

            break;
        }
    }

    if (action->wait_time != 0) msleep(action->wait_time);
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
                                        "    -w dirname[,n=n_files]    Sends the contents of directory dirname to File Storage Server.\n" \
                                        "                              All subdirectories are visited recursively sending up to n_files.\n" \
                                        "                              If n=0 or n is unspecifed, all contents are sent to File Storage Server\n" \
                                        "    -W file1[,file2...]       Sends files specifed as arguments separated by commas to File Storage Server\n" \
                                        "    -D dirname                Specifies the directory dirname where to write files expelled by File Storage Server\n" \
                                        "                              in case of capacity misses. Option -D should be coupled with -w or -W, otherwise an error\n" \
                                        "                              message is print and all expelled files are trashed. If not specified all expelled files are trashed\n" \
                                        "    -r file1[,file2...]       Reads files specifed as arguments separated by commas from File Storage Server\n" \
                                        "    -R [n=n_files]            Reads n_files files from File Storage Server. If n=0 or unspecified reads all files in File Storage Server\n" \
                                        "    -d dirname                Specifies the directory dirname where to write files read from File Strage Server.\n" \
                                        "                              Option -d should be coupled with -r or -R, otherwise an error message is print and read files are not saved\n" \
                                        "    -t time                   Times in milleseconds to wait in between requests to File Storage Server\n" \
                                        "    -l file1[,file2...]       List of files to acquire mutual exclusion on\n" \
                                        "    -u file1[,file2...]       List of files to release mutual exclusion on\n" \
                                        "    -c file1[,file2...]       List of files to delete from File Storage Server\n");
}

