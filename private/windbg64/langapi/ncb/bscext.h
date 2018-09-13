// bscext.h
//	Extension for the bsc interface
#ifndef __BSCEXT_H__
#define __BSCEXT_H__

#include <bsc.h>

/*
enum LineContext
{
	less,
	equal,
	greater
};
*/

// NCB language attributes:
#define NCB_LANGUAGE_ALL	0xff
#define NCB_LANGUAGE_CPP	0x01
#define NCB_LANGUAGE_JAVA	0x02
#define NCB_LANGUAGE_ODL	0x04
#define NCB_LANGUAGE_FORTRAN	0x08
#define NCB_LANGUAGE_HTML	0x10

interface BscEx : public Bsc
{

	// right now it is still empty
	// but pretty soon we want to have at least the following:

/*
	virtual BOOL iinstFrLine(IMOD imod, LINE line, LineContext lc, OUT IINST *piinst) pure;
*/
	virtual BOOL irefEndInfo(IREF iref, OUT SZ *pszModule, OUT LINE *piline) pure;
	virtual BOOL idefEndInfo(IDEF idef, OUT SZ *pszModule, OUT LINE *piline) pure;
	virtual BOOL getGlobalsFrImod (IMOD imod, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
	virtual void setLanguage (BYTE bLanguage) pure;
	virtual void getLanguage (BYTE * pbLanguage) pure;
	virtual BOOL isModInLang (BYTE bLanguage, IMOD imod) pure;
// IDL interfaces
	virtual BOOL getIDLAttrib (IINST iinst, OUT IINST **ppiiAttr, OUT ULONG *pciinst) pure;
	virtual BOOL getIDLAttribVal (IINST iiAttr, OUT SZ *pszValue) pure;
	virtual BOOL isIDLAttrib (IINST iinst, SZ szAttrib, OUT IINST *piiAttr, OUT SZ *pszValue) pure;
	virtual BOOL filterInTypeArray (IINST * piinst, ULONG ciinst, TYP type, 
									OUT IINST ** ppiinstOut, OUT ULONG * pciinstOut) pure;
	virtual BOOL filterOutTypeArray (IINST * piinst, ULONG ciinst, TYP type, 
									OUT IINST ** ppiinstOut, OUT ULONG * pciinstOut) pure;

	virtual BOOL getIDLMFCComment (IINST iinst, OUT IINST **ppiiComment, OUT ULONG * pciinst) pure;
	virtual BOOL getIDLMFCCommentClass (IINST iiComment, OUT SZ * pszClass) pure;
	virtual BOOL isIDLMFCComment (IINST iinst, SZ szType, OUT IINST * piiComment, OUT SZ * pszValue) pure;

// back to general interface
	// get the line number for the declaration
	virtual BOOL ideclInfo(IINST iinst, OUT SZ *pszModule, OUT LINE *piline) pure;
	virtual BOOL getMapIinst (IINST iiClass, SZ szMapType, OUT IINST **ppIinst, OUT ULONG * pciinst) pure;

	virtual BOOL getAllArray (MBF mbf, OUT IINST ** ppiinst, OUT ULONG * pciinst) pure;
	virtual BOOL isLangInProject (BYTE bLanguage) pure;
};

#endif __BSCEXT_H__