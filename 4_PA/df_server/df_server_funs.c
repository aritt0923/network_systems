
#include "df_server.h"
#include "../file_cabinet.h"

int handle_cmd_dfs(hash_table *file_cabinet, const char *client_req, int sockfd, sem_t *socket_sem)
{
    printf("entered handle_cmd\n");
    // printf("HANDLE COMMAND\n");
    enable_keepalive(sockfd);
    // printf("client req: %s\n", client_req);
    df_serv_cmd *cmd = get_serv_cmd_ptr(1);
    char *client_req_cpy = calloc_wrap(MAX_MSG_SZ, sizeof(char));
    memcpy(client_req_cpy, client_req, strlen(client_req));
    str_to_serv_cmd(cmd, client_req_cpy);

    if ((strncmp(cmd->cmd, "list", strlen("list"))) == 0)
    {
        handle_ls_dfs(file_cabinet, sockfd, socket_sem);
    }
    else if ((strncmp(cmd->cmd, "put", strlen("put"))) == 0)
    {
        handle_put_dfs(file_cabinet, cmd, client_req, sockfd, socket_sem);
    }
    else if ((strncmp(cmd->cmd, "get", strlen("get"))) == 0)
    {
        printf("client_req %s\n", client_req);
        handle_get_dfs(file_cabinet, cmd, sockfd, socket_sem);
    }
    return 0;
}

int build_serv_cmd(char *header_buf, df_serv_cmd *cmd, struct file_cabinet_node *chunk)
{
    memcpy(cmd->cmd, "get", strlen("get"));
    memcpy(cmd->filename_hr, chunk->filename_hr, strlen(chunk->filename_hr));
    memcpy(cmd->filename_md5, chunk->filename_md5, strlen(chunk->filename_md5));
    memcpy(cmd->filename_tmstmp_md5, chunk->filename_md5, strlen(chunk->filename_tmstmp_md5));
    memcpy(cmd->chunk_name, chunk->chunk_name, strlen(chunk->chunk_name));

    memcpy(cmd->offset, chunk->offset, strlen(chunk->offset));
    memcpy(cmd->chunk_size, chunk->chunk_size, strlen(chunk->chunk_size));
    memcpy(cmd->filesize, chunk->filesize, strlen(chunk->filesize));

    cmd->chunk = chunk->chunk;
    printf("chunk_size: %s\n", cmd->chunk_size);

    serv_cmd_to_str(cmd, header_buf);
    return 0;
}

int handle_put_dfs(hash_table *file_cabinet, df_serv_cmd *cmd, char *client_req, int sockfd, sem_t *socket_sem)
{
    recieve_chunk(file_cabinet, cmd, client_req, sockfd, socket_sem);

    char *message_buf = calloc_wrap(MAX_MSG_SZ, sizeof(char));
    char *client_req_cpy = calloc_wrap(MAX_MSG_SZ, sizeof(char));
    while (1)
    {
        memset(message_buf, '\0', MAX_MSG_SZ);
        memset(client_req_cpy, '\0', MAX_MSG_SZ);
        memset(cmd->cmd, '\0', MAX_CMD_LEN);

        sem_wait(socket_sem);
        recv(sockfd, message_buf, MAX_CMD_STR_SIZE, 0);
        sem_post(socket_sem);

        memcpy(client_req_cpy, message_buf, strlen(message_buf));
        str_to_serv_cmd(cmd, client_req_cpy);

        if (strncmp(cmd->cmd, "put", strlen("put")) == 0)
        {
            recieve_chunk(file_cabinet, cmd, message_buf, sockfd, socket_sem);
        }
        else
        {
            break;
        }
    }
    free(message_buf);
    free(client_req_cpy);
}

int recieve_chunk(hash_table *file_cabinet, df_serv_cmd *cmd, char *client_req, int sockfd, sem_t *socket_sem)
{
    record_to_file_list(file_cabinet, cmd, sockfd, socket_sem);
    // printf("CHUNK %c of %s, by thread %ld\n", cmd->chunk, cmd->filename_hr, pthread_self());
    char *full_filepath = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    memcpy(full_filepath, file_cabinet->dfs_dir, strlen(file_cabinet->dfs_dir));
    memcpy(&full_filepath[strlen(full_filepath)], "/", strlen("/"));
    memcpy(&full_filepath[strlen(full_filepath)], cmd->chunk_name, strlen(cmd->chunk_name));

    struct file_cabinet_node *file_node = get_file_cabinet_node();
    populate_file_cabinet_node(file_node, cmd);

    add_file_cabinet_entry(file_cabinet, file_node);

    get_ll_wlock(file_cabinet, file_node);

    FILE *fp = fopen(full_filepath, "wb");
    fclose(fp);
    fp = fopen(full_filepath, "a");

    char *message_buf = calloc_wrap(MAX_MSG_SZ, sizeof(char));

    int chunk_size;
    str2int(&chunk_size, cmd->chunk_size);

    int bytes_remaining = chunk_size;
    int bytes_recvd = 1;
    int limit = chunk_size;
    int break_flag = 0;
    if (chunk_size > MAX_MSG_SZ)
    {
        limit = MAX_MSG_SZ;
    }

    while (bytes_recvd > 0)
    {
        if (bytes_remaining < limit)
        {
            limit = bytes_remaining;
            break_flag = 1;
        }
        memset(message_buf, '\0', MAX_MSG_SZ + 1);
        sem_wait(socket_sem);
        bytes_recvd = recv(sockfd, message_buf, limit, 0);
        bytes_remaining -= bytes_recvd;
        sem_post(socket_sem);

        fprintf(fp, message_buf);
        int filesize = get_file_size(fp);
        if (break_flag)
        {
            break;
        }
    }
    fclose(fp);
    release_ll_rwlock(file_cabinet, file_node);
    return 0;
}

int record_to_file_list(hash_table *file_cabinet, df_serv_cmd *cmd, int sockfd, sem_t *socket_sem)
{

    char *full_filepath = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    memcpy(full_filepath, file_cabinet->dfs_dir, strlen(file_cabinet->dfs_dir));
    memcpy(&full_filepath[strlen(full_filepath)], "/", strlen("/"));
    memcpy(&full_filepath[strlen(full_filepath)], "filelist", strlen("filelist"));

    pthread_rwlock_rdlock(file_cabinet->file_list_lock);
    FILE *filelist;
    filelist = fopen(full_filepath, "r");
    if (filelist)
    {
        char *list_entry = calloc_wrap(MAX_FILENAME_LEN, sizeof(char));
        size_t len = MAX_FILENAME_LEN;
        ssize_t read;

        while ((read = getline(&list_entry, &len, filelist)) != -1)
        {
            if (strncmp(list_entry, cmd->filename_hr, strlen(cmd->filename_hr)) == 0)
            {
                fclose(filelist);
                return 0;
            }
        }
        fclose(filelist);
    }

    filelist = fopen(full_filepath, "a");
    fprintf(filelist, "%s\n", cmd->filename_hr);
    fclose(filelist);
    pthread_rwlock_unlock(file_cabinet->file_list_lock);
    free(full_filepath);
    return 0;
}

int handle_get_dfs(hash_table *file_cabinet, df_serv_cmd *cmd, int sockfd, sem_t *socket_sem)
{
    struct file_cabinet_node *chunk;
    if (cmd->req_tmstmp == '0')
    {
        chunk = get_file_agnostic(file_cabinet, cmd->filename_md5);
        if (chunk == NULL)
        {
            fprintf(stderr, "notfound1\n");
            return 1;
        }
    }
    else
    {
        chunk = get_file_gnostic(file_cabinet, cmd);
        if (chunk == NULL)
        {
            sleep(0.01);
            chunk = get_file_agnostic(file_cabinet, cmd);
            if (chunk == NULL)
            {
                fprintf(stderr, "notfound2\n");
                return 1;
            }
        }
    }

    if(chunk->sibling == NULL)
    {
        fprintf(stderr, "notfound3\n");
        return 1;   
    }
    send_chunk(file_cabinet, cmd, chunk, sockfd, socket_sem);
    send_chunk(file_cabinet, cmd, chunk->sibling, sockfd, socket_sem);
    

    return 0;
}

int send_chunk(hash_table *file_cabinet, df_serv_cmd *cmd, struct file_cabinet_node *chunk, int sockfd, sem_t *socket_sem)
{
    get_ll_rdlock(file_cabinet, chunk);

    char data[KILO] = {0};
    char *header = calloc_wrap(MAX_CMD_STR_SIZE, sizeof(char));
    build_serv_cmd(header, cmd, chunk);
    sem_wait(socket_sem);
    send_wrap(sockfd, header, MAX_CMD_STR_SIZE, 0);
    sem_post(socket_sem);

    char *full_filepath = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    memcpy(full_filepath, file_cabinet->dfs_dir, strlen(file_cabinet->dfs_dir));
    memcpy(&full_filepath[strlen(full_filepath)], "/", strlen("/"));
    memcpy(&full_filepath[strlen(full_filepath)], cmd->chunk_name, strlen(cmd->chunk_name));

    FILE *fp = fopen(full_filepath, "r");
    if (fp == NULL)
    {
        release_ll_rwlock(file_cabinet, chunk);
        fprintf(stderr, "notfound4\n");
        return 1;
    }
    while (fread(data, sizeof(char), MAX_MSG_SZ, fp) != 0)
    {
        sem_wait(socket_sem);
        send_wrap(sockfd, data, MAX_MSG_SZ, 0);
        printf("%s\n", data);
        sem_post(socket_sem);
        bzero(data, KILO);
    }
    release_ll_rwlock(file_cabinet, chunk);

    return 0;
    
}

int handle_ls_dfs(hash_table *file_cabinet, int sockfd, sem_t *socket_sem)
{
    printf("testing ls\n");
    return 0;
}

void join_threads(int num_threads, pthread_t *thr_arr)
{
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(thr_arr[i], NULL);
    }
}
