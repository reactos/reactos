<?php
if ( ! defined( 'MEDIAWIKI' ) )
	die( -1 );
/**
 * A basic extension that's used by the parser tests to test whether the parser
 * calls extensions when they're called inside comments, it shouldn't do that
 *
 * @file
 * @ingroup Maintenance
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, 2006 Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

$wgHooks['ParserTestParser'][] = 'wfParserTestStaticParserHookSetup';

function wfParserTestStaticParserHookSetup( &$parser ) {
	$parser->setHook( 'statictag', 'wfParserTestStaticParserHookHook' );

	return true;
}

function wfParserTestStaticParserHookHook( $in, $argv ) {
	static $buf = null;
	
	if ( ! count( $argv ) ) {
		$buf = $in;
		return '';
	} else if ( count( $argv ) === 1 && $argv['action'] === 'flush' && $in === null ) {
		// Clear the buffer, we probably don't need to
		$tmp = $buf;
		$buf = null;
		return $tmp;
	} else
		// wtf?
		die(
			"\nCall this extension as <statictag>string</statictag> or as" .
			" <statictag action=flush/>, not in any other way.\n" .
			"text: " . var_export( $in, true ) . "\n" .
			"argv: " . var_export( $argv, true ) . "\n"
		);
}

