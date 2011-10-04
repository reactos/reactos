#!/usr/bin/perl -w
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************


use strict;

use lib '../perldriver';

use PerfFramework;

my $options = {
	       "title"=>"Character property performance: ICU vs. STDLib",
	       "headers"=>"StdLib ICU",
	       "operationIs"=>"code point",
	       "timePerOperationIs"=>"Time per code point",
	       "passes"=>"10",
	       "time"=>"5",
	       #"outputType"=>"HTML",
	       "dataDir"=>"c:/src/perf/data",
	       "outputDir"=>"../results"
	      };


# programs
# tests will be done for all the programs. Results will be stored and connected
my $p = "charperf.exe";

my $tests = { 
"isAlpha",        ["$p TestStdLibIsAlpha"        , "$p TestIsAlpha"        ],
"isUpper",        ["$p TestStdLibIsUpper"        , "$p TestIsUpper"        ],
"isLower",        ["$p TestStdLibIsLower"        , "$p TestIsLower"        ],	
"isDigit",        ["$p TestStdLibIsDigit"        , "$p TestIsDigit"        ],	
"isSpace",        ["$p TestStdLibIsSpace"        , "$p TestIsSpace"        ],	
"isAlphaNumeric", ["$p TestStdLibIsAlphaNumeric" , "$p TestIsAlphaNumeric" ],
"isPrint",        ["$p TestStdLibIsPrint"        , "$p TestIsPrint"        ],     
"isControl",      ["$p TestStdLibIsControl"      , "$p TestIsControl"      ],
"toLower",        ["$p TestStdLibToLower"        , "$p TestToLower"        ],     
"toUpper",        ["$p TestStdLibToUpper"        , "$p TestToUpper"        ],     
"isWhiteSpace",   ["$p TestStdLibIsWhiteSpace"   , "$p TestIsWhiteSpace"   ],
};

my $dataFiles;

runTests($options, $tests, $dataFiles);
