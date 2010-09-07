Copyright (c) 2002-2007, International Business Machines Corporation and others. All Rights Reserved.
This is an exercise for the ICU Workshop (September 2000).
  http://www.icu-project.org/docs/workshop_2000/agenda.html

  Day 2: September 12th 2000
  Pre-requisite:
  1. All the hardware and software requirements from Day 1.
  2. Attended or fully understand Day 1 material.
  3. Read through the ICU user's guide at
  http://www.icu-project.org/userguide/.

  #Transformation Support
  10:45am - 12:00pm
  Alan Liu

  Topics:
  1. What is the Unicode normalization?
  2. What kind of case mapping support is available in ICU?
  3. What is Transliteration and how do I use a Transliterator on a document?
  4. How do I add my own Transliterator?


INSTRUCTIONS
------------

This exercise was developed and tested on ICU release 1.6.0, Win32,
Microsoft Visual C++ 6.0.  It should work on other ICU releases and
other platforms as well.

 MSVC:
   Open the file "translit.sln" in Microsoft Visual C++.

 Unix:
   - Build and install ICU with a prefix, for example '--prefix=/home/srl/ICU'
   - Set the variable  ICU_PREFIX=/home/srl/ICU and use GNU make in 
        this directory.
   - You may use 'make check' to invoke this sample.


PROBLEMS
--------

Problem 0:

  To start with, the program prints out a series of dates formatted in
  Greek.  Set up the program, build it, and run it.

Problem 1: Basic Transliterator (Easy)

  The Greek text shows up almost entirely as Unicode escapes.  These
  are unreadable on a US machine.  Use an existing system
  transliterator to transliterate the Greek text to Latin so it can be
  phonetically read on a US machine.  If you don't know the names of
  the system transliterators, use Transliterator::getAvailableID() and
  Transliterator::countAvailableIDs(), or look directly in the index
  table icu/data/translit_index.txt.

Problem 2: RuleBasedTransliterator (Medium)

  Some of the text is still unreadable and shows up as Unicode escape
  sequences.  Create a RuleBasedTransliterator to change the
  unreadable characters to close ASCII equivalents.  For example, the
  rule "\u00C0 > A;" will change an 'A' with a grave accent to a plain
  'A'.

  To save typing, use UnicodeSets to handle ranges of characters.

  See the included file "U0080.pdf" for a table of the U+00C0 to U+00FF
  Unicode block.

Problem 3: Transliterator subclassing; Normalizer (Difficult)

  The rule-based approach is flexible and, in most cases, the best
  choice for creating a new transliterator.  Sometimes, however, a
  more elegant algorithmic solution is available.  Instead of typing
  in a list of rules, you can write C++ code to accomplish the desired
  transliteration.

  Use a Normalizer to remove accents from characters.  You will need
  to convert each character to a sequence of base and combining
  characters by applying a canonical denormalization transformation.
  Then discard the combining characters (the accents etc.) leaving the
  base character.  Wrap this all up in a subclass of the
  Transliterator class that overrides the pure virtual
  handleTransliterate() method.


ANSWERS
-------

The exercise includes answers.  These are in the "answers" directory,
and are numbered 1, 2, etc.  In some cases new files that the user
needs to create are included in the answers directory.

If you get stuck and you want to move to the next step, copy the
answers file into the main directory in order to proceed.  E.g.,
"main_1.cpp" contains the original "main.cpp" file.  "main_2.cpp"
contains the "main.cpp" file after problem 1.  Etc.


Have fun!
