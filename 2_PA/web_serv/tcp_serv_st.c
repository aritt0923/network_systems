#include <tcp_serv.h>

int main(int argc, char *argv[])
{
    int socket_desc, client_sock, c, read_size;
    struct sockaddr_in server, client;
    char client_message[MAX_CLNT_MSG_SZ];

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


    while (1)
    {

        // Listen
        listen(socket_desc, 3);

        // Accept and incoming connection
        puts("Waiting for incoming connections...");
        c = sizeof(struct sockaddr_in);

        // accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
        if (client_sock < 0)
        {
            perror("accept failed");
            return 1;
        }
        puts("Connection accepted");

        // Receive a message from client
        read_size = recv(client_sock, client_message, MAX_CLNT_MSG_SZ, 0);

        // Send the message back to client
        int res = 0;
        res = handle_get(client_message, client_sock);


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
    return 0;
}
