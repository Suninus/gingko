/**
 * socket.h
 *
 *  Created on: 2011-7-17
 *      Author: auxten
 **/

#ifndef SOCKET_H_
#define SOCKET_H_

/// Set non-blocking
int setnonblock(int fd);
/// Set blocking
int setblock(int fd);
/// gracefully close a socket, for client side
int close_socket(int sock);
/// connect to a host
int connect_host(const s_host_t * h, const int recv_sec, const int send_sec);

#endif /** SOCKET_H_ **/
