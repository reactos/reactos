#include <msvcrt/mbstring.h>

int _ismbbalpha(unsigned char c);
int _ismbbalnum(unsigned char c);

int _ismbcalnum( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return _ismbbalnum(c);

	return 0;
}

int _ismbcalpha( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return _ismbbalpha(c);

	return 0;
}
  
int _ismbcdigit( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return 0;
//		return _ismbbdigit(c);

	return 0;
}

int _ismbcprint( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return 0;
//		return _ismbbdigit(c);

	return 0;
}

int _ismbcsymbol( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return 0;
//		return _ismbbdigit(c);

	return 0;
}

int _ismbcspace( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return 0;
//		return _ismbbdigit(c);

	return 0;
}
int _ismbclegal(unsigned int c)
{
	if ((c & 0xFF00) != 0) {
		return _ismbblead(c>>8) && _ismbbtrail(c&0xFF);
	}
	else
		return _ismbbtrail(c&0xFF);

	return 0;
}

int _ismbcl0(unsigned int c)
{
  return 0;
}

int _ismbcl1(unsigned int c)
{
  return 0;
}

int _ismbcl2(unsigned int c)
{
  return 0;
}
