/*
 * cache.h
 * Xi Lin(xlin2)
 *
 * Header file for cache module
 */

#ifndef CACHE_H
#define CACHE_H

#include <string.h>

#define MAX_CACHE_SIZE  1049000
#define MAX_OBJECT_SIZE 102400

/*
 * structure of each cache line
 */
typedef struct line {
	struct line* prev;
	struct line* next;
	char *tag;        /* used for indexing a specific cache line */
	char *object;     /* stores the data */
	int length;       /* length of the data stored in the cache line */
	
} CacheLine; 

void init_cache();

CacheLine *get_object(char *uri);

void add_object(char *uri, char *object, int length);

/* Helper functions */
void insert_cache_line(CacheLine *target);

void evict_cache_line(int size);

void remove_cache_line(CacheLine *target);

void free_cache();

void free_cache_line(CacheLine *target);

void traverse_cache();

#endif
