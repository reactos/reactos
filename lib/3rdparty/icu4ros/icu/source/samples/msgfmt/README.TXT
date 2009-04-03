Copyright (c) 2002-2007, International Business Machines Corporation and others. All Rights Reserved.
This is an exercise for the ICU Workshop (September 2000).

  http://www.icu-project.org/docs/workshop_2000/agenda.html

  Day 2: September 12th 2000
  Pre-requisites:
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
   Open the file "msgfmt.sln" in Microsoft Visual C++.

 Unix:
   - Build and install ICU with a prefix, for example '--prefix=/home/srl/ICU'
   - Set the variable  ICU_PREFIX=/home/srl/ICU and use GNU make in 
        this directory.
   - You may use 'make check' to invoke this sample.

PROBLEMS
--------

Problem 0:

  Set up the program, build it, and run it.  To start with, the
  program prints out the word "Message".

Problem 1: Basic Message Formatting (Easy)

  Use a MessageFormat to create a message that prints out "Received
  <n> argument(s) on <d>.", where n is the number of command line
  arguments (use argc-1), and d is the date (use Calendar::getNow()).

  HINT: Your message pattern should have a "number" element and a
  "date" element, and you will need to use Formattable.

Problem 2: ChoiceFormat (Medium)

  We can do better than "argument(s)".  Instead, we can display more
  idiomatic strings, such as "no arguments", "one argument", "two
  arguments", and for higher values, we can use a number format.
 
  This kind of value-based switching is done using a ChoiceFormat.
  However, you seldom needs to create a ChoiceFormat by itself.
  Instead, most of the time you will supply the ChoiceFormat pattern
  within a MessageFormat pattern.

  Use a ChoiceFormat pattern within the MessageFormat pattern, instead
  of the "number" element, to display more idiomatic strings.

  EXTRA: Embed a number element within the choice element to handle
  values greater than two.


ANSWERS
-------

The exercise includes answers.  These are in the "answers" directory,
and are numbered 1, 2, etc.

If you get stuck and you want to move to the next step, copy the
answers file into the main directory in order to proceed.  E.g.,
"main_1.cpp" contains the original "main.cpp" file.  "main_2.cpp"
contains the "main.cpp" file after problem 1.  Etc.


Have fun!
