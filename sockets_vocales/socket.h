#ifndef _SOCKET_H
#define _SOCKET_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef enum _socket_type {
	SOCK_ACTIVE = 0,
	SOCK_PASSIVE
} socket_type;

typedef struct socket socket_t;

/*
 * Creates a new socket of the specified type
 */
socket_t* socket_create(socket_type type);

/*
 * Closes and destroys a socket, freeing any addrinfo
 */
void socket_destroy(socket_t* self);

/*
 * Attemps to bind the socket to a specific hostname and service.
 */
int socket_bind(socket_t* self, char* hostname, char* port);

/*
 * Puts a passive socket in a state ready to accept incomming connections.
 */
int socket_listen(socket_t* self, int backlog);

/*
 * Blocks until a new connection is established and then retunrs a new to a new socket
 */
socket_t* socket_accept(socket_t* self);

/*
 * Attemps to connect to the hostname and port passed as parameters.
 */
int socket_conect(socket_t* self, char* hostname, char* port);

/*
 * Continously reads bytes from socket in a blocking fashion, until size bytes are
 * read or an error occurs. Read bytes are stored on buffer.
 */
int socket_receive(socket_t* self, char* buffer, size_t tamanio);

/*
 * Continously write bytes to a socket, until size bytes are writen
 * or an error occurs.
 */
int socket_send(socket_t* self, const char* buffer, size_t tamanio);


#endif // _SOCKET_H
