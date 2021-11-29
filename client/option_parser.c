#include "client/option_parser.h"

#include <getopt.h>
#include <errno.h>

#include "utils/utilities.h"
#include "utils/logger.h"

int
parse_options(int argc, char const *argv[], const char *options_string, list_t *parsed_options)
{
    int opt, socket_set = 0;

    while ((opt = getopt(argc, argv, options_string)) != -1) {

        option_t    *new_option = malloc(sizeof(option_t));

        switch (opt) {

            case 'h': free(new_option); return 1;

            case 'f': {
                if (socket_set == 0) {
                  if (strlen(optarg) + 1 > MAX_PATH) {
                        log_error("socket path too long");
                        // set err
                        break;
                    }
                    strcpy(socket_name, optarg);
                    socket_set = 1;
                } else {
                    log_info("socket path already set, it won't be overwritten\n");
                }
                break;
            }

            case 'p': {
                if (VERBOSE) { log_info("printing already enabled\n"); }
                else { log_info("printing enabled\n"); VERBOSE = true; }
                break;
            }

            // TODO
            case 't': {
                break;
            }

            case 'W': {
                new_option->type = WRITE;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-w arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }

            case 'w': {
                new_option->type = WRITE_DIR;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-W arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }

            case 'r': {
                new_option->type = READ;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-r arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }

            case 'R': {
                new_option->type = READ_N;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-w arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }

            case 'l': {
                new_option->type = LOCK;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-l arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }

            case 'u': {
                new_option->type = UNLOCK;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-u arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }

            case 'c': {
                new_option->type = REMOVE;
                if (strlen(optarg) + 1 > MAX_ARG_LENGTH) {
                    log_error("-c arguments too long\n");
                    break;
                }

                strcpy(new_option->arguments, optarg);
                list_insert_tail(parsed_options, new_option);
                break;
            }
        }
    
        // free(new_option);
    }

    return 0;
}