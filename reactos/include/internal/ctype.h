#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H


#ifdef USE_OLD_CTYPE_IMPLEMENTATION

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

extern unsigned char _ctype[];
extern char _ctmp;

#define isalnum(c) ((_ctype+1)[c]&(_U|_L|_D))
#define isalpha(c) ((_ctype+1)[c]&(_U|_L))
#define iscntrl(c) ((_ctype+1)[c]&(_C))
#define isdigit(c) ((_ctype+1)[c]&(_D))
#define isgraph(c) ((_ctype+1)[c]&(_P|_U|_L|_D))
#define islower(c) ((_ctype+1)[c]&(_L))
#define isprint(c) ((_ctype+1)[c]&(_P|_U|_L|_D|_SP))
#define ispunct(c) ((_ctype+1)[c]&(_P))
#define isspace(c) ((_ctype+1)[c]&(_S))
#define isupper(c) ((_ctype+1)[c]&(_U))
#define isxdigit(c) ((_ctype+1)[c]&(_D|_X))

#define isascii(c) (((unsigned) c)<=0x7f)
#define toascii(c) (((unsigned) c)&0x7f)

#define tolower(c) (_ctmp=c,isupper(_ctmp)?_ctmp-('A'-'a'):_ctmp)
#define toupper(c) (_ctmp=c,islower(_ctmp)?_ctmp-('a'-'A'):_ctmp)

#else

#define upalpha ('A' - 'a')

extern inline int isspace(char c)
{
   return(c==' '||c=='\t');
}

extern inline char toupper(char c)
{
   if ((c>='a') && (c<='z')) return (c+upalpha);
   return(c);
}

extern inline int islower(char c)
{
   if ((c>='a') && (c<='z')) return 1;
   return 0;
}

extern inline int isdigit(char c)
{
   if ((c>='0') && (c<='9')) return 1;
   return 0;
}

extern inline int isxdigit(char c)
{
   if (((c>='0') && (c<='9')) || ((toupper(c)>='A') && (toupper(c)<='Z')))
     {
	return 1;
     }
   return 0;
}


#endif

#endif
