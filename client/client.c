#include "client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>

#include "api/fileserver_api.h"
#include "utils/linked_list.h"
#include "utils/utilities.h"

bool VERBOSE = false;
char *socket_path = NULL;

int 
main(int argc, char * const argv[])
{
    if ( argc < 2 ) {
        fprintf(stderr, "client program requires at least one argument, rerun with -h for usage\n");
        exit(EXIT_FAILURE);
    }

    // Initialization of list containing client requests
    list_t *action_list;
    action_list = list_create(NULL, free_action, NULL);
    if ( action_list == NULL ) {
        fprintf(stderr, "request list could not be initialized, exiting\n");
        exit(EXIT_FAILURE);
    }

    // Parsing of arguments from command line
    int res;
    if ( (res = parse_options_in_list(argc, argv, action_list)) != 0 ) {
        
        if ( res == -1 ) { // fatal error occurred
            fprintf(stderr, "could not parse client options");
            list_destroy(action_list);
            exit(EXIT_FAILURE);
        }

        if ( res == 1 ) { // option -h found
            print_help_msg();
            list_destroy(action_list);
            exit(EXIT_SUCCESS);
        }
    }

    // If socket path is not yet specified it gets set to default
    if ( socket_path == NULL ) {
        socket_path = malloc(strlen(DEFAULT_SOCKET_PATH) + 1);
        strcpy(socket_path, DEFAULT_SOCKET_PATH);
    }

    // Attempts to open a connection
    if ( openConnection(socket_path, 0, (struct timespec){0,0}) != 0 ) {
        perror("openConnection()");
        list_destroy(action_list);
        exit(EXIT_FAILURE);
    }

    // Starts executing requests
    while (!list_is_empty(action_list)) {

        action_t *current_action = (action_t*) list_remove_head(action_list);

        if ( execute_action(current_action) != 0 ) {
            fprintf(stderr, "Could not execute request\n");
        }

        free_action(current_action);
    }

    // Closing connection
    if ( closeConnection(socket_path) != 0 ) {
        fprintf(stderr, "Connection could not be closed\n");
    }

    // Clean up
    free(socket_path);
    list_destroy(action_list);
    return 0;
}

int 
get_files_from_directory(char *source_dir, int *how_many, list_t *files_list)
{
	
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

int
parse_options_in_list(int n_options, char * const option_vector[], list_t *action_list)
{
    int opt, socket_set = 0;

    while ((opt = getopt(n_options, option_vector, CLIENT_OPTIONS)) != -1) {

        switch (opt) {

            case 'h': return 1;

            case 'f': {
                if ( socket_set == 0 ) { // socket path is not yet defined
                    
                    if ( strlen(optarg) + 1 > MAX_PATH ) {
                        fprintf(stderr, "socket path too long");
                        break;
                    }

                    socket_path = malloc(strlen(optarg) + 1);
                    if ( socket_path == NULL ) return -1;

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
                // Modifies last action with specified wait time
                action_t *prev_action = (action_t*)list_remove_tail(action_list);
                if ( prev_action == NULL ) {
                    fprintf(stderr, "Could not set wait time for previous request\n");
                    break;
                }
                 
                prev_action->wait_time = strtol(optarg, NULL, 10);
                if ( list_insert_tail(action_list, prev_action) != 0 ) {
                    fprintf(stderr, "Could not set wait time for previous request\n");
                    break;
                }

                break;
            }

            case 'a': {
                // Creating new append request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = APPEND;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add append request to list of actions\n");
                }
                break;
            }

            case 'W': {
                // Creating new write request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = WRITE;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add write request to list of actions\n");
                }
                break;
            }

            case 'w': {
                // Creating new write directory request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = WRITE_DIR;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add write directory to list of actions\n");
                }
                break;
            }

            case 'D': {
                // Modifies last write action with specified directory
                action_t *write_action = (action_t*)list_remove_tail(action_list);
                if ( write_action == NULL ) {
                    fprintf(stderr, "Could not set directory for previous request\n");
                    break;
                }

                // Checking whether last action was a write
                if ( (write_action->code == WRITE) || (write_action->code == WRITE_DIR) 
                    || (write_action->code == APPEND) ) {

                    if ( write_action->directory == NULL ) {
                        write_action->directory = malloc(strlen(optarg) + 1);
                        if ( write_action->directory == NULL ) {
                            errno = ENOMEM;
                            return -1;
                        } 
                        strcpy(write_action->directory, optarg);
                    } else {
                        fprintf(stderr, "Expelled directory already defined for request\n");
                    }
                } else {
                    fprintf(stderr, "option -D needs to be paired with write request\n");
                }

                if ( list_insert_tail(action_list, write_action) != 0 ) {
                    fprintf(stderr, "Could not add write request to list of actions\n");
                }
                break;
            }

            case 'r': {
                // Creating new read request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = READ;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add read request to list of actions\n");
                }
                break;
            }

            case 'R': {
                action_t    *new_action = malloc(sizeof(action_t));
                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = READ_N;

                new_action->arguments = malloc(strlen(optarg) + 1);
                strcpy(new_action->arguments, optarg);
                list_insert_tail(action_list, new_action);
                break;
            }

            case 'd': {
                // Modifies last read action with specified directory
                action_t *read_action = (action_t*)list_remove_tail(action_list);
                if ( read_action == NULL ) {
                    fprintf(stderr, "Could not set directory for previous request\n");
                    break;
                }

                // Checking whether last action was a read
                if ( (read_action->code == READ) || (read_action->code == READ_N) ) {
                    if ( read_action->directory == NULL ) {
                        read_action->directory = malloc(strlen(optarg) + 1);
                        if ( read_action->directory == NULL ) {
                            errno = ENOMEM;
                            return -1;
                        } 
                        strcpy(read_action->directory, optarg);
                    } else {
                        fprintf(stderr, "Reading directory already defined for request\n");
                    }
                } else {
                    fprintf(stderr, "option -d needs to be paired with read request\n");
                }

                if ( list_insert_tail(action_list, read_action) != 0 ) {
                    fprintf(stderr, "Could not add read request to list of actions\n");
                }
                break;
            }

            case 'l': {
                // Creating new lock request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = LOCK;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add lock request to list of actions\n");
                }
                break;
            }

            case 'u': {
                // Creating new unlock request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = UNLOCK;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add unlock request to list of actions\n");
                }
                break;
            }

            case 'c': {
                // Creating new remove request action
                action_t    *new_action = malloc(sizeof(action_t));
                if ( new_action == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                new_action->directory = NULL;
                new_action->wait_time = 0;
                new_action->code = REMOVE;

                new_action->arguments = malloc(strlen(optarg) + 1);
                if ( new_action->arguments == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(new_action->arguments, optarg);
                if ( list_insert_tail(action_list, new_action) != 0 ) {
                    fprintf(stderr, "Could not add remove request to list of actions\n");
                }
                break;
            }
        }
    }
    return 0;
}

int
execute_action(action_t *action)
{
    if (action == NULL) return -1;

    switch (action->code) {
        
        case WRITE: {
            
            // Getting directory name from action
            char *dirname = NULL;
            if ( action->directory != NULL ) {
                dirname = malloc(strlen(action->directory) + 1);
                if ( dirname == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(dirname, action->directory);
            }

            // Starts parsing arguments and executing requests
            const char *file_path = strtok(action->arguments, ",");
            while (file_path != NULL) {

                writeFile(file_path, action->directory);
                if (VERBOSE) display_request_result();

                file_path = strtok(NULL, ",");
    
                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case APPEND: {
            
            // Getting directory name from action
            char *dirname = NULL;
            if ( action->directory != NULL ) {
                dirname = malloc(strlen(action->directory) + 1);
                if ( dirname == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }
                
                strcpy(dirname, action->directory);
            }
            
            // Starts parsing arguments and executing requests
            const char *file_path = strtok(action->arguments, ","); // File path in server
            const char *source_path = strtok(NULL, ","); // File containing contents to append
        
            char *absolute_path = realpath(source_path, NULL);

            // Opening source path from disk
            FILE *file_ptr = fopen(absolute_path, "rb");
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


            appendToFile(file_path, file_data, file_size, dirname);
            if (VERBOSE) display_request_result();
            
            free(absolute_path);
            fclose(file_ptr);

            if (action->wait_time != 0) msleep(action->wait_time);

            break;
        }

        case WRITE_DIR: {

            // Getting directory name from action
            char *dirname = NULL;
            if ( action->directory != NULL ) {
                dirname = malloc(strlen(action->directory) + 1);
                if ( dirname == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(dirname, action->directory);
            }
            
            // Parsing of directory path
            const char *source_dirname = strtok(action->arguments, ","); 
            char *absolute_dirname = realpath(source_dirname, NULL);

            // Parsing of number of files
            const char *tmp = strtok(NULL, " "); 
            long n;
            if ( is_number(tmp, &n) != 0) {
                fprintf(stderr, "Write directory expects a number\n");
                free(absolute_dirname);
                return -1;
            }

            int how_many = n;

            // Reading directory contents
            list_t *files_list = list_create(string_compare, free_string, NULL);
            if ( get_files_from_directory(absolute_dirname, &how_many, files_list) != 0 ) {
                fprintf(stderr, "Could not read files from directory\n");
                list_destroy(files_list);
                free(absolute_dirname);
                return -1;
            }

            // Executing write requests
            while (!list_is_empty(files_list)) {
                
                char *path = (char*)list_remove_head(files_list);

                writeFile(path, action->directory);
                if (VERBOSE) display_request_result();

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            list_destroy(files_list);

            break;
        }

        case READ: {

            // Getting directory name from action
            char *dirname = NULL;
            if ( action->directory != NULL ) {
                dirname = malloc(strlen(action->directory) + 1);
                if ( dirname == NULL ) {
                    errno = ENOMEM;
                    return -1;
                }

                strcpy(dirname, action->directory);
            }
            
            // Starts parsing arguments and executing requests
            const char *file_path = strtok(action->arguments, ",");
            
            while (file_path != NULL) {

                void *read_buffer = NULL;
                size_t buffer_size;

                readFile(file_path, &read_buffer, &buffer_size);
                if (VERBOSE) display_request_result();

                if (dirname != NULL) {
                    write_file_in_directory(dirname, (char*)file_path, buffer_size, read_buffer);
                }

                if (read_buffer != NULL) free(read_buffer);

                file_path = strtok(NULL, ",");

                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case READ_N: {

            int N = atoi(action->arguments);

            readNFiles(N, action->directory);
            if (VERBOSE) display_request_result();

            /* if (readNFiles(N, action->directory) != 0) {
                if (VERBOSE) api_perror("readNFiles(%d, %s) failed", N, action->directory);
            } else {
                if (VERBOSE) api_perror("readNFiles(%d, %s)", N, action->directory);
            } */
            break;
        }

        case LOCK: {

            // Starts parsing arguments and executing requests
            const char *file_path = strtok(action->arguments, ",");
            while (file_path != NULL) {

                lockFile(file_path);
                if (VERBOSE) display_request_result();

                file_path = strtok(NULL, ",");
    
                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case UNLOCK: {

            // Starts parsing arguments and executing requests
            const char *file_path = strtok(action->arguments, ",");
            while (file_path != NULL) {

                unlockFile(file_path);
                if (VERBOSE) display_request_result();

                file_path = strtok(NULL, ",");
    
                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }

        case REMOVE: {
            
            // Starts parsing arguments and executing requests
            const char *file_path = strtok(action->arguments, ",");
            while (file_path != NULL) {

                removeFile(file_path);
                if (VERBOSE) display_request_result();

                file_path = strtok(NULL, ",");
    
                if (action->wait_time != 0) msleep(action->wait_time);
            }

            break;
        }
    }

    return 0;
}

void
free_action(void *e)
{
    action_t *a = (action_t*)e;
    if (a->directory) free(a->directory);
    if (a->arguments) free(a->arguments);
    free(a);
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


