#!/usr/bin/perl
#*
#*******************************************************************************
#*   Copyright (C) 2006, International Business Machines
#*   Corporation and others.  All Rights Reserved.
#*******************************************************************************
#*
#*   file name:  genspva.pl
#*   encoding:   US-ASCII
#*   tab size:   8 (not used)
#*   indentation:4
#*
#*   Created by: Ram Viswanadha
#*
#* This file filters iso15924-utf8-<date>.txt
#*

use File::Find;
use File::Basename;
use IO::File;
use Cwd;
use File::Copy;
use Getopt::Long;
use File::Path;
use File::Copy;

#run the program
main();

#---------------------------------------------------------------------
# The main program

sub main(){
    GetOptions(
           "--destdir=s" => \$destdir,
           "--iso15924=s"  => \$iso,
           "--prop=s"  => \$prop,
           "--code-start=s"  => \$code,
           );
    usage() unless defined $destdir;
    usage() unless defined $iso;
    usage() unless defined $prop;
    
    $outfile = "$destdir/SyntheticPropertyValueAliases.txt";
    $propFH = IO::File->new($prop,"r")
            or die  "could not open the file $prop for reading: $! \n";
    $isoFH = IO::File->new($iso,"r")
            or die  "could not open the file $iso for reading: $! \n";
    $outFH = IO::File->new($outfile,"w")
            or die  "could not open the file $outfile for reading: $! \n";
    my @propLines;
    while (<$propFH>) {
        next if(!($_ =~/sc ; /));
        push(@propLines, $_);
    }
    printHeader($outFH);
    if(defined $code){
        print "Please add the following to UScriptCode enum in uscript.h.\n";
        print "#ifndef U_HIDE_DRAFT_API\n";
    }
    while (<$isoFH>) {
        next if($_=~/^#/);#skip if the line starts with a comment char
        ($script, $t, $name, $rest) = split(/;/,$_,4);
        #sc ; Arab
        $outstr = "sc ; $script";
        $encoded = 0; #false
        
        # seach the propLines to make sure that this scipt code is not 
        # encoded in Unicode
        foreach $key (@propLines){
            if($key =~ /$outstr/){
                $encoded = 1;
            }
        }
        next if($encoded == 1);
        #ignore private use codes 
        next if($script =~ /Qa[ab][a-z]/);
        
        #if($script eq "Qaaa"){
        #    $outstr = $outstr." ; Private_Use_Start\n";
        #}elsif($script eq  "Qabx"){
        #    $outstr = $outstr." ; Private_Use_End\n";
        #}else{
        #    $outstr = $outstr." ; $script \n";
        #} 
        
        $outstr = $outstr." ; $script \n";
        print $outFH $outstr;
        
        #print to console
        if(defined $code){
            if($name =~ /[(\s,\x80-\xFF]/){
                $name = $script;
            }
            $name =~s/-/_/g;
        
            $scriptcode =  "USCRIPT_".uc($name);
            print "      $scriptcode          = $code, /* $script */\n";
            $code++;
        }
        
    }
    if(defined $code){
        print "#endif /* U_HIDE_DRAFT_API */\n";
    }
    for($i=0; $i<2; $i++){
        
    }
    close($isoFH);
    close($propFH);
    close($outFH);
}
#-----------------------------------------------------------------------
sub printHeader{
    ($outFH) = @_;
    ($DAY, $MONTH, $YEAR) = (localtime)[3,4,5];
    $YEAR += 1900;
    #We will print our copyright here + warnings
print $outFH <<END_HEADER_COMMENT;
########################################################################
# Copyright (c) 2006-$YEAR, International Business Machines
# Corporation and others.  All Rights Reserved.
########################################################################
#   file name:      SyntheticPropertyValueAliases.txt
#   encoding:       US-ASCII
#   tab size:       8 (not used)
#   indentation:    4
#   created by:     gensvpa.pl
########################################################################

# This file follows the format of PropertyValueAliases.txt
# It contains synthetic property value aliases not present
# in the UCD.  Unlike PropertyValueAliases.txt, it should
# NOT contain a version number.

########################################################################
#  THIS FILE IS MACHINE-GENERATED, DON'T PLAY WITH IT IF YOU DON'T KNOW
#  WHAT YOU ARE DOING, OTHERWISE VERY BAD THINGS WILL HAPPEN!
########################################################################

# set the same names as short and long names to fit the syntax without 
# inventing names that we would have to support forever

# Script (sc)

END_HEADER_COMMENT
}
#-----------------------------------------------------------------------
sub usage {
    print << "END";
Usage:
gensvpa.pl
Options:
        --destdir=<directory>
        --iso15924=<file name>
        --prop=<PropertyValueAliases.txt>
        --code-start=s
e.g.: gensvpa.pl  --destdir=<icu>/source/tools/genpname --iso15924=iso15924-utf8-20041025.txt --prop=<icu>/source/data/unidata --code-start=60
END
    exit(0);
}