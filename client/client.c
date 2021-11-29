#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "api/fileserver_api.h"

#include "utils/linked_list.h"
#include "utils/logger.h"

#include "client/option_parser.h"

#define CLIENT_OPTIONS  "w:W:r:R:l:u:c:f:dDhtp"

list_t *request_list;
bool VERBOSE;
char socket_name[MAX_PATH];

void
print_help_msg();

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Client program requires at least one argument, rerun with -h for usage\n");
        exit(EXIT_FAILURE);
    }

#ifdef DEBUG
   log_init(argv[1]);
   set_log_level(LOG_DEBUG);
#else
   log_init(NULL);
#endif

    request_list = list_create(NULL, free_option, print_option);
    if (request_list == NULL) {
        log_fatal("request list could not be initialized, exiting\n");
        exit(EXIT_FAILURE);
    }

    int res;
    if ((res = parse_options(argc, argv, CLIENT_OPTIONS, request_list)) != 0) {
        if (res == 1) {
            print_help_msg();
            goto _client_exit;
        }
        log_warning("some options have not been parsed correctly\n");
    }

    if (socket_name != NULL) {
        log_info("socket name: %s\n", socket_name);
    }

    

    if (openConnection(DEFAULT_SOCKET_PATH, 0, (struct timespec){0,0}) != 0) {
        perror("openConnection() failed");
    }

    while (!list_is_empty(request_list)) {

        option_t *request = (option_t*) list_remove_head(request_list);

        switch (request->type) {
            
            case WRITE: {
                char *token = strtok(request->arguments, ",");
                while (token) {

                    int flags;
                    SET_FLAG(flags, O_CREATE|O_LOCK);
                    if (openFile(token, O_CREATE|O_LOCK) != 0) {
                        perror("openFile() failed");
                    }
                    if (writeFile(token, NULL) != 0) {
                        perror("writeFile() failed");
                    } 
                    /* if (closeFile(token) != 0) {
                        perror("closeFile() failed");
                    } */

                    token = strtok(NULL, ",");
                    sleep(1);
                }
                fprintf(stdout, "\n");
                break;
            }

            case READ: {
            
                char *token = strtok(request->arguments, ",");
                while (token) {
                   
                    int flags;
                    SET_FLAG(flags, O_NOFLAG);
                    if (openFile(token, O_NOFLAG) != 0) {
                        perror("openFile() failed");
                    }
                    void* buf;
                    size_t size;
                    if (readFile(token, &buf, &size) != 0) {
                        perror("readFile() failed");
                    } else {
                        printf("*** %s ***\n", token);
                        printf("\n%s\n\n\n", (char*)buf);
                    }
                    token = strtok(NULL, ",");
                }
                fprintf(stdout, "\n");
                break;
            }

            case WRITE_DIR: {
                break;
            }

            case READ_N: {
                break;
            }

            case LOCK: {

                char *token = strtok(request->arguments, ",");
                while (token) {
                    
                    openFile(token, O_NOFLAG);
                    lockFile(token);
                    // closeFile(token);

                    token = strtok(NULL, ",");
                }
                fprintf(stdout, "\n");
                break;
            }
            
            case UNLOCK: {
              
                char *token = strtok(request->arguments, ",");
                while (token) {
                    
                    openFile(token, O_NOFLAG);
                    unlockFile(token);
                    closeFile(token);

                    token = strtok(NULL, ",");
                }
                fprintf(stdout, "\n");
                break;
            }

            case REMOVE: {
               
                char *token = strtok(request->arguments, ",");
                while (token) {
                    
                    openFile(token, O_NOFLAG);
                    removeFile(token);
                    // closeFile(token);

                    token = strtok(NULL, ",");
                }
                fprintf(stdout, "\n");
                break;
            }
        }

        free(request);
    }


_client_exit:
    closeConnection(DEFAULT_SOCKET_PATH);
    list_destroy(request_list);
    close_log();
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

