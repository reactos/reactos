<?php
if ( ! defined( 'MEDIAWIKI' ) )
	die( -1 );
/**
 * A basic extension that's used by the parser tests to test date magic words
 *
 * Handy so that we don't have to upgrade the parsertests every second to
 * compensate with the passage of time and certainly less expensive than a
 * time-freezing device, get yours now!
 *
 * @file
 * @ingroup Maintenance
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, 2006 Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

$wgHooks['ParserGetVariableValueTs'][] = 'wfParserTimeSetup';

function wfParserTimeSetup( &$parser, &$ts ) {
	$ts = 123; //$ perl -le 'print scalar localtime 123' ==> Thu Jan  1 00:02:03 1970
	
	return true;
}

