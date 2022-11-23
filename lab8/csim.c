//Zhang Yuechen 520021910266

#include "cachelab.h"
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

//cache_line
typedef struct {
	int valid_bit;
	int tag;
	int counter;
}block;

int hit_count = 0, miss_count = 0, eviction_count = 0;
//construct a cache
block** cache;
int s, E, b;
char file[100];

void PrintHelp()
{
	printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}

void update(unsigned int address) {
	unsigned set = (address >> b) & ((-1U) >> (64 - s));
	unsigned tag = address >> (s + b);

	//hit
	for (int i = 0; i < E; i++) {
		if (cache[set][i].tag == tag) {
			cache[set][i].counter = 0;
			hit_count++;
			return;
		}
	}

	//miss, but empty block exists
	for (int i = 0; i < E; i++) {
		if (cache[set][i].valid_bit == 0) {
			//set the empty block
			cache[set][i].valid_bit = 1;
			cache[set][i].tag = tag;
			cache[set][i].counter = 0;
			miss_count++;
			return;
		}
	}

	//miss, and no empty block exists
	int index, max_counter = -1;
	//find the block to kick
	for (int i = 0; i < E; i++) {
		if (cache[set][i].counter > max_counter) {
			index = i;
			max_counter = cache[set][i].counter;
		}
	}
	//reset the block
	cache[set][index].tag = tag;
	cache[set][index].counter = 0;
	miss_count++;
	eviction_count++;
}

int main(int argc, char* argv[])
{
    int op;
	//parsing elements on the unix command line
	while ((op = getopt(argc, argv, "hvs:E:b:t:"))!= -1)
	{
		//optarg points to the value of the option argument
		switch (op)
		{
		case 'h': case 'v':
			PrintHelp();
			break;
		case 's':
			s = atoi(optarg);
			break;
		case 'E':
			E = atoi(optarg);
			break;
		case 'b':
			b = atoi(optarg);
			break;
		case 't':
			strcpy(file, optarg);
			break;
		default:
			printf("wrong argument\n"); 
			break;
		}
	}

	//S =  2^s
	int S = 1 << s;

	//init the cache
	cache = (block**)malloc(sizeof(block*) * S);
	for (int i = 0; i < S; i++)
		cache[i] = (block*)malloc(sizeof(block) * E);
	for (int i = 0; i < S; i++) {
		for (int j = 0; j < E; j++) {
			cache[i][j].valid_bit = 0;
			cache[i][j].tag = -1;
			cache[i][j].counter = -1;	
		}
	}

	FILE* pFile;
	pFile = fopen(file, "r");
	char identifier;
	unsigned int address;
	int	size;
	//parse the command
	while (fscanf(pFile, " %c %x, %d", &identifier, &address, &size) > 0)
	{
		switch (identifier) {
		case 'L':
			update(address);
			break;
		case 'M':
			update(address); //fall through
		case 'S':
			update(address);
			break;
		}
		//update the counter
		for (int i = 0; i < S; i++) {
			for (int j = 0; j < E; j++) {
				if (cache[i][j].valid_bit == 1)
					cache[i][j].counter++;
			}
		}
	}
	fclose(pFile);

	//free memory
	for (int i = 0; i < S; i++)
		free(cache[i]);
	free(cache);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
