@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

#
# This program generates LUTs for converting between sRGB and scRGB.
# Also does round-trip test for sRGB -> scRGB -> sRGB, if both conversions use these tables.
#
# Run it from the directory the script is in.
#


use POSIX;

$incfile = "..\\..\\common\\shared\\GammaLUTs.inc";
$cppfile = "..\\..\\common\\shared\\GammaLUTs.cpp";

# This value is a small bias that can be added to the tosrgb table.

$roundingbias = 0;  # We don't need one for this case. (But it might be useful again, like it
                    # has been in the past, if we make changes e.g. increase the table size.)

# LUT sizes.
#
#   For tosrgb, the size is obtained by searching for a size which passes all the tests.
#       (This was done offline, using the same script, but in "search mode".)

# Set this to non-zero to enable "search mode":
$search_tosrgb = 0;

# If in "search mode": This is the first size to examine (must be > 1).
# Otherwise: This is the table size to use (and $end_tblsize_tosrgb is ignored).
$start_tblsize_tosrgb = 3355;

# If in "search mode": The last size to try in the search
$end_tblsize_tosrgb = 1 << 15;

# Whether to output test failure results:
$output_failures = !$search_tosrgb;


# The toscrgb table size is obvious because the table is designed for integer inputs from 0..255.
$tblsize_toscrgb = (1 << 8);

$tblname_tosrgb = "GammaLUT_scRGB_to_sRGB";
$tblname_toscrgb = "GammaLUT_sRGB_to_scRGB";



# +++ Begin tosrgb     # Mark this code so that we can include it in the generated comments.
sub scRGBTosRGB
{
   my ($x) = @_;       # PerlOnly
                       # PerlOnly
   if (!($x > 0.0))    # Handles NaN case too
   {
       return 0.0;
   }
   elsif ($x <= 0.0031308)
   {
       return $x * 12.92;
   }
   elsif ($x < 1.0)
   {
       return ((1.055 * pow($x, 1.0/2.4))-0.055);
   }
   else
   {
       return 1.0;
   }
}
# +++ End


# +++ Begin toscrgb    # Mark this code so that we can include it in the generated comments.
sub sRGBToscRGB
{
   my ($x) = @_;       # PerlOnly
                       # PerlOnly
   if (!($x > 0.0))    # Handles NaN case too
   {
      return 0.0;
   }
   elsif ($x <= 0.04045)
   {
      return $x / 12.92;
   }
   elsif ($x < 1.0)
   {
      return pow(($x + 0.055) / 1.055, 2.4);
   }
   else
   {
      return 1.0;
   }
}
# +++ End


# Read the script to capture source code (to be included in LUT comments)
# Sets %scriptcode.
#
# This is probably overengineering - now that we have decided on a gamma function,
# a bit of cut and paste probably wouldn't hurt here.
#
# But during development, it did change surprisingly many times.

sub parseScript
{
   open (SCRIPT, $0) || die "Could not open $0\n";
   $func = "";

   while (<SCRIPT>)
   {
      if (/^ *# \+\+\+ Begin ([a-zA-Z]+) *(#.*)?$/)
      {
         $func = $1;
         $scriptcode{$func} = "";
      }
      elsif (/^ *# \+\+\+ End *(#.*)?$/)
      {
         $func = "";
      }
      else
      {
         if ($func)
         {
            if (!/# PerlOnly$/)
            {
               # Conversion to C code
               s/elsif/else if/;
               s/^sub (.*)/double $1(double x)/;
               s/\$//g;
               s{#}{//};

               $scriptcode{$func} = $scriptcode{$func} . $_;
            }
         }
      }
   }
   close (SCRIPT);
}


sub outputIndentedComment
{
   my ($text) = @_;

   $text =~ s{^}{//   }mg;
   print CPP $text;
}

sub round
{
   POSIX::floor($_[0] + 0.5);
}

&parseScript;

# If searching is enabled, this loop searches through reasonable table sizes
# to find one which passes the tests.

$searching = $search_tosrgb;
my $scale_tosrgb;

TABLESIZE:
for ($tblsize_tosrgb = $start_tblsize_tosrgb;;
     $tblsize_tosrgb++)
{
   if ($searching && ($tblsize_tosrgb > $end_tblsize_tosrgb))
   {
      last TABLESIZE;
   }

   print "\n\nSize $tblsize_tosrgb:\n";
   $scale_tosrgb = $tblsize_tosrgb - 1;


   #
   # Generate the tables
   #

   for ($i=0; $i<$tblsize_tosrgb; $i++) {
      $val = $i / $scale_tosrgb;
      $lut_scrgb_to_srgb[$i] = round( 255 * &scRGBTosRGB($val) + $roundingbias);
   }

   for ($i=0; $i<$tblsize_toscrgb; $i++) {
      $val = $i / ($tblsize_toscrgb - 1);

      # The "255 *" factor seems undesirable. This table is currently only
      # used for gamma-correct gradients.

      $lut_srgb_to_scrgb[$i] = sprintf("%3.9f", 255 * &sRGBToscRGB($val));
   }

   #
   # Run some round-trip tests
   #

   # Test that TosRGB(ToscRGB(x))=x for all x.
   #
   # (This implementation of the test is not perfect, because in Perl I don't know a way to
   #  restrict floating-point precision to float.)

   # Test 1: Use the LUT version of sRGBToscRGB.
   #         This is what happens when we're given a DWORD sRGB color.

   $failed1=0;
   for ($i=0; $i<=255; $i++) {
      my $scrgb = $lut_srgb_to_scrgb[$i] / 255;
      my $srgb = $lut_scrgb_to_srgb[round(($scale_tosrgb * 1.0 * $scrgb))];

      if ($i != $srgb)
      {
         next TABLESIZE if !$output_failures;

         if (!$failed1)
         {
            print "\nRound-trip test failures, using LUT version of sRGBToscRGB:\n";
            $failed1 = 1;
         }

         print "$i -> $scrgb -> $srgb\n";
      }
   }

   # Test 2: As test 1, but use an accurate (i.e. non-LUT) version of ToscRGB.
   #
   #         This is what happens (in color.cs) when we're given a floating-point sRGB color.
   #         (Well, almost: this Perl code works in doubles, not floats)

   $failed2=0;
   for ($i=0; $i<=255; $i++) {
      my $scrgb = &sRGBToscRGB($i / 255.0);
      my $srgb = $lut_scrgb_to_srgb[round(($scale_tosrgb * $scrgb))];

      if ($i != $srgb)
      {
         next TABLESIZE if !$output_failures;

         if (!$failed2)
         {
            print "\nRound-trip test failures for n/255, using color.cs version of sRGBToscRGB:\n";
            $failed2 = 1;
         }

         print "$i -> $scrgb -> $srgb\n";
      }
   }

   # Test 3: As test 2, but test it for n/100 values instead of n/255.
   #         There's some expectation (from the test team, at least) that this rounds
   #         exactly the same as if there were no gamma conversion. Especially for 0.5 (i.e. 50/100).

   $failed3=0;

   for ($i=0; $i<=100; $i++) {
      my $srgbfloat = $i / 100.0;
      my $scrgb = &sRGBToscRGB($srgbfloat);
      my $srgb = $lut_scrgb_to_srgb[round(($scale_tosrgb * $scrgb))];

      $expected = round(255 * $srgbfloat);

      if ($srgb != $expected)
      {
         next TABLESIZE if !$output_failures;

         if (!$failed3)
         {
            print "\nRound-trip test failures for n/100, using color.cs version of sRGBToscRGB:\n";
            $failed3 = 1;
         }

         print "$srgbfloat (expect $expected) -> $scrgb -> $srgb\n";
      }
   }

   if (!$searching)
   {
      last TABLESIZE;
   }

   if (!$failed1 && !$failed2 && !$failed3)
   {
      print "Tests passed.\n";
      last TABLESIZE;
   }
}

if (!($searching && ($tblsize_tosrgb > $end_tblsize_tosrgb)))
{
   #
   # Output the tables
   #

   $scriptfilename = $0;
   $scriptfilename =~ s/^.*\\([^\\]*)/$1/;

   open (CPP, ">$cppfile") || die "Could not open $cppfile\n";
   open (INC, ">$incfile") || die "Could not open $incfile\n";

   if ($roundingbias)
   {
      $hrv_txt1 = ",\n// and also a rounding bias of $roundingbias.";
      $hrv_txt2 = "bias and table size are";
   }
   else
   {
      $hrv_txt1 = ".";
      $hrv_txt2 = "table size is";
   }

   my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
   $year += 1900;
   $mon++;

   my $name_size_tosrgb = $tblname_tosrgb."_size";

   my $text_scale_tosrgb = $scale_tosrgb.".0f";
   my $name_scale_tosrgb = $tblname_tosrgb."_scale";

   my $generationWarning = <<EOT;
// WARNING!
//
// This file is generated! Please don't edit it by hand - instead
// edit the script and rerun it. See top of file for script location.
EOT

   my $header = <<EOT;
//-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Look-up tables for gamma conversion (converting a color channel
//      from sRGB to scRGB, or vice versa).
//

EOT

   print INC $header;
   print CPP $header;

   print CPP <<EOT;
#include "precomp.hpp"

// Look-up table for converting an scRGB color channel to sRGB.
//
// To use this table:
//   * Preferably, just use Convert_scRGB_Channel_To_sRGB_Byte or something that uses it.
//   * Or if you must, do what it does ... be careful.
//
// Speaking approximately, you can call this a "gamma 1/2.2" table.
// But it actually uses the inverse of the sRGB gamma profile (which is a little different)$hrv_txt1
//
// The $hrv_txt2 chosen so that if you:
//   1) take an input floating-point sRGB value of n/255 or n/100 (n is an integer)
//   2) convert it to floating-point scRGB using the sRGB gamma profile
//   3) use this table to convert it back to an sRGB value
// you get the same result as round(x * 255).
//
// i.e. the fact that we're using scRGB internally is reasonably well hidden from users who
// are working entirely in sRGB.
//
// This table was generated by $scriptfilename
// Code for the corresponding function:
EOT

   &outputIndentedComment($scriptcode{"tosrgb"});

   print INC <<EOT;
const UINT $name_size_tosrgb = $tblsize_tosrgb;
const REAL $name_scale_tosrgb = $text_scale_tosrgb;

extern const BYTE $tblname_tosrgb [$name_size_tosrgb];

//+----------------------------------------------------------------------------
//
//  Function:  Convert_scRGB_Channel_To_sRGB_Byte
//
//  Synopsis:  Convert a color channel from scRGB to sRGB byte (0 to 255)
//
//  Returns:   Converted color channel
//
//-----------------------------------------------------------------------------

$generationWarning
inline BYTE Convert_scRGB_Channel_To_sRGB_Byte(
    float rColorComponent
    )
{
    if (!(rColorComponent > 0.0f))   // Handles the NaN case
        return 0;
    else if (rColorComponent < 1.0f)
    {
        // $name_scale_tosrgb is generated so we do know that index is correct
        // and hence can tell "__bound" to satisfy PreFast.
        __bound UINT index = CFloatFPU::RoundWithHalvesUp($name_scale_tosrgb * rColorComponent);
        return $tblname_tosrgb\[index];
    }
    else
        return 255;
}


//+----------------------------------------------------------------------------
//
//  Function:  Get_Smallest_scRGB_Significant_for_sRGB
//
//  Synopsis:  Return smallest argument for Convert_scRGB_Channel_To_sRGB_Byte
//             that causes nonzero result
//-----------------------------------------------------------------------------

$generationWarning
inline float Get_Smallest_scRGB_Significant_for_sRGB()
{
    float rColor = 0.5f / $name_scale_tosrgb;
    
    Assert(Convert_scRGB_Channel_To_sRGB_Byte(rColor) == 1);
    Assert(Convert_scRGB_Channel_To_sRGB_Byte(CFloatFPU::NextSmaller(rColor)) == 0);
    
    return rColor;
}

EOT

   print CPP "const BYTE $tblname_tosrgb [$name_size_tosrgb] = {    \n    ";

   for ($i=0; $i<$tblsize_tosrgb; $i++) {
       print CPP sprintf("%3d", $lut_scrgb_to_srgb[$i]);
       if ($i != ($tblsize_tosrgb - 1)) {
           print CPP ",";
           if (!(($i+1) & 7)) {
               print CPP "\n    ";
           } else {
               print CPP " ";
           }
       }
   }
   print CPP <<EOT;
};


// Look-up table for converting an sRGB color channel to scRGB.
//
// To use on a channel, use the following code:
//
//   REAL ConvertChannel_sRGB_scRGB(BYTE bInput)
//   {
//       return $tblname_toscrgb\[bInput];
//   }
//
// Speaking approximately, you can call this a "gamma 2.2" table.
// But it actually uses the sRGB gamma profile (which is a little different).
//
//
// This table was generated by $scriptfilename
// Code for the corresponding function:
EOT

   &outputIndentedComment($scriptcode{"toscrgb"});

   print INC <<EOT;
$generationWarning
extern const REAL $tblname_toscrgb [$tblsize_toscrgb];
EOT
   print CPP "\nconst REAL $tblname_toscrgb [$tblsize_toscrgb] = {\n    ";

   for ($i=0; $i<$tblsize_toscrgb; $i++) {
       print CPP $lut_srgb_to_scrgb[$i] . "f";
       if ($i != ($tblsize_toscrgb - 1)) {
           print CPP ",";
           if (!(($i+1) & 3)) {
               print CPP "\n    ";
           } else {
               print CPP " ";
           }
       }
   }
   print CPP "\n};\n\n";

   close(CPP);
   close(INC);
}

print "Tables written to $incfile, $cppfile\n";




