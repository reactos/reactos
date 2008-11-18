<?php
# Copyright (C) 2004,2008 Brion Vibber <brion@pobox.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * This script generates Utf8Case.inc from the Unicode Character Database
 * and supplementary files.
 *
 * @ingroup UtfNormal
 * @access private
 */

/** */

if( php_sapi_name() != 'cli' ) {
	die( "Run me from the command line please.\n" );
}

require_once 'UtfNormalUtil.php';

$in = fopen("UnicodeData.txt", "rt" );
if( !$in ) {
	print "Can't open UnicodeData.txt for reading.\n";
	print "If necessary, fetch this file from the internet:\n";
	print "http://www.unicode.org/Public/UNIDATA/UnicodeData.txt\n";
	exit(-1);
}
$wikiUpperChars = array();
$wikiLowerChars = array();

print "Reading character definitions...\n";
while( false !== ($line = fgets( $in ) ) ) {
	$columns = split(';', $line);
	$codepoint = $columns[0];
	$name = $columns[1];
	$simpleUpper = $columns[12];
	$simpleLower = $columns[13];
	
	$source = codepointToUtf8( hexdec( $codepoint ) );
	if( $simpleUpper ) {
		$wikiUpperChars[$source] = codepointToUtf8( hexdec( $simpleUpper ) );
	}
	if( $simpleLower ) {
		$wikiLowerChars[$source] = codepointToUtf8( hexdec( $simpleLower ) );
	}
}
fclose( $in );

$out = fopen("Utf8Case.php", "wt");
if( $out ) {
	$outUpperChars = escapeArray( $wikiUpperChars );
	$outLowerChars = escapeArray( $wikiLowerChars );
	$outdata = "<" . "?php
/**
 * Simple 1:1 upper/lowercase switching arrays for utf-8 text
 * Won't get context-sensitive things yet
 *
 * Hack for bugs in ucfirst() and company
 *
 * These are pulled from memcached if possible, as this is faster than filling
 * up a big array manually.
 * @ingroup Language
 */

/*
 * Translation array to get upper case character
 */

\$wikiUpperChars = $outUpperChars;

/*
 * Translation array to get lower case character
 */
\$wikiLowerChars = $outLowerChars;\n";
	fputs( $out, $outdata );
	fclose( $out );
	print "Wrote out Utf8Case.php\n";
} else {
	print "Can't create file Utf8Case.php\n";
	exit(-1);
}


function escapeArray( $arr ) {
	return "array(\n" .
		implode( ",\n",
			array_map( "escapeLine",
				array_keys( $arr ),
				array_values( $arr ) ) ) .
		"\n)";
}

function escapeLine( $key, $val ) {
	$encKey = escapeSingleString( $key );
	$encVal = escapeSingleString( $val );
	return "\t'$encKey' => '$encVal'";
}
