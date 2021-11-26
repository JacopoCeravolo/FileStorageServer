#ifndef FILESERVER_API_H
#define FILESERVER_API_H

#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utils/protocol.h"

int socket_fd;

/**
 * \brief Tries to open a connection to the socket file specified in the path variable socketname, 
 *        if the connection is not accepted immediatly tries again for msec millisecond until abstime
 * 
 * \param sockname  path to the socket file to connect to
 * \param msec      interval in milliseconds between two attempts
 * \param abstime   absolute time availble for the connection attempt    
 * 
 * \return 0 if the connection is successfully opened, -1 otherwise. ERRNO is correctly set
 */
int 
openConnection(const char* sockname, int msec, const struct timespec abstime);

/**
 * \brief Tries to close a connection to the socket file specified in the path variable socketname
 * 
 * \param sockname  path to the socket file to remove connection from    
 * 
 * \return 0 if the connection is successfully closed, -1 otherwise. ERRNO is correctly set
 */
int 
closeConnection(const char* sockname);

/**
 * \brief Tries to open the file specified in the path variable pathname with the specified flags.
 * 
 * \param pathname  path to the file to open
 * \param flags     flags specified for opening   
 * 
 * \return 0 if the file is correctly opened, -1 otherwise. ERRNO is correctly set
 */
int
openFile(const char* pathname, int flags);

/**
 * \brief Tries to read the file specified in the path variable pathname. If the file exists 
 *          it is temporarly saved in the buffer buf updating its size in the variable size.
 * 
 * \param pathname  path to the file to read
 * \param buf       buffer used for saving the file
 * \param size      size of the buffer used for saving
 * 
 * \return 0 if the file is correctly read, -1 otherwise. ERRNO is correctly set
 */
int 
readFile(const char* pathname, void** buf, size_t* size);

/**
 * \brief Tries to read N random files from the server. If N is 0 or greater than the actual
 *          number of files in the storage, it reads all file present in storage. If dirname is 
 *          not NULL the files are saved in the directory specified by dirname, otherwise they are
 *          flushed.
 * 
 * \param N         number of files to be read
 * \param dirname   directory for reading the files (can be NULL)
 * 
 * \return 0 if all files are correctly read, -1 otherwise. ERRNO is correctly set
 */
int 
readNFiles(int N, const char* dirname);

/**
 * \brief Tries to write the file specified in the path variable pathname. If dirname is not NULL,
 *          files expelled from server during insertion are saved in the directory dirname, otherwise
 *          they are thrashed.
 * 
 * \param pathname  path to the file to write
 * \param dirname   directory for saving the expelled files (can be NULL)
 * 
 * \return 0 if the file is correctly written, -1 otherwise. ERRNO is correctly set
 */
int 
writeFile(const char* pathname, const char* dirname);

/**
 * \brief Tries to lock the append the contents in buffer buf of size size to the file specified in
 *          the path variable pathname. If dirname is not NULL, if any file is expelled during the append, 
 *          they are saved in the directory dirname, otherwise they are thrashed.
 * 
 * \param pathname  path to the file to append to
 * \param buf       buffer containing the stream to append
 * \param size      size of the buffer
 * \param dirname   directory for saving the expelled files (can be NULL)
 * 
 * \return 0 if the file is correctly locked, -1 otherwise. ERRNO is correctly set
 */
int 
appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

/**
 * \brief Tries to lock the file specified in the path variable pathname.
 * 
 * \param pathname  path to the file to lock
 * 
 * \return 0 if the file is correctly locked, -1 otherwise. ERRNO is correctly set
 */
int 
lockFile(const char* pathname);

/**
 * \brief Tries to unlock the file specified in the path variable pathname.
 * 
 * \param pathname  path to the file to unlock
 * 
 * \return 0 if the file is correctly unlocked, -1 otherwise. ERRNO is correctly set
 */
int 
unlockFile(const char* pathname);

/**
 * \brief Tries to close the file specified in the path variable pathname.
 * 
 * \param pathname  path to the file to close
 * 
 * \return 0 if the file is correctly closed, -1 otherwise. ERRNO is correctly set
 */
int 
closeFile(const char* pathname);

/**
 * \brief Tries to delete the file specified in the path variable pathname.
 * 
 * \param pathname  path to the file to delete
 * 
 * \return 0 if the file is correctly deleted, -1 otherwise. ERRNO is correctly set
 */
int 
removeFile(const char* pathname);

#endif