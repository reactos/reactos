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
	                  va_list args );
};


class InvalidOperationException : public Exception
{
public:
	InvalidOperationException ( const char* filename,
	                            const int linenumber);
	InvalidOperationException ( const char* filename,
	                            const int linenumber,
	                            const char* message,
	                            ... );
};


class OutOfMemoryException : public Exception
{
public:
	OutOfMemoryException ();
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
	InvalidBuildFileException ( const std::string& location,
	                            const char* message,
	                            ...);
	void SetLocationMessage ( const std::string& location,
	                          const char* message,
	                          va_list args );
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
	RequiredAttributeNotFoundException ( const std::string& location,
	                                     const std::string& attributeName,
	                                     const std::string& elementName );
};


class InvalidAttributeValueException : public InvalidBuildFileException
{
public:
	InvalidAttributeValueException ( const std::string& location,
	                                 const std::string& name,
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

class UnknownModuleTypeException : public InvalidBuildFileException
{
public:
	UnknownModuleTypeException ( const std::string& location,
	                             int moduletype );
};


class InvocationFailedException : public Exception
{
public:
	InvocationFailedException ( const std::string& command,
	                            int exitcode );
	std::string Command;
	int ExitCode;
};

#endif /* __EXCEPTION_H */
