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
	void Assert(char *message);
	void IsTrue(bool condition);
	void IsFalse(bool condition);
	void AreEqual(int expected,
	              int actual);
	void AreNotEqual(int expected,
	                 int actual);
private:
	void Fail();
};


class ModuleTest : public BaseTest
{
public:
	void Run();
};

#endif /* __TEST_H */
