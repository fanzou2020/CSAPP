#ifndef __RIO_H__
#define __RIO_H__

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;         // Descriptor for this internal buf
    int rio_cnt;        // Unread bytes in internal buf
    char *rio_bufptr;   // Next unread byte in internal buf
    char rio_buf[RIO_BUFSIZE]; // internal buffer
} rio_t;



/** Unbuffered Rio input and output */
/* Transfer up to n bytes from the current file position of descriptor fd to memory location usrbuf
 * Return number of bytes transferred if OK, 0 on EOF, -1 on error
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);


// Buffered Rio input and output
/* Called once per open descriptor, it associates the descriptor fd with a read buffer of 
 * type rio_t at address rp.
*/
void rio_readinitb(rio_t *rp, int fd);

/**  
 * Reads the next line from file rp(including the terminating newline character), copies it to
 * memory location usrbuf, and terminates the text line with NULL(zero) character.
*/
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/**
 * Reads up to n bytes from file rp to memory location usrbuf
 */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);

#endif