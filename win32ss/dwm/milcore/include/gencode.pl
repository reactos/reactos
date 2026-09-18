# Licensed to the .NET Foundation under one or more agreements.
# The .NET Foundation licenses this file to you under the MIT license.
# See the LICENSE file in the project root for more information.


use strict;
use warnings;
use FileHandle;

my $wcp_version_suffix = '';
$wcp_version_suffix = $ENV{"WCP_VERSION_SUFFIX"} if defined $ENV{"WCP_VERSION_SUFFIX"};

my $mil_version_suffix = '';
$mil_version_suffix = $ENV{"MIL_VERSION_SUFFIX"} if defined $ENV{"MIL_VERSION_SUFFIX"};

my $doingStruct = 0;
my $doingEnum = 0;
my $isTypedef= 0;
my $doingPublic = 0;
my $csEnumPrefix = '';

my $cppDoingEnum = 0;
my $cppEnumType = '';
my $cppEnumScopePrefix = '';
my $cppEnumCompatDefines = '';

#################################################################
#
#   Sub: TranslateCSLine
#
#   This method converts a single C++ line of code to C#.
#   Currently only enumerations and simple structures
#   (structures without string or union members) are supported
#
##################################################################
sub TranslateCSLine
{
    my ($csfile, $line) = @_;
    my $dummy;
    my $decl;
    my ($prefix, $name, $equals, $value, $suffix);
    my ($space, $flag);

    # Some preprocessor directives (#ifndef MILCORE_KERNEL_COMPONENT)
    # are in common but only apply to CPP (thankfully), so strip them
    if($line =~ /^\#ifndef/ || $line =~ /^\#endif/)
    {
        return;
    }

    # Indent line 4 spaces over
    print $ csfile "    ";

    # Translate lines within a structure definitions
    if( $doingStruct == 1)
    {
        # Recognize end of strucure definition
        if($line =~ /}/)
        {
            if ($isTypedef != 0)
            {
                $line = "\};\n";
                $isTypedef = 0;
            }
            $doingStruct = 0;
        }

        # if the line has a comment at the beginning, just print it
        if ($line =~ /^\s*\/\//)
        {
           print $csfile "$line";
        }
        # if the line contains alpha, assume there is a
        # member and append "internal" or "public" to it
        # (depending on doingPublic value)
        elsif( $line =~ /\w+/)
        {
            # Match whitespace, then anything else
            $line =~ /( *)(.*)/;

            # Print Whitespace + "internal" or "public",  & save everything else
            if ($doingPublic == 1)
            {
               print $csfile "$1public ";
            }
            else
            {
               print $csfile "$1internal ";
            }

            $line = $2;

            # Recognize pointer type
            if( $line =~ /(.*)\*(.*)/)
            {
                print $csfile "IntPtr $2\n";
            }
            else
            {
                # Remove ::Enum or ::Flags from typename
                $line =~ s/::Enum//;
                $line =~ s/::Flags//;
              
                # Translate data type
                if( $line =~ /(.*)UINT32(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)ULONG(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)UINT16(.*)/)
                {
                    print $csfile "$1UInt16$2\n";
                }
                elsif( $line =~ /(.*)BYTE(.*)/)
                {
                    print $csfile "$1Byte$2\n";
                }
                elsif( $line =~ /(.*)INT32(.*)/)
                {
                    print $csfile "$1Int32$2\n";
                }
                elsif( $line =~ /(.*)WMG_HANDLE64(.*)/)
                {
                    print $csfile "$1UInt64$2\n";
                }
                elsif( $line =~ /(.*)WMG_HANDLE(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)HMIL_OBJECT(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)HMIL_CONTEXT(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)HMIL_RESOURCE(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)HMIL_COMPNODE(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)MIL_VERTEXFORMAT(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)MILRTInitializationFlags(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)MILGradientWrapMode(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)MILBitmapWrapMode(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)MILBitmapOptions(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)GUID(.*)/)
                {
                    print $csfile "$1Guid$2\n";
                }
                elsif( $line =~ /(.*)HWND(.*)/)
                {
                    print $csfile "$1IntPtr$2\n";
                }
                elsif( $line =~ /(.*)HANDLE(.*)/)
                {
                    print $csfile "$1IntPtr$2\n";
                }
                elsif( $line =~ /(.*)PVOID(.*)/)
                {
                    print $csfile "$1IntPtr$2\n";
                }
                elsif( $line =~ /(.*)UINT(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)DWORD(.*)/)
                {
                    print $csfile "$1UInt32$2\n";
                }
                elsif( $line =~ /(.*)WORD(.*)/)
                {
                    print $csfile "$1UInt16$2\n";
                }
                elsif( $line =~ /(.*)MilPixelFormat::Enum(.*)/)
                {
                    print $csfile "$1Int32$2\n";
                }
                elsif( $line =~ /(.*)BOOL(.*)/)
                {
                    print $csfile "$1Int32$2\n";
                }
                elsif( $line =~ /(.*)FLOAT(.*)/)
                {
                    print $csfile "$1float$2\n";
                }
                elsif( $line =~ /(.*)DOUBLE(.*)/)
                {
                    print $csfile "$1double$2\n";
                }
                # Data type not recognized, print it as-is
                else
                {
                    print $csfile "$line\n";
                }
            }

        }
        # No alpha characters, print line as-is
        else
        {
            print $csfile $line;
        }

    }
    # Translate a line inside an enum
    elsif( $doingEnum == 1)
    {
        # Replace END_MIL*ENUM with dword force value and closing }
        $line =~ s/(\s*)END_MIL(FLAG)?ENUM/$1    FORCE_DWORD = unchecked((int)0xffffffff)\n    $1};/;

        # Match an enum identifier being assigned to a number
        # e.g.  MyEnumConstant = 0x1000,
        if ( ($prefix, $name, $equals, $value, $suffix) =
            $line =~ /(\W*)(\w+)([^=]+=\s*)([x0-9A-Fa-f]+)(.*)/)
        {
            print $csfile "$prefix$csEnumPrefix$name$equals";

            # If the number is greater than INT_MAX,
            # the cast needs to be cast as unchecked
            if( hex $value >= 0x8000000)
            {
                print $csfile "unchecked((int)$value)";
            }
            # Otherwise print line as-is
            else
            {
                print $csfile $value;
            }

            print $csfile "$suffix\n";

        }
        # Otherwise print line as-is
        else
        {
            # Recognize end of enum definition
            if($line =~ /}/)
            {
                if ($isTypedef != 0)
                {
                    $line = "\};\n";
                    $isTypedef = 0;
                }
                $doingEnum = 0;
                $csEnumPrefix = '';
            }
            print $csfile "$line";
        }
    }
    # Translate first line of an enum definition
    # if the enum is part of a typedef, then the enum name must be 
    # the typedef name prefixed with an underscore, e.g.:
    # typedef enum _FOO_ENUM {
    # } FOO_ENUM;
    elsif( $line =~ /enum( |\n)/ )
    {
        ($decl) = $line =~ /enum\s+(\w+)/;
        if ($line =~ /typedef\s+/)
        {
            # trim any prefixed '_' off the enum name 
            $decl =~ s/^_//;
            $isTypedef = 1;
        }

        # Do public declerations if specified
        if( $doingPublic == 1 )
        {
           print $csfile "public enum $decl";
        }
        # Otherwise declerations should be internal
        else
        {
           print $csfile "internal enum $decl";
        }

        $doingEnum = 1;
    }
    # Translate first line of a structure definition
    # if the struct is part of a typedef, then the struct name must be 
    # the typedef name prefixed with an underscore, e.g.:
    # typedef struct _FOO_STRUCT {
    # } FOO_STRUCT;
    elsif($line =~ /struct( |\n)/)
    {
        ($decl) = $line =~ /struct\s+(\w+)/;
        if ($line =~ /typedef\s+/)
        {
            # trim any prefixed '_' off the struct name 
            $decl =~ s/^_//;
            $isTypedef = 1;
        }
        
        print $csfile "[StructLayout(LayoutKind.Sequential, Pack=1)]\n";

        # Do public declerations if specified
        if( $doingPublic == 1 )
        {
           print $csfile "    public struct $decl";
        }
        else
        {
           print $csfile "    internal struct $decl";
        }

        $doingStruct = 1;
    }
    # Translate first line of an flag enum definition
    elsif( ($space, $flag, $name, my $compatOptions) =
           ($line =~ /^(\s*)BEGIN_MIL(FLAG|)ENUM\s*\(\s*(\w+)\s*\)(?:\s*\/\/\s*(.*))?/ ) )
    {
        if ($compatOptions)
        {
            $csEnumPrefix = $1 if ($compatOptions =~ /CSPrefix:(\S+)/);
        }

        if ($flag)
        {
            print $csfile "$space\[System.Flags]\n    ";
            $name .= "Flags";
        }

        # Do public declerations if specified
        # Otherwise declerations should be internal
        my $exposure = ( $doingPublic == 1 ) ? "public" : "internal";

        print $csfile "$space$exposure enum $name\n    $space\{\n";

        $doingEnum = 1;
    }
    # No translation necessary - print line
    else
    {
        print $csfile $line;
    }
}

#################################################################
#
#   Sub: ProcessCPPLine
#
#   This method fixes up a single C++ line of code.
#   Currently does nothing much.
#
##################################################################
sub ProcessCPPLine
{
    my ($cppfile, $line) = @_;
    print $cppfile $line;
}

#######################################################
#
#   Sub:  WriteOnlySection
#
#   This method simple prints an entire block
#   the the output file, ignoring comments
#
#######################################################
sub WriteOnlySection
{
    my($infile, $outfile) = @_;

    my $currentLine = <$infile>;

    # Until end of block is encountered
    until( $currentLine =~ /;end/ )
    {
        if ($currentLine =~ /^\s*;include_header\s+(\S+)$/)
        {
            IncludeHeaderFile($1, $outfile, 0);
        }
        elsif ($currentLine =~ /^\s*;include\s+(\S+)$/)
        {
            IncludeExternalFile($1, $outfile);
        }
        elsif ($currentLine =~ /(.*)WCP_VERSION_SUFFIX(.*)/)
        {
            print $outfile "$1$wcp_version_suffix$2\n";
        }
        elsif ($currentLine =~ /(.*)MIL_VERSION_SUFFIX(.*)/)
        {
            print $outfile "$1$mil_version_suffix$2\n";
        }
        # Ignore comment lines that begin with a ;
        elsif(!($currentLine =~ /^(\;)/))
        {
            print $outfile $currentLine;
        }

        $currentLine = <$infile>;
    }
}

#######################################################
#
#   Sub:  WriteCommonSection
#
#   Writes a common section to both the cpp and cs files.
#   Because the COMMON template definitions are in C++,
#   the lines are printed as-is to the C++ file.
#   TranslateCSLine is called to translate lines
#   to C# for the cs file.
#
#######################################################
sub WriteCommonSection
{
    my($infile, $cppfile, $csfile) = @_;

    my $currentLine = <$infile>;

    until( $currentLine =~ /;end/ )
    {
        if ($currentLine =~ /^\s*;include_header\s+(\S+)$/)
        {
            IncludeHeaderFile($1, $cppfile, $csfile);
        }
        elsif ($currentLine =~ /^\s*;include\s+(\S+)$/)
        {
            IncludeExternalFile($1, $csfile);
            IncludeExternalFile($1, $cppfile);
        }
        # Ignore comment lines that begin with a ;
        elsif (!($currentLine =~ /^(\;)/))
        {
            ProcessCPPLine($cppfile, $currentLine);
            TranslateCSLine($csfile, $currentLine);
        }

        $currentLine = <$infile>;
    }
}

#######################################################
#
#   Sub:  IncludeExternalFile
#
#   This routine will load the specified file
#   and save it out to the appropriate stream
#
#######################################################
sub IncludeExternalFile
{
    my($fileName, $outfile) = @_;

    open(EXTERNALFILE, $fileName) or die "Unable to open file: $fileName";

    while(<EXTERNALFILE>)
    {
        print $outfile $_;
    }

    close(EXTERNALFILE);
}

#######################################################
#
#   Sub:  IncludeHeaderFile
#
#   This routine will load the specified header file
#   and save it out to the appropriate stream.
#
#   The initial comments and #pragma once directive
#   are stripped. The C# version is indented and 
#   translated.
#
#######################################################
sub IncludeHeaderFile
{
    my($fileName, $cppfile, $csfile) = @_;

    open(EXTERNALFILE, $fileName) or die "Unable to open file: $fileName";

    # Skip the initial comment header
    while(<EXTERNALFILE> =~ /^\/\//)
    {
    }
    
    # Translate and indent the rest of the header
    while(<EXTERNALFILE>)
    {
        my $currentLine = $_;
        
        # Skip the #pragma once directive
        if (!($currentLine =~ /#pragma once/))
        {
            print $cppfile $currentLine;

            if ($csfile != 0)
            {
                TranslateCSLine($csfile, $currentLine);
            }
        }
    }

    close(EXTERNALFILE);
}

##########################
#
#   Script entry point
#
##########################
my $dummy;
my $templatefile;
my $csfile;
my $cppfile;

# Parse command line
foreach my $arg (@ARGV)
{
    if( $arg =~ /-template=.*/ )
    {

        ($dummy, $templatefile) = split(/=/, $arg, 2);
    }
    elsif( $arg =~ /-csfile=.*/ )
    {
        ($dummy, $csfile) = split(/=/, $arg, 2);
    }
    elsif( $arg =~ /-cppfile=.*/ )
    {
        ($dummy, $cppfile) = split(/=/, $arg, 2);
    }
}

# Validate command line
die "Usage: gencode.pl -template=<template file> -cppfile=<C++ file name> -csfile=<C# file name>\n"
    unless ( defined($templatefile) && defined($csfile) && defined($cppfile) );

print "Generating output files from: $templatefile\n";

# Open files
open(TEMPLATE, $templatefile) or die "Unable to open file: $templatefile\n";
open(CSFILE, ">$csfile") or die "Unable to open file: $csfile\n";
open(CPPFILE, ">$cppfile") or die "Unable to open file: $cppfile\n";

# Recognize, then process each block within the template file
while( <TEMPLATE> )
{
    # If this is a CSHARP_ONLY block
    if( $_ =~ /;begin +CSHARP_ONLY/ )
    {
       WriteOnlySection(\*TEMPLATE, \*CSFILE);
    }
    #If this is a CPP_ONLY block
    elsif( $_ =~ /;begin +CPP_ONLY/ )
    {
        WriteOnlySection(\*TEMPLATE, \*CPPFILE);
    }
    #If this is a common block
    elsif( $_ =~ /;begin +COMMON/ )
    {
        WriteCommonSection(\*TEMPLATE, \*CPPFILE, \*CSFILE);
    }
    # If all the definitions below this block need to be public
    # Note: This limits public sections to be outside the CPP/CSHARP/COMMON sections
    elsif($_ =~ /;begin +PUBLIC/)
    {
       $doingPublic = 1;
    }
    # If all the definitions below this block need not be public
    elsif($_ =~ /;end +PUBLIC/)
    {
       $doingPublic = 0;
    }

}

# Close Files
close(TEMPLATE);
close(CSFILE);
close(CPPFILE);

print "Done!\n";

