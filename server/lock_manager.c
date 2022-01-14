#include "lock_manager.h"

void*
lock_manager_thread(void* args)
{
    while (shutdown_now == 0) {

        lock_return(&(storage->access), NULL);

        // Start iterating over files list
        node_t *curr = storage->fifo_queue->head;
       
        while (curr != NULL) {

            file_t *file = storage_get_file(storage, (char*)curr->data);
            if ( file == NULL ) break;

            // File is not locked and there are clients waiting
            if (!CHK_FLAG(file->flags, O_LOCK) && !list_is_empty(file->waiting_on_lock)) {
                
                void *tmp = list_remove_head(file->waiting_on_lock);
                if ( tmp == NULL ) break;
            
                int client_fd = *(int*)tmp;

                log_debug("updating file [%s] lock\n", file->path);

                SET_FLAG(file->flags, O_LOCK);
                file->locked_by = client_fd;

                storage_update_file(storage, file);

                log_debug("replying to client\n");

                send_response(client_fd, SUCCESS, get_status_message(SUCCESS), strlen(file->path) + 1, file->path, 0, NULL);

                log_info("(LOCK MAN) [  %s  ]  %-21s : %s\n", "lockFile", get_status_message(SUCCESS), file->path);
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