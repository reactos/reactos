#!/usr/bin/perl -w
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2005, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

use strict;

use lib '../perldriver';

use PerfFramework;


my $options = {
	       "title"=>"Utf performance: ICU",
	       "headers"=>"ICU",
	       "operationIs"=>"gb18030 encoding string",
	       "passes"=>"1",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>"../data",
	       "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p = "debug/utfperf.exe -e gb18030";

my $tests = { 
	     "UTF-8",  ["$p UTF_8"],
	     "UTF-8 small buffer",  ["$p UTF_8_SB"],
	     "SCSU",  ["$p SCSU"],
	     "SCSU small buffer",  ["$p SCSU_SB"],
	     "BOCU_1",  ["$p BOCU_1"],
	     "BOCU_1 small buffer",  ["$p BOCU_1_SB"],
	    };

my $dataFiles = {
		 "",
		 [
		  "four.txt"
		 ]
		};

runTests($options, $tests, $dataFiles);
