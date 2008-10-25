
#include "nouveau_state_cache.h"
#include "nouveau_context.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"

#define BEGIN_RING_NOFLUSH(subchannel,tag,size) do {					\
	if (nmesa->fifo.free <= (size))							\
		WAIT_RING(nmesa,(size));						\
	OUT_RING( ((size)<<18) | ((subchannel) << 13) | (tag));				\
	nmesa->fifo.free -= ((size) + 1);                                               \
}while(0)

// flush all the dirty state
void nouveau_state_cache_flush(nouveauContextPtr nmesa)
{
	int i=0;
	int run=0;

	// fast-path no state changes
	if (!nmesa->state_cache.dirty)
		return;
	nmesa->state_cache.dirty=0;

	do
	{
		// jump to a dirty state
		while((nmesa->state_cache.hdirty[i/NOUVEAU_STATE_CACHE_HIER_SIZE]==0)&&(i<NOUVEAU_STATE_CACHE_ENTRIES))
			i=(i&~(NOUVEAU_STATE_CACHE_HIER_SIZE-1))+NOUVEAU_STATE_CACHE_HIER_SIZE;
		while((nmesa->state_cache.atoms[i].dirty==0)&&(i<NOUVEAU_STATE_CACHE_ENTRIES))
			i++;

		// figure out a run of dirty values
		run=0;
		while((nmesa->state_cache.atoms[i+run].dirty)&&(i+run<NOUVEAU_STATE_CACHE_ENTRIES))
			run++;

		// output everything as a single run
		if (run>0) {
			int j;

			BEGIN_RING_NOFLUSH(NvSub3D, i*4, run);
			for(j=0;j<run;j++)
			{
				OUT_RING(nmesa->state_cache.atoms[i+j].value);
				nmesa->state_cache.atoms[i+j].dirty=0;
				if ((i+j)%NOUVEAU_STATE_CACHE_HIER_SIZE==0)
					nmesa->state_cache.hdirty[(i+j)/NOUVEAU_STATE_CACHE_HIER_SIZE-1]=0;
			}
			i+=run;
		}
	}
	while(i<NOUVEAU_STATE_CACHE_ENTRIES);
	nmesa->state_cache.hdirty[NOUVEAU_STATE_CACHE_HIER_SIZE/NOUVEAU_STATE_CACHE_HIER_SIZE-1]=0;
}


// inits the state cache
void nouveau_state_cache_init(nouveauContextPtr nmesa)
{
	int i;
	for(i=0;i<NOUVEAU_STATE_CACHE_ENTRIES;i++)
	{
		nmesa->state_cache.atoms[i].dirty=0;
		nmesa->state_cache.atoms[i].value=0xDEADBEEF; // nvidia cards like beef
	}
	nmesa->state_cache.dirty=0;
}

