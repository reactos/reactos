<?php

/**
 * Maintenance script to show the cached statistics.
 * Give out the same output as [[Special:Statistics]]
 *
 * @file
 * @ingroup Maintenance
 * @author Ashar Voultoiz <hashar@altern.org>
 * Based on initStats.php by:
 * @author Brion Vibber
 * @author Rob Church <robchur@gmail.com>
 *
 * @license GNU General Public License 2.0 or later
 */

require_once( 'commandLine.inc' );

#
# Configuration
#
$fields = array(
	'ss_total_views' => 'Total views',
	'ss_total_edits' => 'Total edits',
	'ss_good_articles' => 'Number of articles',
	'ss_total_pages' => 'Total pages',
	'ss_users' => 'Number of users',
	'ss_admins' => 'Number of admins',
	'ss_images' => 'Number of images',
);

// Get cached stats from slave database
$dbr = wfGetDB( DB_SLAVE );
$fname = 'showStats';
$stats = $dbr->selectRow( 'site_stats', '*', '' );

// Get maximum size for each column
$max_length_value = $max_length_desc = 0;
foreach( $fields as $field => $desc ) {
	$max_length_value = max( $max_length_value, strlen( $stats->$field ) );
	$max_length_desc  = max( $max_length_desc , strlen( $desc )) ;
}

// Show them
foreach( $fields as $field => $desc ) {
	printf( "%-{$max_length_desc}s: %{$max_length_value}d\n", $desc, $stats->$field );
}

