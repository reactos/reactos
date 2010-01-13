#!/bin/perl -w
#*******************************************************************
# COPYRIGHT:
# Copyright (c) 2002-2006, International Business Machines Corporation and
# others. All Rights Reserved.
#*******************************************************************

# This script reads in UCD files PropertyAliases.txt and
# PropertyValueAliases.txt and correlates them with ICU enums
# defined in uchar.h and uscript.h.  It then outputs a header
# file which contains all names and enums.  The header is included
# by the genpname tool C++ source file, which produces the actual
# binary data file.
#
# See usage note below.
#
# TODO: The Property[Value]Alias.txt files state that they can support
# more than 2 names per property|value.  Currently (Unicode 3.2) there
# are always 1 or 2 names.  If more names were supported, presumably
# the format would be something like:
#    nv        ; Numeric_Value
#    nv        ; Value_Numerique
# CURRENTLY, this script assumes that there are 1 or two names.  Any
# duplicates it sees are flagged as an error.  If multiple aliases
# appear in a future version of Unicode, modify this script to support
# that.
#
# NOTE: As of ICU 2.6, this script has been modified to know about the
# pseudo-property gcm/General_Category_Mask, which corresponds to the
# uchar.h property UCHAR_GENERAL_CATEGORY_MASK.  This property
# corresponds to General_Category but is a bitmask value.  It does not
# exist in the UCD.  Therefore, I special case it in several places
# (search for General_Category_Mask and gcm).
#
# NOTE: As of ICU 2.6, this script reads an auxiliary data file,
# SyntheticPropertyAliases.txt, containing property aliases not
# present in the UCD but present in ICU.  This file resides in the
# same directory as this script.  Its contents are merged into those
# of PropertyAliases.txt as if the two files were appended.
#
# NOTE: The following names are handled specially.  See script below
# for details.
#
#   T/True
#   F/False
#   No_Block
#
# Author: Alan Liu
# Created: October 14 2002
# Since: ICU 2.4

use FileHandle;
use strict;
use Dumpvalue;

my $DEBUG = 1;
my $DUMPER = new Dumpvalue;

my $count = @ARGV;
my $ICU_DIR = shift() || '';
my $OUT_FILE = shift() || 'data.h';
my $HEADER_DIR = "$ICU_DIR/source/common/unicode";
my $UNIDATA_DIR = "$ICU_DIR/source/data/unidata";

# Get the current year from the system
my $YEAR = 1900+@{[localtime]}[5]; # Get the current year

# Used to make "n/a" property [value] aliases (Unicode or Synthetic) unique
my $propNA = 0;
my $valueNA = 0;

#----------------------------------------------------------------------
# Top level property keys for binary, enumerated, string, and double props
my @TOP     = qw( _bp _ep _sp _dp _mp );

# This hash governs how top level properties are grouped into output arrays.
#my %TOP_PROPS = ( "VALUED"   => [ '_bp', '_ep' ],
#                  "NO_VALUE" => [ '_sp', '_dp' ] );m
#my %TOP_PROPS = ( "BINARY"   => [ '_bp' ],
#                  "ENUMERATED" => [ '_ep' ],
#                  "STRING" => [ '_sp' ],
#                  "DOUBLE" => [ '_dp' ] );
my %TOP_PROPS = ( ""   => [ '_bp', '_ep', '_sp', '_dp', '_mp' ] );

my %PROP_TYPE = (Binary => "_bp",
                 String => "_sp",
                 Double => "_dp",
                 Enumerated => "_ep",
                 Bitmask => "_mp");
#----------------------------------------------------------------------

# Properties that are unsupported in ICU
my %UNSUPPORTED = (Composition_Exclusion => 1,
                   Decomposition_Mapping => 1,
                   Expands_On_NFC => 1,
                   Expands_On_NFD => 1,
                   Expands_On_NFKC => 1,
                   Expands_On_NFKD => 1,
                   FC_NFKC_Closure => 1,
                   ID_Start_Exceptions => 1,
                   Special_Case_Condition => 1,
                   );

# Short names of properties that weren't seen in uchar.h.  If the
# properties weren't seen, don't complain about the property values
# missing.
my %MISSING_FROM_UCHAR;

# Additional property aliases beyond short and long names,
# like space in addition to WSpace and White_Space in Unicode 4.1.
# Hashtable, maps long name to alias.
# For example, maps White_Space->space.
#
# If multiple additional aliases are defined,
# then they are separated in the value string with '|'.
# For example, White_Space->space|outer_space
my %additional_property_aliases;

#----------------------------------------------------------------------

# Emitted class names
my ($STRING_CLASS, $ALIAS_CLASS, $PROPERTY_CLASS) = qw(AliasName Alias Property);

if ($count < 1 || $count > 2 ||
    !-d $HEADER_DIR ||
    !-d $UNIDATA_DIR) {
    my $me = $0;
    $me =~ s|.+[/\\]||;
    my $lm = ' ' x length($me);
    print <<"END";

$me: Reads ICU4C headers and Unicode data files and creates
$lm  a C header file that is included by genpname.  The header
$lm  file matches constants defined in the ICU4C headers with
$lm  property|value aliases in the Unicode data files.

Usage: $me <icu_dir> [<out_file>]

<icu_dir>   ICU4C root directory, containing
               source/common/unicode/uchar.h
               source/common/unicode/uscript.h
               source/data/unidata/Blocks.txt
               source/data/unidata/PropertyAliases.txt
               source/data/unidata/PropertyValueAliases.txt
<out_file>  File name of header to be written;
            default is 'data.h'.

The Unicode versions of all input files must match.
END
    exit(1);
}

my ($h, $version) = readAndMerge($HEADER_DIR, $UNIDATA_DIR);

if ($DEBUG) {
    print "Merged hash:\n";
    for my $key (sort keys %$h) {
        my $hh = $h->{$key};
        for my $subkey (sort keys %$hh) {
            print "$key:$subkey:", $hh->{$subkey}, "\n";
        }
    }
}

my $out = new FileHandle($OUT_FILE, 'w');
die "Error: Can't write to $OUT_FILE: $!" unless (defined $out);
my $save = select($out);
formatData($h, $version);
select($save);
$out->close();

exit(0);

#----------------------------------------------------------------------
# From PropList.html: "The properties of the form Other_XXX
# are used to generate properties in DerivedCoreProperties.txt.
# They are not intended for general use, such as in APIs that
# return property values.
# Non_Break is not a valid property as of 3.2.
sub isIgnoredProperty {
    local $_ = shift;
    /^Other_/i || /^Non_Break$/i;
}

# 'qc' is a pseudo-property matching any quick-check property
# see PropertyValueAliases.txt file comments.  'binprop' is
# a synthetic binary value alias "True"/"False", not present
# in PropertyValueAliases.txt.
sub isPseudoProperty {
    $_[0] eq 'qc' ||
        $_[0] eq 'binprop';
}

#----------------------------------------------------------------------
# Emit the combined data from headers and the Unicode database as a
# C source code header file.
#
# @param ref to hash with the data
# @param Unicode version, as a string
sub formatData {
    my $h = shift;
    my $version = shift;

    my $date = scalar localtime();
    print <<"END";
/**
 * Copyright (C) 2002-$YEAR, International Business Machines Corporation and
 * others. All Rights Reserved.
 *
 * MACHINE GENERATED FILE.  !!! Do not edit manually !!!
 *
 * Generated from
 *   uchar.h
 *   uscript.h
 *   Blocks.txt
 *   PropertyAliases.txt
 *   PropertyValueAliases.txt
 *
 * Date: $date
 * Unicode version: $version
 * Script: $0
 */

END

    #------------------------------------------------------------
    # Emit Unicode version
    print "/* Unicode version $version */\n";
    my @v = split(/\./, $version);
    push @v, '0' while (@v < 4);
    for (my $i=0; $i<@v; ++$i) {
        print "const uint8_t VERSION_$i = $v[$i];\n";
    }
    print "\n";

    #------------------------------------------------------------
    # Emit String table
    # [A table of all identifiers, that is, all long or short property
    # or value names.  The list need NOT be sorted; it will be sorted
    # by the C program.  Strings are referenced by their index into
    # this table.  After sorting, a REMAP[] array is used to map the
    # old position indices to the new positions.]
    my %strings;
    for my $prop (sort keys %$h) {
        my $hh = $h->{$prop};
        for my $enum (sort keys %$hh) {
            my @a = split(/\|/, $hh->{$enum});
            for (@a) {
                $strings{$_} = 1 if (length($_));
            }
        }
    }
    my @strings = sort keys %strings;
    unshift @strings, "";

    print "const int32_t STRING_COUNT = ", scalar @strings, ";\n\n"; 

    # while printing, create a mapping hash from string table entry to index
    my %stringToID;
    print "/* to be sorted */\n";
    print "const $STRING_CLASS STRING_TABLE[] = {\n";
    for (my $i=0; $i<@strings; ++$i) {
        print "    $STRING_CLASS(\"$strings[$i]\", $i),\n";
        $stringToID{$strings[$i]} = $i;
    }
    print "};\n\n";

    # placeholder for the remapping index.  this is used to map
    # indices that we compute here to indices of the sorted
    # STRING_TABLE.  STRING_TABLE will be sorted by the C++ program
    # using the uprv_comparePropertyNames() function.  this will
    # reshuffle the order.  we then use the indices (passed to the
    # String constructor) to create a REMAP[] array.
    print "/* to be filled in */\n";
    print "int32_t REMAP[", scalar @strings, "];\n\n";
    
    #------------------------------------------------------------
    # Emit the name group table
    # [A table of name groups.  A name group is one or more names
    # for a property or property value.  The Unicode data files specify
    # that there may be more than 2, although as of Unicode 3.2 there
    # are at most 2.  The name group table looks like this:
    #
    #  114, -115, 116, -117, 0, -118, 65, -64, ...
    #  [0]        [2]        [4]      [6]
    #
    # The entry at [0] consists of 2 strings, 114 and 115.
    # The entry at [2] consists of 116 and 117.  The entry at
    # [4] is one string, 118.  There is always at least one
    # string; typically there are two.  If there are two, the first
    # is the SHORT name and the second is the LONG.  If there is
    # one, then the missing entry (always the short name, in 3.2)
    # is zero, which is by definition the index of "".  The
    # 'preferred' name will generally be the LONG name, if there are
    # more than 2 entries.  The last entry is negative.

    # Build name group list and replace string refs with nameGroup indices
    my @nameGroups;
    
    # Check for duplicate name groups, and reuse them if possible
    my %groupToInt; # Map group strings to ints
    for my $prop (sort keys %$h) {
        my $hh = $h->{$prop};
        for my $enum (sort keys %$hh) {
            my $groupString = $hh->{$enum};
            my $i;
            if (exists $groupToInt{$groupString}) {
                $i = $groupToInt{$groupString};
            } else {
                my @names = split(/\|/, $groupString);
                die "Error: Wrong number of names in " . $groupString if (@names < 1);
                $i = @nameGroups; # index of group we are making 
                $groupToInt{$groupString} = $i; # Cache for reuse
                push @nameGroups, map { $stringToID{$_} } @names;
                $nameGroups[$#nameGroups] = -$nameGroups[$#nameGroups]; # mark end
            }
            # now, replace string list with ref to name group
            $hh->{$enum} = $i;
        }
    }

    print "const int32_t NAME_GROUP_COUNT = ",
          scalar @nameGroups, ";\n\n";

    print "int32_t NAME_GROUP[] = {\n";
    # emit one group per line, with annotations
    my $max_names = 0;
    for (my $i=0; $i<@nameGroups; ) {
        my @a;
        my $line;
        my $start = $i;
        for (;;) {
            my $j = $nameGroups[$i++];
            $line .= "$j, ";
            push @a, abs($j);
            last if ($j < 0);
        }
        print "    ",
              $line,
              ' 'x(20-length($line)),
              "/* ", sprintf("%3d", $start),
              ": \"", join("\", \"", map { $strings[$_] } @a), "\" */\n";
        $max_names = @a if(@a > $max_names);
          
    }
    print "};\n\n";
    
    # This is fixed for 3.2 at "2" but should be calculated dynamically
    # when more than 2 names appear in Property[Value]Aliases.txt.
    print "#define MAX_NAMES_PER_GROUP $max_names\n\n";

    #------------------------------------------------------------
    # Emit enumerated property values
    for my $prop (sort keys %$h) {
        next if ($prop =~ /^_/);
        my $vh = $h->{$prop};
        my $count = scalar keys %$vh;

        print "const int32_t VALUES_${prop}_COUNT = ",
              $count, ";\n\n";
        
        print "const $ALIAS_CLASS VALUES_${prop}\[] = {\n";
        for my $enum (sort keys %$vh) {
            #my @names = split(/\|/, $vh->{$enum});
            #die "Error: Wrong number of names for $prop:$enum in [" . join(",", @names) . "]"
            #    if (@names != 2);
            print "    $ALIAS_CLASS((int32_t) $enum, ", $vh->{$enum}, "),\n";
                  #$stringToID{$names[0]}, ", ",
                  #$stringToID{$names[1]}, "),\n";
            #      "\"", $names[0], "\", ",
            #      "\"", $names[1], "\"),\n";
        }
        print "};\n\n";
    }

    #------------------------------------------------------------
    # Emit top-level properties (binary, enumerated, etc.)
    for my $topName (sort keys %TOP_PROPS) {
        my $a = $TOP_PROPS{$topName};
        my $count = 0;
        for my $type (@$a) { # "_bp", "_ep", etc.
            $count += scalar keys %{$h->{$type}};
        }

        print "const int32_t ${topName}PROPERTY_COUNT = $count;\n\n";
        
        print "const $PROPERTY_CLASS ${topName}PROPERTY[] = {\n";

        for my $type (@$a) { # "_bp", "_ep", etc.
            my $p = $h->{$type};

            for my $enum (sort keys %$p) {
                my $name = $strings[$nameGroups[$p->{$enum}]];
            
                my $valueRef = "0, NULL";
                if ($type eq '_bp') {
                    $valueRef = "VALUES_binprop_COUNT, VALUES_binprop";
                }
                elsif (exists $h->{$name}) {
                    $valueRef = "VALUES_${name}_COUNT, VALUES_$name";
                }
                
                print "    $PROPERTY_CLASS((int32_t) $enum, ",
                      $p->{$enum}, ", $valueRef),\n";
            }
        }
        print "};\n\n";
    }

    print "/*eof*/\n";
}

#----------------------------------------------------------------------
# Read in the files uchar.h, uscript.h, Blocks.txt,
# PropertyAliases.txt, and PropertyValueAliases.txt,
# and combine them into one hash.
#
# @param directory containing headers
# @param directory containin Unicode data files
#
# @return hash ref, Unicode version
sub readAndMerge {

    my ($headerDir, $unidataDir) = @_;

    my $h = read_uchar("$headerDir/uchar.h");
    my $s = read_uscript("$headerDir/uscript.h");
    my $b = read_Blocks("$unidataDir/Blocks.txt");
    my $pa = {};
    read_PropertyAliases($pa, "$unidataDir/PropertyAliases.txt");
    read_PropertyAliases($pa, "SyntheticPropertyAliases.txt");
    my $va = {};
    read_PropertyValueAliases($va, "$unidataDir/PropertyValueAliases.txt");
    read_PropertyValueAliases($va, "SyntheticPropertyValueAliases.txt");
    
    # Extract property family hash
    my $fam = $pa->{'_family'};
    delete $pa->{'_family'};
    
    # Note: uscript.h has no version string, so don't check it
    my $version = check_versions([ 'uchar.h', $h ],
                                 [ 'Blocks.txt', $b ],
                                 [ 'PropertyAliases.txt', $pa ],
                                 [ 'PropertyValueAliases.txt', $va ]);
    
    # Do this BEFORE merging; merging modifies the hashes
    check_PropertyValueAliases($pa, $va);
    
    # Dump out the $va hash for debugging
    if ($DEBUG) {
        print "Property values hash:\n";
        for my $key (sort keys %$va) {
            my $hh = $va->{$key};
            for my $subkey (sort keys %$hh) {
                print "$key:$subkey:", $hh->{$subkey}, "\n";
            }
        }
    }
    
    # Dump out the $s hash for debugging
    if ($DEBUG) {
        print "Script hash:\n";
        for my $key (sort keys %$s) {
            print "$key:", $s->{$key}, "\n";
        }
    }
    
    # Link in the script data
    $h->{'sc'} = $s;
    
    merge_Blocks($h, $b);
    
    merge_PropertyAliases($h, $pa, $fam);
    
    merge_PropertyValueAliases($h, $va);
    
    ($h, $version);
}

#----------------------------------------------------------------------
# Ensure that the version strings in the given hashes (under the key
# '_version') are compatible.  Currently this means they must be
# identical, with the exception that "X.Y" will match "X.Y.0".
# All hashes must define the key '_version'.
#
# @param a list of pairs of (file name, hash reference)
#
# @return the version of all the hashes.  Upon return, the '_version'
# will be removed from all hashes.
sub check_versions {
    my $version = '';
    my $msg = '';
    foreach my $a (@_) {
        my $name = $a->[0];
        my $h    = $a->[1];
        die "Error: No version found" unless (exists $h->{'_version'});
        my $v = $h->{'_version'};
        delete $h->{'_version'};

        # append ".0" if necessary, to standardize to X.Y.Z
        $v .= '.0' unless ($v =~ /\.\d+\./);
        $v .= '.0' unless ($v =~ /\.\d+\./);
        $msg .= "$name = $v\n";
        if ($version) {
            die "Error: Mismatched Unicode versions\n$msg"
                unless ($version eq $v);
        } else {
            $version = $v;
        }
    }
    $version;
}

#----------------------------------------------------------------------
# Make sure the property names in PropertyValueAliases.txt match those
# in PropertyAliases.txt.
#
# @param a hash ref from read_PropertyAliases.
# @param a hash ref from read_PropertyValueAliases.
sub check_PropertyValueAliases {
    my ($pa, $va) = @_;

    # make a reverse hash of short->long
    my %rev;
    for (keys %$pa) { $rev{$pa->{$_}} = $_; }
    
    for my $prop (keys %$va) {
        if (!exists $rev{$prop} && !isPseudoProperty($prop)) {
            print "Warning: Property $prop from PropertyValueAliases not listed in PropertyAliases\n";
        }
    }
}

#----------------------------------------------------------------------
# Merge blocks data into uchar.h enum data.  In the 'blk' subhash all
# code point values, as returned from read_uchar, are replaced by
# block names, as read from Blocks.txt and returned by read_Blocks.
# The match must be 1-to-1.  If there is any failure of 1-to-1
# mapping, an error is signaled.  Upon return, the read_Blocks hash
# is emptied of all contents, except for those that failed to match.
#
# The mapping in the 'blk' subhash, after this function returns, is
# from uchar.h enum name, e.g. "UBLOCK_BASIC_LATIN", to Blocks.h
# pseudo-name, e.g. "Basic Latin".
#
# @param a hash ref from read_uchar.
# @param a hash ref from read_Blocks.
sub merge_Blocks {
    my ($h, $b) = @_;

    die "Error: No blocks data in uchar.h"
        unless (exists $h->{'blk'});
    my $blk = $h->{'blk'};
    for my $enum (keys %$blk) {
        my $cp = $blk->{$enum};
        if ($cp && !exists $b->{$cp}) {
            die "Error: No block found at $cp in Blocks.txt";
        }
        # Convert code point to pseudo-name:
        $blk->{$enum} = $b->{$cp};
        delete $b->{$cp};
    }
    my $err = '';
    for my $cp (keys %$b) {
        $err .= "Error: Block " . $b->{$cp} . " not listed in uchar.h\n";
    }
    die $err if ($err);
}

#----------------------------------------------------------------------
# Merge property alias names into the uchar.h hash.  The subhashes
# under the keys _* (b(inary, e(numerated, s(tring, d(ouble) are
# examined and the values of those subhashes are assumed to be long
# names in PropertyAliases.txt.  They are validated and replaced by
# "<short>|<long>".  Upon return, the read_PropertyAliases hash is
# emptied of all contents, except for those that failed to match.
# Unmatched names in PropertyAliases are listed as a warning but do
# NOT cause the script to die.
#
# @param a hash ref from read_uchar.
# @param a hash ref from read_PropertyAliases.
# @param a hash mapping long names to property family (e.g., 'binary')
sub merge_PropertyAliases {
    my ($h, $pa, $fam) = @_;

    for my $k (@TOP) {
        die "Error: No properties data for $k in uchar.h"
            unless (exists $h->{$k});
    }

    for my $subh (map { $h->{$_} } @TOP) {
        for my $enum (keys %$subh) {
            my $long_name = $subh->{$enum};
            if (!exists $pa->{$long_name}) {
                die "Error: Property $long_name not found (or used more than once)";
            }

            my $value;
            if($pa->{$long_name} =~ m|^n/a\d*$|) {
                # replace an "n/a" short name with an empty name (nothing before "|");
                # don't remove it (don't remove the "|"): there must always be a long name,
                # and if the short name is removed, then the long name becomes the
                # short name and there is no long name left (unless there is another alias)
                $value = "|" . $long_name;
            } else {
                $value = $pa->{$long_name} . "|" . $long_name;
            }
            if (exists $additional_property_aliases{$long_name}) {
                $value .= "|" . $additional_property_aliases{$long_name};
            }
            $subh->{$enum} = $value;
            delete $pa->{$long_name};
        }
    }

    my @err;
    for my $name (keys %$pa) {
        $MISSING_FROM_UCHAR{$pa->{$name}} = 1;
        if (exists $UNSUPPORTED{$name}) {
            push @err, "Info: No enum for " . $fam->{$name} . " property $name in uchar.h";
        } elsif (!isIgnoredProperty($name)) {
            push @err, "Warning: No enum for " . $fam->{$name} . " property $name in uchar.h";
        }
    }
    print join("\n", sort @err), "\n" if (@err);
}

#----------------------------------------------------------------------
# Return 1 if two names match ignoring whitespace, '-', and '_'.
# Used to match names in Blocks.txt with those in PropertyValueAliases.txt
# as of Unicode 4.0.
sub matchesLoosely {
    my ($a, $b) = @_;
    $a =~ s/[\s\-_]//g;
    $b =~ s/[\s\-_]//g;
    $a =~ /^$b$/i;
}

#----------------------------------------------------------------------
# Merge PropertyValueAliases.txt data into the uchar.h hash.  All
# properties other than blk, _bp, and _ep are analyzed and mapped to
# the names listed in PropertyValueAliases.  They are then replaced
# with a string of the form "<short>|<long>".  The short or long name
# may be missing.
#
# @param a hash ref from read_uchar.
# @param a hash ref from read_PropertyValueAliases.
sub merge_PropertyValueAliases {
    my ($h, $va) = @_;

    my %gcCount;
    for my $prop (keys %$h) {
        # _bp, _ep handled in merge_PropertyAliases
        next if ($prop =~ /^_/);

        # Special case: gcm
        my $prop2 = ($prop eq 'gcm') ? 'gc' : $prop;

        # find corresponding PropertyValueAliases data
        die "Error: Can't find $prop in PropertyValueAliases.txt"
            unless (exists $va->{$prop2});
        my $pva = $va->{$prop2};

        # match up data
        my $hh = $h->{$prop};
        for my $enum (keys %$hh) {

            my $name = $hh->{$enum};

            # look up both long and short & ignore case
            my $n;
            if (exists $pva->{$name}) {
                $n = $name; 
            } else {
                # iterate (slow)
                for my $a (keys %$pva) {
                    # case-insensitive match
                    # & case-insensitive reverse match
                    if ($a =~ /^$name$/i ||
                        $pva->{$a} =~ /^$name$/i) {
                        $n = $a;
                        last;
                    }
                }
            }
                
            # For blocks, do a loose match from Blocks.txt pseudo-name
            # to PropertyValueAliases long name.
            if (!$n && $prop eq 'blk') {
                for my $a (keys %$pva) {
                    # The block is only going to match the long name,
                    # but we check both for completeness.  As of Unicode
                    # 4.0, blocks do not have short names.
                    if (matchesLoosely($name, $pva->{$a}) ||
                        matchesLoosely($name, $a)) {
                        $n = $a;
                        last;
                    }
                }
            }
            
            die "Error: Property value $prop:$name not found" unless ($n);

            my $l = $n;
            my $r = $pva->{$n};
            # convert |n/a\d*| to blank
            $l = '' if ($l =~ m|^n/a\d*$|);
            $r = '' if ($r =~ m|^n/a\d*$|);

            $hh->{$enum} = "$l|$r";
            # Don't delete the 'gc' properties because we need to share
            # them between 'gc' and 'gcm'.  Count each use instead.
            if ($prop2 eq 'gc') {
                ++$gcCount{$n};
            } else {
                delete $pva->{$n};
            }
        }
    }

    # Merge the combining class values in manually
    # Add the same values to the synthetic lccc and tccc properties
    die "Error: No ccc data"
        unless exists $va->{'ccc'};
    for my $ccc (keys %{$va->{'ccc'}}) {
        die "Error: Can't overwrite ccc $ccc"
            if (exists $h->{'ccc'}->{$ccc});
        $h->{'lccc'}->{$ccc} =
        $h->{'tccc'}->{$ccc} =
        $h->{'ccc'}->{$ccc} = $va->{'ccc'}->{$ccc};
    }
    delete $va->{'ccc'};

    # Merge synthetic binary property values in manually.
    # These are the "True" and "False" value aliases.
    die "Error: No True/False value aliases"
        unless exists $va->{'binprop'};
    for my $bp (keys %{$va->{'binprop'}}) {
        $h->{'binprop'}->{$bp} = $va->{'binprop'}->{$bp};
    }
    delete $va->{'binprop'};

    my $err = '';
    for my $prop (sort keys %$va) {
        my $hh = $va->{$prop};
        for my $subkey (sort keys %$hh) {
            # 'gc' props are shared with 'gcm'; make sure they were used
            # once or twice.
            if ($prop eq 'gc') {
                my $n = $gcCount{$subkey};
                next if ($n >= 1 && $n <= 2);
            }
            $err .= "Warning: Enum for value $prop:$subkey not found in uchar.h\n"
                unless exists $MISSING_FROM_UCHAR{$prop};
        }
    }
    print $err if ($err);
}

#----------------------------------------------------------------------
# Read the PropertyAliases.txt file.  Return a hash that maps the long
# name to the short name.  The special key '_version' will map to the
# Unicode version of the file.  The special key '_family' holds a
# subhash that maps long names to a family string, for descriptive
# purposes.
#
# @param a filename for PropertyAliases.txt
# @param reference to hash to receive data.  Keys are long names.
# Values are short names.
sub read_PropertyAliases {

    my $hash = shift;         # result

    my $filename = shift; 

    my $fam = {};  # map long names to family string
    $fam = $hash->{'_family'} if (exists $hash->{'_family'});

    my $family; # binary, enumerated, etc.

    my $in = new FileHandle($filename, 'r');
    die "Error: Cannot open $filename" if (!defined $in);

    while (<$in>) {

        # Read version (embedded in a comment)
        if (/PropertyAliases-(\d+\.\d+\.\d+)/i) {
            die "Error: Multiple versions in $filename"
                if (exists $hash->{'_version'});
            $hash->{'_version'} = $1;
        }

        # Read family heading
        if (/^\s*\#\s*(.+?)\s*Properties\s*$/) {
            $family = $1;
        }

        # Ignore comments and blank lines
        s/\#.*//;
        next unless (/\S/);

        if (/^\s*(.+?)\s*;/) {
            my $short = $1;
            my @fields = /;\s*([^\s;]+)/g;
            if (@fields < 1 || @fields > 2) {
                my $number = @fields;
                die "Error: Wrong number of fields ($number) in $filename at $_";
            }

            # Make "n/a" strings unique
            if ($short eq 'n/a') {
                $short .= sprintf("%03d", $propNA++);
            }
            my $long = $fields[0];
            if ($long eq 'n/a') {
                $long .= sprintf("%03d", $propNA++);
            }

            # Add long name->short name to the hash=pa hash table
            if (exists $hash->{$long}) {
                die "Error: Duplicate property $long in $filename"
            }
            $hash->{$long} = $short;
            $fam->{$long} = $family;

            # Add the list of further aliases to the additional_property_aliases hash table,
            # using the long property name as the key.
            # For example:
            #   White_Space->space|outer_space
            if (@fields > 1) {
                my $value = pop @fields;
                while (@fields > 1) {
                    $value .= "|" . pop @fields;
                }
                $additional_property_aliases{$long} = $value;
            }
        } else {
            die "Error: Can't parse $_ in $filename";
        }
    }

    $in->close();

    $hash->{'_family'} = $fam;
}

#----------------------------------------------------------------------
# Read the PropertyValueAliases.txt file.  Return a two level hash
# that maps property_short_name:value_short_name:value_long_name.  In
# the case of the 'ccc' property, the short name is the numeric class
# and the long name is "<short>|<long>".  The special key '_version'
# will map to the Unicode version of the file.
#
# @param a filename for PropertyValueAliases.txt
#
# @return a hash reference.
sub read_PropertyValueAliases {

    my $hash = shift;         # result

    my $filename = shift; 

    my $in = new FileHandle($filename, 'r');
    die "Error: Cannot open $filename" if (!defined $in);

    while (<$in>) {

        # Read version (embedded in a comment)
        if (/PropertyValueAliases-(\d+\.\d+\.\d+)/i) {
            die "Error: Multiple versions in $filename"
                if (exists $hash->{'_version'});
            $hash->{'_version'} = $1;
        }

        # Ignore comments and blank lines
        s/\#.*//;
        next unless (/\S/);

        if (/^\s*(.+?)\s*;/i) {
            my $prop = $1;
            my @fields = /;\s*([^\s;]+)/g;
            die "Error: Wrong number of fields in $filename"
                if (@fields < 2 || @fields > 3);
            # Make "n/a" strings unique
            $fields[0] .= sprintf("%03d", $valueNA++) if ($fields[0] eq 'n/a');
            # Squash extra fields together
            while (@fields > 2) {
                my $f = pop @fields;
                $fields[$#fields] .= '|' . $f;
            }
            addDatum($hash, $prop, @fields);
        }

        else {
            die "Error: Can't parse $_ in $filename";
        }
    }

    $in->close();

    # Script Copt=Qaac (Coptic) is a special case.
    # Before the Copt code was defined, the private-use code Qaac was used.
    # Starting with Unicode 4.1, PropertyValueAliases.txt contains
    # Copt as the short name as well as Qaac as an alias.
    # For use with older Unicode data files, we add here a Qaac->Coptic entry.
    # This should not do anything for 4.1-and-later Unicode data files.
    # See also UAX #24: Script Names http://www.unicode.org/unicode/reports/tr24/
    $hash->{'sc'}->{'Qaac'} = 'Coptic'
        unless (exists $hash->{'sc'}->{'Qaac'} || exists $hash->{'sc'}->{'Copt'});

    # Add T|True and F|False -- these are values we recognize for
    # binary properties (NOT from PropertyValueAliases.txt).  These
    # are of the same form as the 'ccc' value aliases.
    $hash->{'binprop'}->{'0'} = 'F|False';
    $hash->{'binprop'}->{'1'} = 'T|True';
}

#----------------------------------------------------------------------
# Read the Blocks.txt file.  Return a hash that maps the code point
# range start to the block name.  The special key '_version' will map
# to the Unicode version of the file.
#
# As of Unicode 4.0, the names in the Blocks.txt are no longer the
# proper names.  The proper names are now listed in PropertyValueAliases.
# They are similar but not identical.  Furthermore, 4.0 introduces
# a new block name, No_Block, which is listed only in PropertyValueAliases
# and not in Blocks.txt.  As a result, we handle blocks as follows:
#
# 1. Read Blocks.txt to map code point range start to quasi-block name.
# 2. Add to Blocks.txt a synthetic No Block code point & name:
#    X -> No Block
# 3. Map quasi-names from Blocks.txt (including No Block) to actual
#    names from PropertyValueAliases.  This occurs in
#    merge_PropertyValueAliases.
#
# @param a filename for Blocks.txt
#
# @return a ref to a hash.  Keys are code points, as text, e.g.,
# "1720".  Values are pseudo-block names, e.g., "Hanunoo".
sub read_Blocks {

    my $filename = shift; 

    my $hash = {};         # result

    my $in = new FileHandle($filename, 'r');
    die "Error: Cannot open $filename" if (!defined $in);

    while (<$in>) {

        # Read version (embedded in a comment)
        if (/Blocks-(\d+\.\d+\.\d+)/i) {
            die "Error: Multiple versions in $filename"
                if (exists $hash->{'_version'});
            $hash->{'_version'} = $1;
        }

        # Ignore comments and blank lines
        s/\#.*//;
        next unless (/\S/);

        if (/^([0-9a-f]+)\.\.[0-9a-f]+\s*;\s*(.+?)\s*$/i) {
            die "Error: Duplicate range $1 in $filename"
                if (exists $hash->{$1});
            $hash->{$1} = $2;
        }

        else {
            die "Error: Can't parse $_ in $filename";
        }
    }

    $in->close();

    # Add pseudo-name for No Block
    $hash->{'none'} = 'No Block';

    $hash;
}

#----------------------------------------------------------------------
# Read the uscript.h file and compile a mapping of Unicode symbols to
# icu4c enum values.
#
# @param a filename for uscript.h
#
# @return a ref to a hash.  The keys of the hash are enum symbols from
# uscript.h, and the values are script names.
sub read_uscript {

    my $filename = shift; 

    my $mode = '';         # state machine mode and submode
    my $submode = '';

    my $last = '';         # for line folding

    my $hash = {};         # result
    my $key;               # first-level key

    my $in = new FileHandle($filename, 'r');
    die "Error: Cannot open $filename" if (!defined $in);

    while (<$in>) {
        # Fold continued lines together
        if (/^(.*)\\$/) {
            $last = $1;
            next;
        } elsif ($last) {
            $_ = $last . $_;
            $last = '';
        }

        # Exit all modes here
        if ($mode && $mode ne 'DEPRECATED') {
            if (/^\s*\}/) {
                $mode = '';
                next;
            }
        }

        # Handle individual modes

        if ($mode eq 'UScriptCode') {
            if (m|^\s*(USCRIPT_\w+).+?/\*\s*(\w+)|) {
                my ($enum, $code) = ($1, $2);
                die "Error: Duplicate script $enum"
                    if (exists $hash->{$enum});
                $hash->{$enum} = $code;
            }
        }

        elsif ($mode eq 'DEPRECATED') {
            if (/\s*\#ifdef/) {
                die "Error: Nested #ifdef";
                }
            elsif (/\s*\#endif/) {
                $mode = '';
            }
        }

        elsif (!$mode) {
            if (/^\s*typedef\s+enum\s+(\w+)\s*\{/ ||
                   /^\s*typedef\s+enum\s+(\w+)\s*$/) {
                $mode = $1;
                #print "Parsing $mode\n";
            }

            elsif (/^\s*\#ifdef\s+ICU_UCHAR_USE_DEPRECATES\b/) {
                $mode = 'DEPRECATED';
            }
        }
    }

    $in->close();

    $hash;
}

#----------------------------------------------------------------------
# Read the uchar.h file and compile a mapping of Unicode symbols to
# icu4c enum values.
#
# @param a filename for uchar.h
#
# @return a ref to a hash.  The keys of the hash are '_bp' for binary
# properties, '_ep' for enumerated properties, '_dp'/'_sp'/'_mp' for
# double/string/mask properties, and 'gc', 'gcm', 'bc', 'blk',
# 'ea', 'dt', 'jt', 'jg', 'lb', or 'nt' for corresponding property
# value aliases.  The values of the hash are subhashes.  The subhashes
# have a key of the uchar.h enum symbol, and a value of the alias
# string (as listed in PropertyValueAliases.txt).  NOTE: The alias
# string is whatever alias uchar.h lists.  This may be either short or
# long, depending on the specific enum.  NOTE: For blocks ('blk'), the
# value is a hex code point for the start of the associated block.
# NOTE: The special key _version will map to the Unicode version of
# the file.
sub read_uchar {

    my $filename = shift; 

    my $mode = '';         # state machine mode and submode
    my $submode = '';

    my $last = '';         # for line folding

    my $hash = {};         # result
    my $key;               # first-level key

    my $in = new FileHandle($filename, 'r');
    die "Error: Cannot open $filename" if (!defined $in);

    while (<$in>) {
        # Fold continued lines together
        if (/^(.*)\\$/) {
            $last .= $1;
            next;
        } elsif ($last) {
            $_ = $last . $_;
            $last = '';
        }

        # Exit all modes here
        if ($mode && $mode ne 'DEPRECATED') {
            if (/^\s*\}/) {
                $mode = '';
                next;
            }
        }

        # Handle individual modes

        if ($mode eq 'UProperty') {
            if (/^\s*(UCHAR_\w+)\s*[,=]/ || /^\s+(UCHAR_\w+)\s*$/) {
                if ($submode) {
                    addDatum($hash, $key, $1, $submode);
                    $submode = '';
                } else {
                    #print "Warning: Ignoring $1\n";
                }
            }

            elsif (m|^\s*/\*\*\s*(\w+)\s+property\s+(\w+)|i) {
                die "Error: Unmatched tag $submode" if ($submode);
                die "Error: Unrecognized UProperty comment: $_"
                    unless (exists $PROP_TYPE{$1});
                $key = $PROP_TYPE{$1};
                $submode = $2;
            }
        }

        elsif ($mode eq 'UCharCategory') {
            if (/^\s*(U_\w+)\s*=/) {
                if ($submode) {
                    addDatum($hash, 'gc', $1, $submode);
                    $submode = '';
                } else {
                    #print "Warning: Ignoring $1\n";
                }
            }

            elsif (m|^\s*/\*\*\s*([A-Z][a-z])\s|) {
                die "Error: Unmatched tag $submode" if ($submode);
                $submode = $1;
            }
        }

        elsif ($mode eq 'UCharDirection') {
            if (/^\s*(U_\w+)\s*[,=]/ || /^\s+(U_\w+)\s*$/) {
                if ($submode) {
                    addDatum($hash, $key, $1, $submode);
                    $submode = '';
                } else {
                    #print "Warning: Ignoring $1\n";
                }
            }

            elsif (m|/\*\*\s*([A-Z]+)\s|) {
                die "Error: Unmatched tag $submode" if ($submode);
                $key = 'bc';
                $submode = $1;
            }
        }

        elsif ($mode eq 'UBlockCode') {
            if (m|^\s*(UBLOCK_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'blk', $1, $2);
            }
        }

        elsif ($mode eq 'UEastAsianWidth') {
            if (m|^\s*(U_EA_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'ea', $1, $2);
            }
        }

        elsif ($mode eq 'UDecompositionType') {
            if (m|^\s*(U_DT_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'dt', $1, $2);
            }
        }

        elsif ($mode eq 'UJoiningType') {
            if (m|^\s*(U_JT_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'jt', $1, $2);
            }
        }

        elsif ($mode eq 'UJoiningGroup') {
            if (/^\s*(U_JG_(\w+))/) {
                addDatum($hash, 'jg', $1, $2) unless ($2 eq 'COUNT');
            }
        }

        elsif ($mode eq 'UGraphemeClusterBreak') {
            if (m|^\s*(U_GCB_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'GCB', $1, $2);
            }
        }

        elsif ($mode eq 'UWordBreakValues') {
            if (m|^\s*(U_WB_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'WB', $1, $2);
            }
        }

        elsif ($mode eq 'USentenceBreak') {
            if (m|^\s*(U_SB_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'SB', $1, $2);
            }
        }

        elsif ($mode eq 'ULineBreak') {
            if (m|^\s*(U_LB_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'lb', $1, $2);
            }
        }

        elsif ($mode eq 'UNumericType') {
            if (m|^\s*(U_NT_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'nt', $1, $2);
            }
        }

        elsif ($mode eq 'UHangulSyllableType') {
            if (m|^\s*(U_HST_\w+).+?/\*\[(.+?)\]\*/|) {
                addDatum($hash, 'hst', $1, $2);
            }
        }

        elsif ($mode eq 'DEPRECATED') {
            if (/\s*\#ifdef/) {
                die "Error: Nested #ifdef";
                }
            elsif (/\s*\#endif/) {
                $mode = '';
            }
        }

        elsif (!$mode) {
            if (/^\s*\#define\s+(\w+)\s+(.+)/) {
                # #define $left $right
                my ($left, $right) = ($1, $2);

                if ($left eq 'U_UNICODE_VERSION') {
                    my $version = $right;
                    $version = $1 if ($version =~ /^\"(.*)\"/);
                    # print "Unicode version: ", $version, "\n";
                    die "Error: Multiple versions in $filename"
                        if (defined $hash->{'_version'});
                    $hash->{'_version'} = $version;
                }

                elsif ($left =~ /U_GC_(\w+?)_MASK/) {
                    addDatum($hash, 'gcm', $left, $1);
                }
            }

            elsif (/^\s*typedef\s+enum\s+(\w+)\s*\{/ ||
                   /^\s*typedef\s+enum\s+(\w+)\s*$/) {
                $mode = $1;
                #print "Parsing $mode\n";
            }

            elsif (/^\s*enum\s+(\w+)\s*\{/ ||
                   /^\s*enum\s+(\w+)\s*$/) {
                $mode = $1;
                #print "Parsing $mode\n";
            }

            elsif (/^\s*\#ifdef\s+ICU_UCHAR_USE_DEPRECATES\b/) {
                $mode = 'DEPRECATED';
            }
        }
    }

    $in->close();

    # hardcode known values for the normalization quick check properties
    # see unorm.h for the UNormalizationCheckResult enum

    addDatum($hash, 'NFC_QC', 'UNORM_NO',    'N');
    addDatum($hash, 'NFC_QC', 'UNORM_YES',   'Y');
    addDatum($hash, 'NFC_QC', 'UNORM_MAYBE', 'M');

    addDatum($hash, 'NFKC_QC', 'UNORM_NO',    'N');
    addDatum($hash, 'NFKC_QC', 'UNORM_YES',   'Y');
    addDatum($hash, 'NFKC_QC', 'UNORM_MAYBE', 'M');

    # no "maybe" values for NF[K]D

    addDatum($hash, 'NFD_QC', 'UNORM_NO',    'N');
    addDatum($hash, 'NFD_QC', 'UNORM_YES',   'Y');

    addDatum($hash, 'NFKD_QC', 'UNORM_NO',    'N');
    addDatum($hash, 'NFKD_QC', 'UNORM_YES',   'Y');

    $hash;
}

#----------------------------------------------------------------------
# Add a new value to a two-level hash.  That is, given a ref to
# a hash, two keys, and a value, add $hash->{$key1}->{$key2} = $value.
sub addDatum {
    my ($h, $k1, $k2, $v) = @_;
    if (exists $h->{$k1}->{$k2}) {
        die "Error: $k1:$k2 already set to " .
            $h->{$k1}->{$k2} . ", cannot set to " . $v;
    }
    $h->{$k1}->{$k2} = $v;
}

#eof
