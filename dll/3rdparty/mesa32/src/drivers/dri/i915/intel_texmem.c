#include "texmem.h"
#include "simple_list.h"
#include "imports.h"
#include "macros.h"

#include "intel_tex.h"

static GLuint
driLog2( GLuint n )
{
   GLuint log2;

   for ( log2 = 1 ; n > 1 ; log2++ ) {
      n >>= 1;
   }

   return log2;
}

static void calculate_heap_size( driTexHeap * heap, unsigned size, 
				 unsigned nr_regions, unsigned alignmentShift )
{
   unsigned     l;

   l = driLog2( (size - 1) / nr_regions );
   if ( l < alignmentShift )
   {
      l = alignmentShift;
   }

   heap->logGranularity = l;
   heap->size = size & ~((1L << l) - 1);
}


GLboolean 
intel_driReinitTextureHeap( driTexHeap *heap,
			    unsigned size )
{
   driTextureObject *t, *tmp;

   /* Kick out everything:
    */
   foreach_s ( t, tmp, & heap->texture_objects ) {
      if ( t->tObj != NULL ) {
	 driSwapOutTextureObject( t );
      }
      else {
	 driDestroyTextureObject( t );
      }
   }
   
   /* Destroy the memory manager:
    */
   mmDestroy( heap->memory_heap );
      
   /* Recreate the memory manager:
    */
   calculate_heap_size(heap, size, heap->nrRegions, heap->alignmentShift);
   heap->memory_heap = mmInit( 0, heap->size );
   if ( heap->memory_heap == NULL ) {
      fprintf(stderr, "driReinitTextureHeap: couldn't recreate memory heap\n");
      FREE( heap );
      return GL_FALSE;
   }

   make_empty_list( & heap->texture_objects );

   return GL_TRUE;
}


