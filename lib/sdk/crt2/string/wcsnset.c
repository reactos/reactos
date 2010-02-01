wchar_t* _CDECL _wcsnset( wchar_t* str, wchar_t c, size_t n )
{
  wchar_t* ret = str;
  while ((n-- > 0) && *str)
	  *str++ = c;
  return ret;
}

