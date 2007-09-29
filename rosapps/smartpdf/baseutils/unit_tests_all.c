/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "str_util.h"

extern void str_util_test(void); /* in str_util_test.c */

int main(int argc, char **argv)
{
    printf("starting unit tests\n");
    str_util_test();
    printf("finished unit tests\n");
    return 0;
}
