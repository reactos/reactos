//----------------------------------------------------------------------------
// Exports from fileop.
//----------------------------------------------------------------------------

BOOL  ExtendedCreate(LPTSTR pName, DWORD dwFileAttributes, HFILE *pfh);
void  SetDateTime(HFILE fh, WORD wDate, WORD wTime);

#ifdef WIN32
#define SetFileRaw(fh)
#define IsSerialDevice(fh) FALSE
#else
void  SetFileRaw(HFILE fh);
BOOL  IsSerialDevice(HFILE fh);
#endif

