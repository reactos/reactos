
#define upalpha ('A' - 'a')

int isalpha(char c)
{
   return(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

int isalnum(char c)
{
   return(isalpha(c)||isdigit(c));
}

int iscntrl(char c)
{
   return((c >=0x00 && c <= 0x1f) || c == 0x7f);
}

int isdigit(char c)
{
   return((c>='0') && (c<='9'));
}

int isgraph(char c)
{
   return(c>=0x21 && c<=0x7e);
}

int islower(char c)
{
   return((c>='a') && (c<='z'));
}

int isprint(char c)
{
   return(c>=0x20 && c<=0x7e);
}

int ispunct(char c)
{
   return((c>=0x21 && c<=0x2f)||(c>=0x3a && c<=0x40)||(c>=0x5b && c<=0x60));
}

int isspace(char c)
{
   return(c==' '||c=='\t');
}

int isupper(char c)
{
   return((c>='A') && (c<='Z'));
}

int isxdigit(char c)
{
   return(((c>='0') && (c<='9')) || ((toupper(c)>='A') && (toupper(c)<='Z')));
}


char tolower(char c)
{
   if (c>='A' && c <= 'Z')
       return (c - upalpha);
   return(c);
}

char toupper(char c)
{
   if ((c>='a') && (c<='z'))
      return (c+upalpha);
   return(c);
}

