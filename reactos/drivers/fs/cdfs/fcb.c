#include <ddk/ntddk.h>
#include "cdfs.h"

/* 
   CDFS: FCB system (Perhaps there should be a library to make this easier) 
*/
#if 0
typedef struct _fcb_system {
  int fcbs_in_use;
  int fcb_table_size;
  int fcb_table_mask;
  FsdFcbEntry **fcb_table;
  FsdFcbEntry *parent;
} fcb_system;
#endif

// Create a hash over this name for table
static int FsdNameHash( FsdFcbEntry *parent, wchar_t *name ) {
  int i;
  int hashval = 0;

  for( i = 0; name[i]; i++ )
    hashval ^= name[i] << (i & 0xf);

  hashval ^= (int)parent;

  return hashval;
}

// Init the fcb system
fcb_system *FsdFcbInit() {
  fcb_system *fss = ExAllocatePool( NonPagedPool, 
				    sizeof( fcb_system ) );

  if( !fss ) return NULL;

  RtlZeroMemory( fss, sizeof( *fss ) );

  fss->fcb_table_size = 128;
  fss->fcb_table_mask = fss->fcb_table_size - 1;
  fss->fcb_table = ExAllocatePool( NonPagedPool,
				   sizeof( FsdFcbEntry ** ) * 
				   fss->fcb_table_size );

  if( !fss->fcb_table ) {
    ExFreePool( fss );
    return NULL;
  }

  RtlZeroMemory( fss->fcb_table, sizeof( FsdFcbEntry ** ) * 
		 fss->fcb_table_size );

  return fss;
}

// Delete the fcb system
void FsdFcbDeinit( fcb_system *fss ) {
  if( fss->fcb_table ) ExFreePool( fss->fcb_table );
}

// Get one entry from the FCB system by name...
FsdFcbEntry *FsdGetFcbEntry( fcb_system *fss, FsdFcbEntry *parent, 
			     wchar_t *name ) {
  int hashval;
  FsdFcbEntry *table_ent = NULL;

  hashval = FsdNameHash( parent, name ) & fss->fcb_table_mask;
  table_ent = fss->fcb_table[hashval];

  while( table_ent && _wcsicmp( table_ent->name, name ) &&
	 table_ent->parent != parent )
    table_ent = table_ent->next;

  return table_ent;
}

// Create an fcb with the given name...
FsdFcbEntry *FsdCreateFcb( fcb_system *fss, FsdFcbEntry *parent,
			   wchar_t *filename ) {
  int hashval;
  int tableval;
  FsdFcbEntry *table_ent = FsdGetFcbEntry( fss, parent, filename );

  if( table_ent ) return table_ent;

  hashval = FsdNameHash( parent, filename );
  tableval = hashval & fss->fcb_table_mask;

  table_ent = ExAllocatePool( NonPagedPool, sizeof( FsdFcbEntry ) );

  if( table_ent ) {
    table_ent->next = fss->fcb_table[tableval];
    table_ent->hashval = hashval;
    table_ent->parent = parent;
    wcscpy( table_ent->name, filename );
    fss->fcb_table[tableval] = table_ent;
  }

  return table_ent;
}

// Delete this fcb...
void FsdDelete( fcb_system *fss, FsdFcbEntry *which ) {
  int tableval = which->hashval & fss->fcb_table_mask;
  FsdFcbEntry *table_ent = fss->fcb_table[tableval];
  
  if( table_ent == which ) {
    fss->fcb_table[tableval] = fss->fcb_table[tableval]->next;
    ExFreePool( which );
    return;
  }

  if( !table_ent ) return;

  while( table_ent->next ) {
    if( table_ent->next == which ) {
      table_ent->next = table_ent->next->next;
      ExFreePool( which );
      return;
    }
    table_ent = table_ent->next;
  }
}
