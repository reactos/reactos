#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

#include <stdarg.h>
#include "rbuild.h"

using std::string;

Exception::Exception()
{
}

Exception::Exception(const string& message)
{
	Message = message;
}

Exception::Exception(const char* format,
                     ...)
{
	va_list args;
	va_start(args,
	         format);
	Message = ssvprintf(format,
	                    args);
	va_end(args);
}

void Exception::SetMessage(const char* message,
                           va_list args)
{
	Message = ssvprintf(message,
	                    args);
}


FileNotFoundException::FileNotFoundException(const string& filename)
	: Exception ( "File '%s' not found.", filename.c_str() )
{
	Filename = filename;
}


InvalidBuildFileException::InvalidBuildFileException(const char* message,
                                                     ...)
{
	va_list args;
	va_start( args, message);
	SetMessage(message, args);
	va_end(args);
}
