
#include "pch.h"

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


AccessDeniedException::AccessDeniedException(const string& filename)
	: Exception ( "Access denied to file '%s'.", filename.c_str() )
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

InvalidBuildFileException::InvalidBuildFileException()
{
}


XMLSyntaxErrorException::XMLSyntaxErrorException ( const std::string& location,
	                                               const char* message,
	                                               ... )
{
	va_list args;
	va_start ( args, message );
	Message = location + ": " + ssvprintf ( message, args );
	va_end ( args );
}


RequiredAttributeNotFoundException::RequiredAttributeNotFoundException(const std::string& attributeName,
                                                                       const std::string& elementName)
	: InvalidBuildFileException ( "Required attribute '%s' not found on '%s'.",
	                              attributeName.c_str (),
	                              elementName.c_str ())
{
}

InvalidAttributeValueException::InvalidAttributeValueException(const std::string& name,
	                                                           const std::string& value)
	: InvalidBuildFileException ( "Attribute '%s' has an invalid value '%s'.",
	                              name.c_str (),
	                              value.c_str ())
{
	
}
