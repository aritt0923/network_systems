#include <proxy_serv_funs.h>
#include <wrappers.h>

/*
 * NOT THREAD SAFE
 */
int create_hash_table(hash_table *cache, int buckets, int ttl_seconds)
{
    cache->arr = calloc_wrap(buckets, sizeof(cache_ll_head *));
    for (int i = 0; i < buckets; i++)
    {
        cache->arr[i] = get_cache_ll_head();
    }
    cache->ttl_seconds = ttl_seconds;
    cache->buckets = buckets;
    return 0;
}

/*
 * NOT THREAD SAFE
 */
int free_hash_table(hash_table *cache)
{
    for (int i = 0; i < cache->buckets; i++)
    {
        destroy_cache_ll(cache->arr[i]);
    }
    free(cache->arr);
    return 0;
}

int free_cache_node(struct cache_node *file)
{
    free(file->filetype);
    free(file->md5_hash);
    free(file);
    return 0;
    
}

int free_cache_ll_head(cache_ll_head *head_node)
{
    pthread_rwlock_destroy(head_node->rwlock);
    free(head_node->rwlock);
    free(head_node);
    return 0;
}

cache_ll_head *get_cache_ll_head()
{
    cache_ll_head *new_ll_head = calloc_wrap(1, sizeof(cache_ll_head));
    init_cache_ll_head(new_ll_head);
    return new_ll_head;
}
struct cache_node *get_cache_node()
{
    struct cache_node *node = calloc_wrap(1, sizeof(struct cache_node));
    init_cache_node(node);
    return node;
}

/*
 * struct cache_node struct members:
 *  char * url;
 *  char * md5_hash;
 *  unsigned int filesize;
 *  char * filetype;
 */
int init_cache_node(struct cache_node *file)
{
    file->md5_hash = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    file->filesize = 0;
    file->filetype = calloc_wrap(MAX_FILETYPE_SIZE, sizeof(char));
    file->next = NULL;
    return 0;
    
}

int init_cache_ll_head(cache_ll_head *head_node)
{
    head_node->next = NULL;
    
    head_node->rwlock = calloc_wrap(1, sizeof(pthread_rwlock_t));
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(head_node->rwlock, NULL);
    pthread_rwlockattr_destroy(&attr);

    return 0;
}

int destroy_cache_ll(cache_ll_head *head_node)
{
    struct cache_node *curr_node = head_node->next;
    struct cache_node *next_node = NULL;
    if (curr_node == NULL)
    {
        int res;
        if ((res = pthread_rwlock_destroy(head_node->rwlock)) != 0)
        {
            fprintf(stderr, "Error destroying rwlock in destroy_cache_ll()\n");
        }
        return res;
    }

    while (curr_node->next)
    {
        next_node = curr_node->next;
        free_cache_node(curr_node);
        curr_node = next_node;
    }
    free_cache_node(curr_node);
    free_cache_ll_head(head_node);
    return 0;
}

int add_cache_entry(hash_table *cache, struct cache_node *file)
{
    cache_ll_head *head_node;
    head_node = cache->arr[cache_hash(file->md5_hash, cache->buckets)];
    pthread_rwlock_wrlock(head_node->rwlock);

    struct cache_node *curr_node = head_node->next;
    if (curr_node == NULL)
    {
        head_node->next = file;
        pthread_rwlock_unlock(head_node->rwlock); 
        return 0;
    }
    while (curr_node->next)
    {
        if (strncmp(file->md5_hash, curr_node->md5_hash, EVP_MAX_MD_SIZE) == 0)
        {
            pthread_rwlock_unlock(head_node->rwlock); 
            return 0;
        }
        curr_node = curr_node->next;
    }
    if (strncmp(file->md5_hash, curr_node->md5_hash, EVP_MAX_MD_SIZE) == 0)
    {
        pthread_rwlock_unlock(head_node->rwlock);   
        return 0;
    }
    curr_node->next = file;
    pthread_rwlock_unlock(head_node->rwlock);
    return 0;
}

struct cache_node *get_cache_entry(hash_table *cache, char *md5_hash)
{
    int bucket;
    bucket = cache_hash(md5_hash, cache->buckets);
    cache_ll_head *head_node = cache->arr[bucket];
    pthread_rwlock_rdlock(head_node->rwlock);
    struct cache_node *curr_node = head_node->next;
    
    if (curr_node == NULL)
    {
        pthread_rwlock_unlock(head_node->rwlock);
        return NULL;
    }

    while (curr_node)
    {
        if (strncmp(md5_hash, curr_node->md5_hash, EVP_MAX_MD_SIZE) == 0)
        {
            break;
        }
        curr_node = curr_node->next;
    }
    pthread_rwlock_unlock(head_node->rwlock);
    return curr_node;
}

unsigned long cache_hash(char *md5_hash, int buckets)
{
    return (djb2((unsigned char *)md5_hash) % buckets);
}

int get_ll_rdlock(hash_table *cache, struct cache_node *file)
{
    cache_ll_head *head_node = cache->arr[cache_hash(file->md5_hash, cache->buckets)];
    pthread_rwlock_rdlock(head_node->rwlock);
    return 0;
}

int get_ll_wlock(hash_table *cache, struct cache_node *file)
{
    cache_ll_head *head_node = cache->arr[cache_hash(file->md5_hash, cache->buckets)];
    pthread_rwlock_wrlock(head_node->rwlock);
    return 0;
    
}

int release_ll_rwlock(hash_table *cache, struct cache_node *file)
{
    cache_ll_head *head_node = cache->arr[cache_hash(file->md5_hash, cache->buckets)];
    pthread_rwlock_unlock(head_node->rwlock);
    return 0;
    
}

//djb2
unsigned long djb2(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}