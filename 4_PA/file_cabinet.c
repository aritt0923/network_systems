
#include "file_cabinet.h"
#include "utilities.h"
/*
 * NOT THREAD SAFE
 */
int create_hash_table(hash_table *file_cabinet, int buckets)
{
    file_cabinet->arr = calloc_wrap(buckets, sizeof(file_cabinet_ll_head *));
    for (int i = 0; i < buckets; i++)
    {
        file_cabinet->arr[i] = get_file_cabinet_ll_head();
    }
    
    file_cabinet->file_list_lock = calloc_wrap(1, sizeof(pthread_rwlock_t));
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(file_cabinet->file_list_lock, &attr);
    pthread_rwlockattr_destroy(&attr);
    
    
    file_cabinet->buckets = buckets;
    file_cabinet->dfs_dir = calloc_wrap(12, sizeof(char));
    return 0;
}

int init_file_cabinet_node(struct file_cabinet_node *file)
{
    file->filename_hr = calloc_wrap(MAX_FILENAME_LEN, sizeof(char));
    file->filename_md5 = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    file->filename_tmstmp_md5 = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    file->chunk_name = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    file->offset = calloc_wrap(MAX_FILESIZE_STR, sizeof(char));
    file->chunk_size = calloc_wrap(MAX_FILESIZE_STR, sizeof(char));
    file->filesize = calloc_wrap(MAX_FILESIZE_STR, sizeof(char));
    file->next = NULL;
    file->prev = NULL;
    file->sibling=NULL;
    return 0;
}

int populate_file_cabinet_node(struct file_cabinet_node *file, df_serv_cmd *cmd)
{
    memcpy(file->filename_hr, cmd->filename_hr, strlen(cmd->filename_hr));
    memcpy(file->filename_md5, cmd->filename_md5, strlen(cmd->filename_md5));
    memcpy(file->filename_tmstmp_md5, cmd->filename_md5, strlen(cmd->filename_tmstmp_md5));
    memcpy(file->chunk_name, cmd->chunk_name, strlen(cmd->chunk_name));
     
    memcpy(file->offset, cmd->offset, strlen(cmd->offset));
    memcpy(file->chunk_size, cmd->chunk_size, strlen(cmd->chunk_size));
    memcpy(file->filesize, cmd->filesize, strlen(cmd->filesize));

    file->chunk = cmd->chunk;
}

int free_file_cabinet_node(struct file_cabinet_node *file)
{
    free(file->filename_hr);
    free(file->filename_md5);
    free(file->filename_tmstmp_md5);
    free(file->chunk_name);
    free(file->offset);
    free(file->chunk_size);
    free(file->filesize);
    free(file);
    return 0;
    
}

int free_file_cabinet_ll_head(file_cabinet_ll_head *head_node)
{
    pthread_rwlock_destroy(head_node->drawer_lock);
    free(head_node->drawer_lock);
    free(head_node);
    return 0;
}

file_cabinet_ll_head *get_file_cabinet_ll_head()
{
    file_cabinet_ll_head *new_ll_head = calloc_wrap(1, sizeof(file_cabinet_ll_head));
    init_file_cabinet_ll_head(new_ll_head);
    return new_ll_head;
}
struct file_cabinet_node *get_file_cabinet_node()
{
    struct file_cabinet_node *node = calloc_wrap(1, sizeof(struct file_cabinet_node));
    init_file_cabinet_node(node);
    return node;
}


int init_file_cabinet_ll_head(file_cabinet_ll_head *head_node)
{
    head_node->next = NULL;
    head_node->tail = NULL;
    head_node->drawer_lock = calloc_wrap(1, sizeof(pthread_rwlock_t));
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(head_node->drawer_lock, &attr);
    pthread_rwlockattr_destroy(&attr);

    return 0;
}

int destroy_file_cabinet_ll(file_cabinet_ll_head *head_node)
{
    struct file_cabinet_node *curr_node = head_node->next;
    struct file_cabinet_node *next_node = NULL;
    if (curr_node == NULL)
    {
        free_file_cabinet_ll_head(head_node);
        return 0;
    }

    while (curr_node->next)
    {
        next_node = curr_node->next;
        free_file_cabinet_node(curr_node);
        curr_node = next_node;
    }
    free_file_cabinet_node(curr_node);
    free_file_cabinet_ll_head(head_node);
    return 0;
}

int add_file_cabinet_entry(hash_table *file_cabinet, struct file_cabinet_node *file)
{
    file_cabinet_ll_head *head_node;
    int bucket = file_cabinet_hash(file->filename_md5, file_cabinet->buckets);
    head_node = file_cabinet->arr[bucket];
    pthread_rwlock_wrlock(head_node->drawer_lock);

    struct file_cabinet_node *curr_node = head_node->tail;
    if (curr_node == NULL)
    { // list was empty
        head_node->tail = file;
        pthread_rwlock_unlock(head_node->drawer_lock); 
        return 0;
    }
    
    // file becomes new tail
    curr_node->next = file;
    file->prev = curr_node;
    head_node->tail = file;
    
    pthread_rwlock_unlock(head_node->drawer_lock);
    return 0;
}

/*
int add_file_cabinet_entry_non_block(hash_table *file_cabinet, struct file_cabinet_node *file)
{
    file_cabinet_ll_head *head_node;
    head_node = file_cabinet->arr[file_cabinet_hash(file->filename_md5, file_cabinet->buckets)];

    struct file_cabinet_node *curr_node = head_node->next;
    if (curr_node == NULL)
    {
        head_node->next = file;
        return 0;
    }
    while (curr_node->next)
    {
        if (strncmp(file->filename_md5, curr_node->filename_md5, EVP_MAX_MD_SIZE) == 0)
        {
            return 0;
        }
        curr_node = curr_node->next;
    }
    if (strncmp(file->filename_md5, curr_node->filename_md5, EVP_MAX_MD_SIZE) == 0)
    {
        return 0;
    }
    curr_node->next = file;
    return 0;
}
*/
 
struct file_cabinet_node *get_file_agnostic(hash_table *file_cabinet, char *md5_hash)
{
    int bucket;
    bucket = file_cabinet_hash(md5_hash, file_cabinet->buckets);
    
    file_cabinet_ll_head *head_node = file_cabinet->arr[bucket];
    pthread_rwlock_rdlock(head_node->drawer_lock);
    struct file_cabinet_node *curr_node = head_node->tail;
    
    if (curr_node == NULL)
    {
        pthread_rwlock_unlock(head_node->drawer_lock);
        printf("no tail\n");
        return NULL;
    }

    while (curr_node)
    {
        if (strncmp(md5_hash, curr_node->filename_md5, EVP_MAX_MD_SIZE) == 0)
        {
            break;
        }
        curr_node = curr_node->prev;
    }
    
    if(curr_node == NULL)
    {
        return NULL;
    }
    find_chunk_sibling(head_node, curr_node);
    pthread_rwlock_unlock(head_node->drawer_lock);
    return curr_node;
}

struct file_cabinet_node *get_file_gnostic(hash_table *file_cabinet, df_serv_cmd * cmd)
{
    int bucket;
    bucket = file_cabinet_hash(cmd->filename_md5, file_cabinet->buckets);
    file_cabinet_ll_head *head_node = file_cabinet->arr[bucket];
    pthread_rwlock_rdlock(head_node->drawer_lock);
    struct file_cabinet_node *curr_node = head_node->tail;
    
    if (curr_node == NULL)
    {
        pthread_rwlock_unlock(head_node->drawer_lock);
        return NULL;
    }

    while (curr_node)
    {
        if ((strncmp(cmd->filename_tmstmp_md5, curr_node->filename_tmstmp_md5, EVP_MAX_MD_SIZE) )== 0)
        {
            break;
        }
        curr_node = curr_node->prev;
    }
    
    if(curr_node == NULL)
    {
        return NULL;
    }
    find_chunk_sibling(head_node, curr_node);
    pthread_rwlock_unlock(head_node->drawer_lock);
    return curr_node;
}

int find_chunk_sibling(file_cabinet_ll_head *head_node, struct file_cabinet_node *file)
{
    struct file_cabinet_node *curr_node = head_node->tail;
    
    if(curr_node == NULL)
    {
        printf("should never happen\n");
        return -1;
    }
    while(curr_node)
    {
        if((strncmp(curr_node->filename_tmstmp_md5, file->filename_tmstmp_md5, strlen(file->filename_tmstmp_md5))) == 0)
        {
            if (curr_node->chunk != file->chunk)
            {
                file->sibling = curr_node;
                return 0;
            }
        }
        curr_node = curr_node->prev;
    }
    return 1;
}




unsigned long file_cabinet_hash(char *md5_hash, int buckets)
{
    int bucket = djb2((unsigned char *)md5_hash) % buckets;
    return bucket;
}

int get_ll_rdlock(hash_table *file_cabinet, struct file_cabinet_node *file)
{
    file_cabinet_ll_head *head_node = file_cabinet->arr[file_cabinet_hash(file->filename_md5, file_cabinet->buckets)];
    pthread_rwlock_rdlock(head_node->drawer_lock);
    return 0;
}

int get_ll_wlock(hash_table *file_cabinet, struct file_cabinet_node *file)
{
    file_cabinet_ll_head *head_node = file_cabinet->arr[file_cabinet_hash(file->filename_md5, file_cabinet->buckets)];
    pthread_rwlock_wrlock(head_node->drawer_lock);
    return 0;
    
}

int release_ll_rwlock(hash_table *file_cabinet, struct file_cabinet_node *file)
{
    file_cabinet_ll_head *head_node = file_cabinet->arr[file_cabinet_hash(file->filename_md5, file_cabinet->buckets)];
    pthread_rwlock_unlock(head_node->drawer_lock);
    return 0;
    
}

//djb2
unsigned long djb2(unsigned char *str)
{
    //printf("djb2\n");
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

/*
 * NOT THREAD SAFE
 */
int free_hash_table(hash_table *file_cabinet)
{
    for (int i = 0; i < file_cabinet->buckets; i++)
    {
        destroy_file_cabinet_ll(file_cabinet->arr[i]);
    }
    free(file_cabinet->arr);
    free(file_cabinet);
    return 0;
}