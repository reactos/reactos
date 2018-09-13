#ifndef _CCSV
#define _CCSV

#include <windows.h>
#define CCSVFILE_BUFFER_SIZE 2*512

// simple file i/o for phone books
class CCSVFile
{
	
	public:
		void far * operator new( size_t cb ) { return GlobalAlloc(GPTR,cb); };
		void operator delete( void far * p ) {GlobalFree(p); };

		CCSVFile();
		~CCSVFile();
		BOOLEAN Open(LPCSTR pszFileName);
		BOOLEAN ReadToken(LPSTR pszDest, DWORD cbMax);	// reads up to comma or newline, returns fFalse on EOF
		void Close(void);
		inline int ILastRead(void)
			{
			return m_iLastRead;
			}

	private:
		BOOL 	FReadInBuffer(void);
		inline int 	ChNext(void);
		char 	m_rgchBuf[CCSVFILE_BUFFER_SIZE]; //buffer
		LPSTR 	m_pchBuf;			//pointer to the next item in the buffer to read
		LPSTR	m_pchLast;			//pointer to the last item in the buffer
		int  	m_iLastRead;		//the character last read.
		DWORD 	m_cchAvail;
		HANDLE 	m_hFile;

}; // ccsv
#endif //_CCSV
