// iert.h - Definitions, and Prototypes for the Internet Explorer 
//          implementation of the c-runtime library.
//
// History:
//     Created on 16-May-1997 by Vince Roggero (vincentr)
//

#ifdef __cplusplus
extern "C" 
{
#endif

/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*******************************************************************************/
char* __cdecl StrTokEx (char ** pstring, const char * control);


/***
* double StrToDbl(const char *str, char **strStop) - convert string to double
*
* Purpose:
*           To convert a string into a double.  This function supports
*           simple double representations like '1.234', '.5678'.  It also support
*           the a killobyte computaion by appending 'k' to the end of the string
*           as in '1.5k' or '.5k'.  The results would then become 1536 and 512.5.
*
* Return:
*           The double representation of the string.
*           strStop points to the character that caused the scan to stop.
*
*******************************************************************************/
double __cdecl StrToDbl(const char *strIn, char **strStop);

#ifdef __cplusplus
}   // extern "C"
#endif


