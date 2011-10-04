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
	       "title"=>"Normalization performance regression (ICU 2.8 and 3.0)",
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

my $p1 = "icu-2.8/icu/bin/normperf28.exe -b -u";
my $p2 = "icu-3.0/icu/bin/normperf30.exe -b -u";

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
	     "NFC_NFD_Text",  ["$p1 TestICU_NFC_NFD_Text"  ,  "$p2 TestICU_NFC_NFD_Text" ],
	     "NFC_NFC_Text",  ["$p1 TestICU_NFC_NFC_Text"  ,  "$p2 TestICU_NFC_NFC_Text" ],
	     "NFC_Orig_Text", ["$p1 TestICU_NFC_Orig_Text" ,  "$p2 TestICU_NFC_Orig_Text"],
	     "NFD_NFD_Text",  ["$p1 TestICU_NFD_NFD_Text"  ,  "$p2 TestICU_NFD_NFD_Text" ],
	     "NFD_NFC_Text",  ["$p1 TestICU_NFD_NFC_Text"  ,  "$p2 TestICU_NFD_NFC_Text" ],
	     "NFD_Orig_Text", ["$p1 TestICU_NFD_Orig_Text" ,  "$p2 TestICU_NFD_Orig_Text"],
	     ##
	     "QC_NFC_NFD_Text",  ["$p1 TestQC_NFC_NFD_Text"  ,  "$p2 TestQC_NFC_NFD_Text" ],
	     "QC_NFC_NFC_Text",  ["$p1 TestQC_NFC_NFC_Text"  ,  "$p2 TestQC_NFC_NFC_Text" ],
	     "QC_NFC_Orig_Text", ["$p1 TestQC_NFC_Orig_Text" ,  "$p2 TestQC_NFC_Orig_Text"],
	     "QC_NFD_NFD_Text",  ["$p1 TestQC_NFD_NFD_Text"  ,  "$p2 TestQC_NFD_NFD_Text" ],
	     "QC_NFD_NFC_Text",  ["$p1 TestQC_NFD_NFC_Text"  ,  "$p2 TestQC_NFD_NFC_Text" ],
	     "QC_NFD_Orig_Text", ["$p1 TestQC_NFD_Orig_Text" ,  "$p2 TestQC_NFD_Orig_Text"],
	     ##
	     "IsNormalized_NFC_NFD_Text",  ["$p1 TestIsNormalized_NFC_NFD_Text"  ,  "$p2 TestIsNormalized_NFC_NFD_Text" ],
	     "IsNormalized_NFC_NFC_Text",  ["$p1 TestIsNormalized_NFC_NFC_Text"  ,  "$p2 TestIsNormalized_NFC_NFC_Text" ],
	     "IsNormalized_NFC_Orig_Text", ["$p1 TestIsNormalized_NFC_Orig_Text" ,  "$p2 TestIsNormalized_NFC_Orig_Text"],
	     "IsNormalized_NFD_NFD_Text",  ["$p1 TestIsNormalized_NFD_NFD_Text"  ,  "$p2 TestIsNormalized_NFD_NFD_Text" ],
	     "IsNormalized_NFD_NFC_Text",  ["$p1 TestIsNormalized_NFD_NFC_Text"  ,  "$p2 TestIsNormalized_NFD_NFC_Text" ],
	     "IsNormalized_NFD_Orig_Text", ["$p1 TestIsNormalized_NFD_Orig_Text" ,  "$p2 TestIsNormalized_NFD_Orig_Text"]
	    };


runTests($options, $tests, $dataFiles);


