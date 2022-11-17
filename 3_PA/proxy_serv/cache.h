#ifndef CACHE_H_
#define CACHE_H_

#include <proxy_serv_funs.h>

struct cache_node
{
    char * md5_hash;
    unsigned int filesize;
    char * filetype;
    struct cache_node *next;
};

typedef struct 
{
    struct cache_node *next;
    pthread_rwlock_t *rwlock;
} cache_ll_head;

typedef struct 
{
    int ttl_seconds;
    int buckets;
    cache_ll_head **arr;
} hash_table;


/*
 * NOT THREAD SAFE
 */
int create_hash_table(hash_table *cache, int buckets, int ttl_seconds);

/*
 * NOT THREAD SAFE
 */
int free_hash_table(hash_table *cache);

int free_cache_node(struct cache_node *file);

cache_ll_head *get_cache_ll_head();
struct cache_node * get_cache_node();

/*
 * struct cache_node struct members:
 *  char * url;
 *  char * md5_hash;
 *  unsigned int filesize;
 *  char * filetype;
 */
int init_cache_node(struct cache_node *file);

 
int init_cache_ll_head(cache_ll_head *head_node);

int destroy_cache_ll(cache_ll_head *head_node);

int add_cache_entry(hash_table *cache, struct cache_node *file);
int add_cache_entry_non_block(hash_table *cache, struct cache_node *file);


struct cache_node *get_cache_entry(hash_table *cache, char * md5_hash);


unsigned long cache_hash(char *md5_hash, int buckets);

int get_ll_rdlock(hash_table *cache, struct cache_node *file);

int get_ll_wlock(hash_table *cache, struct cache_node *file);

int release_ll_rwlock(hash_table *cache, struct cache_node *file);

unsigned long djb2(unsigned char *str);

#endif //CACHE_H_