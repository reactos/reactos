/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "netstr.h"

void netstr_ut(void)
{
    assert(1 == digits_for_number(0));
    assert(1 == digits_for_number(9));
    assert(2 == digits_for_number(10));
    assert(2 == digits_for_number(19));
    assert(2 == digits_for_number(25));
    assert(3 == digits_for_number(125));
    assert(4 == digits_for_number(3892));
    assert(5 == digits_for_number(38392));
    assert(6 == digits_for_number(889931));
    assert(7 == digits_for_number(7812345));

    assert(1 == digits_for_number(-0));
    assert(2 == digits_for_number(-9));
    assert(3 == digits_for_number(-10));
    assert(4 == digits_for_number(-125));
    assert(5 == digits_for_number(-3892));
    assert(6 == digits_for_number(-38392));
    assert(7 == digits_for_number(-889931));
    assert(8 == digits_for_number(-7812345));
}
