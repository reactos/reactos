
// types for the SQL debugger components
// Note: this must work for C and C++

// extract SQL from ADDR
#define	ADDR_EXTRACT_LINE( p )	(WORD)((p).addr.off & 0xFFFF)
#define	ADDR_EXTRACT_COOKIE(p)	(WORD)((p).addr.off>>16)

// create ADDR from SQL
#define	ADDR_MAKE_PC( ad, sp, li )		(ad).addr.off = (UOFFSET) (((DWORD)sp)<<16) | li

#define	ADDR_SET_LINE( ad, li )			(ad).addr.off = ((ad).addr.off & 0xFFFF0000) | (li)
#define	ADDR_SET_SQL( ad )				(ad).mode.fSql = 1
#define	ADDR_SET_SQL_EMI( ad )			(ad).emi = (HEMI)SQL_EMI

// write to StackFrame structure
#define	SQL_VTHREAD_MAKE_PC( pt, cook, li )	(pt).AddrPC.Offset = (UOFFSET) (((DWORD)cook)<<16) | li
#define	SQL_VTHREAD_MAKE_SP( pt, level )	(pt).AddrStack.Offset = (UOFFSET) ((level)<<16) | SQL_STACK_WORD

#define	SQL_VTHREAD_GET_ADDR( ad, preg )	(ad).addr.off = (preg).AddrPC.Offset
#define	SQL_VTHREAD_GET_SP(   ad, preg )	(ad).addr.off = (preg).AddrStack.Offset
#define	SQL_VTHREAD_CHECK_SP( uoff )		(((uoff) & 0xFFFF)==SQL_STACK_WORD)
#define	SQL_VTHREAD_GET_LEVEL( uoff )		(WORD)((uoff) >> 16)

#define	SQL_EMI			0xBEEF
#define	SQL_STACK_WORD	0xF00D

#define	SQL_SEG_FIXED	0x1996
#define	SQL_SEG_UNFIXED	0x1964

// one of our HSYMs points to a SQLSYM struct. We have to be able to tell these apart from
// SYMTYPEs. A symtype starts with a word of reclen then one of rectype,
// so the magic value is chosen to make this very unlikely (length=FEED)

#define	SQLSYM_MAGIC	0x1996FEED

struct SQLSYM
{
	DWORD m_Magic;
	ADDR m_SqlAddr;
	int m_CodeLines;		// how many 'lines of code' in this SP, <0 means cannot find
	LPWORD m_pLineList;		// ptr to said list, may be NULL
	char m_Name[1];			// zero-terminated
};
typedef struct SQLSYM * SQLSYMPTR;


