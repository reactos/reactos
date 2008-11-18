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
 * Test feeds random 16-byte strings to both the pure PHP and ICU-based
 * UtfNormal::cleanUp() code paths, and checks to see if there's a
 * difference. Will run forever until it finds one or you kill it.
 *
 * @ingroup UtfNormal
 * @access private
 */

if( php_sapi_name() != 'cli' ) {
	die( "Run me from the command line please.\n" );
}

/** */
require_once( 'UtfNormal.php' );
require_once( '../DifferenceEngine.php' );

dl('php_utfnormal.so' );

# mt_srand( 99999 );

function randomString( $length, $nullOk, $ascii = false ) {
	$out = '';
	for( $i = 0; $i < $length; $i++ )
		$out .= chr( mt_rand( $nullOk ? 0 : 1, $ascii ? 127 : 255 ) );
	return $out;
}

/* Duplicate of the cleanUp() path for ICU usage */
function donorm( $str ) {
	# We exclude a few chars that ICU would not.
	$str = preg_replace( '/[\x00-\x08\x0b\x0c\x0e-\x1f]/', UTF8_REPLACEMENT, $str );
	$str = str_replace( UTF8_FFFE, UTF8_REPLACEMENT, $str );
	$str = str_replace( UTF8_FFFF, UTF8_REPLACEMENT, $str );

	# UnicodeString constructor fails if the string ends with a head byte.
	# Add a junk char at the end, we'll strip it off
	return rtrim( utf8_normalize( $str . "\x01", UNORM_NFC ), "\x01" );
}

function wfMsg($x) {
	return $x;
}

function showDiffs( $a, $b ) {
	$ota = explode( "\n", str_replace( "\r\n", "\n", $a ) );
	$nta = explode( "\n", str_replace( "\r\n", "\n", $b ) );

	$diffs = new Diff( $ota, $nta );
	$formatter = new TableDiffFormatter();
	$funky = $formatter->format( $diffs );
	$matches = array();
	preg_match_all( '/<(?:ins|del) class="diffchange">(.*?)<\/(?:ins|del)>/', $funky, $matches );
	foreach( $matches[1] as $bit ) {
		$hex = bin2hex( $bit );
		echo "\t$hex\n";
	}
}

$size = 16;
$n = 0;
while( true ) {
	$n++;
	echo "$n\n";

	$str = randomString( $size, true);
	$clean = UtfNormal::cleanUp( $str );
	$norm = donorm( $str );

	echo strlen( $clean ) . ", " . strlen( $norm );
	if( $clean == $norm ) {
		echo " (match)\n";
	} else {
		echo " (FAIL)\n";
		echo "\traw: " . bin2hex( $str ) . "\n" .
			 "\tphp: " . bin2hex( $clean ) . "\n" .
			 "\ticu: " . bin2hex( $norm ) . "\n";
		echo "\n\tdiffs:\n";
		showDiffs( $clean, $norm );
		die();
	}


	$str = '';
	$clean = '';
	$norm = '';
}
