/*
 *  Phusion Passenger - http://www.modrails.com/
 *  Copyright (c) 2010 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#ifndef _PASSENGER_IO_UTILS_H_
#define _PASSENGER_IO_UTILS_H_

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string>
#include "../StaticString.h"

namespace Passenger {

using namespace std;

enum ServerAddressType {
	SAT_UNIX,
	SAT_TCP,
	SAT_UNKNOWN
};

typedef ssize_t (*WritevFunction)(int fildes, const struct iovec *iov, int iovcnt);

/**
 * Accepts a server address in one of the following formats, and returns which one it is:
 *
 * Unix domain sockets
 *    Format: "unix:/path/to/a/socket"
 *    Returns: SAT_UNIX
 *
 * TCP sockets
 *    Format: "tcp://host:port"
 *    Returns: SAT_TCP
 *
 * Other
 *    Returns: SAT_UNKNOWN
 */
ServerAddressType getSocketAddressType(const StaticString &address);

/**
 * Parses a Unix domain socket address, as accepted by getSocketAddressType(),
 * and returns the socket filename.
 *
 * @throw ArgumentException <tt>address</tt> is not a valid Unix domain socket address.
 */
string parseUnixSocketAddress(const StaticString &address);

/**
 * Parses a TCP socket address, as accepted by getSocketAddressType(),
 * and returns the host and port.
 *
 * @throw ArgumentException <tt>address</tt> is not a valid TCP socket address.
 */
void parseTcpSocketAddress(const StaticString &address, string &host, unsigned short &port);

/**
 * Returns whether the given socket address (as accepted by getSocketAddressType())
 * is an address that can only refer to a server on the local system.
 *
 * @throw ArgumentException <tt>address</tt> is not a valid TCP socket address.
 */
bool isLocalSocketAddress(const StaticString &address);

/**
 * Sets a socket in non-blocking mode.
 *
 * @throws SystemException Something went wrong.
 * @ingroup Support
 */
void setNonBlocking(int fd);

/**
 * Create a new Unix or TCP server socket, depending on the address type.
 *
 * @param address An address as defined by getSocketAddressType().
 * @param backlogSize The size of the socket's backlog. Specify 0 to use
 *                    the paltform's maximu mallowed backlog size.
 * @param autoDelete If <tt>address</tt> is a Unix socket that already exists,
 *                   whether that should be deleted. Otherwise this argument
 *                   is ignored.
 * @return The file descriptor of the newly created Unix server socket.
 * @throws ArgumentException The given address cannot be parsed.
 * @throws RuntimeException Something went wrong.
 * @throws SystemException Something went wrong while creating the Unix server socket.
 * @throws boost::thread_interrupted A system call has been interrupted.
 * @ingroup Support
 */
int createServer(const StaticString &address, unsigned int backlogSize = 0, bool autoDelete = true);

/**
 * Create a new Unix server socket which is bounded to <tt>filename</tt>.
 *
 * @param filename The filename to bind the socket to.
 * @param backlogSize The size of the socket's backlog. Specify 0 to use the
 *                    platform's maximum allowed backlog size.
 * @param autoDelete Whether <tt>filename</tt> should be deleted, if it already exists.
 * @return The file descriptor of the newly created Unix server socket.
 * @throws RuntimeException Something went wrong.
 * @throws SystemException Something went wrong while creating the Unix server socket.
 * @throws boost::thread_interrupted A system call has been interrupted.
 * @ingroup Support
 */
int createUnixServer(const StaticString &filename, unsigned int backlogSize = 0, bool autoDelete = true);

/**
 * Create a new TCP server socket which is bounded to the given address and port.
 * SO_REUSEADDR will be set on the socket.
 *
 * @param address The IP address to bind the socket to.
 * @param port The port to bind the socket to, or 0 to have the OS automatically
 *             select a free port.
 * @param backlogSize The size of the socket's backlog. Specify 0 to use the
 *                    platform's maximum allowed backlog size.
 * @return The file descriptor of the newly created server socket.
 * @throws SystemException Something went wrong while creating the server socket.
 * @throws ArgumentException The given address cannot be parsed.
 * @throws boost::thread_interrupted A system call has been interrupted.
 * @ingroup Support
 */
int createTcpServer(const char *address = "0.0.0.0", unsigned short port = 0, unsigned int backlogSize = 0);

/**
 * Connect to a server at the given address.
 *
 * @param address An address as accepted by getSocketAddressType().
 * @return The file descriptor of the connected client socket.
 * @throws RuntimeException Something went wrong.
 * @throws SystemException Something went wrong while connecting to the server.
 * @throws IOException Something went wrong while connecting to the server.
 * @throws boost::thread_interrupted A system call has been interrupted.
 * @ingroup Support
 */
int connectToServer(const StaticString &address);

/**
 * Connect to a Unix server socket at <tt>filename</tt>.
 *
 * @param filename The filename of the socket to connect to.
 * @return The file descriptor of the connected client socket.
 * @throws RuntimeException Something went wrong.
 * @throws SystemException Something went wrong while connecting to the Unix server.
 * @throws boost::thread_interrupted A system call has been interrupted.
 * @ingroup Support
 */
int connectToUnixServer(const StaticString &filename);

/**
 * Connect to a TCP server socket at the given host name and port.
 *
 * @param hostname The host name of the TCP server.
 * @param port The port number of the TCP server.
 * @return The file descriptor of the connected client socket.
 * @throws IOException Something went wrong while connecting to the Unix server.
 * @throws SystemException Something went wrong while connecting to the server.
 * @throws boost::thread_interrupted A system call has been interrupted.
 * @ingroup Support
 */
int connectToTcpServer(const StaticString &hostname, unsigned int port);

/**
 * Writes a bunch of data to the given file descriptor using a gathering I/O interface.
 * Instead of accepting a single buffer, this function accepts multiple buffers plus
 * a special 'rest' buffer. The rest buffer is written out first, and the data buffers
 * are then written out in the order as they appear. This all is done with a single
 * writev() system call without concatenating all data into a single buffer.
 *
 * This function is designed for use with non-blocking sockets. It returns the number
 * of bytes that have been written, and ensures that restBuffer will contain all data
 * that has not been written, i.e. should be written out as soon as the file descriptor
 * is writeable again. If everything has been successfully written out then restBuffer
 * will be empty.
 * A return value of 0 indicates that nothing could be written without blocking.
 *
 * Returns -1 if an error occurred other than one which indicates blocking. In this
 * case, <tt>errno</tt> is set appropriately.
 *
 * This function also takes care of all the stupid writev() limitations such as
 * IOV_MAX. It ensures that no more than IOV_MAX items will be passed to writev().
 *
 * @param fd The file descriptor to write to.
 * @param data The data to write.
 * @param dataCount Number of elements in <tt>data</tt>.
 * @param restBuffer The rest buffer, as documented above.
 * @return The number of bytes that have been written out, or -1 on any error that
 *         isn't related to non-blocking writes.
 * @throws boost::thread_interrupted
 */
ssize_t gatheredWrite(int fd, const StaticString data[], unsigned int dataCount, string &restBuffer);

/**
 * Sets a writev-emulating function that gatheredWrite() should call instead of the real writev().
 * Useful for unit tests. Pass NULL to restore back to the real writev().
 */
void setWritevFunction(WritevFunction func);

} // namespace Passenger

#endif /* _PASSENGER_IO_UTILS_H_ */