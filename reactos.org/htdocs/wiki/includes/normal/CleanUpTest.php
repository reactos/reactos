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


if( php_sapi_name() != 'cli' ) {
	die( "Run me from the command line please.\n" );
}

/** */
if( isset( $_SERVER['argv'] ) && in_array( '--icu', $_SERVER['argv'] ) ) {
	dl( 'php_utfnormal.so' );
}

#ini_set( 'memory_limit', '40M' );

require_once 'PHPUnit/Framework.php';
require_once 'PHPUnit/TextUI/TestRunner.php';

require_once 'UtfNormal.php';

/**
 * Additional tests for UtfNormal::cleanUp() function, inclusion
 * regression checks for known problems.
 * Requires PHPUnit.
 *
 * @ingroup UtfNormal
 * @private
 */
class CleanUpTest extends PHPUnit_Framework_TestCase {
	/** @todo document */
	function setUp() {
	}

	/** @todo document */
	function tearDown() {
	}

	/** @todo document */
	function testAscii() {
		$text = 'This is plain ASCII text.';
		$this->assertEquals( $text, UtfNormal::cleanUp( $text ) );
	}

	/** @todo document */
	function testNull() {
		$text = "a \x00 null";
		$expect = "a \xef\xbf\xbd null";
		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testLatin() {
		$text = "L'\xc3\xa9cole";
		$this->assertEquals( $text, UtfNormal::cleanUp( $text ) );
	}

	/** @todo document */
	function testLatinNormal() {
		$text = "L'e\xcc\x81cole";
		$expect = "L'\xc3\xa9cole";
		$this->assertEquals( $expect, UtfNormal::cleanUp( $text ) );
	}

	/**
	 * This test is *very* expensive!
	 * @todo document
	 */
	function XtestAllChars() {
		$rep = UTF8_REPLACEMENT;
		global $utfCanonicalComp, $utfCanonicalDecomp;
		for( $i = 0x0; $i < UNICODE_MAX; $i++ ) {
			$char = codepointToUtf8( $i );
			$clean = UtfNormal::cleanUp( $char );
			$x = sprintf( "%04X", $i );
			if( $i % 0x1000 == 0 ) echo "U+$x\n";
			if( $i == 0x0009 ||
			    $i == 0x000a ||
			    $i == 0x000d ||
			    ($i > 0x001f && $i < UNICODE_SURROGATE_FIRST) ||
			    ($i > UNICODE_SURROGATE_LAST && $i < 0xfffe ) ||
			    ($i > 0xffff && $i <= UNICODE_MAX ) ) {
				if( isset( $utfCanonicalComp[$char] ) || isset( $utfCanonicalDecomp[$char] ) ) {
				    $comp = UtfNormal::NFC( $char );
					$this->assertEquals(
						bin2hex( $comp ),
						bin2hex( $clean ),
						"U+$x should be decomposed" );
				} else {
					$this->assertEquals(
						bin2hex( $char ),
						bin2hex( $clean ),
						"U+$x should be intact" );
				}
			} else {
				$this->assertEquals( bin2hex( $rep ), bin2hex( $clean ), $x );
			}
		}
	}

	/** @todo document */
	function testAllBytes() {
		$this->doTestBytes( '', '' );
		$this->doTestBytes( 'x', '' );
		$this->doTestBytes( '', 'x' );
		$this->doTestBytes( 'x', 'x' );
	}

	/** @todo document */
	function doTestBytes( $head, $tail ) {
		for( $i = 0x0; $i < 256; $i++ ) {
			$char = $head . chr( $i ) . $tail;
			$clean = UtfNormal::cleanUp( $char );
			$x = sprintf( "%02X", $i );
			if( $i == 0x0009 ||
			    $i == 0x000a ||
			    $i == 0x000d ||
			    ($i > 0x001f && $i < 0x80) ) {
				$this->assertEquals(
					bin2hex( $char ),
					bin2hex( $clean ),
					"ASCII byte $x should be intact" );
				if( $char != $clean ) return;
			} else {
				$norm = $head . UTF8_REPLACEMENT . $tail;
				$this->assertEquals(
					bin2hex( $norm ),
					bin2hex( $clean ),
					"Forbidden byte $x should be rejected" );
				if( $norm != $clean ) return;
			}
		}
	}

	/** @todo document */
	function testDoubleBytes() {
		$this->doTestDoubleBytes( '', '' );
		$this->doTestDoubleBytes( 'x', '' );
		$this->doTestDoubleBytes( '', 'x' );
		$this->doTestDoubleBytes( 'x', 'x' );
	}

	/**
	 * @todo document
	 */
	function doTestDoubleBytes( $head, $tail ) {
		for( $first = 0xc0; $first < 0x100; $first++ ) {
			for( $second = 0x80; $second < 0x100; $second++ ) {
				$char = $head . chr( $first ) . chr( $second ) . $tail;
				$clean = UtfNormal::cleanUp( $char );
				$x = sprintf( "%02X,%02X", $first, $second );
				if( $first > 0xc1 &&
				    $first < 0xe0 &&
				    $second < 0xc0 ) {
				    $norm = UtfNormal::NFC( $char );
					$this->assertEquals(
						bin2hex( $norm ),
						bin2hex( $clean ),
						"Pair $x should be intact" );
				    if( $norm != $clean ) return;
				} elseif( $first > 0xfd || $second > 0xbf ) {
					# fe and ff are not legal head bytes -- expect two replacement chars
					$norm = $head . UTF8_REPLACEMENT . UTF8_REPLACEMENT . $tail;
					$this->assertEquals(
						bin2hex( $norm ),
						bin2hex( $clean ),
						"Forbidden pair $x should be rejected" );
					if( $norm != $clean ) return;
				} else {
					$norm = $head . UTF8_REPLACEMENT . $tail;
					$this->assertEquals(
						bin2hex( $norm ),
						bin2hex( $clean ),
						"Forbidden pair $x should be rejected" );
					if( $norm != $clean ) return;
				}
			}
		}
	}

	/** @todo document */
	function testTripleBytes() {
		$this->doTestTripleBytes( '', '' );
		$this->doTestTripleBytes( 'x', '' );
		$this->doTestTripleBytes( '', 'x' );
		$this->doTestTripleBytes( 'x', 'x' );
	}

	/** @todo document */
	function doTestTripleBytes( $head, $tail ) {
		for( $first = 0xc0; $first < 0x100; $first++ ) {
			for( $second = 0x80; $second < 0x100; $second++ ) {
				#for( $third = 0x80; $third < 0x100; $third++ ) {
				for( $third = 0x80; $third < 0x81; $third++ ) {
					$char = $head . chr( $first ) . chr( $second ) . chr( $third ) . $tail;
					$clean = UtfNormal::cleanUp( $char );
					$x = sprintf( "%02X,%02X,%02X", $first, $second, $third );
					if( $first >= 0xe0 &&
						$first < 0xf0 &&
						$second < 0xc0 &&
						$third < 0xc0 ) {
						if( $first == 0xe0 && $second < 0xa0 ) {
							$this->assertEquals(
								bin2hex( $head . UTF8_REPLACEMENT . $tail ),
								bin2hex( $clean ),
								"Overlong triplet $x should be rejected" );
						} elseif( $first == 0xed &&
							( chr( $first ) . chr( $second ) . chr( $third ))  >= UTF8_SURROGATE_FIRST ) {
							$this->assertEquals(
								bin2hex( $head . UTF8_REPLACEMENT . $tail ),
								bin2hex( $clean ),
								"Surrogate triplet $x should be rejected" );
						} else {
							$this->assertEquals(
								bin2hex( UtfNormal::NFC( $char ) ),
								bin2hex( $clean ),
								"Triplet $x should be intact" );
						}
					} elseif( $first > 0xc1 && $first < 0xe0 && $second < 0xc0 ) {
						$this->assertEquals(
							bin2hex( UtfNormal::NFC( $head . chr( $first ) . chr( $second ) ) . UTF8_REPLACEMENT . $tail ),
							bin2hex( $clean ),
							"Valid 2-byte $x + broken tail" );
					} elseif( $second > 0xc1 && $second < 0xe0 && $third < 0xc0 ) {
						$this->assertEquals(
							bin2hex( $head . UTF8_REPLACEMENT . UtfNormal::NFC( chr( $second ) . chr( $third ) . $tail ) ),
							bin2hex( $clean ),
							"Broken head + valid 2-byte $x" );
					} elseif( ( $first > 0xfd || $second > 0xfd ) &&
					            ( ( $second > 0xbf && $third > 0xbf ) ||
					              ( $second < 0xc0 && $third < 0xc0 ) ||
					              ( $second > 0xfd ) ||
					              ( $third > 0xfd ) ) ) {
						# fe and ff are not legal head bytes -- expect three replacement chars
						$this->assertEquals(
							bin2hex( $head . UTF8_REPLACEMENT . UTF8_REPLACEMENT . UTF8_REPLACEMENT . $tail ),
							bin2hex( $clean ),
							"Forbidden triplet $x should be rejected" );
					} elseif( $first > 0xc2 && $second < 0xc0 && $third < 0xc0 ) {
						$this->assertEquals(
							bin2hex( $head . UTF8_REPLACEMENT . $tail ),
							bin2hex( $clean ),
							"Forbidden triplet $x should be rejected" );
					} else {
						$this->assertEquals(
							bin2hex( $head . UTF8_REPLACEMENT . UTF8_REPLACEMENT . $tail ),
							bin2hex( $clean ),
							"Forbidden triplet $x should be rejected" );
					}
				}
			}
		}
	}

	/** @todo document */
	function testChunkRegression() {
		# Check for regression against a chunking bug
		$text   = "\x46\x55\xb8" .
		          "\xdc\x96" .
		          "\xee" .
		          "\xe7" .
		          "\x44" .
		          "\xaa" .
		          "\x2f\x25";
		$expect = "\x46\x55\xef\xbf\xbd" .
		          "\xdc\x96" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\x44" .
		          "\xef\xbf\xbd" .
		          "\x2f\x25";

		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testInterposeRegression() {
		$text   = "\x4e\x30" .
		          "\xb1" .		# bad tail
		          "\x3a" .
		          "\x92" .		# bad tail
		          "\x62\x3a" .
		          "\x84" .		# bad tail
		          "\x43" .
		          "\xc6" .		# bad head
		          "\x3f" .
		          "\x92" .		# bad tail
		          "\xad" .		# bad tail
		          "\x7d" .
		          "\xd9\x95";

		$expect = "\x4e\x30" .
		          "\xef\xbf\xbd" .
		          "\x3a" .
		          "\xef\xbf\xbd" .
		          "\x62\x3a" .
		          "\xef\xbf\xbd" .
		          "\x43" .
		          "\xef\xbf\xbd" .
		          "\x3f" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\x7d" .
		          "\xd9\x95";

		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testOverlongRegression() {
		$text   = "\x67" .
		          "\x1a" . # forbidden ascii
		          "\xea" . # bad head
		          "\xc1\xa6" . # overlong sequence
		          "\xad" . # bad tail
		          "\x1c" . # forbidden ascii
		          "\xb0" . # bad tail
		          "\x3c" .
		          "\x9e";  # bad tail
		$expect = "\x67" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\x3c" .
		          "\xef\xbf\xbd";
		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testSurrogateRegression() {
		$text   = "\xed\xb4\x96" . # surrogate 0xDD16
		          "\x83" . # bad tail
		          "\xb4" . # bad tail
		          "\xac";  # bad head
		$expect = "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd";
		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testBomRegression() {
		$text   = "\xef\xbf\xbe" . # U+FFFE, illegal char
		          "\xb2" . # bad tail
		          "\xef" . # bad head
		          "\x59";
		$expect = "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\xef\xbf\xbd" .
		          "\x59";
		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testForbiddenRegression() {
		$text   = "\xef\xbf\xbf"; # U+FFFF, illegal char
		$expect = "\xef\xbf\xbd";
		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}

	/** @todo document */
	function testHangulRegression() {
		$text = "\xed\x9c\xaf" . # Hangul char
				"\xe1\x87\x81";  # followed by another final jamo
		$expect = $text;         # Should *not* change.
		$this->assertEquals(
			bin2hex( $expect ),
			bin2hex( UtfNormal::cleanUp( $text ) ) );
	}
}


$suite = new PHPUnit_Framework_TestSuite( 'CleanUpTest' );
$result = PHPUnit_TextUI_TestRunner::run( $suite );

if( !$result->wasSuccessful() ) {
	exit( -1 );
}
exit( 0 );
