/*
	Copyright (c) 2008 KJK::Hyperion

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pseh/pseh2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include <wine/test.h>

extern void no_op(void);
extern int return_arg(int);

extern int return_zero(void);
extern int return_positive(void);
extern int return_negative(void);
extern int return_one(void);
extern int return_minusone(void);

extern int return_zero_2(void *);
extern int return_positive_2(void *);
extern int return_negative_2(void *);
extern int return_one_2(void *);
extern int return_minusone_2(void *);

extern int return_zero_3(int);
extern int return_positive_3(int);
extern int return_negative_3(int);
extern int return_one_3(int);
extern int return_minusone_3(int);

extern int return_zero_4(void *, int);
extern int return_positive_4(void *, int);
extern int return_negative_4(void *, int);
extern int return_one_4(void *, int);
extern int return_minusone_4(void *, int);

extern void set_positive(int *);

static int call_test(int (*)(void));

#define DEFINE_TEST(NAME_) static int NAME_(void)

/* Empty statements *///{{{
DEFINE_TEST(test_empty_1)
{
	_SEH2_TRY { } _SEH2_EXCEPT(0) { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_2)
{
	_SEH2_TRY { } _SEH2_EXCEPT(-1) { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_3)
{
	_SEH2_TRY { } _SEH2_EXCEPT(1) { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_4)
{
	_SEH2_TRY { } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_5)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_EXCEPT(0) { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_6)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_EXCEPT(-1) { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_7)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_EXCEPT(1) { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(test_empty_8)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_FINALLY { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}
//}}}

/* Static exception filters *///{{{
DEFINE_TEST(test_execute_handler_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(EXCEPTION_CONTINUE_EXECUTION)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_search_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_EXCEPT(EXCEPTION_CONTINUE_SEARCH)
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_execute_handler_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(12345)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(-12345)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* Dynamic exception filters *///{{{
DEFINE_TEST(test_execute_handler_3)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_one())
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_3)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_minusone())
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_search_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_EXCEPT(return_zero())
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_execute_handler_4)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_positive())
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_4)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_negative())
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* Dynamic exception filters, using _SEH2_GetExceptionInformation() *///{{{
DEFINE_TEST(test_execute_handler_5)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_one_2(_SEH2_GetExceptionInformation()))
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_5)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_minusone_2(_SEH2_GetExceptionInformation()))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_search_3)
{
	static int ret;

	ret = return_positive();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_EXCEPT(return_zero_2(_SEH2_GetExceptionInformation()))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_execute_handler_6)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_positive_2(_SEH2_GetExceptionInformation()))
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_6)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_negative_2(_SEH2_GetExceptionInformation()))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* Dynamic exception filters, using _SEH2_GetExceptionCode() *///{{{
DEFINE_TEST(test_execute_handler_7)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_one_3(_SEH2_GetExceptionCode()))
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_7)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_minusone_3(_SEH2_GetExceptionCode()))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_search_4)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_EXCEPT(return_zero_3(_SEH2_GetExceptionCode()))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_execute_handler_8)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_positive_3(_SEH2_GetExceptionCode()))
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_8)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_negative_3(_SEH2_GetExceptionCode()))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* Dynamic exception filters, using _SEH2_GetExceptionInformation() and _SEH2_GetExceptionCode() *///{{{
DEFINE_TEST(test_execute_handler_9)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_one_4(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_9)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_minusone_4(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_search_5)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_EXCEPT(return_zero_4(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_execute_handler_10)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(return_positive_4(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_10)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_positive();
	}
	_SEH2_EXCEPT(return_negative_4(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* Constant exception filters with side effects *///{{{
DEFINE_TEST(test_execute_handler_11)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(set_positive(&ret), EXCEPTION_EXECUTE_HANDLER)
	{
		ret = ret ? return_positive() : return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_11)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = ret ? return_positive() : return_zero();
	}
	_SEH2_EXCEPT(set_positive(&ret), EXCEPTION_CONTINUE_EXECUTION)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_search_6)
{
	static int ret;
	static int ret2;

	ret = return_zero();
	ret2 = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
			ret2 = return_zero();
		}
		_SEH2_EXCEPT(set_positive(&ret), EXCEPTION_CONTINUE_SEARCH)
		{
			ret = return_zero();
			ret2 = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(set_positive(&ret2), EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
		ret2 = return_arg(ret2);
	}
	_SEH2_END;

	return ret == return_positive() && ret2 == return_positive();
}

DEFINE_TEST(test_execute_handler_12)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(set_positive(&ret), 12345)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_continue_execution_12)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(set_positive(&ret), -12345)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* _SEH2_LEAVE *///{{{
DEFINE_TEST(test_leave_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_positive();
		_SEH2_LEAVE;
		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_leave_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_positive();
		_SEH2_LEAVE;

		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_leave_3)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_positive();

		if(return_one())
			_SEH2_LEAVE;

		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_leave_4)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		int i;
		int n = return_one() + return_one();

		for(i = return_zero(); i < n; ++ i)
		{
			if(i == return_one())
			{
				ret = return_positive();
				_SEH2_LEAVE;
			}
		}

		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_leave_5)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		switch(return_one())
		{
		case 0: ret = return_zero();
		case 1: ret = return_positive(); _SEH2_LEAVE;
		case 2: ret = return_zero();
		}

		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_leave_6)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_LEAVE;
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			ret = return_zero();
		}
		_SEH2_END;

		ret = return_positive();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* _SEH2_YIELD() *///{{{
static
int test_yield_1_helper(void)
{
	_SEH2_TRY
	{
		_SEH2_YIELD(return return_positive());
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		_SEH2_YIELD(return return_zero());
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_yield_1)
{
	return test_yield_1_helper() == return_positive();
}

static
int test_yield_2_helper(void)
{
	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		_SEH2_YIELD(return return_zero());
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		_SEH2_YIELD(return return_positive());
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_yield_2)
{
	return test_yield_2_helper() == return_positive();
}

static
int test_yield_3_helper(void)
{
	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_YIELD(return return_positive());
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			_SEH2_YIELD(return return_zero());
		}
		_SEH2_END;

		_SEH2_YIELD(return return_zero());
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		_SEH2_YIELD(return return_zero());
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_yield_3)
{
	return test_yield_3_helper() == return_positive();
}

static
int test_yield_4_helper(void)
{
	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			_SEH2_YIELD(return return_zero());
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			_SEH2_YIELD(return return_positive());
		}
		_SEH2_END;

		_SEH2_YIELD(return return_zero());
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		_SEH2_YIELD(return return_zero());
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_yield_4)
{
	return test_yield_4_helper() == return_positive();
}

static int test_yield_5_ret;

static
int test_yield_5_helper(void)
{
	test_yield_5_ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_YIELD(return return_positive());
	}
	_SEH2_FINALLY
	{
		test_yield_5_ret = return_positive();
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_yield_5)
{
	return test_yield_5_helper() == return_positive() && test_yield_5_ret == return_positive();
}

int test_yield_6_ret;

static
int test_yield_6_helper(void)
{
	test_yield_6_ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_YIELD(return return_positive());
		}
		_SEH2_FINALLY
		{
			test_yield_6_ret = return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		test_yield_6_ret += return_one();
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_yield_6)
{
	return test_yield_6_helper() == return_positive() && test_yield_6_ret == return_positive() + return_one();
}
//}}}

/* Termination blocks *///{{{
DEFINE_TEST(test_finally_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
	}
	_SEH2_FINALLY
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_finally_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
		_SEH2_LEAVE;
	}
	_SEH2_FINALLY
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_finally_3)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
		_SEH2_YIELD(goto leave);
	}
	_SEH2_FINALLY
	{
		ret = return_positive();
	}
	_SEH2_END;

leave:
	return ret == return_positive();
}

static int test_finally_4_ret;

static int test_finally_4_helper(void)
{
	test_finally_4_ret = return_zero();

	_SEH2_TRY
	{
		test_finally_4_ret = return_arg(test_finally_4_ret);
		_SEH2_YIELD(return return_positive());
	}
	_SEH2_FINALLY
	{
		test_finally_4_ret = return_positive();
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_finally_4)
{
	return test_finally_4_helper() == return_positive() && test_finally_4_ret;
}

DEFINE_TEST(test_finally_5)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_FINALLY
		{
			ret = return_positive();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_finally_6)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			ret = return_arg(ret);
		}
		_SEH2_FINALLY
		{
			if(ret == return_zero())
				ret = return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		if(ret == return_positive())
			ret = return_positive() + return_one();
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_finally_7)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			ret = return_arg(ret);
			_SEH2_LEAVE;
		}
		_SEH2_FINALLY
		{
			if(ret == return_zero())
				ret = return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		if(ret == return_positive())
			ret = return_positive() + return_one();
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_finally_8)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			ret = return_arg(ret);
			_SEH2_YIELD(goto leave);
		}
		_SEH2_FINALLY
		{
			if(ret == return_zero())
				ret = return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		if(ret == return_positive())
			ret = return_positive() + return_one();
	}
	_SEH2_END;

leave:
	return ret == return_positive() + return_one();
}

static int test_finally_9_ret;

static int test_finally_9_helper(void)
{
	test_finally_9_ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			test_finally_9_ret = return_arg(test_finally_9_ret);
			_SEH2_YIELD(return return_positive());
		}
		_SEH2_FINALLY
		{
			if(test_finally_9_ret == return_zero())
				test_finally_9_ret = return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		if(test_finally_9_ret == return_positive())
			test_finally_9_ret = return_positive() + return_one();
	}
	_SEH2_END;

	return return_zero();
}

DEFINE_TEST(test_finally_9)
{
	return test_finally_9_helper() == return_positive() && test_finally_9_ret == return_positive() + return_one();
}

DEFINE_TEST(test_finally_10)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_TRY
			{
				RaiseException(0xE00DEAD0, 0, 0, NULL);
				ret = return_zero();
			}
			_SEH2_FINALLY
			{
				if(ret == return_zero())
					ret = return_positive();
			}
			_SEH2_END;
		}
		_SEH2_FINALLY
		{
			if(ret == return_positive())
				ret = return_positive() + return_one();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_finally_11)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_TRY
			{
				ret = return_arg(ret);
			}
			_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
			{
				ret = return_zero();
			}
			_SEH2_END;
		}
		_SEH2_FINALLY
		{
			ret = return_positive();
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		if(ret == return_positive())
			ret += return_one();
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_finally_12)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_TRY
			{
				ret = return_arg(ret);
			}
			_SEH2_FINALLY
			{
				ret = return_positive();
				RaiseException(0xE00DEAD0, 0, 0, NULL);
				ret = return_zero();
			}
			_SEH2_END;
		}
		_SEH2_FINALLY
		{
			if(ret == return_positive())
				ret += return_one();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		if(ret == return_positive() + return_one())
			ret += return_one();
	}
	_SEH2_END;

	return ret == return_positive() + return_one() + return_one();
}

static int test_finally_13_ret;

static
void test_finally_13_helper(void)
{
	test_finally_13_ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			test_finally_13_ret = return_positive();
			_SEH2_YIELD(return);
			test_finally_13_ret = return_zero();
		}
		_SEH2_FINALLY
		{
			if(test_finally_13_ret == return_positive())
				test_finally_13_ret += return_one();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		if(test_finally_13_ret == return_positive() + return_one())
			test_finally_13_ret += return_one();

		RaiseException(0xE00DEAD0, 0, 0, NULL);
		test_finally_13_ret = return_zero();
	}
	_SEH2_END;

	test_finally_13_ret = return_zero();
}

DEFINE_TEST(test_finally_13)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
		test_finally_13_helper();
		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive() && test_finally_13_ret == return_positive() + return_one() + return_one();
}

static int test_finally_14_ret;

static
void test_finally_14_helper(void)
{
	test_finally_14_ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_TRY
			{
				test_finally_14_ret = return_positive();
				RaiseException(0xE00DEAD0, 0, 0, NULL);
				test_finally_14_ret = return_zero();
			}
			_SEH2_FINALLY
			{
				if(test_finally_14_ret == return_positive())
					test_finally_14_ret += return_one();
			}
			_SEH2_END;
		}
		_SEH2_FINALLY
		{
			if(test_finally_14_ret == return_positive() + return_one())
				test_finally_14_ret += return_one();

			RaiseException(0xE00DEAD0, 0, 0, NULL);
			test_finally_14_ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		if(test_finally_14_ret == return_positive() + return_one() + return_one())
			test_finally_14_ret += return_one();
	}
	_SEH2_END;

	test_finally_14_ret = return_arg(test_finally_14_ret);
}

DEFINE_TEST(test_finally_14)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
		test_finally_14_helper();
		ret = return_positive();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive() && test_finally_14_ret == return_positive() + return_one() + return_one() + return_one();
}
//}}}

/* _SEH2_GetExceptionInformation() *///{{{
static
int verify_xpointers(struct _EXCEPTION_POINTERS * ep, DWORD code, DWORD flags, DWORD argc, const ULONG_PTR * argv, int * ret, int filter)
{
	*ret =
		ep &&
		ep->ExceptionRecord &&
		ep->ContextRecord &&
		ep->ExceptionRecord->ExceptionCode == code &&
		ep->ExceptionRecord->ExceptionFlags == flags &&
		ep->ExceptionRecord->NumberParameters == argc &&
		(argv || !argc) &&
		memcmp(ep->ExceptionRecord->ExceptionInformation, argv, sizeof(argv[0]) * argc) == 0;

	if(*ret)
		*ret = return_positive();

	return filter;
}

DEFINE_TEST(test_xpointers_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, 0, 0, NULL, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, NULL, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_3)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, args);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, args, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_4)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 1, args);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 1, args, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_5)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 2, args);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 2, args, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_6)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 3, args);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 3, args, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_7)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, 0, 0, NULL, &ret, EXCEPTION_CONTINUE_EXECUTION))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_8)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, args);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, 0, 0, args, &ret, EXCEPTION_CONTINUE_EXECUTION))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_9)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 1, args);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, 0, 1, args, &ret, EXCEPTION_CONTINUE_EXECUTION))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_10)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 2, args);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, 0, 2, args, &ret, EXCEPTION_CONTINUE_EXECUTION))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_11)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 3, args);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, 0, 3, args, &ret, EXCEPTION_CONTINUE_EXECUTION))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_12)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, NULL);
		}
		_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, NULL, &ret, EXCEPTION_CONTINUE_SEARCH))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_13)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, args);
		}
		_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 0, args, &ret, EXCEPTION_CONTINUE_SEARCH))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_14)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 1, args);
		}
		_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 1, args, &ret, EXCEPTION_CONTINUE_SEARCH))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_15)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 2, args);
		}
		_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 2, args, &ret, EXCEPTION_CONTINUE_SEARCH))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xpointers_16)
{
	static int ret;
	static const ULONG_PTR args[] = { 1, 2, 12345 };

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 3, args);
		}
		_SEH2_EXCEPT(verify_xpointers(_SEH2_GetExceptionInformation(), 0xE00DEAD0, EXCEPTION_NONCONTINUABLE, 3, args, &ret, EXCEPTION_CONTINUE_SEARCH))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* _SEH2_GetExceptionCode() *///{{{
static
int verify_xcode(int code, int xcode, int * ret, int filter)
{
	*ret = code == xcode;

	if(*ret)
		*ret = return_positive();

	return filter;
}

DEFINE_TEST(test_xcode_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_zero();
	}
	_SEH2_EXCEPT(verify_xcode(_SEH2_GetExceptionCode(), 0xE00DEAD0, &ret, EXCEPTION_EXECUTE_HANDLER))
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xcode_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
		ret = return_arg(ret);
	}
	_SEH2_EXCEPT(verify_xcode(_SEH2_GetExceptionCode(), 0xE00DEAD0, &ret, EXCEPTION_CONTINUE_EXECUTION))
	{
		ret = return_zero();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_xcode_3)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_EXCEPT(verify_xcode(_SEH2_GetExceptionCode(), 0xE00DEAD0, &ret, EXCEPTION_CONTINUE_SEARCH))
		{
			ret = return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}
//}}}

/* _SEH2_AbnormalTermination() *///{{{
DEFINE_TEST(test_abnorm_1)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
	}
	_SEH2_FINALLY
	{
		ret = _SEH2_AbnormalTermination() ? return_zero() : return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_abnorm_2)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_LEAVE;
	}
	_SEH2_FINALLY
	{
		ret = _SEH2_AbnormalTermination() ? return_zero() : return_positive();
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_abnorm_3)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_YIELD(goto leave);
	}
	_SEH2_FINALLY
	{
		ret = _SEH2_AbnormalTermination() ? return_positive() : return_zero();
	}
	_SEH2_END;

leave:
	return ret == return_positive();
}

DEFINE_TEST(test_abnorm_4)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
			ret = return_zero();
		}
		_SEH2_FINALLY
		{
			ret = _SEH2_AbnormalTermination() ? return_positive() : return_zero();
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive();
}

DEFINE_TEST(test_abnorm_5)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			ret = return_arg(ret);
		}
		_SEH2_FINALLY
		{
			ret = _SEH2_AbnormalTermination() ? return_zero() : return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		ret = ret == return_positive() && !_SEH2_AbnormalTermination() ? return_positive() + return_one() : ret;
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_abnorm_6)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_LEAVE;
		}
		_SEH2_FINALLY
		{
			ret = _SEH2_AbnormalTermination() ? return_zero() : return_positive();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		ret = ret == return_positive() && !_SEH2_AbnormalTermination() ? return_positive() + return_one() : ret;
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_abnorm_7)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_YIELD(goto leave);
		}
		_SEH2_FINALLY
		{
			ret = _SEH2_AbnormalTermination() ? return_positive() : return_zero();
		}
		_SEH2_END;
	}
	_SEH2_FINALLY
	{
		ret = ret == return_positive() && _SEH2_AbnormalTermination() ? return_positive() + return_one() : ret;
	}
	_SEH2_END;

leave:
	return ret == return_positive() + return_one();
}

DEFINE_TEST(test_abnorm_8)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			_SEH2_TRY
			{
				RaiseException(0xE00DEAD0, 0, 0, NULL);
				ret = return_zero();
			}
			_SEH2_FINALLY
			{
				ret = _SEH2_AbnormalTermination() ? return_positive() : return_zero();
			}
			_SEH2_END;
		}
		_SEH2_FINALLY
		{
			ret = ret == return_positive() && _SEH2_AbnormalTermination() ? return_positive() + return_one() : ret;
		}
		_SEH2_END;
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_arg(ret);
	}
	_SEH2_END;

	return ret == return_positive() + return_one();
}
//}}}

/* System support *///{{{
// TODO
//}}}

/* CPU faults *///{{{
// TODO
//}}}

/* Past bugs, to detect regressions *///{{{
/* #4004: volatile registers clobbered when catching across frames (originally misreported) *///{{{
static
void test_bug_4004_helper_1(void)
{
	int i1, i2, i3;

	i1 = return_positive();
	i2 = return_positive();
	i3 = return_positive();
	(void)return_arg(i1 + i2 + i3);

	_SEH2_TRY
	{
		RaiseException(0xE00DEAD0, 0, 0, NULL);
	}
	_SEH2_FINALLY
	{
	}
	_SEH2_END;
}

static
void test_bug_4004_helper_2(void)
{
	_SEH2_TRY
	{
		test_bug_4004_helper_1();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	}
	_SEH2_END;
}

DEFINE_TEST(test_bug_4004)
{
	int i1, i2, i3;

	i1 = return_positive();
	i2 = return_positive();
	i3 = return_positive();

	test_bug_4004_helper_2();

	return return_arg(i1) + return_arg(i2) + return_arg(i3) == return_positive() * 3;
}
//}}}

/* #4663: *///{{{
DEFINE_TEST(test_bug_4663)
{
	int i1, i2;

	i1 = return_zero();
	i2 = return_zero();

	_SEH2_TRY
	{
		_SEH2_TRY
		{
			RaiseException(0xE00DEAD0, 0, 0, NULL);
		}
		_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
		{
			if (i1 == return_zero())
			{
				i1 = return_one();
			}
		}
		_SEH2_END;

		if (i1 == return_one())
		{
			i1 = return_minusone();
			RaiseException(0xE00DEAD0, 0, 0, NULL);
		}
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		i2 = return_one();
	}
	_SEH2_END;

	return ((i1 == return_minusone()) && (i2 == return_one()));
}
//}}}
//}}}

static
LONG WINAPI unhandled_exception(PEXCEPTION_POINTERS ExceptionInfo)
{
	ok(0, "unhandled exception %08lX thrown from %p\n", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress);
	return EXCEPTION_CONTINUE_SEARCH;
}

#if defined(_M_IX86)
struct volatile_context
{
	void * esp;
	void * ebp;
	void * ebx;
	void * esi;
	void * edi;
};
#else
struct volatile_context
{
	int _ignore;
};
#endif

static
DECLSPEC_NOINLINE
int sanity_check(int ret, struct volatile_context * before, struct volatile_context * after)
{
	if(ret && memcmp(before, after, sizeof(before)))
	{
		ok(0, "volatile context corrupted\n");
		ret = 0;
	}

	return ret;
}

static
int passthrough_handler(struct _EXCEPTION_RECORD * e, void * f, struct _CONTEXT * c, void * d)
{
	return ExceptionContinueSearch;
}

static
DECLSPEC_NOINLINE
int call_test(int (* func)(void))
{
	static int ret;
	static struct volatile_context before, after;
	static LPTOP_LEVEL_EXCEPTION_FILTER prev_unhandled_exception;
	static _SEH2Registration_t * prev_frame;
	_SEH2Registration_t passthrough_frame;

	prev_unhandled_exception = SetUnhandledExceptionFilter(&unhandled_exception);

#if defined(_X86_)
	prev_frame = (_SEH2Registration_t *)__readfsdword(0);
	passthrough_frame.SER_Prev = prev_frame;
	passthrough_frame.SER_Handler = passthrough_handler;
	__writefsdword(0, (unsigned long)&passthrough_frame);
#endif

#if defined(__GNUC__) && defined(__i386__)
	__asm__ __volatile__
	(
		"mov %%esp, 0x00 + %c[before]\n"
		"mov %%ebp, 0x04 + %c[before]\n"
		"mov %%ebx, 0x08 + %c[before]\n"
		"mov %%esi, 0x0c + %c[before]\n"
		"mov %%edi, 0x10 + %c[before]\n"
		"call *%[test]\n"
		"mov %%esp, 0x00 + %c[after]\n"
		"mov %%ebp, 0x04 + %c[after]\n"
		"mov %%ebx, 0x08 + %c[after]\n"
		"mov %%esi, 0x0c + %c[after]\n"
		"mov %%edi, 0x10 + %c[after]\n"
		"push %[after]\n"
		"push %[before]\n"
		"push %[ret]\n"
		"call %c[sanity_check]\n"
		"pop %%ecx\n"
		"pop %%ecx\n"
		"pop %%ecx\n" :
		[ret] "=a" (ret) :
		[test] "r" (func), [before] "i" (&before), [after] "i" (&after), [sanity_check] "i" (&sanity_check) :
		"ebx", "ecx", "edx", "esi", "edi", "flags", "memory"
	);
#else
	ret = func();
#endif

#if defined(_X86_)
	if((_SEH2Registration_t *)__readfsdword(0) != &passthrough_frame || passthrough_frame.SER_Prev != prev_frame)
	{
		ok(0, "exception registration list corrupted\n");
		ret = 0;
	}
	else
		__writefsdword(0, (unsigned long)prev_frame);
#endif

	SetUnhandledExceptionFilter(prev_unhandled_exception);
	return ret;
}

#define USE_TEST_NAME_(NAME_) # NAME_
#define USE_TEST_NAME(NAME_) USE_TEST_NAME_(NAME_)
#define USE_TEST(NAME_) { USE_TEST_NAME(NAME_), NAME_ }

struct subtest
{
	const char * name;
	int (* func)(void);
};

void testsuite_syntax(void)
{
	const struct subtest testsuite[] =
	{
		USE_TEST(test_empty_1),
		USE_TEST(test_empty_2),
		USE_TEST(test_empty_3),
		USE_TEST(test_empty_4),
		USE_TEST(test_empty_5),
		USE_TEST(test_empty_6),
		USE_TEST(test_empty_7),
		USE_TEST(test_empty_8),

		USE_TEST(test_execute_handler_1),
		USE_TEST(test_continue_execution_1),
		USE_TEST(test_continue_search_1),
		USE_TEST(test_execute_handler_2),
		USE_TEST(test_continue_execution_2),

		USE_TEST(test_execute_handler_3),
		USE_TEST(test_continue_execution_3),
		USE_TEST(test_continue_search_2),
		USE_TEST(test_execute_handler_4),
		USE_TEST(test_continue_execution_4),

		USE_TEST(test_execute_handler_5),
		USE_TEST(test_continue_execution_5),
		USE_TEST(test_continue_search_3),
		USE_TEST(test_execute_handler_6),
		USE_TEST(test_continue_execution_6),

		USE_TEST(test_execute_handler_7),
		USE_TEST(test_continue_execution_7),
		USE_TEST(test_continue_search_4),
		USE_TEST(test_execute_handler_8),
		USE_TEST(test_continue_execution_8),

		USE_TEST(test_execute_handler_9),
		USE_TEST(test_continue_execution_9),
		USE_TEST(test_continue_search_5),
		USE_TEST(test_execute_handler_10),
		USE_TEST(test_continue_execution_10),

		USE_TEST(test_execute_handler_11),
		USE_TEST(test_continue_execution_11),
		USE_TEST(test_continue_search_6),
		USE_TEST(test_execute_handler_12),
		USE_TEST(test_continue_execution_12),

		USE_TEST(test_leave_1),
		USE_TEST(test_leave_2),
		USE_TEST(test_leave_3),
		USE_TEST(test_leave_4),
		USE_TEST(test_leave_5),
		USE_TEST(test_leave_6),

		USE_TEST(test_yield_1),
		USE_TEST(test_yield_2),
		USE_TEST(test_yield_3),
		USE_TEST(test_yield_4),
		USE_TEST(test_yield_5),
		USE_TEST(test_yield_6),

		USE_TEST(test_finally_1),
		USE_TEST(test_finally_2),
		USE_TEST(test_finally_3),
		USE_TEST(test_finally_4),
		USE_TEST(test_finally_5),
		USE_TEST(test_finally_6),
		USE_TEST(test_finally_7),
		USE_TEST(test_finally_8),
		USE_TEST(test_finally_9),
		USE_TEST(test_finally_10),
		USE_TEST(test_finally_11),
		USE_TEST(test_finally_12),
		USE_TEST(test_finally_13),
		USE_TEST(test_finally_14),

		USE_TEST(test_xpointers_1),
		USE_TEST(test_xpointers_2),
		USE_TEST(test_xpointers_3),
		USE_TEST(test_xpointers_4),
		USE_TEST(test_xpointers_5),
		USE_TEST(test_xpointers_6),
		USE_TEST(test_xpointers_7),
		USE_TEST(test_xpointers_8),
		USE_TEST(test_xpointers_9),
		USE_TEST(test_xpointers_10),
		USE_TEST(test_xpointers_11),
		USE_TEST(test_xpointers_12),
		USE_TEST(test_xpointers_13),
		USE_TEST(test_xpointers_14),
		USE_TEST(test_xpointers_15),
		USE_TEST(test_xpointers_16),

		USE_TEST(test_xcode_1),
		USE_TEST(test_xcode_2),
		USE_TEST(test_xcode_3),

		USE_TEST(test_abnorm_1),
		USE_TEST(test_abnorm_2),
		USE_TEST(test_abnorm_3),
		USE_TEST(test_abnorm_4),
		USE_TEST(test_abnorm_5),
		USE_TEST(test_abnorm_6),
		USE_TEST(test_abnorm_7),
		USE_TEST(test_abnorm_8),

		USE_TEST(test_bug_4004),
		USE_TEST(test_bug_4663),
	};

	size_t i;

	for(i = 0; i < sizeof(testsuite) / sizeof(testsuite[0]); ++ i)
	{
		//printf("%s\n", testsuite[i].name);
		ok(call_test(testsuite[i].func), "%s failed\n", testsuite[i].name);
	}
}

const struct test winetest_testlist[] = {
	{ "pseh2_syntax", testsuite_syntax },
	{ 0, 0 }
};

/* EOF */
