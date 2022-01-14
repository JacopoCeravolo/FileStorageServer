#include "lock_manager.h"

void*
lock_manager_thread(void* args)
{
    while (shutdown_now == 0) {

        lock_return(&(storage->access), NULL);

        node_t *curr = storage->fifo_queue->head;
       
        while (curr != NULL) {

            file_t *file = storage_get_file(storage, (char*)curr->data);
            if (file == NULL) {
                // writes to pipe
                curr = curr->next;
                unlock_return(&(storage->access), NULL);
                break;
            }

            if (!CHK_FLAG(file->flags, O_LOCK) && !list_is_empty(file->waiting_on_lock)) {
                void *tmp = list_remove_head(file->waiting_on_lock);
                if ( tmp == NULL ) {
                    log_error("Dequeued invalid file descriptor\n");
                    unlock_return(&(storage->access), NULL);
                    break;
                }
            
                int client_fd = *(int*)tmp;

                log_debug("(LOCK HANDLER) updating file [%s] lock with client (%d)\n", file->path, client_fd);

                SET_FLAG(file->flags, O_LOCK);
                file->locked_by = client_fd;

                storage_update_file(storage, file);

                log_debug("(LOCK HANDLER) replying to client (%d)\n", client_fd);

                send_response(client_fd, SUCCESS, get_status_message(SUCCESS), strlen(file->path) + 1, file->path, 0, NULL);

                log_info("(LOCK HANDLER) file [%s] lock reassigned\n", file->path);
            }

            curr = curr->next;
        }

        unlock_return(&(storage->access), NULL);
    }
    
    return NULL;
}

int 
setup_lock_manager(int *lock_manager_pipe, pthread_t *lock_manager_id)
{
    if( pthread_create(lock_manager_id, NULL, &lock_manager_thread, (void*)lock_manager_pipe) != 0 ) return -1;
    return 0;
}