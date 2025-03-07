#ifndef __SEEXCEPTION_H__
#define __SEEXCEPTION_H__

#include <windows.h>

class fbtSeException
{
	public:
		fbtSeException(unsigned int nSeCode, _EXCEPTION_POINTERS* pExcPointers);
		fbtSeException(fbtSeException & CseExc);

		unsigned int GetSeCode(void);

	private:
		unsigned int m_nSeCode;
		_EXCEPTION_POINTERS* m_pExcPointers;

};

void fbtXcptEnableSEHandling();

#endif //__SEEXCEPTION_H__
