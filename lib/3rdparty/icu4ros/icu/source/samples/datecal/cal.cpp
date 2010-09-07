/*
*******************************************************************************
*
*   Copyright (C) 2002-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*/

#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include <stdio.h>

extern "C" {  extern void c_main(); };

void cpp_main()
{
  UErrorCode status = U_ZERO_ERROR;
  puts("C++ sample");
  GregorianCalendar* gc = new GregorianCalendar(status);
  if (U_FAILURE(status)) {
    puts("Couldn't create GregorianCalendar");
    return;
  }
  /* set up the date */
  gc->set(2000, UCAL_FEBRUARY, 26);
  gc->set(UCAL_HOUR_OF_DAY, 23);
  gc->set(UCAL_MINUTE, 0);
  gc->set(UCAL_SECOND, 0);
  gc->set(UCAL_MILLISECOND, 0);
  /* Iterate through the days and print it out. */
  for (int32_t i = 0; i < 30; i++) {
    /* print out the date. */
    /* You should use the DateFormat to properly format it */
    printf("year: %d, month: %d (%d in the implementation), day: %d\n",
           gc->get(UCAL_YEAR, status),
           gc->get(UCAL_MONTH, status) + 1,
           gc->get(UCAL_MONTH, status),
           gc->get(UCAL_DATE, status));
    if (U_FAILURE(status))
      {
        puts("Calendar::get failed");
        return;
      }
    /* Add a day to the date */
    gc->add(UCAL_DATE, 1, status);
    if (U_FAILURE(status)) {
      puts("Calendar::add failed");
      return;
    }
  }
  delete gc;
}
                

/* Creating and using text boundaries */
int main( void )
{
  puts("Date-Calendar sample program");

  cpp_main();

  c_main();

  return 0;
}

