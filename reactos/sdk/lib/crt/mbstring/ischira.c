#include <precomp.h>
#include <mbctype.h>

/*********************************************************************
 *		_ismbchira(MSVCRT.@)
 */
int CDECL _ismbchira(unsigned int c)
{
  if(get_mbcinfo()->mbcodepage == 932)
  {
    /* Japanese/Hiragana, CP 932 */
    return (c >= 0x829f && c <= 0x82f1);
  }
  return 0;
}

/*********************************************************************
 *		_ismbckata(MSVCRT.@)
 */
int CDECL _ismbckata(unsigned int c)
{
  if(get_mbcinfo()->mbcodepage == 932)
  {
    /* Japanese/Katakana, CP 932 */
    return (c >= 0x8340 && c <= 0x8396 && c != 0x837f);
  }
  return 0;
}

/*********************************************************************
 *		_mbctohira (MSVCRT.@)
 *
 *              Converts a sjis katakana character to hiragana.
 */
unsigned int CDECL _mbctohira(unsigned int c)
{
    if(_ismbckata(c) && c <= 0x8393)
        return (c - 0x8340 - (c >= 0x837f ? 1 : 0)) + 0x829f;
    return c;
}

/*********************************************************************
 *		_mbctokata (MSVCRT.@)
 *
 *              Converts a sjis hiragana character to katakana.
 */
unsigned int CDECL _mbctokata(unsigned int c)
{
    if(_ismbchira(c))
        return (c - 0x829f) + 0x8340 + (c >= 0x82de ? 1 : 0);
    return c;
}


