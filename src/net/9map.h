#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>


struct mapElement {
	struct mapElement* next; /* Next element in list */
	uint8_t* key; /* the key this entry is identified by */
	uint32_t nval; /* size of value */
	void* value; /* the value */
};

struct mapMap {
	uint32_t size; /* the size of the dict */
	uint32_t stored; /* how many elements are stored */
	struct mapElement** table; /* the actual data */
};

/* how big the table should be at start */
#define INITIAL_SIZE 1024
/* when table gets full, make it n-times bigger */
#define GROWTH_FACTOR 2
#define MAX_LOAD_FACTOR 1
/* Any prime number will do here */
#define MULTIPLIER 97

struct mapMap* mapCreate(uint32_t size);
void mapDestroy(struct mapMap* map);
static uint32_t mapHash(const uint8_t* s);
static void mapGrow(struct mapMap* map);
void mapInsert(struct mapMap* map, const uint8_t* key, void* value, uint32_t nval);
void* mapSearch(struct mapMap* map, const uint8_t* key);
void mapDelete(struct mapMap* map, const uint8_t* key);
