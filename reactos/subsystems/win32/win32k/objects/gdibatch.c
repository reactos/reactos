
#include <w32k.h>

#define NDEBUG
#include <debug.h>


//
//
// Gdi Batch Flush support functions.
//


//
// Process the batch.
//
ULONG
FASTCALL
GdiFlushUserBatch(HDC hDC, PGDIBATCHHDR pHdr)
{
  PDC dc = DC_LockDc(hDC);
  if (!dc) return 0;
  // The thread is on the end of sunset.
  switch(pHdr->Cmd)
  {
     case GdiBCPatBlt: // Highest pri first!
        break;
     case GdiBCPolyPatBlt:
        break;
     case GdiBCTextOut:
        break;
     case GdiBCExtTextOut:
        break;
     case GdiBCSetBrushOrg:
     {
        PGDIBSSETBRHORG pgSBO = (PGDIBSSETBRHORG) pHdr;
        dc->Dc_Attr.ptlBrushOrigin = pgSBO->ptlBrushOrigin;
        break;
     }
     case GdiBCExtSelClipRgn:
        break;
     case GdiBCSelObj:
        break;
     case GdiBCDelObj:
     case GdiBCDelRgn:
     {
        PGDIBSOBJECT pgO = (PGDIBSOBJECT) pHdr;
        NtGdiDeleteObject( pgO->hgdiobj );
        break;
     }
     default:
       DC_UnlockDc(dc);
       return 0;
  }
  DC_UnlockDc(dc);
  return pHdr->Size; // Return the full size of the structure.
}

/*
 * NtGdiFlush
 *
 * Flushes the calling thread's current batch.
 */
VOID
APIENTRY
NtGdiFlush(VOID)
{
  // Hack! FIXME!
  NtYieldExecution(); // Force thread to sunset and run the flush.
  UNIMPLEMENTED;
}

/*
 * NtGdiFlushUserBatch
 *
 * Callback for thread batch flush routine.
 *
 * Think small & fast!
 */
NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
  PTEB pTeb = NtCurrentTeb();
  ULONG GdiBatchCount = pTeb->GdiBatchCount;
  
  if( (GdiBatchCount > 0) && (GdiBatchCount <= GDIBATCHBUFSIZE))
  {
    HDC hDC = (HDC) pTeb->GdiTebBatch.HDC;
//
//  If hDC is zero and the buffer fills up with delete objects we need to run
//  anyway. So, hard code to the system batch limit.
//
    if ((hDC) || (GdiBatchCount >= GDI_BATCH_LIMIT))
    {
       PULONG pHdr = &pTeb->GdiTebBatch.Buffer[0];
       // No need to init anything, just go!
       for (; GdiBatchCount > 0; GdiBatchCount--)
       {
           // Process Gdi Batch!
           pHdr += GdiFlushUserBatch( hDC, (PGDIBATCHHDR) pHdr );
       }
       // Exit and clear out for the next round.
       pTeb->GdiTebBatch.Offset = 0;
       pTeb->GdiBatchCount = 0;
       pTeb->GdiTebBatch.HDC = 0;
    }
  }
  return STATUS_SUCCESS;
}

