
#include <ctype.h>

#define upalpha ('A' - 'a')

int isalpha(int c)
{
   return(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

int isalnum(int c)
{
   return(isalpha(c)||isdigit(c));
}

int iscntrl(int c)
{
   return((c >=0x00 && c <= 0x1f) || c == 0x7f);
}

int isdigit(int c)
{
   return((c>='0') && (c<='9'));
}

int isgraph(int c)
{
   return(c>=0x21 && c<=0x7e);
}

int islower(int c)
{
   return((c>='a') && (c<='z'));
}

int isprint(int c)
{
   return(c>=0x20 && c<=0x7e);
}

int ispunct(int c)
{
   return((c>=0x21 && c<=0x2f)||(c>=0x3a && c<=0x40)||(c>=0x5b && c<=0x60));
}

int isspace(int c)
{
   return(c==' '||c=='\t');
}

int isupper(int c)
{
   return((c>='A') && (c<='Z'));
}

int isxdigit(int c)
{
   return(((c>='0') && (c<='9')) || ((toupper(c)>='A') && (toupper(c)<='Z')));
}

int _tolower(int c)
{
   if (c>='A' && c <= 'Z')
       return (c - upalpha);
   return(c);
}

int _toupper(int c)
{
   if ((c>='a') && (c<='z'))
      return (c+upalpha);
   return(c);
}

int tolower(int c)
{
   if (c>='A' && c <= 'Z')
       return (c - upalpha);
   return(c);
}

int toupper(int c)
{
   if ((c>='a') && (c<='z'))
      return (c+upalpha);
   return(c);
}

wchar_t towlower(wchar_t c)
{
   if (c>='A' && c <= 'Z')
       return (c - upalpha);
   return(c);
}

wchar_t towupper(wchar_t c)
{
   if ((c>='a') && (c<='z'))
      return (c+upalpha);
   return(c);
}
