/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>
#undef tolower
int tolower(int c)
{
 return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}
#undef towlower
wchar_t towlower(wchar_t c)
{
 return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}

int _tolower(int c)
{
 return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}

wchar_t _towlower(wchar_t c)
{
 return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}



