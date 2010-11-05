Copyright (c) 2002-2007, International Business Machines Corporation and others. All Rights Reserved.
This is an exercise for the ICU Workshop (September 2000).
** Workshop homepage is:  
  http://www.icu-project.org/docs/workshop_2000/agenda.html

  Day 2: September 12th 2000
  Pre-requsit:
  1. All the hardware and software requirements from Day 1.
  2. Attended or fully understand Day 1 material.
  3. Read through the ICU user's guide at
  http://www.icu-project.org/userguide/.

  #Date/Time/Number Formatting Support
  9:30am - 10:30am
  Alan Liu

  Topics:
  1. What is the date/time support in ICU?
  2. What is the timezone support in ICU?
  3. What kind of formatting and parsing support is available in ICU, i.e.
  NumberFormat, DateFormat, MessageFormat?


INSTRUCTIONS
------------

This exercise was first developed and tested on ICU release 1.6.0, Win32,
Microsoft Visual C++ 6.0.  It should work on other ICU releases and
other platforms as well.

 MSVC:
   Open the file "datefmt.sln" in Microsoft Visual C++.

 Unix:
   - Build and install ICU with a prefix, for example '--prefix=/home/srl/ICU'
   - Set the variable  ICU_PREFIX=/home/srl/ICU and use GNU make in 
        this directory.
   - You may use 'make check' to invoke this sample.

PROBLEMS
--------

Problem 0:

  Set up the program, build it, and run it.  To start with, the
  program prints out a list of languages.

Problem 1: Basic Date Formatting (Easy)

  Create a calendar, and use it to get the UDate for June 4, 1999,
  0:00 GMT (or any date of your choosing).  You will have to create a
  TimeZone (use the createZone() function already defined in main.cpp)
  and a Calendar object, and make the calendar use the time zone.

  Once you have the UDate, create a DateFormat object in each of the
  languages in the LANGUAGE array, and display the date in that
  language.  Use the DateFormat::createDateInstance() method to create
  the date formatter.

Problem 2: Date Formatting, Specific Time Zone (Medium)

  To really localize a time display, one can also specify the time
  zone in which the time should be displayed.  For each language,
  also create different time zones from the TIMEZONE list.

  To format a date with a specific calendar and zone, you must deal with
  three objects: a DateFormat, a Calendar, and a TimeZone.  Each object
  must be linked to another in correct sequence:  The Calendar must use
  the TimeZone, and the DateFormat must use the Calendar.

    DateFormat  =uses=>  Calendar  =uses=>  TimeZone

  Use either setFoo() or adoptFoo() methods, depending on where you
  want to have ownership.

  NOTE: It's not always desirable to change the time to a local time
  zone before display.  For instance, if some even occurs at 0:00 GMT
  on the first of the month, it's probably clearer to just state that.
  Stating that it occurs at 5:00 PM PDT on the day before in the
  summer, and 4:00 PM PST on the day before in the winter will just
  confuse the issue.


NOTES
-----

To see a list of system TimeZone IDs, use the TimeZone::create-
AvailableIDs() methods.  Alternatively, look at the file
icu/docs/tz.htm.  This has a hyperlinked list of current system zones.


ANSWERS
-------

The exercise includes answers.  These are in the "answers" directory,
and are numbered 1, 2, etc.

If you get stuck and you want to move to the next step, copy the
answers file into the main directory in order to proceed.  E.g.,
"main_1.cpp" contains the original "main.cpp" file.  "main_2.cpp"
contains the "main.cpp" file after problem 1.  Etc.


Have fun!
