#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include "pch.h"

class Exception
{
public:
	Exception(const std::string& message);
	Exception(const char* format,
	          ...);
	std::string Message;
protected:
	Exception();
	void SetMessage(const char* message,
	                va_list args);
};


class FileNotFoundException : public Exception
{
public:
	FileNotFoundException(const std::string& filename);
	std::string Filename;
};


class AccessDeniedException : public Exception
{
public:
	AccessDeniedException(const std::string& filename);
	std::string Filename;
};


class InvalidBuildFileException : public Exception
{
public:
	InvalidBuildFileException(const char* message,
	                          ...);
};


class RequiredAttributeNotFoundException : public InvalidBuildFileException
{
public:
	RequiredAttributeNotFoundException(const std::string& attributeName,
	                                   const std::string& elementName);
};

#endif /* __EXCEPTION_H */
