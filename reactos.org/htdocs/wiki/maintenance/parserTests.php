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
 * @file
 * @ingroup Maintenance
 */

/** */
require('parserTests.inc');

if( isset( $options['help'] ) ) {
    echo <<<ENDS
MediaWiki $wgVersion parser test suite
Usage: php parserTests.php [--quick] [--quiet] [--show-output]
                           [--color[=(yes|no)]]
                           [--regex=<expression>] [--file=<testfile>]
                           [--record] [--compare]
                           [--help]
Options:
  --quick          Suppress diff output of failed tests
  --quiet          Suppress notification of passed tests (shows only failed tests)
  --show-output    Show expected and actual output
  --color          Override terminal detection and force color output on or off
                   use wgCommandLineDarkBg = true; if your term is dark 
  --regex          Only run tests whose descriptions which match given regex
  --file           Run test cases from a custom file instead of parserTests.txt
  --record         Record tests in database
  --compare        Compare with recorded results, without updating the database.
  --keep-uploads   Re-use the same upload directory for each test, don't delete it
  --help           Show this help message


ENDS;
    exit( 0 );
}

# There is a convention that the parser should never
# refer to $wgTitle directly, but instead use the title
# passed to it.
$wgTitle = Title::newFromText( 'Parser test script do not use' );
$tester = new ParserTest();

if( isset( $options['file'] ) ) {
	$files = array( $options['file'] );
} else {
	// Default parser tests and any set from extensions or local config
	$files = $wgParserTestFiles;
}

# Print out software version to assist with locating regressions
$version = SpecialVersion::getVersion();
echo( "This is MediaWiki version {$version}.\n\n" );
$ok = $tester->runTestsFromFiles( $files );

exit ($ok ? 0 : -1);

