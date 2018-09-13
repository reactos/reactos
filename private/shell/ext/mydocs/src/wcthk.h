#ifndef _WIDE_CHAR_THUNK_HEADER_
#define _WIDE_CHAR_THUNK_HEADER_

/*

    All of these macros require that you supply two
    buffers, which each hold exactly the same number
    of *characters*.  The third parameter to each macro is
    the length of each buffer, less the terminating null 
    character.  All strings must be null-terminated.

*/

/*

    Macros which convert between TCHARs and WCHARs, regardless 
    of how TCHAR is defined for the current compile session.
    Useful for solving the Win95/OLE string incompatibility
    problem.

*/

#ifdef UNICODE
#define TCHARtoWCHAR(szIn, szOut, cch) lstrcpyn(szOut, szIn, cch)
#else
#define TCHARtoWCHAR(szIn, szOut, cch) MultiByteToWideChar(CP_ACP, 0L, szIn, -1L, szOut, cch)
#endif /* UNICODE */

#ifdef UNICODE
#define WCHARtoTCHAR(szIn, szOut, cch) lstrcpyn(szOut, szIn, cch)
#else
#define WCHARtoTCHAR(szIn, szOut, cch) WideCharToMultiByte(CP_ACP, 0L, szIn, -1L, szOut, cch, NULL, NULL)
#endif /* UNICODE */

/*

    Macros which convert between TCHARs and CHARs, regardless
    of how TCHAR is defined for the current compile session.
    Useful for solving the argv/TCHAR string incompatibility
    problem.

*/

#ifdef UNICODE
#define TCHARtoCHAR(szIn, szOut, cch) WideCharToMultiByte(CP_ACP, 0L, szIn, -1L, szOut, cch, NULL, NULL)
#else
#define TCHARtoCHAR(szIn, szOut, cch) lstrcpyn(szOut, szIn, cch)
#endif /* UNICODE */

#ifdef UNICODE
#define CHARtoTCHAR(szIn, szOut, cch) MultiByteToWideChar(CP_ACP, 0L, szIn, -1L, szOut, cch)
#else
#define CHARtoTCHAR(szIn, szOut, cch) lstrcpyn(szOut, szIn, cch)
#endif /* UNICODE */

#endif /* _WIDE_CHAR_THUNK_HEADER_ */