<?php
/**
 * @file
 * @ingroup Maintenance
 */

function wfInitStats( $options=array() ) {
	$dbr = wfGetDB( DB_SLAVE );

	echo "Counting total edits...";
	$edits = $dbr->selectField( 'revision', 'COUNT(*)', '', __METHOD__ );
	$edits += $dbr->selectField( 'archive', 'COUNT(*)', '', __METHOD__ );
	echo "{$edits}\nCounting number of articles...";

	global $wgContentNamespaces;
	$good  = $dbr->selectField( 'page', 'COUNT(*)', array( 'page_namespace' => $wgContentNamespaces, 'page_is_redirect' => 0, 'page_len > 0' ), __METHOD__ );
	echo "{$good}\nCounting total pages...";

	$pages = $dbr->selectField( 'page', 'COUNT(*)', '', __METHOD__ );
	echo "{$pages}\nCounting number of users...";

	$users = $dbr->selectField( 'user', 'COUNT(*)', '', __METHOD__ );
	echo "{$users}\nCounting number of admins...";

	$admin = $dbr->selectField( 'user_groups', 'COUNT(*)', array( 'ug_group' => 'sysop' ), __METHOD__ );
	echo "{$admin}\nCounting number of images...";

	$image = $dbr->selectField( 'image', 'COUNT(*)', '', __METHOD__ );
	echo "{$image}\n";

	if( !isset( $options['noviews'] ) ) {
		echo "Counting total page views...";
		$views = $dbr->selectField( 'page', 'SUM(page_counter)', '', __METHOD__ );
		echo "{$views}\n";
	}

	echo "\nUpdating site statistics...";

	$dbw = wfGetDB( DB_MASTER );
	$values = array( 'ss_total_edits' => $edits,
					'ss_good_articles' => $good,
					'ss_total_pages' => $pages,
					'ss_users' => $users,
					'ss_admins' => $admin,
					'ss_images' => $image );
	$conds = array( 'ss_row_id' => 1 );
	$views = array( 'ss_total_views' => isset( $views ) ? $views : 0 );
				
	if( isset( $options['update'] ) ) {
		$dbw->update( 'site_stats', $values, $conds, __METHOD__ );
	} else {
		$dbw->delete( 'site_stats', $conds, __METHOD__ );
		$dbw->insert( 'site_stats', array_merge( $values, $conds, $views ), __METHOD__ );
	}

	echo( "done.\n" );
}
