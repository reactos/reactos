#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include "pch.h"

class Exception
{
public:
	Exception ( const std::string& message );
	Exception ( const char* format,
	            ...);
	std::string Message;
protected:
	Exception ();
	void SetMessage ( const char* message,
	                  va_list args);
};


class InvalidOperationException : public Exception
{
public:
	InvalidOperationException ( const char* filename,
	                            const int linenumber);
};


class FileNotFoundException : public Exception
{
public:
	FileNotFoundException ( const std::string& filename );
	std::string Filename;
};


class AccessDeniedException : public Exception
{
public:
	AccessDeniedException ( const std::string& filename );
	std::string Filename;
};


class InvalidBuildFileException : public Exception
{
public:
	InvalidBuildFileException ( const char* message,
	                            ...);
protected:
	InvalidBuildFileException ();
};


class XMLSyntaxErrorException : public InvalidBuildFileException
{
public:
	XMLSyntaxErrorException ( const std::string& location,
	                          const char* message,
	                          ... );
};


class RequiredAttributeNotFoundException : public InvalidBuildFileException
{
public:
	RequiredAttributeNotFoundException ( const std::string& attributeName,
	                                     const std::string& elementName );
};


class InvalidAttributeValueException : public InvalidBuildFileException
{
public:
	InvalidAttributeValueException ( const std::string& name,
	                                 const std::string& value );
};


class BackendNameConflictException : public Exception
{
public:
	BackendNameConflictException ( const std::string& name );
};


class UnknownBackendException : public Exception
{
public:
	UnknownBackendException ( const std::string& name );
};

#endif /* __EXCEPTION_H */
