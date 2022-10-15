/*
	C socket server example, handles multiple clients using threads
*/

#include <tcp_serv.h>

//the thread function
void *connection_handler(void *);

int main(int argc , char *argv[])
{
    int socket_desc;
    struct sockaddr_in server;

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    
    int optval = 1;
    
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9312);

    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        // print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
    sem_t listen;
    sem_init(&listen, 0, 1);
    
    thread_args args; // struct passed to requester threads
    args.socket_desc = socket_desc;
    args.listen = &listen;
    
    int num_threads = 10;
    pthread_t thread_id_arr[num_threads];
    for (int i = 0; i < num_threads; i++)
    { // create requesters
        pthread_create_wrap(&(thread_id_arr[i]), NULL, connection_handler, (void*)&args);
    }
    join_threads(num_threads, thread_id_arr);
    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *vargs)
{
    struct sockaddr_in client;
    char * client_message = malloc_wrap(MAX_CLNT_MSG_SZ);
    
    thread_args *args = (thread_args *)vargs;
    while (1)
    {
        memset(client_message, MAX_CLNT_MSG_SZ, sizeof(char));
        
        /**** ONE THREAD LISTENS AT A TIME ****/
        
        sem_wait(args->listen);  
        listen(args->socket_desc, 3);

        puts("Waiting for incoming connections...");
        int c = sizeof(struct sockaddr_in);

        // accept connection from an incoming client
        int client_sock = accept_wrap(args->socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
        sem_post(args->listen);
        puts("Connection accepted");
        
        /**** END EXCLUSION ZONE ****/
        

        // Receive a message from client
        int read_size = recv(client_sock, client_message, MAX_CLNT_MSG_SZ, 0);

        int res = 0;
        if((res = handle_get(client_message, client_sock)) != 0)
        {
            fprintf(stderr, "handle_get returned %d\n", res);    
        }

        if (read_size == 0)
        {
            puts("Client disconnected");
            fflush(stdout);
            close(client_sock);
        }
        else if (read_size == -1)
        {
            perror("recv failed");
            close(client_sock);
        }
    }
    free(client_message);
    return 0;
}