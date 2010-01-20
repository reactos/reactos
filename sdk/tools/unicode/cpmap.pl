#!/usr/bin/perl
#
# Generate code page .c files from ftp.unicode.org descriptions
#
# Copyright 2000 Alexandre Julliard
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

# base directory for ftp.unicode.org files
$BASEDIR = "ftp.unicode.org/Public/";
$MAPPREFIX = $BASEDIR . "MAPPINGS/";

# UnicodeData file
$UNICODEDATA = $BASEDIR . "UNIDATA/UnicodeData.txt";

# Sort keys file
$SORTKEYS = "www.unicode.org/reports/tr10/allkeys.txt";

# Defaults mapping
$DEFAULTS = "./defaults";

# Default char for undefined mappings
$DEF_CHAR = ord '?';

@allfiles =
(
    [ 37,    "VENDORS/MICSFT/EBCDIC/CP037.TXT",   0, "IBM EBCDIC US Canada" ],
    [ 424,   "VENDORS/MISC/CP424.TXT",            0, "IBM EBCDIC Hebrew" ],
    [ 437,   "VENDORS/MICSFT/PC/CP437.TXT",       1, "OEM United States" ],
    [ 500,   "VENDORS/MICSFT/EBCDIC/CP500.TXT",   0, "IBM EBCDIC International" ],
    [ 737,   "VENDORS/MICSFT/PC/CP737.TXT",       1, "OEM Greek 437G" ],
    [ 775,   "VENDORS/MICSFT/PC/CP775.TXT",       1, "OEM Baltic" ],
    [ 850,   "VENDORS/MICSFT/PC/CP850.TXT",       1, "OEM Multilingual Latin 1" ],
    [ 852,   "VENDORS/MICSFT/PC/CP852.TXT",       1, "OEM Slovak Latin 2" ],
    [ 855,   "VENDORS/MICSFT/PC/CP855.TXT",       1, "OEM Cyrillic" ],
    [ 856,   "VENDORS/MISC/CP856.TXT",            0, "Hebrew PC" ],
    [ 857,   "VENDORS/MICSFT/PC/CP857.TXT",       1, "OEM Turkish" ],
    [ 860,   "VENDORS/MICSFT/PC/CP860.TXT",       1, "OEM Portuguese" ],
    [ 861,   "VENDORS/MICSFT/PC/CP861.TXT",       1, "OEM Icelandic" ],
    [ 862,   "VENDORS/MICSFT/PC/CP862.TXT",       1, "OEM Hebrew" ],
    [ 863,   "VENDORS/MICSFT/PC/CP863.TXT",       1, "OEM Canadian French" ],
    [ 864,   "VENDORS/MICSFT/PC/CP864.TXT",       0, "OEM Arabic" ],
    [ 865,   "VENDORS/MICSFT/PC/CP865.TXT",       1, "OEM Nordic" ],
    [ 866,   "VENDORS/MICSFT/PC/CP866.TXT",       1, "OEM Russian" ],
    [ 869,   "VENDORS/MICSFT/PC/CP869.TXT",       1, "OEM Greek" ],
    [ 874,   "VENDORS/MICSFT/PC/CP874.TXT",       1, "ANSI/OEM Thai" ],
    [ 875,   "VENDORS/MICSFT/EBCDIC/CP875.TXT",   0, "IBM EBCDIC Greek" ],
    [ 878,   "VENDORS/MISC/KOI8-R.TXT",           0, "Russian KOI8" ],
    [ 932,   "VENDORS/MICSFT/WINDOWS/CP932.TXT",  0, "ANSI/OEM Japanese Shift-JIS" ],
    [ 936,   "VENDORS/MICSFT/WINDOWS/CP936.TXT",  0, "ANSI/OEM Simplified Chinese GBK" ],
    [ 949,   "VENDORS/MICSFT/WINDOWS/CP949.TXT",  0, "ANSI/OEM Korean Unified Hangul" ],
    [ 950,   "VENDORS/MICSFT/WINDOWS/CP950.TXT",  0, "ANSI/OEM Traditional Chinese Big5" ],
    [ 1006,  "VENDORS/MISC/CP1006.TXT",           0, "IBM Arabic" ],
    [ 1026,  "VENDORS/MICSFT/EBCDIC/CP1026.TXT",  0, "IBM EBCDIC Latin 5 Turkish" ],
    [ 1250,  "VENDORS/MICSFT/WINDOWS/CP1250.TXT", 0, "ANSI Eastern Europe" ],
    [ 1251,  "VENDORS/MICSFT/WINDOWS/CP1251.TXT", 0, "ANSI Cyrillic" ],
    [ 1252,  "VENDORS/MICSFT/WINDOWS/CP1252.TXT", 0, "ANSI Latin 1" ],
    [ 1253,  "VENDORS/MICSFT/WINDOWS/CP1253.TXT", 0, "ANSI Greek" ],
    [ 1254,  "VENDORS/MICSFT/WINDOWS/CP1254.TXT", 0, "ANSI Turkish" ],
    [ 1255,  "VENDORS/MICSFT/WINDOWS/CP1255.TXT", 0, "ANSI Hebrew" ],
    [ 1256,  "VENDORS/MICSFT/WINDOWS/CP1256.TXT", 0, "ANSI Arabic" ],
    [ 1257,  "VENDORS/MICSFT/WINDOWS/CP1257.TXT", 0, "ANSI Baltic" ],
    [ 1258,  "VENDORS/MICSFT/WINDOWS/CP1258.TXT", 0, "ANSI/OEM Viet Nam" ],
    [ 1361,  "OBSOLETE/EASTASIA/KSC/JOHAB.TXT",   0, "Korean Johab" ],
    [ 10000, "VENDORS/MICSFT/MAC/ROMAN.TXT",      0, "Mac Roman" ],
    [ 10006, "VENDORS/MICSFT/MAC/GREEK.TXT",      0, "Mac Greek" ],
    [ 10007, "VENDORS/MICSFT/MAC/CYRILLIC.TXT",   0, "Mac Cyrillic" ],
    [ 10029, "VENDORS/MICSFT/MAC/LATIN2.TXT",     0, "Mac Latin 2" ],
    [ 10079, "VENDORS/MICSFT/MAC/ICELAND.TXT",    0, "Mac Icelandic" ],
    [ 10081, "VENDORS/MICSFT/MAC/TURKISH.TXT",    0, "Mac Turkish" ],
    [ 20127, undef,                               0, "US-ASCII (7bit)" ],
    [ 20866, "VENDORS/MISC/KOI8-R.TXT",           0, "Russian KOI8" ],
    [ 20932, "OBSOLETE/EASTASIA/JIS/JIS0208.TXT", 0, "EUC-JP" ],
    [ 21866, "VENDORS/MISC/KOI8-U.TXT",           0, "Ukrainian KOI8" ],
    [ 28591, "ISO8859/8859-1.TXT",                0, "ISO 8859-1 Latin 1" ],
    [ 28592, "ISO8859/8859-2.TXT",                0, "ISO 8859-2 Latin 2 (East European)" ],
    [ 28593, "ISO8859/8859-3.TXT",                0, "ISO 8859-3 Latin 3 (South European)" ],
    [ 28594, "ISO8859/8859-4.TXT",                0, "ISO 8859-4 Latin 4 (Baltic old)" ],
    [ 28595, "ISO8859/8859-5.TXT",                0, "ISO 8859-5 Cyrillic" ],
    [ 28596, "ISO8859/8859-6.TXT",                0, "ISO 8859-6 Arabic" ],
    [ 28597, "ISO8859/8859-7.TXT",                0, "ISO 8859-7 Greek" ],
    [ 28598, "ISO8859/8859-8.TXT",                0, "ISO 8859-8 Hebrew" ],
    [ 28599, "ISO8859/8859-9.TXT",                0, "ISO 8859-9 Latin 5 (Turkish)" ],
    [ 28600, "ISO8859/8859-10.TXT",               0, "ISO 8859-10 Latin 6 (Nordic)" ],
    [ 28603, "ISO8859/8859-13.TXT",               0, "ISO 8859-13 Latin 7 (Baltic)" ],
    [ 28604, "ISO8859/8859-14.TXT",               0, "ISO 8859-14 Latin 8 (Celtic)" ],
    [ 28605, "ISO8859/8859-15.TXT",               0, "ISO 8859-15 Latin 9 (Euro)" ],
    [ 28606, "ISO8859/8859-16.TXT",               0, "ISO 8859-16 Latin 10 (Balkan)" ]
);


%ctype =
(
    "upper"  => 0x0001,
    "lower"  => 0x0002,
    "digit"  => 0x0004,
    "space"  => 0x0008,
    "punct"  => 0x0010,
    "cntrl"  => 0x0020,
    "blank"  => 0x0040,
    "xdigit" => 0x0080,
    "alpha"  => 0x0100
);

%categories =
(
    "Lu" => $ctype{"alpha"}|$ctype{"upper"}, # Letter, Uppercase
    "Ll" => $ctype{"alpha"}|$ctype{"lower"}, # Letter, Lowercase
    "Lt" => $ctype{"alpha"},    # Letter, Titlecase
    "Mn" => $ctype{"punct"},    # Mark, Non-Spacing
    "Mc" => $ctype{"punct"},    # Mark, Spacing Combining
    "Me" => $ctype{"punct"},    # Mark, Enclosing
    "Nd" => $ctype{"digit"},    # Number, Decimal Digit
    "Nl" => $ctype{"punct"},    # Number, Letter
    "No" => $ctype{"punct"},    # Number, Other
    "Zs" => $ctype{"space"},    # Separator, Space
    "Zl" => $ctype{"space"},    # Separator, Line
    "Zp" => $ctype{"space"},    # Separator, Paragraph
    "Cc" => $ctype{"cntrl"},    # Other, Control
    "Cf" => 0,                  # Other, Format
    "Cs" => 0,                  # Other, Surrogate
    "Co" => 0,                  # Other, Private Use
    "Cn" => 0,                  # Other, Not Assigned
    "Lm" => $ctype{"punct"},    # Letter, Modifier
    "Lo" => $ctype{"alpha"},    # Letter, Other
    "Pc" => $ctype{"punct"},    # Punctuation, Connector
    "Pd" => $ctype{"punct"},    # Punctuation, Dash
    "Ps" => $ctype{"punct"},    # Punctuation, Open
    "Pe" => $ctype{"punct"},    # Punctuation, Close
    "Pi" => $ctype{"punct"},    # Punctuation, Initial quote
    "Pf" => $ctype{"punct"},    # Punctuation, Final quote
    "Po" => $ctype{"punct"},    # Punctuation, Other
    "Sm" => $ctype{"punct"},    # Symbol, Math
    "Sc" => $ctype{"punct"},    # Symbol, Currency
    "Sk" => $ctype{"punct"},    # Symbol, Modifier
    "So" => $ctype{"punct"}     # Symbol, Other
);

# a few characters need additional categories that cannot be determined automatically
%special_categories =
(
    "xdigit" => [ ord('0')..ord('9'),ord('A')..ord('F'),ord('a')..ord('f'),
                  0xff10..0xff19, 0xff21..0xff26, 0xff41..0xff46 ],
    "space"  => [ 0x09..0x0d, 0x85 ],
    "blank"  => [ 0x09, 0x20, 0xa0, 0x3000, 0xfeff ],
    "cntrl"  => [ 0x070f, 0x180b, 0x180c, 0x180d, 0x180e, 0x200c, 0x200d,
                  0x200e, 0x200f, 0x202a, 0x202b, 0x202c, 0x202d, 0x202e,
                  0x206a, 0x206b, 0x206c, 0x206d, 0x206e, 0x206f, 0xfeff,
                  0xfff9, 0xfffa, 0xfffb ]
);

%directions =
(
    "L"   => 1,    # Left-to-Right
    "LRE" => 11,   # Left-to-Right Embedding
    "LRO" => 11,   # Left-to-Right Override
    "R"   => 2,    # Right-to-Left
    "AL"  => 2,    # Right-to-Left Arabic
    "RLE" => 11,   # Right-to-Left Embedding
    "RLO" => 11,   # Right-to-Left Override
    "PDF" => 11,   # Pop Directional Format
    "EN"  => 3,    # European Number
    "ES"  => 4,    # European Number Separator
    "ET"  => 5,    # European Number Terminator
    "AN"  => 6,    # Arabic Number
    "CS"  => 7,    # Common Number Separator
    "NSM" => 0,    # Non-Spacing Mark
    "BN"  => 0,    # Boundary Neutral
    "B"   => 8,    # Paragraph Separator
    "S"   => 9,    # Segment Separator
    "WS"  => 10,   # Whitespace
    "ON"  => 11    # Other Neutrals
);


################################################################
# main routine

READ_DEFAULTS();
my @sortkeys = READ_SORTKEYS_FILE();
DUMP_CASE_MAPPINGS();
DUMP_SORTKEYS(@sortkeys);
DUMP_COMPOSE_TABLES();
DUMP_CTYPE_TABLES();

foreach $file (@allfiles) { HANDLE_FILE( @$file ); }

OUTPUT_CPTABLE();

exit(0);


################################################################
# read in the defaults file
sub READ_DEFAULTS
{
    @unicode_defaults = ();
    @unicode_aliases = ();
    @tolower_table = ();
    @toupper_table = ();
    @digitmap_table = ();
    @compatmap_table = ();
    @category_table = ();
    @direction_table = ();
    @decomp_table = ();
    @compose_table = ();

    # first setup a few default mappings

    open DEFAULTS or die "Cannot open $DEFAULTS";
    print "Loading $DEFAULTS\n";
    while (<DEFAULTS>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        if (/^(([0-9a-fA-F]+)(,[0-9a-fA-F]+)*)\s+([0-9a-fA-F]+|'.'|none)\s+(\#.*)?/)
        {
            my @src = map hex, split /,/,$1;
            my $dst = $4;
            my $comment = $5;
            if ($#src > 0) { push @unicode_aliases, \@src; }
            next if ($dst eq "none");
            $dst = ($dst =~ /\'.\'/) ? ord substr($dst,1,1) : hex $dst;
            foreach $src (@src)
            {
                die "Duplicate value" if defined($unicode_defaults[$src]);
                $unicode_defaults[$src] = $dst;
            }
            next;
        }
        die "Unrecognized line $_\n";
    }

    # now build mappings from the decomposition field of the Unicode database

    open UNICODEDATA or die "Cannot open $UNICODEDATA";
    print "Loading $UNICODEDATA\n";
    while (<UNICODEDATA>)
    {
	# Decode the fields ...
	($code, $name, $cat, $comb, $bidi,
	 $decomp, $dec, $dig, $num, $mirror,
	 $oldname, $comment, $upper, $lower, $title) = split /;/;

        my $src = hex $code;

        die "unknown category $cat" unless defined $categories{$cat};
        die "unknown directionality $bidi" unless defined $directions{$bidi};

        $uniname[$src] = $name;
        $category_table[$src] = $categories{$cat};
        $direction_table[$src] = $directions{$bidi};

        if ($lower ne "")
        {
            $tolower_table[$src] = hex $lower;
            $category_table[$src] |= $ctype{"upper"}|$ctype{"alpha"};
        }
        if ($upper ne "")
        {
            $toupper_table[$src] = hex $upper;
            $category_table[$src] |= $ctype{"lower"}|$ctype{"alpha"};
        }
        if ($dec ne "")
        {
            $category_table[$src] |= $ctype{"digit"};
        }
        if ($dig ne "")
        {
            $digitmap_table[$src] = ord $dig;
        }

        # copy the category and direction for everything between First/Last pairs
        if ($name =~ /, First>/) { $start = $src; }
        if ($name =~ /, Last>/)
        {
            while ($start < $src)
            {
                $category_table[$start] = $category_table[$src];
                $direction_table[$start] = $direction_table[$src];
                $start++;
            }
        }

        next if $decomp eq "";  # no decomposition, skip it

        if ($decomp =~ /^<([a-zA-Z]+)>\s+([0-9a-fA-F]+)$/)
        {
            # decomposition of the form "<foo> 1234" -> use char if type is known
            if (($src >= 0xf900 && $src < 0xfb00) || ($src >= 0xfe30 && $src < 0xfffd))
            {
                # Single char decomposition in the compatibility range
                $compatmap_table[$src] = hex $2;
            }
            next unless ($1 eq "font" ||
                         $1 eq "noBreak" ||
                         $1 eq "circle" ||
                         $1 eq "super" ||
                         $1 eq "sub" ||
                         $1 eq "wide" ||
                         $1 eq "narrow" ||
                         $1 eq "compat" ||
                         $1 eq "small");
            $dst = hex $2;
        }
        elsif ($decomp =~ /^<compat>\s+0020\s+([0-9a-fA-F]+)/)
        {
            # decomposition "<compat> 0020 1234" -> combining accent
            $dst = hex $1;
        }
        elsif ($decomp =~ /^([0-9a-fA-F]+)/)
        {
            # decomposition contains only char values without prefix -> use first char
            $dst = hex $1;
            $category_table[$src] |= $category_table[$dst];
            # store decomposition if it contains two chars
            if ($decomp =~ /^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)$/)
            {
                $decomp_table[$src] = [ hex $1, hex $2 ];
                push @compose_table, [ hex $1, hex $2, $src ];
            }
            elsif ($decomp =~ /^(<[a-z]+>\s)*([0-9a-fA-F]+)$/ &&
                   (($src >= 0xf900 && $src < 0xfb00) || ($src >= 0xfe30 && $src < 0xfffd)))
            {
                # Single char decomposition in the compatibility range
                $compatmap_table[$src] = hex $2;
            }
        }
        else
        {
            next;
        }

        next if defined($unicode_defaults[$src]);  # may have been set in the defaults file

        # check for loops
        for ($i = $dst; ; $i = $unicode_defaults[$i])
        {
            die sprintf("loop detected for %04x -> %04x",$src,$dst) if $i == $src;
            last unless defined($unicode_defaults[$i]);
        }
        $unicode_defaults[$src] = $dst;
    }

    # patch the category of some special characters

    foreach $cat (keys %special_categories)
    {
        my $flag = $ctype{$cat};
        foreach $i (@{$special_categories{$cat}}) { $category_table[$i] |= $flag; }
    }
}


################################################################
# parse the input file
sub READ_FILE
{
    my $name = shift;
    open INPUT,$name or die "Cannot open $name";

    while (<INPUT>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        next if /\x1a/;  # skip ^Z
        next if (/^0x([0-9a-fA-F]+)\s+\#UNDEFINED/);  # undefined char

        if (/^0x([0-9a-fA-F]+)\s+\#DBCS LEAD BYTE/)
        {
            $cp = hex $1;
            push @lead_bytes,$cp;
            $cp2uni[$cp] = 0;
            next;
        }
        if (/^0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\#.*)?/)
        {
            $cp = hex $1;
            $uni = hex $2;
            $cp2uni[$cp] = $uni unless defined($cp2uni[$cp]);
            $uni2cp[$uni] = $cp unless defined($uni2cp[$uni]);
            if ($cp > 0xff && !defined($cp2uni[$cp >> 8]))
            {
                push @lead_bytes,$cp >> 8;
                $cp2uni[$cp >> 8] = 0;
            }
            next;
        }
        die "$name: Unrecognized line $_\n";
    }
}


################################################################
# fill input data for the 20127 (us-ascii) codepage
sub fill_20127_codepage()
{
    for (my $i = 0; $i < 128; $i++) { $cp2uni[$i] = $uni2cp[$i] = $i; }
    for (my $i = 128; $i < 256; $i++) { $cp2uni[$i] = $i & 0x7f; }
}

################################################################
# get a mapping including glyph chars for MB_USEGLYPHCHARS

sub get_glyphs_mapping(@)
{
    $_[0x01] = 0x263a;  # (WHITE SMILING FACE)
    $_[0x02] = 0x263b;  # (BLACK SMILING FACE)
    $_[0x03] = 0x2665;  # (BLACK HEART SUIT)
    $_[0x04] = 0x2666;  # (BLACK DIAMOND SUIT)
    $_[0x05] = 0x2663;  # (BLACK CLUB SUIT)
    $_[0x06] = 0x2660;  # (BLACK SPADE SUIT)
    $_[0x07] = 0x2022;  # (BULLET)
    $_[0x08] = 0x25d8;  # (INVERSE BULLET)
    $_[0x09] = 0x25cb;  # (WHITE CIRCLE)
    $_[0x0a] = 0x25d9;  # (INVERSE WHITE CIRCLE)
    $_[0x0b] = 0x2642;  # (MALE SIGN)
    $_[0x0c] = 0x2640;  # (FEMALE SIGN)
    $_[0x0d] = 0x266a;  # (EIGHTH NOTE)
    $_[0x0e] = 0x266b;  # (BEAMED EIGHTH NOTES)
    $_[0x0f] = 0x263c;  # (WHITE SUN WITH RAYS)
    $_[0x10] = 0x25ba;  # (BLACK RIGHT-POINTING POINTER)
    $_[0x11] = 0x25c4;  # (BLACK LEFT-POINTING POINTER)
    $_[0x12] = 0x2195;  # (UP DOWN ARROW)
    $_[0x13] = 0x203c;  # (DOUBLE EXCLAMATION MARK)
    $_[0x14] = 0x00b6;  # (PILCROW SIGN)
    $_[0x15] = 0x00a7;  # (SECTION SIGN)
    $_[0x16] = 0x25ac;  # (BLACK RECTANGLE)
    $_[0x17] = 0x21a8;  # (UP DOWN ARROW WITH BASE)
    $_[0x18] = 0x2191;  # (UPWARDS ARROW)
    $_[0x19] = 0x2193;  # (DOWNWARDS ARROW)
    $_[0x1a] = 0x2192;  # (RIGHTWARDS ARROW)
    $_[0x1b] = 0x2190;  # (LEFTWARDS ARROW)
    $_[0x1c] = 0x221f;  # (RIGHT ANGLE)
    $_[0x1d] = 0x2194;  # (LEFT RIGHT ARROW)
    $_[0x1e] = 0x25b2;  # (BLACK UP-POINTING TRIANGLE)
    $_[0x1f] = 0x25bc;  # (BLACK DOWN-POINTING TRIANGLE)
    $_[0x7f] = 0x2302;  # (HOUSE)
    return @_;
}

################################################################
# build EUC-JP table from the JIS 0208 file
# FIXME: for proper EUC-JP we should probably read JIS 0212 too
# but this would require 3-byte DBCS characters
sub READ_JIS0208_FILE
{
    my $name = shift;

    # ASCII chars
    for ($i = 0x00; $i <= 0x7f; $i++)
    {
        $cp2uni[$i] = $i;
        $uni2cp[$i] = $i;
    }

    # JIS X 0201 right plane
    for ($i = 0xa1; $i <= 0xdf; $i++)
    {
        $cp2uni[0x8e00 + $i] = 0xfec0 + $i;
        $uni2cp[0xfec0 + $i] = 0x8e00 + $i;
    }

    # lead bytes
    foreach $i (0x8e, 0x8f, 0xa1 .. 0xfe)
    {
        push @lead_bytes,$i;
        $cp2uni[$i] = 0;
    }

    # undefined chars
    foreach $i (0x80 .. 0x8d, 0x90 .. 0xa0, 0xff)
    {
        $cp2uni[$i] = $DEF_CHAR;
    }

    # Shift-JIS compatibility
    $uni2cp[0x00a5] = 0x5c;
    $uni2cp[0x203e] = 0x7e;

    # Fix backslash conversion
    $cp2uni[0xa1c0] = 0xff3c;
    $uni2cp[0xff3c] = 0xa1c0;

    open INPUT, "$name" or die "Cannot open $name";
    while (<INPUT>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        next if /\x1a/;  # skip ^Z
        if (/^0x[0-9a-fA-F]+\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(\#.*)?/)
        {
            $cp = 0x8080 + hex $1;
            $uni = hex $2;
            $cp2uni[$cp] = $uni unless defined($cp2uni[$cp]);
            $uni2cp[$uni] = $cp unless defined($uni2cp[$uni]);
            next;
        }
        die "$name: Unrecognized line $_\n";
    }
}


################################################################
# build the sort keys table
sub READ_SORTKEYS_FILE
{
    my @sortkeys = ();
    for (my $i = 0; $i < 65536; $i++) { $sortkeys[$i] = [ -1, 0, 0, 0, 0 ] };

    open INPUT, "$SORTKEYS" or die "Cannot open $SORTKEYS";
    print "Loading $SORTKEYS\n";
    while (<INPUT>)
    {
        next if /^\#/;  # skip comments
        next if /^$/;  # skip empty lines
        next if /\x1a/;  # skip ^Z
        next if /^\@version/;  # skip @version header
        if (/^([0-9a-fA-F]+)\s+;\s+\[([*.])([0-9a-fA-F]{4})\.([0-9a-fA-F]{4})\.([0-9a-fA-F]{4})\.([0-9a-fA-F]+)\]/)
        {
            my ($uni,$variable) = (hex $1, $2);
            next if $uni > 65535;
            $sortkeys[$uni] = [ $uni, hex $3, hex $4, hex $5, hex $6 ];
            next;
        }
        if (/^([0-9a-fA-F]+\s+)+;\s+\[[*.]([0-9a-fA-F]{4})\.([0-9a-fA-F]{4})\.([0-9a-fA-F]{4})\.([0-9a-fA-F]+)\]/)
        {
            # multiple character sequence, ignored for now
            next;
        }
        die "$SORTKEYS: Unrecognized line $_\n";
    }
    close INPUT;

    # compress the keys to 32 bit:
    # key 1 to 16 bits, key 2 to 8 bits, key 3 to 4 bits, key 4 to 1 bit

    @sortkeys = sort { ${$a}[1] <=> ${$b}[1] or 
                       ${$a}[2] <=> ${$b}[2] or
                       ${$a}[3] <=> ${$b}[3] or
                       ${$a}[4] <=> ${$b}[4] or
                       $a cmp $b; } @sortkeys;

    my ($n2, $n3) = (1, 1);
    my @keys = (-1, -1, -1, -1, -1 );
    my @flatkeys = ();

    for (my $i = 0; $i < 65536; $i++)
    {
        my @current = @{$sortkeys[$i]};
        next if $current[0] == -1;
        if ($current[1] == $keys[1])
        {
            if ($current[2] == $keys[2])
            {
                if ($current[3] == $keys[3])
                {
                    # nothing
                }
                else
                {
                    $keys[3] = $current[3];
                    $n3++;
                    die if ($n3 >= 16);
                }
            }
            else
            {
                $keys[2] = $current[2];
                $keys[3] = $current[3];
                $n2++;
                $n3 = 1;
                die if ($n2 >= 256);
            }
        }
        else
        {
            $keys[1] = $current[1];
            $keys[2] = $current[2];
            $keys[3] = $current[3];
            $n2 = 1;
            $n3 = 1;
        }

        if ($current[2]) { $current[2] = $n2; }
        if ($current[3]) { $current[3] = $n3; }
        if ($current[4]) { $current[4] = 1; }

        $flatkeys[$current[0]] = ($current[1] << 16) | ($current[2] << 8) | ($current[3] << 4) | $current[4];
    }
    return @flatkeys;
}


################################################################
# build the sort keys table
sub DUMP_SORTKEYS
{
    my @keys = @_;

    # count the number of 256-key ranges that contain something

    my @offsets = ();
    my $ranges = 2;
    for (my $i = 0; $i < 256; $i++) { $offsets[$i] = 256; }
    for (my $i = 0; $i < 65536; $i++)
    {
        next unless defined $keys[$i];
        $offsets[$i >> 8] = $ranges * 256;
        $ranges++;
        $i |= 255;
    }

    # output the range offsets

    open OUTPUT,">collation.c.new" or die "Cannot create collation.c";
    printf "Building collation.c\n";
    printf OUTPUT "/* Unicode collation element table */\n";
    printf OUTPUT "/* generated from %s */\n", $SORTKEYS;
    printf OUTPUT "/* DO NOT EDIT!! */\n\n";

    printf OUTPUT "const unsigned int collation_table[%d] =\n{\n", $ranges*256;
    printf OUTPUT "    /* index */\n";
    printf OUTPUT "%s,\n", DUMP_ARRAY( "0x%08x", 0, @offsets );

    # output the default values

    printf OUTPUT "    /* defaults */\n";
    printf OUTPUT "%s", DUMP_ARRAY( "0x%08x", 0, (0xffffffff) x 256 );

    # output all the key ranges

    for (my $i = 0; $i < 256; $i++)
    {
        next if $offsets[$i] == 256;
        printf OUTPUT ",\n    /* 0x%02x00 .. 0x%02xff */\n", $i, $i;
        printf OUTPUT "%s", DUMP_ARRAY( "0x%08x", 0xffffffff, @keys[($i<<8) .. ($i<<8)+255] );
    }
    printf OUTPUT "\n};\n";
    close OUTPUT;
    save_file("collation.c");
}


################################################################
# add default mappings once the file had been read
sub ADD_DEFAULT_MAPPINGS
{
    # Apply aliases

    foreach $alias (@unicode_aliases)
    {
        my $target = undef;
        foreach $src (@$alias)
        {
            if (defined($uni2cp[$src]))
            {
                $target = $uni2cp[$src];
                last;
            }
        }
        next unless defined($target);

        # At least one char of the alias set is defined, set the others to the same value
        foreach $src (@$alias)
        {
            $uni2cp[$src] = $target unless defined($uni2cp[$src]);
        }
    }

    # For every src -> target mapping in the defaults table,
    # make uni2cp[src] = uni2cp[target] if uni2cp[target] is defined

    for ($src = 0; $src < 65536; $src++)
    {
        next if defined($uni2cp[$src]);  # source has a definition already
        next unless defined($unicode_defaults[$src]);  # no default for this char
        my $target = $unicode_defaults[$src];

        # do a recursive mapping until we find a target char that is defined
        while (!defined($uni2cp[$target]) &&
               defined($unicode_defaults[$target])) { $target = $unicode_defaults[$target]; }

        if (defined($uni2cp[$target])) { $uni2cp[$src] = $uni2cp[$target]; }
    }

    # Add an identity mapping for all undefined chars

    for ($i = 0; $i < 256; $i++)
    {
        next if defined($cp2uni[$i]);
        next if defined($uni2cp[$i]);
        $cp2uni[$i] = $uni2cp[$i] = $i;
    }
}

################################################################
# dump an array of integers
sub DUMP_ARRAY
{
    my ($format,$default,@array) = @_;
    my $i, $ret = "    ";
    for ($i = 0; $i < $#array; $i++)
    {
        $ret .= sprintf($format, defined $array[$i] ? $array[$i] : $default);
        $ret .= (($i % 8) != 7) ? ", " : ",\n    ";
    }
    $ret .= sprintf($format, defined $array[$i] ? $array[$i] : $default);
    return $ret;
}

################################################################
# dump an SBCS mapping table
sub DUMP_SBCS_TABLE
{
    my ($codepage, $has_glyphs, $name) = @_;
    my $i;

    # output the ascii->unicode table

    if ($has_glyphs)
    {
        printf OUTPUT "static const WCHAR cp2uni[512] =\n";
        printf OUTPUT "{\n%s", DUMP_ARRAY( "0x%04x", $DEF_CHAR, @cp2uni[0 .. 255] );
        printf OUTPUT ",\n    /* glyphs */\n%s\n};\n\n",
                      DUMP_ARRAY( "0x%04x", $DEF_CHAR, get_glyphs_mapping(@cp2uni[0 .. 255]) );
    }
    else
    {
        printf OUTPUT "static const WCHAR cp2uni[256] =\n";
        printf OUTPUT "{\n%s\n};\n\n", DUMP_ARRAY( "0x%04x", $DEF_CHAR, @cp2uni[0 .. 255] );
    }

    # count the number of unicode->ascii subtables that contain something

    my @filled = ();
    my $subtables = 1;
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $uni2cp[$i];
        $filled[$i >> 8] = 1;
        $subtables++;
        $i |= 255;
    }

    # output all the subtables into a single array

    printf OUTPUT "static const unsigned char uni2cp_low[%d] =\n{\n", $subtables*256;
    for ($i = 0; $i < 256; $i++)
    {
        next unless $filled[$i];
        printf OUTPUT "    /* 0x%02x00 .. 0x%02xff */\n", $i, $i;
        printf OUTPUT "%s,\n", DUMP_ARRAY( "0x%02x", $DEF_CHAR, @uni2cp[($i<<8) .. ($i<<8)+255] );
    }
    printf OUTPUT "    /* defaults */\n";
    printf OUTPUT "%s\n};\n\n", DUMP_ARRAY( "0x%02x", 0, ($DEF_CHAR) x 256 );

    # output a table of the offsets of the subtables in the previous array

    my $pos = 0;
    my @offsets = ();
    for ($i = 0; $i < 256; $i++)
    {
        if ($filled[$i]) { push @offsets, $pos; $pos += 256; }
        else { push @offsets, ($subtables-1) * 256; }
    }
    printf OUTPUT "static const unsigned short uni2cp_high[256] =\n";
    printf OUTPUT "{\n%s\n};\n\n", DUMP_ARRAY( "0x%04x", 0, @offsets );

    # output the code page descriptor

    printf OUTPUT "const struct sbcs_table cptable_%03d =\n{\n", $codepage;
    printf OUTPUT "    { %d, 1, 0x%04x, 0x%04x, \"%s\" },\n",
                  $codepage, $DEF_CHAR, $DEF_CHAR, $name;
    printf OUTPUT "    cp2uni,\n";
    if ($has_glyphs) { printf OUTPUT "    cp2uni + 256,\n"; }
    else { printf OUTPUT "    cp2uni,\n"; }
    printf OUTPUT "    uni2cp_low,\n";
    printf OUTPUT "    uni2cp_high\n};\n";
}


################################################################
# dump a DBCS mapping table
sub DUMP_DBCS_TABLE
{
    my ($codepage, $name) = @_;
    my $i, $x, $y;

    # build a list of lead bytes that are actually used

    my @lblist = ();
    LBLOOP: for ($y = 0; $y <= $#lead_bytes; $y++)
    {
        my $base = $lead_bytes[$y] << 8;
        for ($x = 0; $x < 256; $x++)
        {
            if (defined $cp2uni[$base+$x])
            {
                push @lblist,$lead_bytes[$y];
                next LBLOOP;
            }
        }
    }
    my $unused = ($#lead_bytes > $#lblist);

    # output the ascii->unicode table for the single byte chars

    printf OUTPUT "static const WCHAR cp2uni[%d] =\n", 256 * ($#lblist + 2 + $unused);
    printf OUTPUT "{\n%s,\n", DUMP_ARRAY( "0x%04x", $DEF_CHAR, @cp2uni[0 .. 255] );

    # output the default table for unused lead bytes

    if ($unused)
    {
        printf OUTPUT "    /* unused lead bytes */\n";
        printf OUTPUT "%s,\n", DUMP_ARRAY( "0x%04x", 0, ($DEF_CHAR) x 256 );
    }

    # output the ascii->unicode table for each DBCS lead byte

    for ($y = 0; $y <= $#lblist; $y++)
    {
        my $base = $lblist[$y] << 8;
        printf OUTPUT "    /* lead byte %02x */\n", $lblist[$y];
        printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", $DEF_CHAR, @cp2uni[$base .. $base+255] );
        printf OUTPUT ($y < $#lblist) ? ",\n" : "\n};\n\n";
    }

    # output the lead byte subtables offsets

    my @offsets = ();
    for ($x = 0; $x < 256; $x++) { $offsets[$x] = 0; }
    for ($x = 0; $x <= $#lblist; $x++) { $offsets[$lblist[$x]] = $x + 1; }
    if ($unused)
    {
        # increment all lead bytes offset to take into account the unused table
        for ($x = 0; $x <= $#lead_bytes; $x++) { $offsets[$lead_bytes[$x]]++; }
    }
    printf OUTPUT "static const unsigned char cp2uni_leadbytes[256] =\n";
    printf OUTPUT "{\n%s\n};\n\n", DUMP_ARRAY( "0x%02x", 0, @offsets );

    # count the number of unicode->ascii subtables that contain something

    my @filled = ();
    my $subtables = 1;
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $uni2cp[$i];
        $filled[$i >> 8] = 1;
        $subtables++;
        $i |= 255;
    }

    # output all the subtables into a single array

    printf OUTPUT "static const unsigned short uni2cp_low[%d] =\n{\n", $subtables*256;
    for ($y = 0; $y < 256; $y++)
    {
        next unless $filled[$y];
        printf OUTPUT "    /* 0x%02x00 .. 0x%02xff */\n", $y, $y;
        printf OUTPUT "%s,\n", DUMP_ARRAY( "0x%04x", $DEF_CHAR, @uni2cp[($y<<8) .. ($y<<8)+255] );
    }
    printf OUTPUT "    /* defaults */\n";
    printf OUTPUT "%s\n};\n\n", DUMP_ARRAY( "0x%04x", 0, ($DEF_CHAR) x 256 );

    # output a table of the offsets of the subtables in the previous array

    my $pos = 0;
    my @offsets = ();
    for ($y = 0; $y < 256; $y++)
    {
        if ($filled[$y]) { push @offsets, $pos; $pos += 256; }
        else { push @offsets, ($subtables-1) * 256; }
    }
    printf OUTPUT "static const unsigned short uni2cp_high[256] =\n";
    printf OUTPUT "{\n%s\n};\n\n", DUMP_ARRAY( "0x%04x", 0, @offsets );

    # output the code page descriptor

    printf OUTPUT "const struct dbcs_table cptable_%03d =\n{\n", $codepage;
    printf OUTPUT "    { %d, 2, 0x%04x, 0x%04x, \"%s\" },\n",
                  $codepage, $DEF_CHAR, $DEF_CHAR, $name;
    printf OUTPUT "    cp2uni,\n";
    printf OUTPUT "    cp2uni_leadbytes,\n";
    printf OUTPUT "    uni2cp_low,\n";
    printf OUTPUT "    uni2cp_high,\n";
    DUMP_LB_RANGES();
    printf OUTPUT "};\n";
}


################################################################
# dump the list of defined lead byte ranges
sub DUMP_LB_RANGES
{
    my @list = ();
    my $i = 0;
    foreach $i (@lead_bytes) { $list[$i] = 1; }
    my $on = 0;
    printf OUTPUT "    { ";
    for ($i = 0; $i < 256; $i++)
    {
        if ($on)
        {
            if (!defined $list[$i]) { printf OUTPUT "0x%02x, ", $i-1; $on = 0; }
        }
        else
        {
            if ($list[$i]) { printf OUTPUT "0x%02x, ", $i; $on = 1; }
        }
    }
    if ($on) { printf OUTPUT "0xff, "; }
    printf OUTPUT "0x00, 0x00 }\n";
}


################################################################
# dump the case mapping tables
sub DUMP_CASE_MAPPINGS
{
    open OUTPUT,">casemap.c.new" or die "Cannot create casemap.c";
    printf "Building casemap.c\n";
    printf OUTPUT "/* Unicode case mappings */\n";
    printf OUTPUT "/* Automatically generated; DO NOT EDIT!! */\n\n";
    printf OUTPUT "#include \"wine/unicode.h\"\n\n";

    DUMP_CASE_TABLE( "wine_casemap_lower", @tolower_table );
    DUMP_CASE_TABLE( "wine_casemap_upper", @toupper_table );
    DUMP_CASE_TABLE( "wine_digitmap",  @digitmap_table );
    DUMP_CASE_TABLE( "wine_compatmap", @compatmap_table );
    close OUTPUT;
    save_file("casemap.c");
}


################################################################
# dump a case mapping table
sub DUMP_CASE_TABLE
{
    my ($name,@table) = @_;

    # count the number of sub tables that contain something
    # also compute the low and upper populated bounds

    my @lowerbounds = ( 0, 0 );
    my @upperbounds = ( 0, 255 );
    my $index = 0;
    my @filled = ();
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $table[$i];
        if (!defined $filled[$i >> 8])
        {
          $lowerbounds[$index] = $i & 0xff;
          $upperbounds[$index] = 0xff - $lowerbounds[$index];
          $filled[$i >> 8] = $index * 256 + 512;
          $index++;
        }
        else
        {
          $upperbounds[$index-1] = 0xff - ($i & 0xff);
        }
        $table[$i] = ($table[$i] - $i) & 0xffff;
    }

    # Collapse blocks upwards if possible
    my $removed = 0;
    $index = 0;
    for ($i = 0; $i < 256; $i++)
    {
        next unless defined $filled[$i];
        if ($upperbounds[$index - 1] > $lowerbounds[$index])
        {
           $removed = $removed + $lowerbounds[$index];
        }
        else
        {
           $removed = $removed + $upperbounds[$index - 1];
           $lowerbounds[$index] = $upperbounds[$index - 1];
        }
        $filled[$i] = $filled[$i] - $removed;
        $index++;
    }

    # dump the table

    printf OUTPUT "const WCHAR %s[%d] =\n", $name, $index * 256 + 512 - $removed;
    printf OUTPUT "{\n    /* index */\n";
    printf OUTPUT "%s,\n", DUMP_ARRAY( "0x%04x", 256, @filled );
    printf OUTPUT "    /* defaults */\n";
    printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", 0, (0) x 256 );
    $index = 0;
    for ($i = 0; $i < 256; $i++)
    {
        next unless $filled[$i];
        printf OUTPUT ",\n    /* 0x%02x%02x .. 0x%02xff */\n", $i, $lowerbounds[$index], $i;
        printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", 0,
                      @table[($i<<8) + $lowerbounds[$index] .. ($i<<8)+255] );
        $index++;
    }
    printf OUTPUT "\n};\n";
}


################################################################
# dump the ctype tables
sub DUMP_CTYPE_TABLES
{
    open OUTPUT,">wctype.c.new" or die "Cannot create wctype.c";
    printf "Building wctype.c\n";
    printf OUTPUT "/* Unicode ctype tables */\n";
    printf OUTPUT "/* Automatically generated; DO NOT EDIT!! */\n\n";
    printf OUTPUT "#include \"wine/unicode.h\"\n\n";

    my $i;
    my @array = (0) x 256;

    # add the direction in the high 4 bits of the category
    for ($i = 0; $i < 65536; $i++)
    {
        $category_table[$i] |= $direction_table[$i] << 12;
    }

    # try to merge table rows
    for ($row = 0; $row < 256; $row++)
    {
        my $rowtxt = sprintf "%04x" x 256, @category_table[($row<<8)..($row<<8)+255];
        if (defined($sequences{$rowtxt}))
        {
            # reuse an existing row
            $array[$row] = $sequences{$rowtxt};
        }
        else
        {
            # create a new row
            $sequences{$rowtxt} = $array[$row] = $#array + 1;
            push @array, @category_table[($row<<8)..($row<<8)+255];
        }
    }

    printf OUTPUT "const unsigned short wine_wctype_table[%d] =\n{\n", $#array+1;
    printf OUTPUT "    /* offsets */\n%s,\n", DUMP_ARRAY( "0x%04x", 0, @array[0..255] );
    printf OUTPUT "    /* values */\n%s\n};\n", DUMP_ARRAY( "0x%04x", 0, @array[256..$#array] );

    close OUTPUT;
    save_file("wctype.c");
}


################################################################
# dump the char composition tables
sub DUMP_COMPOSE_TABLES
{
    open OUTPUT,">compose.c.new" or die "Cannot create compose.c";
    printf "Building compose.c\n";
    printf OUTPUT "/* Unicode char composition */\n";
    printf OUTPUT "/* Automatically generated; DO NOT EDIT!! */\n\n";
    printf OUTPUT "#include \"wine/unicode.h\"\n\n";

    ######### composition table

    my @filled = ();
    foreach $i (@compose_table)
    {
        my @comp = @$i;
        push @{$filled[$comp[1]]}, [ $comp[0], $comp[2] ];
    }

    # count how many different second chars we have

    for ($i = $count = 0; $i < 65536; $i++)
    {
        next unless defined $filled[$i];
        $count++;
    }

    # build the table of second chars and offsets

    my $pos = $count + 1;
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $filled[$i];
        push @table, $i, $pos;
        $pos += @{$filled[$i]};
    }
    # terminator with last position
    push @table, 0, $pos;
    printf OUTPUT "const WCHAR unicode_compose_table[0x%x] =\n{\n", 2*$pos;
    printf OUTPUT "    /* second chars + offsets */\n%s", DUMP_ARRAY( "0x%04x", 0, @table );

    # build the table of first chars and mappings

    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $filled[$i];
        my @table = ();
        my @list = sort { $a->[0] <=> $b->[0] } @{$filled[$i]};
        for ($j = 0; $j <= $#list; $j++)
        {
            push @table, $list[$j][0], $list[$j][1];
        }
        printf OUTPUT ",\n    /* 0x%04x */\n%s", $i, DUMP_ARRAY( "0x%04x", 0, @table );
    }
    printf OUTPUT "\n};\n\nconst unsigned int unicode_compose_table_size = %d;\n\n", $count;

    ######### decomposition table

    # first determine all the 16-char subsets that contain something

    my @filled = (0) x 4096;
    my $pos = 16*2;  # for the null subset
    for ($i = 0; $i < 65536; $i++)
    {
        next unless defined $decomp_table[$i];
        $filled[$i >> 4] = $pos;
        $pos += 16*2;
        $i |= 15;
    }
    my $total = $pos;

    # now count the 256-char subsets that contain something

    my @filled_idx = (256) x 256;
    $pos = 256 + 16;
    for ($i = 0; $i < 4096; $i++)
    {
        next unless $filled[$i];
        $filled_idx[$i >> 4] = $pos;
        $pos += 16;
        $i |= 15;
    }
    my $null_offset = $pos;  # null mapping
    $total += $pos;

    # add the index offsets to the subsets positions

    for ($i = 0; $i < 4096; $i++)
    {
        next unless $filled[$i];
        $filled[$i] += $null_offset;
    }

    # dump the main index

    printf OUTPUT "const WCHAR unicode_decompose_table[%d] =\n", $total;
    printf OUTPUT "{\n    /* index */\n";
    printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", 0, @filled_idx );
    printf OUTPUT ",\n    /* null sub-index */\n%s", DUMP_ARRAY( "0x%04x", 0, ($null_offset) x 16 );

    # dump the second-level indexes

    for ($i = 0; $i < 256; $i++)
    {
        next unless ($filled_idx[$i] > 256);
        my @table = @filled[($i<<4)..($i<<4)+15];
        for ($j = 0; $j < 16; $j++) { $table[$j] ||= $null_offset; }
        printf OUTPUT ",\n    /* sub-index %02x */\n", $i;
        printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", 0, @table );
    }

    # dump the 16-char subsets

    printf OUTPUT ",\n    /* null mapping */\n";
    printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", 0, (0) x 32 );

    for ($i = 0; $i < 4096; $i++)
    {
        next unless $filled[$i];
        my @table = (0) x 32;
        for ($j = 0; $j < 16; $j++)
        {
            if (defined $decomp_table[($i<<4) + $j])
            {
                $table[2 * $j] = ${$decomp_table[($i << 4) + $j]}[0];
                $table[2 * $j + 1] = ${$decomp_table[($i << 4) + $j]}[1];
            }
        }
        printf OUTPUT ",\n    /* 0x%03x0 .. 0x%03xf */\n", $i, $i;
        printf OUTPUT "%s", DUMP_ARRAY( "0x%04x", 0, @table );
    }

    printf OUTPUT "\n};\n";
    close OUTPUT;
    save_file("compose.c");
}


################################################################
# read an input file and generate the corresponding .c file
sub HANDLE_FILE
{
    my ($codepage,$filename,$has_glyphs,$comment) = @_;

    @cp2uni = ();
    @lead_bytes = ();
    @uni2cp = ();

    # symbol codepage file is special
    if ($codepage == 20932) { READ_JIS0208_FILE($MAPPREFIX . $filename); }
    elsif ($codepage == 20127) { fill_20127_codepage(); }
    else { READ_FILE($MAPPREFIX . $filename); }

    # hack: 0x00a5 must map to backslash in Shift-JIS
    if ($codepage == 932) { $uni2cp[0x00a5] = 0x5c; }

    ADD_DEFAULT_MAPPINGS();

    my $output = sprintf "c_%03d.c", $codepage;
    open OUTPUT,">$output.new" or die "Cannot create $output";

    printf "Building %s from %s (%s)\n", $output, $filename || "hardcoded data", $comment;

    # dump all tables

    printf OUTPUT "/* code page %03d (%s) */\n", $codepage, $comment;
    if ($filename)
    {
        printf OUTPUT "/* generated from %s */\n", $MAPPREFIX . $filename;
        printf OUTPUT "/* DO NOT EDIT!! */\n\n";
    }
    else
    {
        printf OUTPUT "/* Automatically generated; DO NOT EDIT!! */\n\n";
    }
    printf OUTPUT "#include \"wine/unicode.h\"\n\n";

    if ($#lead_bytes == -1) { DUMP_SBCS_TABLE( $codepage, $has_glyphs, $comment ); }
    else { DUMP_DBCS_TABLE( $codepage, $comment ); }
    close OUTPUT;
    save_file($output);
}


################################################################
# save a file if modified
sub save_file($)
{
    my $file = shift;
    if (-f $file && !system "cmp $file $file.new >/dev/null")
    {
        unlink "$file.new";
    }
    else
    {
        rename "$file.new", "$file";
    }
}


################################################################
# output the list of codepage tables into the cptable.c file
sub OUTPUT_CPTABLE
{
    @tables_decl = ();

    foreach $file (@allfiles)
    {
        my ($codepage,$filename,$comment) = @$file;
        push @tables_decl, sprintf("extern union cptable cptable_%03d;\n",$codepage);
    }

    push @tables_decl, sprintf("\nstatic const union cptable * const cptables[%d] =\n{\n",$#allfiles+1);
    foreach $file (@allfiles)
    {
        my ($codepage,$filename,$comment) = @$file;
        push @tables_decl, sprintf("    &cptable_%03d,\n", $codepage);
    }
    push @tables_decl, "};";
    REPLACE_IN_FILE( "cptable.c", @tables_decl );
}

################################################################
# replace the contents of a file between ### cpmap ### marks

sub REPLACE_IN_FILE
{
    my $name = shift;
    my @data = @_;
    my @lines = ();
    open(FILE,$name) or die "Can't open $name";
    while (<FILE>)
    {
	push @lines, $_;
	last if /\#\#\# cpmap begin \#\#\#/;
    }
    push @lines, @data;
    while (<FILE>)
    {
	if (/\#\#\# cpmap end \#\#\#/) { push @lines, "\n", $_; last; }
    }
    push @lines, <FILE>;
    open(FILE,">$name.new") or die "Can't modify $name";
    print FILE @lines;
    close(FILE);
    save_file($name);
}
