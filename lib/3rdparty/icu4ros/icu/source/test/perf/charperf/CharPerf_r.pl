#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2007, International Business Machines
#  * Corporation and others. All Rights Reserved.
#  ********************************************************************

use strict;

#Assume we are running outside of the icu source
use lib './icu-3.0/icu/source/test/perf/perldriver';

use PerfFramework;

my $options = {
	       "title"=>"Character property performance regression (ICU 2.8 and 3.0)",
	       "headers"=>"ICU28 ICU30",
	       "operationIs"=>"code point",
	       "timePerOperationIs"=>"Time per code point",
	       "passes"=>"10",
	       "time"=>"5",
	       #"outputType"=>"HTML",
	       "dataDir"=>"c:/src/perf/data",
	       "outputDir"=>"results_ICU4C"
	      };

# programs

my $p1 = "icu-2.8/icu/bin/charperf28.exe";
my $p2 = "icu-3.0/icu/bin/charperf30.exe";

my $dataFiles = "";


my $tests = { 
"isAlpha",        ["$p1 TestIsAlpha"        , "$p2 TestIsAlpha"        ],
"isUpper",        ["$p1 TestIsUpper"        , "$p2 TestIsUpper"        ],
"isLower",        ["$p1 TestIsLower"        , "$p2 TestIsLower"        ],	
"isDigit",        ["$p1 TestIsDigit"        , "$p2 TestIsDigit"        ],	
"isSpace",        ["$p1 TestIsSpace"        , "$p2 TestIsSpace"        ],	
"isAlphaNumeric", ["$p1 TestIsAlphaNumeric" , "$p2 TestIsAlphaNumeric" ],
"isPrint",        ["$p1 TestIsPrint"        , "$p2 TestIsPrint"        ],     
"isControl",      ["$p1 TestIsControl"      , "$p2 TestIsControl"      ],
"toLower",        ["$p1 TestToLower"        , "$p2 TestToLower"        ],     
"toUpper",        ["$p1 TestToUpper"        , "$p2 TestToUpper"        ],     
"isWhiteSpace",   ["$p1 TestIsWhiteSpace"   , "$p2 TestIsWhiteSpace"   ],
};

runTests($options, $tests, $dataFiles);


