<?php
/**
 * Makes the required database updates for Special:ProtectedPages
 * to show all protected pages, even ones before the page restrictions
 * schema change. All remaining page_restriction column values are moved
 * to the new table.
 *
 * @file
 * @ingroup Maintenance
 */

define( 'BATCH_SIZE', 100 );

require_once 'commandLine.inc';
	
$db =& wfGetDB( DB_MASTER );
if ( !$db->tableExists( 'page_restrictions' ) ) {
	echo "page_restrictions does not exist\n";
	exit( 1 );
}

migrate_page_restrictions( $db );

function migrate_page_restrictions( $db ) {
	
	$start = $db->selectField( 'page', 'MIN(page_id)', false, __FUNCTION__ );
	$end = $db->selectField( 'page', 'MAX(page_id)', false, __FUNCTION__ );
	# Do remaining chunk
	$end += BATCH_SIZE - 1;
	$blockStart = $start;
	$blockEnd = $start + BATCH_SIZE - 1;
	$encodedExpiry = 'infinity';
	while ( $blockEnd <= $end ) {
		echo "...doing page_id from $blockStart to $blockEnd\n";
		$cond = "page_id BETWEEN $blockStart AND $blockEnd AND page_restrictions !='' AND page_restrictions !='edit=:move='";
		$res = $db->select( 'page', array('page_id', 'page_restrictions'), $cond, __FUNCTION__ );
		$batch = array();
		while ( $row = $db->fetchObject( $res ) ) {
			$oldRestrictions = array();
			foreach( explode( ':', trim( $row->page_restrictions ) ) as $restrict ) {
				$temp = explode( '=', trim( $restrict ) );
				if(count($temp) == 1) {
					// old old format should be treated as edit/move restriction
					$oldRestrictions["edit"] = trim( $temp[0] );
					$oldRestrictions["move"] = trim( $temp[0] );
				} else {
					$oldRestrictions[$temp[0]] = trim( $temp[1] );
				}
			}
			# Update restrictions table
			foreach( $oldRestrictions as $action => $restrictions ) {
				$batch[] = array( 
					'pr_page' => $row->page_id,
					'pr_type' => $action,
					'pr_level' => $restrictions,
					'pr_cascade' => 0,
					'pr_expiry' => $encodedExpiry
				);
			}
		}
		# We use insert() and not replace() as Article.php replaces
		# page_restrictions with '' when protected in the restrictions table
		if ( count( $batch ) ) {
			$db->insert( 'page_restrictions', $batch, __FUNCTION__, array( 'IGNORE' ) );
		}
		$blockStart += BATCH_SIZE - 1;
		$blockEnd += BATCH_SIZE - 1;
		wfWaitForSlaves( 5 );
	}
}


