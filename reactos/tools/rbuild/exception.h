#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include <string>

using std::string;

class Exception
{
public:
	Exception(string message);
	Exception(const char* format,
	          ...);
	string Message;
protected:
	Exception();
	void SetMessage(const char* message,
	                va_list args);
};


class FileNotFoundException : public Exception
{
public:
	FileNotFoundException(string filename);
	string Filename;
};


class InvalidBuildFileException : public Exception
{
public:
	InvalidBuildFileException(const char* message,
	                          ...);
};

#endif /* __EXCEPTION_H */
