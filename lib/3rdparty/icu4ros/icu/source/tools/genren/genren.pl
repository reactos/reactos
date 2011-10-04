#!/usr/bin/perl 
#*
#*******************************************************************************
#*   Copyright (C) 2001-2007, International Business Machines
#*   Corporation and others.  All Rights Reserved.
#*******************************************************************************
#*
#*   file name:  genren.pl
#*   encoding:   US-ASCII
#*   tab size:   8 (not used)
#*   indentation:4
#*
#*   Created by: Vladimir Weinstein
#*   07/19/2001
#*
#*  Used to generate renaming headers.
#*  Run on UNIX platforms (linux) in order to catch all the exports

$headername = 'urename.h';

$path = substr($0, 0, rindex($0, "/")+1)."../../common/unicode/uversion.h";

(-e $path) || die "Cannot find uversion.h";

open(UVERSION, $path);

while(<UVERSION>) {
    if(/\#define U_ICU_VERSION_SUFFIX/) {
        chop;
        s/\#define U_ICU_VERSION_SUFFIX //;
        $U_ICU_VERSION_SUFFIX = "$_";
        last;
    }
}

while($ARGV[0] =~ /^-/) { # detects whether there are any arguments
    $_ = shift @ARGV;      # extracts the argument for processing
    /^-v/ && ($VERBOSE++, next);                      # verbose
    /^-h/ && (&printHelpMsgAndExit, next);               # help
    /^-o/ && (($headername = shift (@ARGV)), next);   # output file
    /^-S/ && (($U_ICU_VERSION_SUFFIX = shift(@ARGV)), next); # pick the suffix
    warn("Invalid option $_\n");
    &printHelpMsgAndExit;
}

unless(@ARGV > 0) {
    warn "No libraries, exiting...\n";
    &printHelpMsgAndExit;
}

#$headername = "uren".substr($ARGV[0], 6, index(".", $ARGV[0])-7).".h";
    
$HEADERDEF = uc($headername);  # this is building the constant for #define
$HEADERDEF =~ s/\./_/;

    
    open HEADER, ">$headername"; # opening a header file

#We will print our copyright here + warnings
print HEADER <<"EndOfHeaderComment";
/*
*******************************************************************************
*   Copyright (C) 2002-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  $headername
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Created by: Perl script written by Vladimir Weinstein
*
*  Contains data for renaming ICU exports.
*  Gets included by umachine.h
*
*  THIS FILE IS MACHINE-GENERATED, DON'T PLAY WITH IT IF YOU DON'T KNOW WHAT
*  YOU ARE DOING, OTHERWISE VERY BAD THINGS WILL HAPPEN!
*/

#ifndef $HEADERDEF
#define $HEADERDEF

/* Uncomment the following line to disable renaming on platforms
   that do not use Autoconf. */
/* #define U_DISABLE_RENAMING 1 */

#if !U_DISABLE_RENAMING
EndOfHeaderComment

for(;@ARGV; shift(@ARGV)) {
    @NMRESULT = `nm -Cg -f s $ARGV[0]`;
    if($?) {
        warn "Couldn't do 'nm' for $ARGV[0], continuing...\n";
        next; # Couldn't do nm for the file
    }
    splice @NMRESULT, 0, 6;
    
    foreach (@NMRESULT) { # Process every line of result and stuff it in $_
        ($_, $address, $type) = split(/\|/);
        &verbose( "type: \"$type\" ");
        if(!($type =~ /[UAwW?]/)) {
            if(/@@/) { # These would be imports
                &verbose( "Import: $_ \"$type\"\n");
            } elsif (/::/) { # C++ methods, stuff class name in associative array
                &verbose( "C++ method: $_\n");
                ## icu_2_0::CharString::~CharString(void) -> CharString
                @CppName = split(/::/); ## remove scope stuff
                if(@CppName>1) {
                    ## MessageFormat virtual table -> MessageFormat
                    @CppName = split(/ /, $CppName[1]); ## remove debug stuff
                }
                ## ures_getUnicodeStringByIndex(UResourceBundle -> ures_getUnicodeStringByIndex
                @CppName = split(/\(/, $CppName[0]); ## remove function args
                $CppClasses{$CppName[0]}++;
            } elsif ( /\(/) { # These are strange functions
                print STDERR "$_\n";
            } elsif ( /icu_/) {
                print STDERR "Skipped strange mangled function $_\n";
            } else { # This is regular C function 
                &verbose( "C func: $_\n");
                @funcname = split(/[\(\s+]/);
                $CFuncs{$funcname[0]}++;
            }
        } else {
            &verbose( "Skipped: $_ $1\n");
        }
    }
}

print HEADER "\n/* C exports renaming data */\n\n";
foreach(sort keys(%CFuncs)) {
    print HEADER "#define $_ $_$U_ICU_VERSION_SUFFIX\n";
}

print HEADER "/* C++ class names renaming defines */\n\n";
print HEADER "#ifdef XP_CPLUSPLUS\n";
print HEADER "#if !U_HAVE_NAMESPACE\n\n";
foreach(sort keys(%CppClasses)) {
    print HEADER "#define $_ $_$U_ICU_VERSION_SUFFIX\n";
}
print HEADER "\n#endif\n";
print HEADER "#endif\n";
print HEADER "\n#endif\n";
print HEADER "\n#endif\n";

close HEADER;

sub verbose {
    if($VERBOSE) {
        print STDERR @_;
    }
}


sub printHelpMsgAndExit {
    print STDERR <<"EndHelpText";
Usage: $0 [OPTIONS] LIBRARY_FILES
  Options: 
    -v - verbose
    -h - help
    -o - output file name (defaults to 'urename.h'
    -S - suffix (defaults to _MAJOR_MINOR of current ICU version)
Will produce a renaming .h file

EndHelpText

    exit 0;

}

