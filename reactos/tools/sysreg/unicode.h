#ifndef UNICODE_H__
#define UNICODE_H__ // unicode.h

#include "user_types.h"

namespace System_
{

	class UnicodeConverter
	{
	public:
//---------------------------------------------------------------------------------------
///
/// UnicodeConverter
///
/// Description: destructor of class UnicodeConverter

	virtual ~UnicodeConverter()
	{}

//---------------------------------------------------------------------------------------
///
/// UnicodeConverter
///
/// Description: converts an ANSI buffer to wide character buffer
/// using standard c routines
///
/// Note: make sure before calling that outbuf is big enough to receive the result
///
/// @param abuf ansi buffer used a source
/// @param outbuf wide character buffer receives result
/// @param length length of abuf
///
/// @return bool

	static bool ansi2Unicode(char * abuf, wchar_t * outbuf, size_t length);


	protected:
//---------------------------------------------------------------------------------------
///
/// UnicodeConverter
///
/// Description: constructor of class UnicodeConverter

		UnicodeConverter()
		{}

	}; // end of class UnicodeConverter



} // end of namespace System_

#endif /* end of UNICODE_H__ */
