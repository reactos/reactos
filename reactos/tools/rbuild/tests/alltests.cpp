/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "pch.h"

#include "rbuild.h"
#include "test.h"

BaseTest::BaseTest()
{
	Failed = false;
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

void BaseTest::IsNull(void* reference,
                      const char* file,
                      int line)
{
	if (reference != NULL)
	{
		Assert("Condition was not NULL at %s:%d\n",
		       file,
		       line);
	}
}

void BaseTest::IsNotNull(void* reference,
                         const char* file,
                         int line)
{
	if (reference == NULL)
	{
		Assert("Condition was NULL at %s:%d\n",
		       file,
		       line);
	}
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

void BaseTest::AreEqual(const std::string& expected,
                        const std::string& actual,
                        const char* file,
                        int line)
{
	if (actual != expected)
	{
		Assert("Expected '%s' was '%s' at %s:%d\n",
		       expected.c_str(),
		       actual.c_str(),
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

class BaseTestList : public std::vector<BaseTest*>
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
			try
			{
				BaseTest& test = *tests[i];
				test.Run();
				if (test.Failed)
					numberOfFailedTests++;
			}
			catch ( Exception& ex )
			{
				printf ( "%s\n", (*ex).c_str () );
				numberOfFailedTests++;
			}
			catch ( XMLException& ex )
			{
				printf ( "%s\n", (*ex).c_str () );
				numberOfFailedTests++;
			}
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
		tests.push_back(new ProjectTest());
		tests.push_back(new ModuleTest());
		tests.push_back(new DefineTest());
		tests.push_back(new IncludeTest());
		tests.push_back(new InvokeTest());
		tests.push_back(new LinkerFlagTest());
		tests.push_back(new IfTest());
		tests.push_back(new FunctionTest());
		tests.push_back(new SourceFileTest());
		tests.push_back(new CDFileTest());
		tests.push_back(new SymbolTest());
		tests.push_back(new CompilationUnitTest());
	}
};


int main(int argc,
         char** argv)
{
	InitializeEnvironment ();
	TestDispatcher testDispatcher;
	testDispatcher.Run();
	return 0;
};
