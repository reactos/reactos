<?php
/**
 * @file
 * @ingroup Maintenance ExternalStorage
 */

define( 'REPORTING_INTERVAL', 100 );

if ( !defined( 'MEDIAWIKI' ) ) {
	$optionsWithArgs = array( 'm' );

	require_once( dirname(__FILE__) . '/../commandLine.inc' );

	resolveStubs();
}

/**
 * Convert history stubs that point to an external row to direct
 * external pointers
 */
function resolveStubs() {
	$fname = 'resolveStubs';

	$dbr = wfGetDB( DB_SLAVE );
	$maxID = $dbr->selectField( 'text', 'MAX(old_id)', false, $fname );
	$blockSize = 10000;
	$numBlocks = intval( $maxID / $blockSize ) + 1;

	for ( $b = 0; $b < $numBlocks; $b++ ) {
		wfWaitForSlaves( 2 );
		
		printf( "%5.2f%%\n", $b / $numBlocks * 100 );
		$start = intval($maxID / $numBlocks) * $b + 1;
		$end = intval($maxID / $numBlocks) * ($b + 1);
		
		$res = $dbr->select( 'text', array( 'old_id', 'old_text', 'old_flags' ),
			"old_id>=$start AND old_id<=$end " .
			# Using a more restrictive flag set for now, until I do some more analysis -- TS
			#"AND old_flags LIKE '%object%' AND old_flags NOT LIKE '%external%' ".
			
			"AND old_flags='object' " .
			"AND LOWER(LEFT(old_text,22)) = 'O:15:\"historyblobstub\"'", $fname );
		while ( $row = $dbr->fetchObject( $res ) ) {
			resolveStub( $row->old_id, $row->old_text, $row->old_flags );
		}
		$dbr->freeResult( $res );

		
	}
	print "100%\n";
}

/**
 * Resolve a history stub
 */
function resolveStub( $id, $stubText, $flags ) {
	$fname = 'resolveStub';

	$stub = unserialize( $stubText );
	$flags = explode( ',', $flags );

	$dbr = wfGetDB( DB_SLAVE );
	$dbw = wfGetDB( DB_MASTER );

	if ( strtolower( get_class( $stub ) ) !== 'historyblobstub' ) {
		print "Error found object of class " . get_class( $stub ) . ", expecting historyblobstub\n";
		return;
	}

	# Get the (maybe) external row
	$externalRow = $dbr->selectRow( 'text', array( 'old_text' ),
		array( 'old_id' => $stub->mOldId, "old_flags LIKE '%external%'" ),
		$fname
	);

	if ( !$externalRow ) {
		# Object wasn't external
		return;
	}

	# Preserve the legacy encoding flag, but switch from object to external
	if ( in_array( 'utf-8', $flags ) ) {
		$newFlags = 'external,utf-8';
	} else {
		$newFlags = 'external';
	}

	# Update the row
	#print "oldid=$id\n";
	$dbw->update( 'text',
		array( /* SET */
			'old_flags' => $newFlags,
			'old_text' => $externalRow->old_text . '/' . $stub->mHash
		),
		array( /* WHERE */
			'old_id' => $id
		), $fname
	);
}

