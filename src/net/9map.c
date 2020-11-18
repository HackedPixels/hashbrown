#include "9map.h"

struct mapMap*
mapCreate(uint32_t size)
{
	struct mapMap* map;
	uint32_t i;

	map = malloc(sizeof(struct mapMap*));
	if (map == NULL) { return NULL; } /* TODO: make this throw an error */

	map->size   = size;
	map->stored = 0;
	map->table  = malloc(sizeof(struct mapElement*) * map->size);

	if (map->table == NULL) { return NULL; } /* TODO: make this throw an error and free map */
	for (i = 0; i < map->size; i++) { map->table[i] = 0; }

	return map;
}

void
mapDestroy(struct mapMap* map)
{
	struct mapElement* elem;
	struct mapElement* next;
	uint32_t i;

	for (i = 0; i < map->size; i++)
	{
		for (elem = map->table[i]; elem != 0; elem = next)
		{
			next = elem->next;
			
			free(elem->key);
			free(elem->value);
			free(elem);
		}
	}

	free(map->table);
	free(map);
}

uint32_t
mapHash(const uint8_t* s)
{
	const uint8_t* us;
	uint32_t hash;

	hash = 0;

	for (us = (const uint8_t*) s; *us != '\0'; us++) {
		hash = hash * MULTIPLIER + *us;
	}

	return hash;
}

static void
mapGrow(struct mapMap* map)
{
	struct mapMap* nmap;
	struct mapMap  swap;
	struct mapElement* elem;
	uint32_t i;
	
	nmap = mapCreate(map->size * GROWTH_FACTOR);

	for (i = 0; i < map->size; i++)
	{
		for(elem = map->table[i]; elem != 0; elem = elem->next)
		{
			/* TODO: avoid strdups */
			mapInsert(nmap, elem->key, elem->value, elem->nval);
		}
	}

	swap = *map;
	*map = *nmap;
	*nmap = swap;

	mapDestroy(nmap);
}

void
mapInsert(struct mapMap* map, const uint8_t* key, void* value, uint32_t nval)
{
	struct mapElement* elem;
	uint32_t hash;

	if (key == NULL) {} /* TODO: Make this throw an error */
	if (value == NULL) {} /* TODO: Make this throw an error */

	elem = malloc(sizeof(struct mapElement));

	if (elem == NULL) {} /* TODO: Make this throw an error */

	elem->key = strdup(key);
	elem->nval = nval;
	elem->value = malloc(nval);
	memcpy(elem->value, value, nval);
//	elem->value = strdup(value);

	hash = mapHash(key) % map->size;

	elem->next = map->table[hash];
	map->table[hash] = elem;

	map->stored++;

	/* if table is full, extend */
	if (map->stored >= map->size * MAX_LOAD_FACTOR) { mapGrow(map); }
}

void*
mapSearch(struct mapMap* map, const uint8_t* key)
{
	struct mapElement* elem;

	for (elem = map->table[mapHash(key) % map->size]; elem != 0; elem = elem->next)
	{
		if (strcmp(elem->key, key) == 0) { return elem->value; }
	}

	return NULL;
}

void
mapDelete(struct mapMap* map, const uint8_t* key)
{
	struct mapElement** prev;
	struct mapElement* elem;

	for (prev = &(map->table[mapHash(key) % map->size]);
		*prev != 0;
		prev = &((*prev)->next))
	{
		if (strcmp((*prev)->key, key))
		{
			elem = *prev;
			*prev = elem->next;

			free(elem->key);
			free(elem->value);
			free(elem);

			return;
		}
	}
}
