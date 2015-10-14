/**
 * @author Xi Lin (xlin2)
 */

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

// states code
#define MISS 1
#define HIT 2
#define MISS_EVICT 3
#define MISS_HIT 4
#define MISS_EVICT_HIT 5

typedef unsigned int u_int;
typedef unsigned long u_long;

// stores parameters of the cache
typedef struct {
	int v;
	int s;
	int E;
	int b;
	int hits;
	int misses;
	int evicts;
} Param;

// one line in the cache
typedef struct {
	int valid;
	unsigned long tag;
	int accessed;
} Line;

// one set in the cache
typedef struct {
	Line *lines;
} Set;

// a cache with many sets
typedef struct {
	Set *sets;
} Cache;

/* 
 * print help information when users input -h or something wrong
 */
void printHelp() {
	printf("Usage: ./csim [-h] [-v] -s <s> -E <E> -b <b> -t <tracefile>\n");
	printf("• -h: Optional help flag that prints usage info\n");
	printf("• -v: Optional verbose flag that displays trace info\n");
	printf("• -s <s>: Number of set index bits (S = 2s is the number of sets)\n");
	printf("• -E <E>: Associativity (number of lines per set) \n");
	printf("• -b <b>: Number of block bits (B = 2b is the block size)\n");
	printf("• -t <tracefile>: Name of the valgrind trace to replay\n");
}

/* 
 * parse the parameters in the instruction and store them in Param structure
 */
void parseArg(int argc, char *argv[], Param *param, char *fileName) {
	int ret;
	int argNum = 0;
	param->v = 0;
	param->hits = 0;
	param->misses = 0;
	param->evicts = 0;
	while ((ret = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
		switch(ret) {
		case 'v':
			param->v = 1;
			break;
		case 's':
			param->s = atoi(optarg);
			++argNum;
			break;
		case 'E':
			param->E = atoi(optarg);
			++argNum;
			break;
		case 'b':
			param->b = atoi(optarg);
			++argNum;
			break;
		case 't':
			strncpy(fileName, optarg, strlen(optarg));
			++argNum;
			break;
		case 'h':
			printHelp();
			exit(0);
		default:
			printHelp();
			exit(1);
		}
	}
	if (argNum < 4) {
		printHelp();
		exit(1);
	}
	
}

/* 
 * initialize the values in the Cache structure 
 */
Cache createCache(Param param) {
	Cache cache;
	u_int setNum = 1 << param.s;
	u_int lineNum = param.E;
	
	cache.sets = (Set *)malloc(sizeof(Set) * setNum);
	for (u_int i = 0; i < setNum; ++i) {
		cache.sets[i].lines = (Line *)malloc(sizeof(Line) * lineNum);
		for (u_int j = 0; j < lineNum; ++j) {
			cache.sets[i].lines[j].valid = 0;
			cache.sets[i].lines[j].accessed = 0;
			cache.sets[i].lines[j].tag = 0;
		}
	}
	
	return cache;
}

// execute instructure and change the values of the cache
int executeInst(Cache *cache, char *inst, Param *param) {
	// instruction variable
	char opt;
	u_long addr;
	u_int bytes;
	
	// index
	u_long tag;
	u_long setIdx;
	u_int tagBits = 64 - param->s - param->b;
	u_int lineNum = param->E;
	u_int emptyIdx;
	u_int evictIdx = 0;
	
	// other variables
	Set curSet;
	int full = 1;
	int maxAccess;
	int minAccess;
	
	// get set index
	sscanf(inst, " %c %lx,%u", &opt, &addr, &bytes);
	tag = addr >> (param->s + param->b);
	setIdx = (addr << tagBits) >> (tagBits + param->b); 
	curSet = cache->sets[setIdx];
	
	// init eviction variables
	maxAccess = curSet.lines[0].accessed;
	minAccess = curSet.lines[0].accessed;
	
	// hit
	for (u_int i = 0; i < lineNum; ++i) {
		if (curSet.lines[i].valid == 1 && curSet.lines[i].tag == tag) {
			if (opt == 'M') {
				param->hits += 2;
			}
			else {
				++param->hits;
			}
			++cache->sets[setIdx].lines[i].accessed;
			return HIT;
		}
		else if (curSet.lines[i].valid == 0 && full) { // check if there is an empty line
			full = 0;
			emptyIdx = i;
		}
		
		// try to find eviction if no hit
		if (curSet.lines[i].accessed < minAccess) {
			minAccess = curSet.lines[i].accessed;
			evictIdx = i;
		}
		else if (curSet.lines[i].accessed > maxAccess){
			maxAccess = curSet.lines[i].accessed;
		}
	}
	
	// miss but no eviction
	++param->misses;
	if (!full) {
		cache->sets[setIdx].lines[emptyIdx].valid = 1;
		cache->sets[setIdx].lines[emptyIdx].tag = tag;
		cache->sets[setIdx].lines[emptyIdx].accessed = maxAccess + 1;
		if (opt == 'M') {
			++param->hits;
			return MISS_HIT;
		}
		else {
			return MISS;
		}
	}
	
	// miss and evict
	++param->evicts;
	cache->sets[setIdx].lines[evictIdx].tag = tag;
	cache->sets[setIdx].lines[evictIdx].accessed = maxAccess + 1;
	if (opt == 'M') {
		++param->hits;
		return MISS_EVICT_HIT;
	}
	else {
		return MISS_EVICT;
	}
}

int main(int argc, char *argv[]) {
	Param param;
	Cache cache;
	FILE *file;
	char fileName[100];
	char inst[100];
	int state;
	
	parseArg(argc, argv, &param, fileName);
	cache = createCache(param);
	
	// execute instructions one by one
	file = fopen(fileName, "r");
	while ((fgets(inst, 99, file)) != NULL) {
		if (inst[0] != ' ') continue;
		state = executeInst(&cache, inst, &param);
		if (param.v == 1) {
			switch(state) {
			case HIT:
				printf("%s hit\n", inst + 1);
				break;
			case MISS:
				printf("%s miss\n", inst + 1);
				break;
			case MISS_HIT:
				printf("%s miss hit\n", inst + 1);
				break;
			case MISS_EVICT:
				printf("%s miss eviction\n", inst + 1);
				break;
			case MISS_EVICT_HIT:
				printf("%s miss eviction hit\n", inst + 1);
				break;
			default:
				break;
			}
		}
	}
	fclose(file);
	
	printSummary(param.hits, param.misses, param.evicts);
	return 0;
}
