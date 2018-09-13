#ifndef IComp_hxx
#define IComp_hxx 1

//--- Additional includes ----------------------------------------------
#include "types.h"
#include "array.hxx"
#include "windows.h"

//--- Class, Enum, Struct and Union Declarations -----------------------
class CCompareInterface
{
public:
	CCompareInterface(char* pCurBuf, char* pRefBuf, HANDLE fileDiff, char* szIntName,
                      BLOCK_TYPE blockType, char* pszMethodAttr);
	~CCompareInterface();
	
	void FindAdditionsAndChanges();
	void FindRemovals();

private:
	char*                   _pCurBuf;
	char*                   _pRefBuf;
	HANDLE                  _fileDiff;
	char*                   _pszIntName;
	CAutoArray<LINEINFO>*   _pCurList;
	CAutoArray<LINEINFO>*   _pRefList;
	bool                    _bFirstTime;
	BLOCK_TYPE              _blockType;
    char *                  _pszMethodAttr;
    char                    _szLogBuff[128];

	void CreateLineIndex( CAutoArray<LINEINFO>* pList, char* pBuf );
    
    void CompareMethodAttributes( LINEINFO* pRef, LINEINFO* pCur  );
    void CompareMethodParameters( LINEINFO* pRef, LINEINFO* pCur  );

    BOOL IsAttributeBreaker( char * pszAttrList, char * pszAttr, unsigned long ulAttrLen );

    void TokenizeParameters( char* pBuf, unsigned long nCnt, CAutoArray<PARAMINFO>* pList );

    void EnsureTitle( BOOL bAddition );

    void WriteAttrChangeString( char* pBuf, unsigned long ulAttrStart, unsigned long ulAttrLength, char* szChangeType );
};

#endif // IComp_Hxx