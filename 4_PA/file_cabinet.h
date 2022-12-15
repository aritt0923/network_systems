#ifndef file_cabinet_H_
#define file_cabinet_H_

#include "client_serv_common.h"
#include "df_client/df_client.h"
#include "wrappers.h"

struct file_cabinet_node
{
    char * filename_hr;
    char * filename_md5;
    char * filename_tmstmp_md5;
    // assuming single digit number of servers (always 4 for this assignment)
    char chunk;
    // Name of the chunk as stored
    // i.e <filename_tmstmp_md5>_<chunk>
    char * chunk_name;

    char * offset;
    char * chunk_size;
    char * filesize;
    struct file_cabinet_node *next;
    struct file_cabinet_node *prev;
    struct file_cabinet_node *sibling;
};

typedef struct 
{
    struct file_cabinet_node *next;
    struct file_cabinet_node *tail;
    pthread_rwlock_t *drawer_lock;
} file_cabinet_ll_head;

typedef struct 
{
    int buckets;
    file_cabinet_ll_head **arr;
    pthread_rwlock_t *file_list_lock;
    char * dfs_dir;
} hash_table;


unsigned long file_cabinet_hash(char *md5_hash, int buckets);


/*
 * NOT THREAD SAFE
 */
int create_hash_table(hash_table *file_cabinet, int buckets);

/*
 * NOT THREAD SAFE
 */
int free_hash_table(hash_table *file_cabinet);


int init_file_cabinet_node(struct file_cabinet_node *file);

int populate_file_cabinet_node(struct file_cabinet_node *file, df_serv_cmd *cmd);


int free_file_cabinet_node(struct file_cabinet_node *file);

file_cabinet_ll_head *get_file_cabinet_ll_head();
struct file_cabinet_node * get_file_cabinet_node();



 
int init_file_cabinet_ll_head(file_cabinet_ll_head *head_node);

int destroy_file_cabinet_ll(file_cabinet_ll_head *head_node);

int add_file_cabinet_entry(hash_table *file_cabinet, struct file_cabinet_node *file);

int add_file_cabinet_entry_non_block(hash_table *file_cabinet, struct file_cabinet_node *file);


struct file_cabinet_node *get_file_agnostic(hash_table *file_cabinet, char * md5_hash);
int find_chunk_sibling(file_cabinet_ll_head *head_node, struct file_cabinet_node *file);


unsigned long str_to_bucket(char *md5_hash, int buckets);

int get_ll_rdlock(hash_table *file_cabinet, struct file_cabinet_node *file);

int get_ll_wlock(hash_table *file_cabinet, struct file_cabinet_node *file);

int release_ll_rwlock(hash_table *file_cabinet, struct file_cabinet_node *file);

unsigned long djb2(unsigned char *str);

#endif //file_cabinet_H_