#include <oskittcp.h>
#include <oskitdebug.h>
#include <ntddk.h>
#include <memtrack.h>

void *fbsd_malloc( size_t size, char *file, int line, ... ) {
    void *v;
    v = ExAllocatePool( NonPagedPool, size );
    if( v ) TrackWithTag(FBSD_MALLOC,v,file,line);
    return v;
}

void fbsd_free( void *data, char *file, int line, ... ) {
    UntrackFL(file,line,data);
    ExFreePool( data );
}
