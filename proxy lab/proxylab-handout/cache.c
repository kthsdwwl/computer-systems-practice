/*
 * cache.c
 * Xi Lin(xlin2)
 *
 * Implementations of cache module functions
 */

#include "cache.h"
#include "csapp.h"

CacheLine *c_head = NULL;
CacheLine *c_tail = NULL;
unsigned remain_size = MAX_CACHE_SIZE;
pthread_rwlock_t read_update_lock;
pthread_rwlock_t read_insert_lock;

/* 
 * init_cache - initialize cache data, set head and tail pointers to be
 * NULL, and set the available size to be MAX_CACHE_SIZE
 */
void init_cache() {
	c_head = NULL;
	c_tail = NULL;
	remain_size = MAX_CACHE_SIZE;
}

/* 
 * get_object - search for a specific object from cache accroding to the
 * given uri string
 */
CacheLine *get_object(char *uri) {
	CacheLine *cursor = c_head;
	int found = 0;
	
	// get read lock so that when traversing the cache, no other thread
	// is able to change the position a cache line or add a new line 
	pthread_rwlock_rdlock(&read_insert_lock);
	pthread_rwlock_rdlock(&read_update_lock);
	
	while (cursor != NULL) {
		if (strcmp(cursor->tag, uri) == 0) {
			found = 1;
			break;
		}
		cursor = cursor->next;
	}
	pthread_rwlock_unlock(&read_update_lock);
	
	// if not found, release the lock and return
	if (found == 0) {
		pthread_rwlock_unlock(&read_insert_lock);
		return NULL;
	}
	
	// if found, move the cache line to the head of cache list
	pthread_rwlock_wrlock(&read_update_lock);
	remove_cache_line(cursor);
	insert_cache_line(cursor);
	pthread_rwlock_unlock(&read_update_lock);
	
	pthread_rwlock_unlock(&read_insert_lock);
	
	return cursor;
}

/* 
 * add_object - add a new cache line to the cache
 */
void add_object(char *uri, char *object, int length)
{
	pthread_rwlock_wrlock(&read_insert_lock);
	
	CacheLine *new_line = Malloc(sizeof(CacheLine));
	
	// set each field of new cache line
	new_line->prev = NULL;
	new_line->next = NULL;
	new_line->length = length;
	new_line->tag = Malloc(MAXLINE);
	new_line->object = Malloc(MAX_OBJECT_SIZE);
	strcpy(new_line->tag, uri);
	strncpy(new_line->object, object, length);
	
	// if remaining size is not enough, evict cache lines that have not
	// been accessed for a long time
	if (remain_size < length) {
		evict_cache_line(length);
	}
	remain_size -= length;
	insert_cache_line(new_line);
	
	pthread_rwlock_unlock(&read_insert_lock);
}

/* 
 * insert_cache_line - insert a cache line to the head of the list
 */
void insert_cache_line(CacheLine *target) {
	if (c_head == NULL) {
		c_head = target;
		c_tail = target;
	}
	else {
		target->next = c_head;
		c_head->prev = target;
		c_head = target;
	}
}

/* 
 * evict_cache_line - remove some cache lines that haven't been accessed 
 * for a long time, so that the remaining size is enough to hold a new
 * line.
 */
void evict_cache_line(int size)
{
	CacheLine *cursor = c_tail;
	unsigned temp_size = remain_size;
	// search for evict lines from the tail of the cache
	while (cursor != NULL && temp_size < size) {
		temp_size += cursor->length;
		remove_cache_line(cursor);
		free_cache_line(cursor);
		cursor = c_tail;
	}
	remain_size = temp_size;
}

/* 
 * remove_cache_line - remove a cache line from the cache list
 */
void remove_cache_line(CacheLine *target)
{
	if (c_head == target) {
		c_head = target->next;
	}
	if (c_tail == target) {
		c_tail = target->prev;
	}
	if (target->prev) {
		target->prev->next = target->next;
	}
	if (target->next) {
		target->next->prev = target->prev;
	}
}

/* 
 * free_cache - free the whole cache line by line
 */
void free_cache() 
{
	CacheLine *cursor = c_head;
	while (cursor != NULL) {
		c_head = c_head->next;
		free_cache_line(cursor);
		cursor = c_head;
	}
	c_tail = NULL;
}

/* 
 * free_cache_line - free a specific cache line from the list
 */
void free_cache_line(CacheLine *target)
{
	Free(target->tag);
	Free(target->object);
	Free(target);
}

/* 
 * traverse_cache - for debug use. Traverse the whole cache and output
 * each line
 */
void traverse_cache()
{
	CacheLine *cursor = c_head;
	
	while (cursor != NULL) {
		printf("tag: %s, element: %s\n", cursor->tag, cursor->object);
		cursor = cursor->next;
	}
	printf("remain size: %d\n", remain_size);
}
