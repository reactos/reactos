Copyright (c) 2002-2005, International Business Machines Corporation and others. All Rights Reserved.


cpysearch.pl is a perl script used to detect the files that might not have the copyright notice. Best when used on windows on a clean checkout. Edit $icuSource to your path. If you are working on other platform, you probably want to edit $ignore to reflect different temporary files that you don't want in the scan. The result will be the list of files that don't have word copyright (case ignored) in first 10 lines. Look at them and fix if needed. 

cpysearch.pl  searches for files modified this year that don't have the
  correct year copyright (i.e. 'copyright 1995') 

cpyscan.pl    searches for all files that don't have any copyright

cpyskip.txt  is part of the ignore list.


Have fun!
weiv


