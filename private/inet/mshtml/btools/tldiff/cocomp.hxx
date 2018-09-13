#ifndef CoComp_hxx
#define CoComp_hxx 1

//--- Additional includes ----------------------------------------------
#include "types.h"
#include "array.hxx"
#include "windows.h"

//--- Class, Enum, Struct and Union Declarations -----------------------
class CCompareCoClass
{
public:
	CCompareCoClass(char* pCurBuf, char* pRefBuf, HANDLE fileDiff, char* pszClassName);
	~CCompareCoClass();
	
	void FindAdditionsAndChanges();
	void FindRemovals();

private:
	char*  _pCurBuf;
	char*  _pRefBuf;
	HANDLE _fileDiff;
	char*  _pszClassName;
	CAutoArray<LINEINFO>* _pCurList;
	CAutoArray<LINEINFO>* _pRefList;
	bool   _bFirstTime;

	void CreateLineIndex( CAutoArray<LINEINFO>* pList, char* pBuf );
};

#endif	// def CoComp_hxx
