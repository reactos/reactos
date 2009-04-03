/*
    Don't use this.
*/

#include <mmdrv.h>

/*
    Complete a partial wave buffer transaction
*/

void
CompleteWaveOverlap(
    DWORD error_code,
    DWORD bytes_transferred,
    LPOVERLAPPED overlapped)
{
    DPRINT("Complete partial wave overlap\n");
}

/*
    Helper function to set up loops
*/

VOID
UpdateWaveLoop(SessionInfo* session_info)
{
}


/*
    The hub of all wave I/O. This ensures a constant stream of buffers are
    passed between the land of usermode and kernelmode.
*/

VOID
PerformWaveIO(
    SessionInfo* session_info)
{

}
