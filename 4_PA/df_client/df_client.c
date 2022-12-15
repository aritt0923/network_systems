#include "df_client.h"
#include "../utilities.h"
#include "../file_cabinet.h"

int main(int argc, char *argv[])
{
    df_serv_info *df_serv_arr = calloc_wrap(NUM_SERVERS, sizeof(df_serv_info));
    read_conf(df_serv_arr);

    df_serv_cmd **cmd_lst;

    int cmd;
    int num_cmds;
    if ((cmd = parse_args(argc, argv, &cmd_lst, &num_cmds)) < 0)
    {
        fprintf(stderr, "error parsing args\n");
        exit(EXIT_FAILURE);
    }

    switch (cmd)
    {

    case LS:
        printf("handle_ls\n");

        handle_ls_dfc(df_serv_arr, cmd_lst[0]);
        break;
    case GET:
        for (int i = 0; i < num_cmds; i++)
        {
            printf("handle_get\n");
            handle_get_dfc(df_serv_arr, cmd_lst[i]);
        }
        break;
    case PUT:
        for (int i = 0; i < num_cmds; i++)
        {
            handle_put_dfc(df_serv_arr, cmd_lst[i]);
        }
        break;

    default:
        fprintf(stderr, "invalid cmd\n");
        break;
    }

    return 0;
}

/*
 * Returns the number of available servers
 */
int get_num_servs(df_serv_info *df_serv_arr)
{
    int sum = 0;
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        int res;
        int serv_sockfd;
        if ((res = connect_remote(&df_serv_arr[i])) != 0)
        {
            df_serv_arr[i].available = 0;
        }
        else
        {
            df_serv_arr[i].available = 1;
        }
        sum += df_serv_arr[i].available;
    }
    return sum;
}

int handle_ls_dfc(df_serv_info *df_serv_arr, df_serv_cmd *cmd)
{
    char *cmd_str = calloc_wrap(MAX_CMD_STR_SIZE, sizeof(char));
    serv_cmd_to_str(cmd, cmd_str);
    int num_up = get_num_servs(df_serv_arr);
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        // connect_remote(&df_serv_arr[i]);
        if (df_serv_arr[i].available)
        {
            printf("available\n");
            send_wrap(df_serv_arr[i].sockfd, cmd_str, strnlen(cmd_str, MAX_CMD_STR_SIZE), 0);
        }
    }
    return 0;
}

int handle_get_dfc(df_serv_info *df_serv_arr, df_serv_cmd *cmd)
{

    char *cmd_str = calloc_wrap(MAX_CMD_STR_SIZE, sizeof(char));
    serv_cmd_to_str(cmd, cmd_str);

    return 0;
}

int get_chunk_from_serv(df_serv_info df_serv, df_serv_cmd *cmd)
{
    return 0;
}

int handle_put_dfc(df_serv_info *df_serv_arr, df_serv_cmd *cmd)
{
    md5_str(cmd->filename_hr, cmd->filename_md5);
    int bucket = file_cabinet_hash(cmd->filename_md5, NUM_SERVERS);

    char *timestamp_buf = calloc_wrap(24, sizeof(char));
    get_timestamp(timestamp_buf);

    char *tmp_str = calloc_wrap(MAX_FILENAME_LEN + 24, sizeof(char));
    memcpy(tmp_str, cmd->filename_hr, strlen(cmd->filename_hr));
    memcpy(&tmp_str[strlen(tmp_str)], "_", strlen("_"));
    memcpy(&tmp_str[strlen(tmp_str)], timestamp_buf, strlen(timestamp_buf));
    md5_str(tmp_str, cmd->filename_tmstmp_md5);

    //free(timestamp_buf);

    FILE *fp = NULL;
    fp = fopen(cmd->filename_hr, "rb");
    
    if (fp == NULL)
    {
        fprintf(stderr, "could not open file\n");
        return 1;
    }
    
    
    int distro[8];
    get_distro(bucket, distro);
    
    int distro_idx = 0;
    for(int i = 0; i < NUM_SERVERS; i++)
    {
        send_chunk_to_serv(df_serv_arr[i], cmd, fp, distro[distro_idx]);
        distro_idx++;
        send_chunk_to_serv(df_serv_arr[i], cmd, fp, distro[distro_idx]);
        distro_idx++;
    }
    
    return 0;
}

int send_chunk_to_serv(df_serv_info df_serv, df_serv_cmd *cmd, FILE *fp, int chunk)
{
    printf("chunk %d, server %s\n", chunk, df_serv.port_no);
    int filesize = get_file_size(fp);
    int chunk_size;
    if(chunk == 4)
    {
        chunk_size = filesize/4 + filesize%4;
    }
    else
    {
        chunk_size = filesize/4;
    }
    printf("chunk_size: %d\n", chunk_size);
    
    unsigned int offset = (filesize/4)*(chunk-1);
    char chunk_str[1];
    
    sprintf(chunk_str, "%d", chunk);
    sprintf(cmd->filesize, "%d", filesize);
    sprintf(cmd->chunk_size, "%d", chunk_size);
    sprintf(cmd->offset, "%d", offset);
    
    memset(cmd->chunk_name, '\0', EVP_MAX_MD_SIZE);
    
    memcpy(cmd->chunk_name, cmd->filename_tmstmp_md5, strlen(cmd->filename_tmstmp_md5));
    memcpy(&cmd->chunk_name[strlen(cmd->chunk_name)], "_", 1);
    memcpy(&cmd->chunk_name[strlen(cmd->chunk_name)], chunk_str, 1);
    
    cmd->chunk = chunk_str[0];
    
    char * cmd_str = calloc_wrap(MAX_CMD_STR_SIZE, sizeof(char));
    serv_cmd_to_str(cmd, cmd_str);
    
    printf("CMD_STR:\n%s\n", cmd_str);
    
    fseek(fp, offset, 0);

    int limit = chunk_size;
    if(chunk_size > KILO)
    {
        limit = KILO;
    }
    char *data = calloc_wrap(KILO+1, sizeof(char));
    
    int num_read = 0;
    int total_read = 0;
    int left_to_read = chunk_size;
    int break_flag = 0;
    
    
    while (1)
    {
        if(left_to_read < limit)
        {
            limit = left_to_read;
            break_flag = 1;
        }
        
        num_read = fread(data, sizeof(char), limit, fp);
        left_to_read -= num_read;
        //send_wrap(sockfd, data, sizeof(data), 0);
        //bzero(&data[limit], KILO);
        printf("%s", data);
        memset(data, '\0', KILO+1);
        if(break_flag)
        {
            break;
        }
    }
    
    printf("\n\n");
    return 0;
}

int connect_remote(df_serv_info *params)
{

    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status;
    if ((status = getaddrinfo(params->ip_addr, params->port_no, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return NOTFOUND;
    }

    // https://beej.us/guide/bgnet/examples/client.c
    char s[INET6_ADDRSTRLEN];

    struct addrinfo *p;
    // loop through all the results and connect to the first we can

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((params->sockfd = socket(p->ai_family, p->ai_socktype,
                                     p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if (setsockopt(params->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                       sizeof(timeout)) < 0)
            perror("setsockopt failed\n");

        if (setsockopt(params->sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                       sizeof(timeout)) < 0)
            perror("setsockopt failed\n");

        if (connect(params->sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(params->sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return NOTFOUND;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo);
    return 0;
}

/*
 * Reads ~/dfc.conf, stores server information
 */
int read_conf(df_serv_info *df_serv_arr)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL)
    {
        printf("no env var set\n");
        homedir = getpwuid(getuid())->pw_dir;
    }
    char *conf_full_path = calloc_wrap(256, sizeof(char));
    memcpy(conf_full_path, homedir, strlen(homedir));

    memcpy(&conf_full_path[strlen(conf_full_path)], "/dfc.conf", strlen("/dfc.conf"));

    fp = fopen(conf_full_path, "r");
    if (fp == NULL)
    {
        printf("could not open file\n");
        free(conf_full_path);
        return 1;
    }
    char *dfsn_ip_port = NULL;
    char *ip_port = NULL;

    int serv_num = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (serv_num > NUM_SERVERS - 1)
        {
            break;
        }
        df_serv_info *server = &df_serv_arr[serv_num];

        dfsn_ip_port = &line[7];
        for (unsigned int i = 0; i < strlen(dfsn_ip_port); i++)
        {
            if (dfsn_ip_port[i] == ' ')
            {
                ip_port = &dfsn_ip_port[i + 1];
                break;
            }
        }
        if (ip_port == NULL)
        {
            printf("no whitespace found in ip_port\n");
        }
        int col_idx = -1;
        for (unsigned int i = 0; i < strlen(ip_port); i++)
        {
            if (ip_port[i] == ':')
            {
                col_idx = i;
                break;
            }
        }
        memcpy(server->ip_addr, ip_port, col_idx);
        memcpy(server->port_no, &ip_port[col_idx + 1], MAX_PORT_LEN);

        serv_num++;
    }

    fclose(fp);
    if (line)
        free(line);
    free(conf_full_path);
    return 0;
}

int parse_args(int argc, char *argv[], df_serv_cmd ***serv_cmd_lst, int *num_cmds)
{
    if (argc < 2)
    {
        fprintf(stderr, "wrong usage\n");
        return -1;
    }
    df_serv_cmd **ptr_lst_tmp;
    if (strncmp(argv[1], "list", strlen("list")) == 0)
    {
        *num_cmds = 1;
        ptr_lst_tmp = calloc_wrap(*num_cmds, sizeof(df_serv_cmd *));
        df_serv_cmd *cmd = get_serv_cmd_ptr(1);

        memcpy(cmd->cmd, "list", strlen("list"));
        ptr_lst_tmp[0] = cmd;
        *serv_cmd_lst = ptr_lst_tmp;
        return LS;
    }
    else if ((strncmp(argv[1], "get", strlen("get")) == 0) || (strncmp(argv[1], "put", strlen("put")) == 0))
    {
        *num_cmds = argc - 2;
        ptr_lst_tmp = calloc_wrap(*num_cmds, sizeof(df_serv_cmd *));

        df_serv_cmd *cmd_lst_tmp = get_serv_cmd_ptr(*num_cmds);
        for (int i = 0; i < *num_cmds; i++)
        {
            ptr_lst_tmp[i] = &cmd_lst_tmp[i];
            memcpy(cmd_lst_tmp[i].cmd, argv[1], strlen(argv[1]));
            memcpy(cmd_lst_tmp[i].filename_hr, argv[i + 2], strlen(argv[i + 2]));
        }
        *serv_cmd_lst = ptr_lst_tmp;

        if ((strncmp(argv[1], "get", strlen("get")) == 0))
        {
            return GET;
        }
        return PUT;
    }
    fprintf(stderr, "invalid command\n");
    return -1;
}

int get_distro(int bucket, int *arr)
{
    int distro_0[8] = {1, 2, 2, 3, 3, 4, 4, 1};
    int distro_1[8] = {4, 1, 1, 2, 2, 3, 3, 4};
    int distro_2[8] = {3, 4, 4, 1, 1, 2, 2, 3};
    int distro_3[8] = {2, 3, 3, 4, 4, 1, 1, 2};
    
    
    
    
    switch (bucket)
    {
    case 0:
        for (int i = 0; i < 8; i++)
        {
            arr[i] = distro_0[i];
        }
        return 0;

    case 1:
        for (int i = 0; i < 8; i++)
        {
            arr[i] = distro_1[i];
        }
        return 0;
        

    case 2:
        for (int i = 0; i < 8; i++)
        {
            arr[i] = distro_2[i];
        }
        return 0;
        

    case 3:
        for (int i = 0; i < 8; i++)
        {
            arr[i] = distro_3[i];
        }
        return 0;
        
    default:
        fprintf(stderr, "error getting buckets\n");
        return -1;
    }
}
