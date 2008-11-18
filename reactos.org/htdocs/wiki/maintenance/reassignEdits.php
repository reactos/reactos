<?php

/**
 * Reassign edits from a user or IP address to another user
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 * @licence GNU General Public Licence 2.0 or later
 */

$options = array( 'force', 'norc', 'quiet', 'report' );
require_once( 'commandLine.inc' );
require_once( 'reassignEdits.inc.php' );

# Set silent mode; --report overrides --quiet
if( !@$options['report'] && @$options['quiet'] )
	setSilent();
	
out( "Reassign Edits\n\n" );

if( @$args[0] && @$args[1] ) {

	# Set up the users involved
	$from =& initialiseUser( $args[0] );
	$to   =& initialiseUser( $args[1] );
	
	# If the target doesn't exist, and --force is not set, stop here
	if( $to->getId() || @$options['force'] ) {
		# Reassign the edits
		$report = @$options['report'];
		$count = reassignEdits( $from, $to, !@$options['norc'], $report );
		# If reporting, and there were items, advise the user to run without --report	
		if( $report )
			out( "Run the script again without --report to update.\n" );
	} else {
		$ton = $to->getName();
		echo( "User '{$ton}' not found.\n" );
	}
	
} else {
	ShowUsage();
}

/** Show script usage information */
function ShowUsage() {
	echo( "Reassign edits from one user to another.\n\n" );
	echo( "Usage: php reassignEdits.php [--force|--quiet|--norc|--report] <from> <to>\n\n" );
	echo( "    <from> : Name of the user to assign edits from\n" );
	echo( "      <to> : Name of the user to assign edits to\n" );
	echo( "   --force : Reassign even if the target user doesn't exist\n" );
	echo( "   --quiet : Don't print status information (except for errors)\n" );
	echo( "    --norc : Don't update the recent changes table\n" );
	echo( "  --report : Print out details of what would be changed, but don't update it\n\n" );
}

