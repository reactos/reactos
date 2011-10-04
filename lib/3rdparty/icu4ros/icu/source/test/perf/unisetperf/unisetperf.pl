#!/usr/bin/perl -w
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2005-2007, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

use strict;
use lib '../perldriver';
use PerfFramework;

my $options = {
	       "title"=>"UnicodeSet span()/contains() performance",
	       "headers"=>"Bv Bv0",
	       "operationIs"=>"tested Unicode code point",
	       "passes"=>"3",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>"/temp/udhr",
	       "outputDir"=>"../results"
	      };

# programs
# tests will be done for all the programs. Results will be stored and connected
my $p =   "Release/unisetperf.exe -e UTF-8";
my $pc =  "$p Contains";
my $p16 = "$p SpanUTF16";
my $p8 =  "$p SpanUTF8";

my $tests = {
	     "Contains",  ["$pc  --type Bv",
	                   "$pc  --type Bv0"
	                   ],
	     "SpanUTF16", ["$p16 --type Bv",
	                   "$p16 --type Bv0"
	                   ]
	    };

my $dataFiles = {
		 "",
		 [
		  "udhr_eng.txt",
          "udhr_deu.txt",
          "udhr_fra.txt",
          "udhr_rus.txt",
          "udhr_tha.txt",
          "udhr_jpn.txt",
          "udhr_cmn.txt",
          "udhr_jpn.html"
		 ]
		};

runTests($options, $tests, $dataFiles);

$options = {
	       "title"=>"UnicodeSet span()/contains() performance",
	       "headers"=>"Bv BvF Bvp BvpF L Bvl",
	       "operationIs"=>"tested Unicode code point",
	       "passes"=>"3",
	       "time"=>"2",
	       #"outputType"=>"HTML",
	       "dataDir"=>"/temp/udhr",
	       "outputDir"=>"../results"
	      };

$tests = {
	     "SpanUTF8",  ["$p8  --type Bv",
	                   "$p8  --type BvF",
	                   "$p8  --type Bvp",
	                   "$p8  --type BvpF",
	                   "$p8  --type L",
	                   "$p8  --type Bvl"
	                   ]
	    };

runTests($options, $tests, $dataFiles);
