#include "rbuild.h"
#include "test.h"

BaseTest::BaseTest()
{
	Failed = true;
}

BaseTest::~BaseTest()
{
}

void BaseTest::Assert(char *message)
{
	printf(message);
	Fail();
}

void BaseTest::IsTrue(bool condition)
{
	if (!condition)
	{
		char message[100];
		sprintf(message,
		        "Condition was not true at %s:%d",
		        __FILE__,
		        __LINE__);
		Assert(message);
	}
}

void BaseTest::IsFalse(bool condition)
{
	if (condition)
	{
		char message[100];
		sprintf(message,
		        "Condition was not false at %s:%d",
		        __FILE__,
		        __LINE__);
		Assert(message);
	}
}

void BaseTest::AreEqual(int expected,
                        int actual)
{
	if (actual != expected)
	{
		char message[100];
		sprintf(message,
		        "Expected %d/0x%.08x was %d/0x%.08x at %s:%d",
		        expected,
		        expected,
		        actual,
		        actual,
		        __FILE__,
		        __LINE__);
		Assert(message);
	}
}

void BaseTest::AreNotEqual(int expected,
                           int actual)
{
	if (actual == expected)
	{
		char message[100];
		sprintf(message,
		        "Actual value expected to be different from %d/0x%.08x at %s:%d",
		        expected,
		        expected,
		        __FILE__,
		        __LINE__);
		Assert(message);
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
		BaseTestList tests = GetTests();
		for (size_t i = 0; i < tests.size(); i++)
		{
			BaseTest& test = *tests[i];
			/*test.Run();*/
			if (test.Failed)
				numberOfFailedTests++;
		}
		
		if (numberOfFailedTests > 0)
			printf("%d tests failed",
			       numberOfFailedTests);
		else
			printf("All tests succeeded");
	}

private:
	BaseTestList GetTests()
	{
		BaseTestList tests;
		tests.push_back(new ModuleTest());
		return tests;
	}
};


int main(int argc,
         char** argv)
{
	TestDispatcher testDispatcher = TestDispatcher();
	testDispatcher.Run();
	return 0;
};
