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
	       "title"=>"Uset performance: ICU",
	       "headers"=>"ICU",
	       "operationIs"=>"unicode string",
	       "passes"=>"1",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>"../data",
	       "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p = "debug/usetperf.exe ";

my $tests = { 
	     "titlecase_letter/add",  ["$p titlecase_letter_add"],
	     "titlecase_letter/contains",  ["$p titlecase_letter_contains"],
	     "titlecase_letter/iterator",  ["$p titlecase_letter_iterator"],
	     "unassigned/add",  ["$p unassigned_add"],
	     "unassigned/contains",  ["$p unassigned_contains"],
	     "unassigned/iterator",  ["$p unassigned_iterator"],
	     "pattern1",  ["$p pattern1"],
	     "pattern2",  ["$p pattern2"],
	     "pattern3",  ["$p pattern3"],
	    };

my $dataFiles = {
		};

runTests($options, $tests, $dataFiles);
