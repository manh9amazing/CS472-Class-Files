#include "http.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define  BUFF_SZ            1024
#define  MAX_REOPEN_TRIES   5

char recv_buff[BUFF_SZ];

char *generate_cc_request(const char *host, int port, const char *path){
	static char req[512] = {0};
	int offset = 0;
	
    //note that all paths should start with "/" when passed in
	offset += sprintf((char *)(req + offset),"GET %s HTTP/1.1\r\n", path);
	offset += sprintf((char *)(req + offset),"Host: %s\r\n", host);
	offset += sprintf((char *)(req + offset),"Connection: Keep-Alive\r\n");
	offset += sprintf((char *)(req + offset),"\r\n");

	printf("DEBUG: %s", req);
	return req;
}


void print_usage(char *exe_name){
    fprintf(stderr, "Usage: %s <hostname> <port> <path...>\n", exe_name);
    fprintf(stderr, "Using default host %s, port %d  and path [\\]\n", DEFAULT_HOST, DEFAULT_PORT); 
}

int reopen_socket(const char *host, uint16_t port) {
    int sock = 0;

    //----------------------------------------------------------------------------
    //TODO: Implement a loop that attempts a certain number of times to open a 
    //      socket with the host and port that is passed in as parameters.
    //
    //      I created a constant called MAX_REOPEN_TRIES (currently set to 5)
    //      that bounds the number of times we will attempt to reopen a socket
    //      with the remote web server.
    //
    //      What you need to do:
    //          1. Create a loop that loops MAX_REOPEN_TRIES
    //          2. In the body of the loop attempt to connect to the server.  
    //             use the sock = socket_connect(host,port)  function to 
    //             do this.
    //          3. The socket_connect function will return a negative number if
    //             the socket_connect() function fails.  Continue looping if this
    //             happens up to MAX_REOPEN_TRIES
    //          4. If socket_connect() returns a positive number it is a valid
    //             socket so just return it, e.g., return sock
    //          5. If we fall out of the loop, we are unable to connect, return
    //             -1 to indicate a failure. 
    //----------------------------------------------------------------------------
    for (int tries = 0; tries < MAX_REOPEN_TRIES; tries++) {
        sock = socket_connect(host, port);
        if (sock >= 0) {
            return sock;
        }
    }
    
    return -1;
}

int server_connect(const char *host, uint16_t port){
    return socket_connect(host, port);
}

void server_disconnect(int sock){
    close(sock);
}

int submit_request(int sock, const char *host, uint16_t port, char *resource){
    int sent_bytes = 0; 

    const char *req = generate_cc_request(host, port, resource);
    int send_sz = strlen(req);

    // This is the initial send, this is where the send will fail with 
    // Keep-Alive if the server closed the socket, sent_bytes will have
    // the number of bytes sent, which should equal send_sz if all went
    // well.  If sent_bytes < 0, this indicates a send error, so we will
    // need to reconnect to the server.
    sent_bytes = send(sock, req, send_sz,0);

    //we have a socket error, perhaps the server closed it, lets try to reopen
    //the socket
    if (sent_bytes < 0){
        //----------------------------------------------------------------------------
        //TODO:  Reimplement the retry logic to reopen the socket
        //
        // The variable sock needs to be reset to a new socket with the server.
        // 1.  Use the sock = reopen_socket() helper you implemented above in an attempt
        //     to reopen_the socket.  If it returns a value less than zero, a socket 
        //     could not be created so return the negative value and exit the function,
        //     e.g., return sock
        //
        //  2. Assuming you got a valid socket, reissue the send again
        //     sent_bytes = send(sock, req, send_sz,0);
        //----------------------------------------------------------------------------
        sock = reopen_socket(host, port);
        if (sock < 0) {
            return sock; 
        }

        sent_bytes = send(sock, req, send_sz, 0);
    }

    //This should not happen, but just checking if we didnt send everything and 
    //handling appropriately 
    if(sent_bytes != send_sz){
        if(sent_bytes < 0)
            perror("send failed after reconnect attempt");
        else
            fprintf(stderr, "Sent bytes %d is not equal to sent size %d\n", sent_bytes, send_sz);
        
        close(sock);
        return -1;
    }

    int bytes_recvd = 0;    //used to track amount of data received on each recv() call
    int total_bytes = 0;    //used to accumulate the total number of bytes across all recv() calls
    
    //do the first recv
    bytes_recvd = recv(sock, recv_buff, sizeof(recv_buff),0);
    if(bytes_recvd < 0) {
        perror("initial receive failed");
        close(sock);
        return -1;
    }

    // uncomment this to test Extra Credit solution, 
    // int * hd = malloc(sizeof(int));
    // int * ct = malloc(sizeof(int));
    // int * hdn = malloc(sizeof(int));
    // int * ctn = malloc(sizeof(int));

    // process_http_header(recv_buff, bytes_recvd, hd, ct);
    // fprintf(stdout, "ORIG HEADER LEN: %d\n", *hd); 
    // fprintf(stdout, "ORIG CONTENT LEN: %d\n", *ct); 
    // process_http_header_single_pass(recv_buff, bytes_recvd, hdn, ctn);
    // fprintf(stdout, "NEW HEADER LEN: %d\n", *hdn); 
    // fprintf(stdout, "NEW CONTENT LEN: %d\n", *ctn); 

    // free(hd);
    // free(ct);
    // free(hdn);
    // free(ctn);

    //remember the first receive we just did has the HTTP header, and likely some body
    //data.  We need to determine how much data we expect

    //--------------------------------------------------------------------------------
    //TODO:  Get the header len
    //
    // 1. Use the get_http_header_len() function to set the header_len variable.
    // 2. The get_http_header_len() function returns a negative value if it fails, so
    //    check the header_len variable and if its negative:
    //          a. close the socket -- close(sock)
    //          b. return -1 to exit this function
    //--------------------------------------------------------------------------------
    int header_len = 0;     //change this to get the header len as per the directions above
    header_len = get_http_header_len(recv_buff, bytes_recvd);
    if (header_len < 0) {
        close(sock);
        return -1;
    }
    
    //--------------------------------------------------------------------------------
    //TODO:  Get the conetent len
    //
    // 1. Use the get_http_content_len() function to set the content_len variable.
    //
    // Note that no error checking is needed, if the get_http_content_len() function
    // cannot find a Content-Length header, its assumed as per the HTTP spec that ther
    // is no body, AKA, content_len is zero;
    //--------------------------------------------------------------------------------
    int content_len = 0;    //Change this to get the content length
    content_len = get_http_content_len(recv_buff, header_len);

    //--------------------------------------------------------------------------------
    // TODO:  Make sure you understand the calculations below
    //
    // You do not have to write any code, but add to this comment your thoughts on 
    // what the following 2 lines of code do to track the amount of data received
    // from the server
    //
    // MY ANSWER:
    //
    // The calculation of initial_data and bytes_remaining serves to manage the reception
    // of the HTTP response data correctly, considering the HTTP header and body are received together 
    // and we receive one chunk/part of data in one receive. This ensures that the client does not prematurely
    // close the connection or attempt to read beyond the end of the current response.
    // 
    // `initial_data = bytes_recvd - header_len;` calculates the amount of data received
    // in the first recv() call that actually belongs to the content (body) of the response.
    // This is done by subtracting the length of the HTTP header from the total bytes received.
    //
    // `bytes_remaining = content_len - initial_data;` determines how much more content (body)
    // data we expect to receive based on the Content-Length header of the response. content_len
    // is the total length of the body as reported by the server, and initial_data is how much
    // of that body we've already received. Subtracting these gives us the remaining amount of
    // body data we need to read from the socket.

    //--------------------------------------------------------------------------------
    int initial_data =  bytes_recvd - header_len;
    int bytes_remaining = content_len - initial_data;


    //This loop keeps going until bytes_remaining is essentially zero, to be more
    //defensive an prevent an infinite loop, i have it set to keep looping as long
    //as bytes_remaining is positive
    while(bytes_remaining > 0){
        //-----------------------------------------------------------------------------
        // TODO:  Continue receiving data from the server
        //
        // 1. make a recv() call to the server, using recv_buff
        // 2. Get the number of bytes received and store in the bytes_recvd variable
        // 3. Check for an error, e.g., bytes_recvd < 0 - if that is the case:
        //      a. close the socket (sock)
        //      b. return -1 to indicate an error
        //-----------------------------------------------------------------------------
        bytes_recvd = recv(sock, recv_buff, sizeof(recv_buff), 0);
        if (bytes_recvd < 0) {
            perror("recv failed");
            close(sock);
            return -1;
        }
        
        //You can uncomment out the fprintf() calls below to see what is going on

        fprintf(stdout, "%.*s", bytes_recvd, recv_buff);
        total_bytes += bytes_recvd;
        fprintf(stdout, "remaining %d, received %d\n", bytes_remaining, bytes_recvd);
        bytes_remaining -= bytes_recvd;
    }

    fprintf(stdout, "\n\nOK\n");
    fprintf(stdout, "TOTAL BYTES: %d\n", total_bytes); // If followed TODO, the bytes here is total bytes received after first receive
    fprintf(stdout, "CONTENT LENGTH: %d\n", total_bytes + initial_data); 
    // To know total bytes received from start to end, it is just total_bytes + initial data or in most case content_len

    //processed the request OK, return the socket, in case we had to reopen
    //so that it can be used in the next request

    //---------------------------------------------------------------------------------
    // TODO:  Documentation
    //
    // You dont have any code to change, but explain why this function, if it gets to this
    // point returns an active socket.
    //
    // MY ANSWER:
    //
    // This function returns an active socket at the end of processing a request successfully
    // to allow for the re-use of the same TCP connection for subsequent HTTP requests. This
    // behavior is in line with the HTTP/1.1 specification, which encourages the use of persistent
    // connections to reduce the overhead (TCP handshake overhead and slow start phase for each new request) 
    // of establishing new connections for each request.
    //
    // Again, if the function gets to this point, it means the request was processed without errors,
    // and the connection remains open and in a good state. Returning the socket allows the
    // `main` function or subsequent calls to `submit_request` to use this persistent connection
    // for further requests. This approach aligns with the `Connection: Keep-Alive` header's
    // intent, keeping the connection alive for multiple HTTP requests/responses, thereby
    // improving the efficiency of network resource utilization. And in the end, close the socket with
    // `server_disconnect` in `main`

    //--------------------------------------------------------------------------------
    return sock;
}

int main(int argc, char *argv[]){
    clock_t start_time = clock();

    int sock;

    const char *host = DEFAULT_HOST;
    uint16_t   port = DEFAULT_PORT;
    char       *resource = DEFAULT_PATH;
    int        remaining_args = 0;

    //YOU DONT NEED TO DO ANYTHING OR MODIFY ANYTHING IN MAIN().  MAKE SURE YOU UNDERSTAND
    //THE CODE HOWEVER
    sock = server_connect(host, port);

    if(argc < 4){
        print_usage(argv[0]);
        //process the default request
        submit_request(sock, host, port, resource);
	} else {
        host = argv[1];
        port = atoi(argv[2]);
        resource = argv[3];
        if (port == 0) {
            fprintf(stderr, "NOTE: <port> must be an integer, using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
        fprintf(stdout, "Running with host = %s, port = %d\n", host, port);
        remaining_args = argc-3;
        for(int i = 0; i < remaining_args; i++){
            resource = argv[3+i];
            fprintf(stdout, "\n\nProcessing request for %s\n\n", resource);
            sock = submit_request(sock, host, port, resource);
        }
    }

    server_disconnect(sock);

    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Elapsed time: %.5f seconds\n", elapsed_time);
}