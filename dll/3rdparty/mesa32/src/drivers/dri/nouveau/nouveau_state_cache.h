
#ifndef __NOUVEAU_STATE_CACHE_H__
#define __NOUVEAU_STATE_CACHE_H__

#include "mtypes.h"

#define NOUVEAU_STATE_CACHE_ENTRIES 2048
// size of a dirty requests block
// you can play with that and tune the value to increase/decrease performance
// but keep it a power of 2 !
#define NOUVEAU_STATE_CACHE_HIER_SIZE  32

typedef struct nouveau_state_atom_t{
	uint32_t value;
	uint32_t dirty;
}nouveau_state_atom;

typedef struct nouveau_state_cache_t{
	nouveau_state_atom atoms[NOUVEAU_STATE_CACHE_ENTRIES];
	uint32_t current_pos;
	// hierarchical dirty flags
	uint8_t hdirty[NOUVEAU_STATE_CACHE_ENTRIES/NOUVEAU_STATE_CACHE_HIER_SIZE];
	// master dirty flag
	uint8_t dirty;
}nouveau_state_cache;


#endif

