/////////////////////////////////////////////////////////
//			 TkeyDef - Key Definitions class           //
//                   - keeped in an array container    //
/////////////////////////////////////////////////////////

#ifndef __TKEYDEF_H
#define __TKEYDEF_H

#include <windows.h>

#ifndef __BORLANDC__ // Ioannou Dec. 8, 1998
// We need these for MSVC6 (Sam Robertson Oct. 8, 1998)
class TKeyDef;
bool operator==(const TKeyDef &t1, const TKeyDef &t2);
bool operator<(const TKeyDef &t1, const TKeyDef &t2);
////
#endif

// Paul Brannan Feb. 5, 1999
enum tn_ops {TN_ESCAPE, TN_SCROLLBACK, TN_DIAL, TN_PASTE, TN_NULL, TN_CR, TN_CRLF};

typedef struct {
	char sendstr;
	tn_ops the_op;
} optype;

union KeyDefType {
	char *szKeyDef;
	optype *op;
};

union KeyDefType_const {
	const char *szKeyDef;
	const optype *op;
};

class TKeyDef {
private:
	KeyDefType	uKeyDef;
	DWORD		vk_code;
	DWORD		dwState;
	
public:
	TKeyDef();
	TKeyDef(char *def, DWORD state, DWORD code);
	TKeyDef(optype op, DWORD state, DWORD code);
	TKeyDef(const TKeyDef &t);
	
	char *operator=(char *def);
	DWORD  operator=(DWORD code);
	TKeyDef& operator=(const TKeyDef &t);
	const optype& operator=(optype op);
		
	~TKeyDef();
	
#ifdef __BORLANDC__
	int operator==(TKeyDef &t);
#else
	// made these into friends for compatibility with stl
	// (Paul Brannan 5/7/98)
	friend bool operator==(const TKeyDef &t1, const TKeyDef &t2);
	friend bool operator<(const TKeyDef &t1, const TKeyDef &t2);
#endif
	
	const char *GetszKey() { return uKeyDef.szKeyDef; }
	const KeyDefType GetKeyDef() { return uKeyDef; }
	DWORD GetCodeKey() { return vk_code; }

};

#endif
