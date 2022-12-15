#ifndef SERV_H_
#define SERV_H_

#include <../client_serv_common.h>
#include "../file_cabinet.h"
#include "../utilities.h"
typedef struct // arguments for requester threads
{
    hash_table *cache;
    int socket_desc;
    sem_t *accept_sem;
    sem_t *socket_sem;
    int thread_id;      
    
} thread_args;

/**
 * 
 * @param df_serv_arr array of df_serv_info structs
 * @param serv_cmd df_serv_cmd
 */
int handle_cmd_dfs(hash_table *file_cabinet, const char *client_req, int sockfd, sem_t *socket_sem);

int build_serv_cmd(char *header_buf, df_serv_cmd *cmd, struct file_cabinet_node *chunk);

int handle_put_dfs(hash_table *file_cabinet, df_serv_cmd *cmd, char * client_req, int sockfd, sem_t *socket_sem);

int record_to_file_list(hash_table *file_cabinet, df_serv_cmd *cmd, int sockfd, sem_t *socket_sem);

int handle_get_dfs(hash_table *file_cabinet, df_serv_cmd *cmd, int sockfd, sem_t *socket_sem);

int handle_ls_dfs(hash_table *file_cabinet, int sockfd, sem_t *socket_sem);

void join_threads(int num_threads, pthread_t *thr_arr);

void *socket_closer(void *vargs);

int recieve_chunk(hash_table *file_cabinet, df_serv_cmd *cmd, char * client_req, int sockfd, sem_t *socket_sem);


#endif //SERV_H_