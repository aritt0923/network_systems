/*
    C socket server example, handles multiple clients using threads
*/

#include <proxy_serv_funs.h>
#include <utilities.h>
#include <status_codes.h>
#include <wrappers.h>


// the thread function
void *connection_handler(void *);
static void sig_handler(int _);

sem_t keep_running_mut; // protects keep_running global

static int keep_running = 1;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: ./tcp_serv <port #> <time to live (seconds)>\n");
        return 1;
    }
    sem_init(&keep_running_mut, 0, 1);

    signal(SIGINT, sig_handler);
    int socket_desc;
    struct sockaddr_in server;

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    int optval = 1;

    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));
    if (socket_desc == -1)
    {
        fprintf(stderr, "Could not create socket\n");
    }

    int port_num;
    if (str2int(&port_num, argv[1]) != 0)
    {
        printf("invalid port number\n");
        exit(1);
    }
    int ttl_seconds;
    if (str2int(&ttl_seconds, argv[2]) != 0)
    {
        printf("invalid time to live\n");
        exit(1);
    }
    
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port_num);

    // Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        // print the error message
        perror("bind failed. Error");
        return 1;
    }
    // puts("bind done");

    listen_wrap(socket_desc, 256);
    
    sem_t accept_sem;
    sem_init(&accept_sem, 0, 1);
    
    sem_t socket_sem;
    sem_init(&socket_sem, 0, 1);
    
    hash_table *cache = calloc_wrap(1, sizeof(hash_table));
    int buckets = 100;
    create_hash_table(cache, buckets, ttl_seconds);
    
    thread_args args; // struct passed to requester threads
    args.cache = cache;
    args.socket_desc = socket_desc;
    args.accept_sem = &accept_sem;
    args.socket_sem = &socket_sem;

    int num_threads = 20;
    pthread_t thread_id_arr[num_threads];
    for (int i = 0; i < num_threads; i++)
    { // create requesters
        pthread_create_wrap(&(thread_id_arr[i]), NULL, connection_handler, (void *)&args);
    }
    pthread_t socket_closer_thread;
    pthread_create_wrap(&socket_closer_thread, NULL, socket_closer, (void *)&args);
    pthread_join(socket_closer_thread, NULL);
    join_threads(num_threads, thread_id_arr);
    free_hash_table(cache);
    printf("All threads joined and hash table freed\n");
    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *vargs)
{
    struct sockaddr_in client;
    char *client_message = malloc_wrap(MAX_MSG_SZ);
    int c = sizeof(struct sockaddr_in);

    thread_args *args = (thread_args *)vargs;
    while (1)
    {
        memset(client_message, '\0', MAX_MSG_SZ);

        /**** ONE THREAD ACCEPTS AT A TIME ****/

        sem_wait(args->accept_sem);
        sem_wait(&keep_running_mut);

        if (!keep_running)
        {
            sem_post(&keep_running_mut);
            sem_post(args->accept_sem);
            break;
        }
        sem_post(&keep_running_mut);

        // accept connection from an incoming client
        // printf("Thread %lu listening on socket %d\n", pthread_self(), args->socket_desc);
        int client_sock = accept(args->socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);

        sem_post(args->accept_sem);

        if (client_sock < 0)
        {
            break;
        }

        // printf("Connection accepted by thread %lu\n", pthread_self());

        /**** END EXCLUSION ZONE ****/

        // Receive a message from client
        sem_wait(args->socket_sem);
        int read_size = recv(client_sock, client_message, MAX_MSG_SZ, 0);
        sem_post(args->socket_sem);
        

        int res = 0;
        if ((res = handle_get(args->cache, client_message, client_sock, args->socket_sem)) != 0)
        {
            fprintf(stderr, "handle_get returned %d\n", res);
        }

        if (read_size == 0)
        {
            puts("Client disconnected");
            fflush(stdout);
        }
        else if (read_size == -1)
        {
            perror("recv failed");
        }
        close(client_sock);
        
    }
    // printf("Thread %lu broke out of while loop.\n", pthread_self());

    free(client_message);
    return 0;
}

void *socket_closer(void *vargs)
{
    thread_args *args = (thread_args *)vargs;
    while (1)
    {
        sem_wait(&keep_running_mut);
        if (!keep_running)
        {
            sem_post(&keep_running_mut);
            break;
        }
        sem_post(&keep_running_mut);
    }
    shutdown(args->socket_desc, SHUT_RD);
    // printf("Closing socket %d\n", args->socket_desc);
    return NULL;
}

static void sig_handler(int _)
{
    (void)_;
    // printf("Entered handler\n");
    sem_wait(&keep_running_mut);
    keep_running = 0;
    sem_post(&keep_running_mut);
}