#ifndef CLIENT_H_
#define CLIENT_H_

#include "../client_serv_common.h"


typedef struct // server ip and port
{
    char ip_addr[MAX_IP_LEN];
    char port_no[MAX_PORT_LEN];
    int sockfd;
    int available;
} df_serv_info;

/*
 * Returns the number of available servers 
 */
int get_num_servs(df_serv_info *df_serv_arr);
/*
 * Reads ~/dfc.conf, stores server information 
 */
int read_conf(df_serv_info *df_serv_arr);

/**
 * Takes in cmd line args. 
 *      Populates: 
 *          serv_cmd->cmd |    
 *          serv_cmd->filename_hr |
 *          serv_cmd->filename_md5 |
 *          serv_cmd->filename_tmstmp_md |
 *          serv_cmd->filesize 
 *         
 * @param argc 
 * @param argv
 * @param serv_cmd pointer to df_serv_cmd struct to be populated
 */
int parse_args(int argc, char *argv[], df_serv_cmd ***serv_cmd_lst, int *num_cmds);



/**
 * Returns list of files from the first available server
 * if less than 3 servers are available,
 * append _incomplete to each file
 * @param df_serv_arr array of df_serv_info structs
 * @param serv_cmd df_serv_cmd
 */
int handle_ls_dfc(df_serv_info *df_serv_arr, df_serv_cmd *cmd);


/**
 * Requires that either servers 1 && 3 or 2 && 4 be available
 * Queries first server for any version of requested file
 * Then the second server, requiring the version returned
 * by the first for the remaining chunks
 * @param df_serv_arr array of df_serv_info structs
 */
int handle_get_dfc(df_serv_info *df_serv_arr, df_serv_cmd *cmd);

/**
 * Comminicates with the passed server
 * recieves file chunk and writes to file
 * @param df_serv_arr array of df_serv_info structs
 */
int get_chunk_from_serv(df_serv_info df_serv, df_serv_cmd *cmd);

/**
 * Requires that all four servers be up
 * Sends file chunks according to hash of filename_hr
 * @param df_serv_arr array of df_serv_info structs
 */
int handle_put_dfc(df_serv_info *df_serv_arr, df_serv_cmd *cmd);

/**
 * \brief Sends file chunk to server
 * @param df_serv_arr array of df_serv_info structs
 */
int send_chunk_to_serv(df_serv_info df_serv, df_serv_cmd *cmd, FILE *fp, int chunk);

int connect_remote(df_serv_info *params);

int get_distro(int bucket, int* arr);


#endif //CLIENT_H_