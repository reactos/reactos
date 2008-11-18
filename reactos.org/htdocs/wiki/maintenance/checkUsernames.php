<?php
/**
 * This script verifies that database usernames are actually valid.
 * An existing usernames can become invalid if User::isValidUserName()
 * is altered or if we change the $wgMaxNameChars
 * @file
 * @ingroup Maintenance
 */

error_reporting(E_ALL ^ E_NOTICE);
require_once 'commandLine.inc';

class checkUsernames {
	var $stderr, $log;

	function checkUsernames() {
		$this->stderr = fopen( 'php://stderr', 'wt' );
	}
	function main() {
		$fname = 'checkUsernames::main';

		$dbr = wfGetDB( DB_SLAVE );

		$res = $dbr->select( 'user',
			array( 'user_id', 'user_name' ),
			null,
			$fname
		);

		while ( $row = $dbr->fetchObject( $res ) ) {
			if ( ! User::isValidUserName( $row->user_name ) ) {
				$out = sprintf( "%s: %6d: '%s'\n", wfWikiID(), $row->user_id, $row->user_name );
				fwrite( $this->stderr, $out );
				wfDebugLog( 'checkUsernames', $out );
			}
		}
	}
}

$cun = new checkUsernames();
$cun->main();

