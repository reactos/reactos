#include <stdarg.h>
#include "rbuild.h"
#include "test.h"

BaseTest::BaseTest()
{
	Failed = true;
}

BaseTest::~BaseTest()
{
}

void BaseTest::Assert(const char *message, ...)
{
	va_list args;
	va_start ( args, message );
	vprintf(message, args);
	va_end ( args );
	Fail();
}

void BaseTest::IsTrue(bool condition,
                      const char* file,
                      int line)
{
	if (!condition)
	{
		Assert("Condition was not true at %s:%d\n",
		       file,
		       line);
	}
}

void BaseTest::IsFalse(bool condition,
                       const char* file,
                       int line)
{
	if (condition)
	{
		Assert("Condition was not false at %s:%d\n",
		       file,
		       line);
	}
}

void BaseTest::AreEqual(int expected,
                        int actual,
                        const char* file,
                        int line)
{
	if (actual != expected)
	{
		Assert("Expected %d/0x%.08x was %d/0x%.08x at %s:%d\n",
		       expected,
		       expected,
		       actual,
		       actual,
		       file,
		       line);
	}
}

void BaseTest::AreNotEqual(int expected,
                           int actual,
                           const char* file,
                           int line)
{
	if (actual == expected)
	{
		Assert("Actual value expected to be different from %d/0x%.08x at %s:%d\n",
		       expected,
		       expected,
		       file,
		       line);
	}
}

void BaseTest::Fail()
{
	Failed = true;
}

class BaseTestList : public vector<BaseTest*>
{
public:
	~BaseTestList()
	{
		for ( size_t i = 0; i < size(); i++ )
		{
			delete (*this)[i];
		}
	}
};

class TestDispatcher
{
public:
	void Run()
	{
		int numberOfFailedTests = 0;
		BaseTestList tests;
		GetTests(tests);
		for (size_t i = 0; i < tests.size(); i++)
		{
			BaseTest& test = *tests[i];
			test.Run();
			if (test.Failed)
				numberOfFailedTests++;
		}
		
		if (numberOfFailedTests > 0)
			printf("%d tests failed\n",
			       numberOfFailedTests);
		else
			printf("All tests succeeded\n");
	}

private:
	void GetTests ( BaseTestList& tests )
	{
		tests.push_back(new ModuleTest());
	}
};


int main(int argc,
         char** argv)
{
	TestDispatcher testDispatcher;
	testDispatcher.Run();
	return 0;
};
