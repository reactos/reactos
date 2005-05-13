#ifndef __TEST_H
#define __TEST_H

#include "rbuild.h"
#include "backend/mingw/mingw.h"

#define RBUILD_BASE "tools" SSEP "rbuild" SSEP
	
class BaseTest
{
public:
	bool Failed;
	BaseTest ();
	virtual ~BaseTest ();
	virtual void Run () = 0;
protected:
	void Assert ( const char *message,
	              ... );
	void IsNull ( void* reference,
	              const char* file,
	              int line );
	void IsNotNull ( void* reference,
	                 const char* file,
	                 int line );
	void IsTrue ( bool condition,
	              const char* file,
	              int line );
	void IsFalse ( bool condition,
	               const char* file,
	               int line );
	void AreEqual ( int expected,
	                int actual,
	                const char* file,
	                int line );
	void AreEqual ( const std::string& expected,
	                const std::string& actual,
	                const char* file,
	                int line );
	void AreNotEqual ( int expected,
	                   int actual,
	                   const char* file,
	                   int line );
private:
	void Fail ();
};

#define IS_NULL(reference) IsNull((void*)reference,__FILE__,__LINE__)
#define IS_NOT_NULL(reference) IsNotNull((void*)reference,__FILE__,__LINE__)
#define IS_TRUE(condition) IsTrue(condition,__FILE__,__LINE__)
#define IS_FALSE(condition) IsFalse(condition,__FILE__,__LINE__)
#define ARE_EQUAL(expected,actual) AreEqual(expected,actual,__FILE__,__LINE__)
#define ARE_NOT_EQUAL(expected,actual) AreNotEqual(expected,actual,__FILE__,__LINE__)

class ProjectTest : public BaseTest
{
public:
	void Run ();
};


class ModuleTest : public BaseTest
{
public:
	void Run ();
};


class DefineTest : public BaseTest
{
public:
	void Run ();
};


class IncludeTest : public BaseTest
{
public:
	void Run ();
};


class InvokeTest : public BaseTest
{
public:
	void Run ();
};


class LinkerFlagTest : public BaseTest
{
public:
	void Run ();
};


class IfTest : public BaseTest
{
public:
	void Run ();
private:
	void TestProjectIf ( Project& project );
	void TestModuleIf ( Project& project );
};


class FunctionTest : public BaseTest
{
public:
	void Run ();
};


class SourceFileTest : public BaseTest
{
public:
	void Run ();
	void IncludeTest ();
	void FullParseTest ();
private:
	bool IsParentOf ( const SourceFile* parent,
	                  const SourceFile* child );

};


class CDFileTest : public BaseTest
{
public:
	void Run ();
};


class SymbolTest : public BaseTest
{
public:
	void Run ();
};

#endif /* __TEST_H */
