#ifndef __TEST_H
#define __TEST_H

#include "rbuild.h"

class BaseTest
{
public:
	bool Failed;
	BaseTest();
	virtual ~BaseTest();
	virtual void Run() = 0;
protected:
	void Assert(const char *message, ...);
	void IsTrue(bool condition,
	            const char* file,
	            int line);
	void IsFalse(bool condition,
	             const char* file,
	             int line);
	void AreEqual(int expected,
	              int actual,
	              const char* file,
	              int line);
	void AreEqual(const std::string& expected,
	              const std::string& actual,
	              const char* file,
	              int line);
	void AreNotEqual(int expected,
	                 int actual,
	                 const char* file,
	                 int line);
private:
	void Fail();
};

#define IS_TRUE(condition) IsTrue(condition,__FILE__,__LINE__)
#define IS_FALSE(condition) IsFalse(condition,__FILE__,__LINE__)
#define ARE_EQUAL(expected,actual) AreEqual(expected,actual,__FILE__,__LINE__)
#define ARE_NOT_EQUAL(expected,actual) AreNotEqual(expected,actual,__FILE__,__LINE__)

class ProjectTest : public BaseTest
{
public:
	void Run();
};


class ModuleTest : public BaseTest
{
public:
	void Run();
};


class DefineTest : public BaseTest
{
public:
	void Run();
};


class IncludeTest : public BaseTest
{
public:
	void Run();
};

#endif /* __TEST_H */
