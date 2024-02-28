#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "http.h"

//---------------------------------------------------------------------------------
// TODO:  Documentation
//
// Note that this module includes a number of helper functions to support this
// assignment.  YOU DO NOT NEED TO MODIFY ANY OF THIS CODE.  What you need to do
// is to appropriately document the socket_connect(), get_http_header_len(), and
// get_http_content_len() functions. 
//
// NOTE:  I am not looking for a line-by-line set of comments.  I am looking for 
//        a comment block at the top of each function that clearly highlights you
//        understanding about how the function works and that you researched the
//        function calls that I used.  You may (and likely should) add additional
//        comments within the function body itself highlighting key aspects of 
//        what is going on.
//
// There is also an optional extra credit activity at the end of this function. If
// you partake, you need to rewrite the body of this function with a more optimal 
// implementation. See the directions for this if you want to take on the extra
// credit. 
//--------------------------------------------------------------------------------

char *strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

char *strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == '\0' || slen-- < 1)
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

/**
 * Establishes a TCP connection to the specified host and port.
 * 
 * This function performs DNS resolution for the given hostname using gethostbyname(),
 * which translates the hostname into an IP address. It then initializes a socket for
 * IPv4 communication using the SOCK_STREAM type to allow for TCP connections. The function
 * attempts to connect to the server using the resolved IP address and specified port.
 * 
 * Parameters:
 * - host: A pointer to a character array containing the hostname or IP address.
 * - port: The port number on which the server is listening.
 * 
 * Returns:
 * - On success, a socket descriptor representing the established connection.
 * - On failure, -1 if the socket couldn't be created, or -2 if the hostname couldn't
 *   be resolved, accompanied by an appropriate error message to stderr.
 */

// Variables:
// - hp: Holds the DNS lookup result for the given host. It's updated by gethostbyname.
// - addr: A struct sockaddr_in that stores the server's address information, including IP and port. It's populated using information from hp and the given port.
// - sock: Represents the socket descriptor. It's obtained from socket() and is the value returned by the function.
// Operation Details:
// - gethostbyname: Converts the hostname into a network address, and hp is updated with this address.
// - The bcopy() function copies the server's address from hp->h_addr to addr.sin_addr.
// - htons(port): Converts the port number from host byte order to network byte order.
// - socket(): Initializes a new socket and updates sock with the socket descriptor.
// - connect(): Attempts to establish a connection to the server with the address specified in addr, affecting the connection status associated with sock.

int socket_connect(const char *host, uint16_t port){
    struct hostent *hp;
    struct sockaddr_in addr;
    int sock;

    if((hp = gethostbyname(host)) == NULL){
		herror("gethostbyname");
		return -2;
	}
    
    
	bcopy(hp->h_addr_list[0], &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, 0); 
	
	if(sock == -1){
		perror("socket");
		return -1;
	}

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
		perror("connect");
		close(sock);
        return -1;
	}

    return sock;
}

/**
 * Determines the length of the HTTP header in a response buffer.
 * 
 * This function searches the response buffer for the end of the HTTP header,
 * identified by the sequence "\r\n\r\n". It uses a custom implementation of
 * strnstr to safely search within the buffer's length, avoiding overflows. The
 * function calculates the header's length by finding the difference between the
 * start of the buffer and the position of the header's end, then adds the length
 * of the header end sequence to account for its presence.
 * 
 * Parameters:
 * - http_buff: A buffer containing the HTTP response.
 * - http_buff_len: The length of the buffer.
 * 
 * Returns:
 * - The length of the HTTP header, including the end sequence, if found.
 * - -1 if the end of the header cannot be found, indicating an invalid or incomplete response.
 */

// Variables:
// - end_ptr: Points to the location within http_buff where the HTTP header end sequence ("\r\n\r\n") is found.
// - header_len: Stores the total length of the HTTP header, calculated based on the position of end_ptr.
// Operation Details:
// - strnstr: Searches for the header end sequence in http_buff, updating end_ptr with the position of this sequence.
// - header_len: Calculated by finding the difference between end_ptr and the start of http_buff, then adding the length of the HTTP header end sequence.

int get_http_header_len(char *http_buff, int http_buff_len){
    char *end_ptr;
    int header_len = 0;
    end_ptr = strnstr(http_buff,HTTP_HEADER_END,http_buff_len);

    if (end_ptr == NULL) {
        fprintf(stderr, "Could not find the end of the HTTP header\n");
        return -1;
    }

    header_len = (end_ptr - http_buff) + strlen(HTTP_HEADER_END);

    return header_len;
}

/**
 * Extracts the content length from an HTTP response header.
 * 
 * Iterates through the response header looking for the "Content-Length" field,
 * using a combination of string searching and case-insensitive comparison. Upon
 * finding the field, it parses the value following the colon (':'), which represents
 * the size of the response body. This function is crucial for understanding how much
 * data needs to be read from the socket after the header has been processed.
 * 
 * Parameters:
 * - http_buff: A buffer containing the HTTP response header.
 * - http_header_len: The length of the header within the buffer.
 * 
 * Returns:
 * - The parsed content length as an integer if the "Content-Length" field is found.
 * - 0 if the field is not found, implying either an error or a response without a body.
 */

// Variables:
// - next_header_line: Iterates through the HTTP header lines within http_buff.
// - header_line: Stores the current header line being processed.
// - content_len: The extracted value of the Content-Length header, initialized to 0 and updated based on the parsed header value.
// Operation Details:
// - The loop iterates through each header line, updating next_header_line to point to the start of the next line after each iteration.
// - strcasestr and strchr: Used to find the "Content-Length" header and its value, respectively. content_len is updated with the parsed integer value of this header.

int get_http_content_len(char *http_buff, int http_header_len){
    char header_line[MAX_HEADER_LINE];

    char *next_header_line = http_buff;
    char *end_header_buff = http_buff + http_header_len;

    while (next_header_line < end_header_buff){
        bzero(header_line,sizeof(header_line));
        sscanf(next_header_line,"%[^\r\n]s", header_line);

        char *isCLHeader2 = strcasecmp(header_line,CL_HEADER);
        char *isCLHeader = strcasestr(header_line,CL_HEADER);
        if(isCLHeader != NULL){
            char *header_value_start = strchr(header_line, HTTP_HEADER_DELIM);
            if (header_value_start != NULL){
                char *header_value = header_value_start + 1;
                int content_len = atoi(header_value);
                return content_len;
            }
        }
        next_header_line += strlen(header_line) + strlen(HTTP_HEADER_EOL);
    }
    fprintf(stderr,"Did not find content length\n");
    return 0;
}

//This function just prints the header, it might be helpful for your debugging
//You dont need to document this or do anything with it, its self explanitory. :-)
void print_header(char *http_buff, int http_header_len){
    fprintf(stdout, "%.*s\n",http_header_len,http_buff);
}

//--------------------------------------------------------------------------------------
//EXTRA CREDIT - 10 pts - READ BELOW
//
// Implement a function that processes the header in one pass to figure out BOTH the
// header length and the content length.  I provided an implementation below just to 
// highlight what I DONT WANT, in that we are making 2 passes over the buffer to determine
// the header and content length.
//
// To get extra credit, you must process the buffer ONCE getting both the header and content
// length.  Note that you are also free to change the function signature, or use the one I have
// that is passing both of the values back via pointers.  If you change the interface dont forget
// to change the signature in the http.h header file :-).  You also need to update client-ka.c to 
// use this function to get full extra credit. 
//--------------------------------------------------------------------------------------
int process_http_header(char *http_buff, int http_buff_len, int *header_len, int *content_len){
    int h_len, c_len = 0;
    h_len = get_http_header_len(http_buff, http_buff_len);

    if (h_len < 0) {
        *header_len = 0;
        *content_len = 0;
        return -1;
    }
    c_len = get_http_content_len(http_buff, http_buff_len);

    if (c_len < 0) {
        *header_len = 0;
        *content_len = 0;
        return -1;
    }

    *header_len = h_len;
    *content_len = c_len;
    return 0; //success
}

// THE FOLLOWING CODE IS MY SOLUTION TO EXTRA CREDIT
// Note: I still keep the code process_http_header above for checking purpose

#define CONTENT_LENGTH "Content-Length:"

// The basic idea is to check one line at a time by detecting \r\n, 
// then extract the value of Content-Length from the line contains it
// if we get \r\n\r\n then we stop looping as it is the end of header,
// now we can calculate the length of header. Finally, we put the values into
// the memories pointed by header_len and content_len

int process_http_header_single_pass(char *http_buff, int http_buff_len, int *header_len, int *content_len) {
    *header_len = 0;
    *content_len = 0;

    int eoh_found = 0; // end of header found flag
    int line_start = 0; // start of the current line

    for (int i = 0; i < http_buff_len - 3; ++i) { // -3 to ensure we have room for \r\n\r\n
        // detect end of the HTTP header
        if (!eoh_found && strncmp(&http_buff[i], HTTP_HEADER_END, 4) == 0) {
            *header_len = i + 4; // including the \r\n\r\n
            eoh_found = 1;
            i += 3; // skip past the end of header
            continue; 
        }

        // detect end of a line within the header
        if (strncmp(&http_buff[i], "\r\n", 2) == 0) {
            if (!eoh_found) { // only run if we are within header
                int line_length = i - line_start;
                char line_buffer[line_length + 1]; 
                strncpy(line_buffer, &http_buff[line_start], line_length);
                line_buffer[line_length] = '\0'; 

                // check if the line contains "Content-Length"
                if (strncmp(line_buffer, CONTENT_LENGTH, strlen(CONTENT_LENGTH)) == 0) {
                    *content_len = atoi(line_buffer + strlen(CONTENT_LENGTH));
                }
            }
            line_start = i + 2; // move to next line
            i++; // skip the \n part of \r\n
        }
    }

    return (*header_len > 0 && eoh_found) ? 0 : -1;
}