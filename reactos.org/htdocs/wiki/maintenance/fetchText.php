<?php
/**
 * Communications protocol...
 *
 * @file
 * @ingroup Maintenance
 */

require "commandLine.inc";

$db = wfGetDB( DB_SLAVE );
$stdin = fopen( "php://stdin", "rt" );
while( !feof( $stdin ) ) {
	$line = fgets( $stdin );
	if( $line === false ) {
		// We appear to have lost contact...
		break;
	}
	$textId = intval( $line );
	$text = doGetText( $db, $textId );
	echo strlen( $text ) . "\n";
	echo $text;
}

/**
 * May throw a database error if, say, the server dies during query.
 */
function doGetText( $db, $id ) {
	$id = intval( $id );
	$row = $db->selectRow( 'text',
		array( 'old_text', 'old_flags' ),
		array( 'old_id' => $id ),
		'TextPassDumper::getText' );
	$text = Revision::getRevisionText( $row );
	if( $text === false ) {
		return false;
	}
	return $text;
}
