<?php
# Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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
 * Implements the conformance test at:
 * http://www.unicode.org/Public/UNIDATA/NormalizationTest.txt
 * @ingroup UtfNormal
 */

/** */
$verbose = true;
#define( 'PRETTY_UTF8', true );

if( defined( 'PRETTY_UTF8' ) ) {
	function pretty( $string ) {
		return preg_replace( '/([\x00-\xff])/e',
			'sprintf("%02X", ord("$1"))',
			$string );
	}
} else {
	/**
	 * @ignore
	 */
	function pretty( $string ) {
		return trim( preg_replace( '/(.)/use',
			'sprintf("%04X ", utf8ToCodepoint("$1"))',
			$string ) );
	}
}

if( isset( $_SERVER['argv'] ) && in_array( '--icu', $_SERVER['argv'] ) ) {
	dl( 'php_utfnormal.so' );
}

require_once 'UtfNormalUtil.php';
require_once 'UtfNormal.php';

if( php_sapi_name() != 'cli' ) {
	die( "Run me from the command line please.\n" );
}

$in = fopen("NormalizationTest.txt", "rt");
if( !$in ) {
	print "Couldn't open NormalizationTest.txt -- can't run tests.\n";
	print "If necessary, manually download this file. It can be obtained at\n";
	print "http://www.unicode.org/Public/UNIDATA/NormalizationTest.txt";
	exit(-1);
}

$normalizer = new UtfNormal;

$total = 0;
$success = 0;
$failure = 0;
$ok = true;
$testedChars = array();
while( false !== ( $line = fgets( $in ) ) ) {
	list( $data, $comment ) = explode( '#', $line );
	if( $data === '' ) continue;
	$matches = array();
	if( preg_match( '/@Part([\d])/', $data, $matches ) ) {
		if( $matches[1] > 0 ) {
			$ok = reportResults( $total, $success, $failure ) && $ok;
		}
		print "Part {$matches[1]}: $comment";
		continue;
	}

	$columns = array_map( "hexSequenceToUtf8", explode( ";", $data ) );
	array_unshift( $columns, '' );

	$testedChars[$columns[1]] = true;
	$total++;
	if( testNormals( $normalizer, $columns, $comment ) ) {
		$success++;
	} else {
		$failure++;
		# print "FAILED: $comment";
	}
	if( $total % 100 == 0 ) print "$total ";
}
fclose( $in );

$ok = reportResults( $total, $success, $failure ) && $ok;

$in = fopen("UnicodeData.txt", "rt" );
if( !$in ) {
	print "Can't open UnicodeData.txt for reading.\n";
	print "If necessary, fetch this file from the internet:\n";
	print "http://www.unicode.org/Public/UNIDATA/UnicodeData.txt\n";
	exit(-1);
}
print "Now testing invariants...\n";
while( false !== ($line = fgets( $in ) ) ) {
	$cols = explode( ';', $line );
	$char = codepointToUtf8( hexdec( $cols[0] ) );
	$desc = $cols[0] . ": " . $cols[1];
	if( $char < "\x20" || $char >= UTF8_SURROGATE_FIRST && $char <= UTF8_SURROGATE_LAST ) {
		# Can't check NULL with the ICU plugin, as null bytes fail in C land.
		# Skip other control characters, as we strip them for XML safety.
		# Surrogates are illegal on their own or in UTF-8, ignore.
		continue;
	}
	if( empty( $testedChars[$char] ) ) {
		$total++;
		if( testInvariant( $normalizer, $char, $desc ) ) {
			$success++;
		} else {
			$failure++;
		}
		if( $total % 100 == 0 ) print "$total ";
	}
}
fclose( $in );

$ok = reportResults( $total, $success, $failure ) && $ok;

if( $ok ) {
	print "TEST SUCCEEDED!\n";
	exit(0);
} else {
	print "TEST FAILED!\n";
	exit(-1);
}

## ------

function reportResults( &$total, &$success, &$failure ) {
	$percSucc = intval( $success * 100 / $total );
	$percFail = intval( $failure * 100 / $total );
	print "\n";
	print "$success tests successful ($percSucc%)\n";
	print "$failure tests failed ($percFail%)\n\n";
	$ok = ($success > 0 && $failure == 0);
	$total = 0;
	$success = 0;
	$failure = 0;
	return $ok;
}

function testNormals( &$u, $c, $comment, $reportFailure = false ) {
	$result = testNFC( $u, $c, $comment, $reportFailure );
	$result = testNFD( $u, $c, $comment, $reportFailure ) && $result;
	$result = testNFKC( $u, $c, $comment, $reportFailure ) && $result;
	$result = testNFKD( $u, $c, $comment, $reportFailure ) && $result;
	$result = testCleanUp( $u, $c, $comment, $reportFailure ) && $result;

	global $verbose;
	if( $verbose && !$result && !$reportFailure ) {
		print $comment;
		testNormals( $u, $c, $comment, true );
	}
	return $result;
}

function verbosify( $a, $b, $col, $form, $verbose ) {
	#$result = ($a === $b);
	$result = (strcmp( $a, $b ) == 0);
	if( $verbose ) {
		$aa = pretty( $a );
		$bb = pretty( $b );
		$ok = $result ? "succeed" : " failed";
		$eq = $result ? "==" : "!=";
		print "  $ok $form c$col '$aa' $eq '$bb'\n";
	}
	return $result;
}

function testNFC( &$u, $c, $comment, $verbose ) {
	$result = verbosify( $c[2], $u->toNFC( $c[1] ), 1, 'NFC', $verbose );
	$result = verbosify( $c[2], $u->toNFC( $c[2] ), 2, 'NFC', $verbose ) && $result;
	$result = verbosify( $c[2], $u->toNFC( $c[3] ), 3, 'NFC', $verbose ) && $result;
	$result = verbosify( $c[4], $u->toNFC( $c[4] ), 4, 'NFC', $verbose ) && $result;
	$result = verbosify( $c[4], $u->toNFC( $c[5] ), 5, 'NFC', $verbose ) && $result;
	return $result;
}

function testCleanUp( &$u, $c, $comment, $verbose ) {
	$x = $c[1];
	$result = verbosify( $c[2], $u->cleanUp( $x ), 1, 'cleanUp', $verbose );
	$x = $c[2];
	$result = verbosify( $c[2], $u->cleanUp( $x ), 2, 'cleanUp', $verbose ) && $result;
	$x = $c[3];
	$result = verbosify( $c[2], $u->cleanUp( $x ), 3, 'cleanUp', $verbose ) && $result;
	$x = $c[4];
	$result = verbosify( $c[4], $u->cleanUp( $x ), 4, 'cleanUp', $verbose ) && $result;
	$x = $c[5];
	$result = verbosify( $c[4], $u->cleanUp( $x ), 5, 'cleanUp', $verbose ) && $result;
	return $result;
}

function testNFD( &$u, $c, $comment, $verbose ) {
	$result = verbosify( $c[3], $u->toNFD( $c[1] ), 1, 'NFD', $verbose );
	$result = verbosify( $c[3], $u->toNFD( $c[2] ), 2, 'NFD', $verbose ) && $result;
	$result = verbosify( $c[3], $u->toNFD( $c[3] ), 3, 'NFD', $verbose ) && $result;
	$result = verbosify( $c[5], $u->toNFD( $c[4] ), 4, 'NFD', $verbose ) && $result;
	$result = verbosify( $c[5], $u->toNFD( $c[5] ), 5, 'NFD', $verbose ) && $result;
	return $result;
}

function testNFKC( &$u, $c, $comment, $verbose ) {
	$result = verbosify( $c[4], $u->toNFKC( $c[1] ), 1, 'NFKC', $verbose );
	$result = verbosify( $c[4], $u->toNFKC( $c[2] ), 2, 'NFKC', $verbose ) && $result;
	$result = verbosify( $c[4], $u->toNFKC( $c[3] ), 3, 'NFKC', $verbose ) && $result;
	$result = verbosify( $c[4], $u->toNFKC( $c[4] ), 4, 'NFKC', $verbose ) && $result;
	$result = verbosify( $c[4], $u->toNFKC( $c[5] ), 5, 'NFKC', $verbose ) && $result;
	return $result;
}

function testNFKD( &$u, $c, $comment, $verbose ) {
	$result = verbosify( $c[5], $u->toNFKD( $c[1] ), 1, 'NFKD', $verbose );
	$result = verbosify( $c[5], $u->toNFKD( $c[2] ), 2, 'NFKD', $verbose ) && $result;
	$result = verbosify( $c[5], $u->toNFKD( $c[3] ), 3, 'NFKD', $verbose ) && $result;
	$result = verbosify( $c[5], $u->toNFKD( $c[4] ), 4, 'NFKD', $verbose ) && $result;
	$result = verbosify( $c[5], $u->toNFKD( $c[5] ), 5, 'NFKD', $verbose ) && $result;
	return $result;
}

function testInvariant( &$u, $char, $desc, $reportFailure = false ) {
	$result = verbosify( $char, $u->toNFC( $char ), 1, 'NFC', $reportFailure );
	$result = verbosify( $char, $u->toNFD( $char ), 1, 'NFD', $reportFailure ) && $result;
	$result = verbosify( $char, $u->toNFKC( $char ), 1, 'NFKC', $reportFailure ) && $result;
	$result = verbosify( $char, $u->toNFKD( $char ), 1, 'NFKD', $reportFailure ) && $result;
	$result = verbosify( $char, $u->cleanUp( $char ), 1, 'cleanUp', $reportFailure ) && $result;
	global $verbose;
	if( $verbose && !$result && !$reportFailure ) {
		print $desc;
		testInvariant( $u, $char, $desc, true );
	}
	return $result;
}
