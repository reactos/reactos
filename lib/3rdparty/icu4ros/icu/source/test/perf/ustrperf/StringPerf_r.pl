#!/usr/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2007, International Business Machines
#  * Corporation and others. All Rights Reserved.
#  ********************************************************************

use strict;

#Assume we are running outside of the ICU source
use lib './ICU-3.0/ICU/source/test/perf/perldriver';

use PerfFramework;

my $options = {
	       "title"=>"Unicode String performance regression (ICU 2.8 and 3.0)",
	       "headers"=>"ICU28 ICU30",
	       "operationIs"=>"Unicode String",
	       "passes"=>"10",
	       "time"=>"5",
	       #"outputType"=>"HTML",
	       "dataDir"=>"c:/src/perf/data",
	       "outputDir"=>"results_ICU4C"
	      };

# programs

my $p1 = "icu-2.8/icu/bin/stringperf28.exe -b -u";
my $p2 = "icu-3.0/icu/bin/stringperf30.exe -b -u";

my $dataFiles = {
		 "",
		 [
		  "TestNames_Asian.txt",
		  "TestNames_Chinese.txt",
		  "TestNames_Japanese.txt",
		  "TestNames_Japanese_h.txt",
		  "TestNames_Japanese_k.txt",
		  "TestNames_Korean.txt",
		  "TestNames_Latin.txt",
		  "TestNames_SerbianSH.txt",
		  "TestNames_SerbianSR.txt",
		  "TestNames_Thai.txt",
		  "Testnames_Russian.txt",
#		  "th18057.txt",
#		  "thesis.txt",
#		  "vfear11a.txt",
		 ]
		};


my $tests = { 
"Object Construction(empty string)",      ["$p1 TestCtor"         , "$p2 TestCtor"         ],
"Object Construction(single char)",       ["$p1 TestCtor1"        , "$p2 TestCtor1"        ],
"Object Construction(another string)",    ["$p1 TestCtor2"        , "$p2 TestCtor2"        ],
"Object Construction(string literal)",    ["$p1 TestCtor3"        , "$p2 TestCtor3"        ],
"String Assignment(helper)",     		  ["$p1 TestAssign"       , "$p2 TestAssign"       ],
"String Assignment(string literal)",      ["$p1 TestAssign1"      , "$p2 TestAssign1"      ],
"String Assignment(another string)",      ["$p1 TestAssign2"      , "$p2 TestAssign2"      ],
"Get String or Character",      		  ["$p1 TestGetch"        , "$p2 TestGetch"        ],
"Concatenation",   						  ["$p1 TestCatenate"     , "$p2 TestCatenate"     ],
"String Scanning(char)",     		      ["$p1 TestScan"         , "$p2 TestScan"         ],
"String Scanning(string)",       		  ["$p1 TestScan1"        , "$p2 TestScan1"        ],
"String Scanning(char set)",       	      ["$p1 TestScan2"        , "$p2 TestScan2"        ],
};


runTests($options, $tests, $dataFiles);


