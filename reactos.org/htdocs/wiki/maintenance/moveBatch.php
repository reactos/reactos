<?php

/**
 * Maintenance script to move a batch of pages
 *
 * @file
 * @ingroup Maintenance
 * @author Tim Starling
 *
 * USAGE: php moveBatch.php [-u <user>] [-r <reason>] [-i <interval>] <listfile>
 *
 * <listfile> - file with two titles per line, separated with pipe characters;
 * the first title is the source, the second is the destination
 * <user> - username to perform moves as
 * <reason> - reason to be given for moves
 * <interval> - number of seconds to sleep after each move
 *
 * This will print out error codes from Title::moveTo() if something goes wrong,
 * e.g. immobile_namespace for namespaces which can't be moved
 */

$oldCwd = getcwd();
$optionsWithArgs = array( 'u', 'r', 'i' );
require_once( 'commandLine.inc' );

chdir( $oldCwd );

# Options processing

$filename = 'php://stdin';
$user = 'Move page script';
$reason = '';
$interval = 0;

if ( isset( $args[0] ) ) {
	$filename = $args[0];
}
if ( isset( $options['u'] ) ) {
	$user = $options['u'];
}
if ( isset( $options['r'] ) ) {
	$reason = $options['r'];
}
if ( isset( $options['i'] ) ) {
	$interval = $options['i'];
}

$wgUser = User::newFromName( $user );


# Setup complete, now start

$file = fopen( $filename, 'r' );
if ( !$file ) {
	print "Unable to read file, exiting\n";
	exit;
}

$dbw = wfGetDB( DB_MASTER );

for ( $linenum = 1; !feof( $file ); $linenum++ ) {
	$line = fgets( $file );
	if ( $line === false ) {
		break;
	}
	$parts = array_map( 'trim', explode( '|', $line ) );
	if ( count( $parts ) != 2 ) {
		print "Error on line $linenum, no pipe character\n";
		continue;
	}
	$source = Title::newFromText( $parts[0] );
	$dest = Title::newFromText( $parts[1] );
	if ( is_null( $source ) || is_null( $dest ) ) {
		print "Invalid title on line $linenum\n";
		continue;
	}


	print $source->getPrefixedText() . ' --> ' . $dest->getPrefixedText();
	$dbw->begin();
	$err = $source->moveTo( $dest, false, $reason );
	if( $err !== true ) {
		print "\nFAILED: $err";
	}
	$dbw->immediateCommit();
	print "\n";

	if ( $interval ) {
		sleep( $interval );
	}
	wfWaitForSlaves( 5 );
}



