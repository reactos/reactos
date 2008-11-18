<?php

/**
 * Maintenance script to re-initialise or update the site statistics table
 *
 * @file
 * @ingroup Maintenance
 * @author Brion Vibber
 * @author Rob Church <robchur@gmail.com>
 * @licence GNU General Public Licence 2.0 or later
 */
 
$options = array( 'help', 'update', 'noviews' );
require_once( 'commandLine.inc' );
echo( "Refresh Site Statistics\n\n" );

if( isset( $options['help'] ) ) {
	showHelp();
	exit();
}

require "$IP/maintenance/initStats.inc";
wfInitStats( $options );

function showHelp() {
	echo( "Re-initialise the site statistics tables.\n\n" );
	echo( "Usage: php initStats.php [--update|--noviews]\n\n" );
	echo( " --update : Update the existing statistics (preserves the ss_total_views field)\n" );
	echo( "--noviews : Don't update the page view counter\n\n" );
}

