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
 * Runs the UTF-8 decoder test at:
 * http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
 *
 * @ingroup UtfNormal
 * @access private
 */

/** */
require_once 'UtfNormalUtil.php';
require_once 'UtfNormal.php';
mb_internal_encoding( "utf-8" );

#$verbose = true;
if( php_sapi_name() != 'cli' ) {
	die( "Run me from the command line please.\n" );
}

$in = fopen( "UTF-8-test.txt", "rt" );
if( !$in ) {
	print "Couldn't open UTF-8-test.txt -- can't run tests.\n";
	print "If necessary, manually download this file. It can be obtained at\n";
	print "http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt";
	exit(-1);
}

$columns = 0;
while( false !== ( $line = fgets( $in ) ) ) {
	$matches = array();
	if( preg_match( '/^(Here come the tests:\s*)\|$/', $line, $matches ) ) {
		$columns = strpos( $line, '|' );
		break;
	}
}

if( !$columns ) {
	print "Something seems to be wrong; couldn't extract line length.\n";
	print "Check that UTF-8-test.txt was downloaded correctly from\n";
	print "http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt";
	exit(-1);
}

# print "$columns\n";

$ignore = array(
	# These two lines actually seem to be corrupt
	'2.1.1', '2.2.1' );

$exceptions = array(
	# Tests that should mark invalid characters due to using long
	# sequences beyond what is now considered legal.
	'2.1.5', '2.1.6', '2.2.4', '2.2.5', '2.2.6', '2.3.5',

	# Literal 0xffff, which is illegal
	'2.2.3' );

$longTests = array(
	# These tests span multiple lines
	'3.1.9', '3.2.1', '3.2.2', '3.2.3', '3.2.4', '3.2.5',
	'3.4' );

# These tests are not in proper subsections
$sectionTests = array( '3.4' );

$section = NULL;
$test = '';
$failed = 0;
$success = 0;
$total = 0;
while( false !== ( $line = fgets( $in ) ) ) {
	$matches = array();
	if( preg_match( '/^(\d+)\s+(.*?)\s*\|/', $line, $matches ) ) {
		$section = $matches[1];
		print $line;
		continue;
	}
	if( preg_match( '/^(\d+\.\d+\.\d+)\s*/', $line, $matches ) ) {
		$test = $matches[1];

		if( in_array( $test, $ignore ) ) {
			continue;
		}
		if( in_array( $test, $longTests ) ) {
			$line = fgets( $in );
			for( $line = fgets( $in ); !preg_match( '/^\s+\|/', $line ); $line = fgets( $in ) ) {
				testLine( $test, $line, $total, $success, $failed );
			}
		} else {
			testLine( $test, $line, $total, $success, $failed );
		}
	}
}

if( $failed ) {
	echo "\nFailed $failed tests.\n";
	echo "UTF-8 DECODER TEST FAILED\n";
	exit (-1);
}

echo "UTF-8 DECODER TEST SUCCESS!\n";
exit (0);


function testLine( $test, $line, &$total, &$success, &$failed ) {
	$stripped = $line;
	UtfNormal::quickisNFCVerify( $stripped );

	$same = ( $line == $stripped );
	$len = mb_strlen( substr( $stripped, 0, strpos( $stripped, '|' ) ) );
	if( $len == 0 ) {
		$len = strlen( substr( $stripped, 0, strpos( $stripped, '|' ) ) );
	}

	global $columns;
	$ok = $same ^ ($test >= 3 );

	global $exceptions;
	$ok ^= in_array( $test, $exceptions );

	$ok &= ($columns == $len);

	$total++;
	if( $ok ) {
		$success++;
	} else {
		$failed++;
	}
	global $verbose;
	if( $verbose || !$ok ) {
		print str_replace( "\n", "$len\n", $stripped );
	}
}
