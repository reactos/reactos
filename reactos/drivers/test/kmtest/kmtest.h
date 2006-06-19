#ifndef PNPTEST_H
#define PNPTEST_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*
    Some macros, structs, and vars are based or inspired from the great
    Wine regression tests Copyright (C) 2002 Alexandre Julliard.
    Everything else is done by Aleksey Bragin based on PnPTest by Filip Navara
*/

extern LONG successes;       /* number of successful tests */
extern LONG failures;        /* number of failures */
//static ULONG todo_successes;  /* number of successful tests inside todo block */
//static ULONG todo_failures;   /* number of failures inside todo block */

// We don't do multithreading, so we just keep this struct in a global var
typedef struct
{
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    int todo_level;                  /* current todo nesting level */
    int todo_do_loop;
} tls_data;

extern tls_data glob_data;

VOID StartTest();
VOID FinishTest(LPSTR TestName);
void kmtest_set_location(const char* file, int line);

#ifdef __GNUC__

extern int kmtest_ok( int condition, const char *msg, ... ) __attribute__((format (printf,2,3) ));

#else /* __GNUC__ */

extern int kmtest_ok( int condition, const char *msg, ... );

#endif /* __GNUC__ */


#define ok_(file, line)     (kmtest_set_location(file, line), 0) ? 0 : kmtest_ok
#define ok     ok_(__FILE__, __LINE__)

#endif /* PNPTEST_H */
