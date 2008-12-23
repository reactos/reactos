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

#define DEFINE_TEST(NAME_) \
	static int test_ ## NAME_(void); \
 \
	static void NAME_(void) {  ok(call_test(test_ ## NAME_), "test failed\n"); } \
 \
	static int test_ ## NAME_(void)

/* Empty statements *///{{{
DEFINE_TEST(empty_1)
{
	_SEH2_TRY { } _SEH2_EXCEPT(0) { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_2)
{
	_SEH2_TRY { } _SEH2_EXCEPT(-1) { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_3)
{
	_SEH2_TRY { } _SEH2_EXCEPT(1) { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_4)
{
	_SEH2_TRY { } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_5)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_EXCEPT(0) { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_6)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_EXCEPT(-1) { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_7)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_EXCEPT(1) { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}

DEFINE_TEST(empty_8)
{
	_SEH2_TRY { _SEH2_TRY { } _SEH2_FINALLY { } _SEH2_END; } _SEH2_FINALLY { } _SEH2_END;
	return 1;
}
//}}}

/* Static exception filters *///{{{
DEFINE_TEST(execute_handler_1)
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

DEFINE_TEST(continue_execution_1)
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

DEFINE_TEST(continue_search_1)
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

DEFINE_TEST(execute_handler_2)
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

DEFINE_TEST(continue_execution_2)
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
DEFINE_TEST(execute_handler_3)
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

DEFINE_TEST(continue_execution_3)
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

DEFINE_TEST(continue_search_2)
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

DEFINE_TEST(execute_handler_4)
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

DEFINE_TEST(continue_execution_4)
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
DEFINE_TEST(execute_handler_5)
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

DEFINE_TEST(continue_execution_5)
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

DEFINE_TEST(continue_search_3)
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

DEFINE_TEST(execute_handler_6)
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

DEFINE_TEST(continue_execution_6)
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
DEFINE_TEST(execute_handler_7)
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

DEFINE_TEST(continue_execution_7)
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

DEFINE_TEST(continue_search_4)
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

DEFINE_TEST(execute_handler_8)
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

DEFINE_TEST(continue_execution_8)
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
DEFINE_TEST(execute_handler_9)
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

DEFINE_TEST(continue_execution_9)
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

DEFINE_TEST(continue_search_5)
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

DEFINE_TEST(execute_handler_10)
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

DEFINE_TEST(continue_execution_10)
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
DEFINE_TEST(execute_handler_11)
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

DEFINE_TEST(continue_execution_11)
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

DEFINE_TEST(continue_search_6)
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

DEFINE_TEST(execute_handler_12)
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

DEFINE_TEST(continue_execution_12)
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
DEFINE_TEST(leave_1)
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

DEFINE_TEST(leave_2)
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

DEFINE_TEST(leave_3)
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

DEFINE_TEST(leave_4)
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

DEFINE_TEST(leave_5)
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

DEFINE_TEST(leave_6)
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

DEFINE_TEST(yield_1)
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

DEFINE_TEST(yield_2)
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

DEFINE_TEST(yield_3)
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

DEFINE_TEST(yield_4)
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

DEFINE_TEST(yield_5)
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

DEFINE_TEST(yield_6)
{
	return test_yield_6_helper() == return_positive() && test_yield_6_ret == return_positive() + return_one();
}
//}}}

/* Termination blocks *///{{{
DEFINE_TEST(finally_1)
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

DEFINE_TEST(finally_2)
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

DEFINE_TEST(finally_3)
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

DEFINE_TEST(finally_4)
{
	return test_finally_4_helper() == return_positive() && test_finally_4_ret;
}

DEFINE_TEST(finally_5)
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

DEFINE_TEST(finally_6)
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

DEFINE_TEST(finally_7)
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

DEFINE_TEST(finally_8)
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

DEFINE_TEST(finally_9)
{
	return test_finally_9_helper() == return_positive() && test_finally_9_ret == return_positive() + return_one();
}

DEFINE_TEST(finally_10)
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

DEFINE_TEST(finally_11)
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

DEFINE_TEST(finally_12)
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

#if 0
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

DEFINE_TEST(finally_13)
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
		test_finally_14_ret = return_zero();
	}
	_SEH2_END;

	test_finally_14_ret = return_zero();
}

DEFINE_TEST(finally_14)
{
	static int ret;

	ret = return_zero();

	_SEH2_TRY
	{
		ret = return_arg(ret);
		test_finally_14_helper();
		ret = return_zero();
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = return_positive();
	}
	_SEH2_END;

	return ret == return_positive() && test_finally_14_ret == return_positive() + return_one() + return_one();
}
#endif
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

DEFINE_TEST(xpointers_1)
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

DEFINE_TEST(xpointers_2)
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

DEFINE_TEST(xpointers_3)
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

DEFINE_TEST(xpointers_4)
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

DEFINE_TEST(xpointers_5)
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

DEFINE_TEST(xpointers_6)
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

DEFINE_TEST(xpointers_7)
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

DEFINE_TEST(xpointers_8)
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

DEFINE_TEST(xpointers_9)
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

DEFINE_TEST(xpointers_10)
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

DEFINE_TEST(xpointers_11)
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

DEFINE_TEST(xpointers_12)
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

DEFINE_TEST(xpointers_13)
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

DEFINE_TEST(xpointers_14)
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

DEFINE_TEST(xpointers_15)
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

DEFINE_TEST(xpointers_16)
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

DEFINE_TEST(xcode_1)
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

DEFINE_TEST(xcode_2)
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

DEFINE_TEST(xcode_3)
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
DEFINE_TEST(abnorm_1)
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

DEFINE_TEST(abnorm_2)
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

DEFINE_TEST(abnorm_3)
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

DEFINE_TEST(abnorm_4)
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

DEFINE_TEST(abnorm_5)
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

DEFINE_TEST(abnorm_6)
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

DEFINE_TEST(abnorm_7)
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

DEFINE_TEST(abnorm_8)
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

#define USE_TEST_NAME_(NAME_) # NAME_
#define USE_TEST_NAME(NAME_) USE_TEST_NAME_(NAME_)
#define USE_TEST(NAME_) { USE_TEST_NAME(NAME_), NAME_ }

const struct test winetest_testlist[] =
{
	USE_TEST(empty_1),
	USE_TEST(empty_2),
	USE_TEST(empty_3),
	USE_TEST(empty_4),
	USE_TEST(empty_5),
	USE_TEST(empty_6),
	USE_TEST(empty_7),
	USE_TEST(empty_8),

	USE_TEST(execute_handler_1),
	USE_TEST(continue_execution_1),
	USE_TEST(continue_search_1),
	USE_TEST(execute_handler_2),
	USE_TEST(continue_execution_2),

	USE_TEST(execute_handler_3),
	USE_TEST(continue_execution_3),
	USE_TEST(continue_search_2),
	USE_TEST(execute_handler_4),
	USE_TEST(continue_execution_4),

	USE_TEST(execute_handler_5),
	USE_TEST(continue_execution_5),
	USE_TEST(continue_search_3),
	USE_TEST(execute_handler_6),
	USE_TEST(continue_execution_6),

	USE_TEST(execute_handler_7),
	USE_TEST(continue_execution_7),
	USE_TEST(continue_search_4),
	USE_TEST(execute_handler_8),
	USE_TEST(continue_execution_8),

	USE_TEST(execute_handler_9),
	USE_TEST(continue_execution_9),
	USE_TEST(continue_search_5),
	USE_TEST(execute_handler_10),
	USE_TEST(continue_execution_10),

	USE_TEST(execute_handler_11),
	USE_TEST(continue_execution_11),
	USE_TEST(continue_search_6),
	USE_TEST(execute_handler_12),
	USE_TEST(continue_execution_12),

	USE_TEST(leave_1),
	USE_TEST(leave_2),
	USE_TEST(leave_3),
	USE_TEST(leave_4),
	USE_TEST(leave_5),
	USE_TEST(leave_6),

	USE_TEST(yield_1),
	USE_TEST(yield_2),
	USE_TEST(yield_3),
	USE_TEST(yield_4),
	USE_TEST(yield_5),
	USE_TEST(yield_6),

	USE_TEST(finally_1),
	USE_TEST(finally_2),
	USE_TEST(finally_3),
	USE_TEST(finally_4),
	USE_TEST(finally_5),
	USE_TEST(finally_6),
	USE_TEST(finally_7),
	USE_TEST(finally_8),
	USE_TEST(finally_9),
	USE_TEST(finally_10),
	USE_TEST(finally_11),
	USE_TEST(finally_12),
#if 0
	USE_TEST(finally_13),
	USE_TEST(finally_14),
#endif

	USE_TEST(xpointers_1),
	USE_TEST(xpointers_2),
	USE_TEST(xpointers_3),
	USE_TEST(xpointers_4),
	USE_TEST(xpointers_5),
	USE_TEST(xpointers_6),
	USE_TEST(xpointers_7),
	USE_TEST(xpointers_8),
	USE_TEST(xpointers_9),
	USE_TEST(xpointers_10),
	USE_TEST(xpointers_11),
	USE_TEST(xpointers_12),
	USE_TEST(xpointers_13),
	USE_TEST(xpointers_14),
	USE_TEST(xpointers_15),
	USE_TEST(xpointers_16),

	USE_TEST(xcode_1),
	USE_TEST(xcode_2),
	USE_TEST(xcode_3),

	USE_TEST(abnorm_1),
	USE_TEST(abnorm_2),
	USE_TEST(abnorm_3),
	USE_TEST(abnorm_4),
	USE_TEST(abnorm_5),
	USE_TEST(abnorm_6),
	USE_TEST(abnorm_7),
	USE_TEST(abnorm_8),

	{ 0, 0 }
};

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
#error TODO
#endif

#if defined(__GNUC__)
static
__attribute__((noinline))
int sanity_check(int ret, struct volatile_context * before, struct volatile_context * after)
{
	if(ret && memcmp(before, after, sizeof(before)))
		ok(0, "volatile context corrupted\n");

	return ret;
}

static
__attribute__((noinline))
int call_test(int (* func)(void))
{
	static int ret;
	static LPTOP_LEVEL_EXCEPTION_FILTER prev_unhandled_exception;

	prev_unhandled_exception = SetUnhandledExceptionFilter(&unhandled_exception);

#if defined(__i386__)
	static struct volatile_context before, after;

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
#error TODO
#endif

	SetUnhandledExceptionFilter(prev_unhandled_exception);
	return ret;
}
#else
#error TODO
#endif

/* EOF */
