<?php
/**
 * Rebuild recent changes table.
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** Public entry; more passes might come in! :) */
function rebuildRecentChangesTable() {
	rebuildRecentChangesTablePass1();
	rebuildRecentChangesTablePass2();
	rebuildRecentChangesTablePass3();
	rebuildRecentChangesTablePass4();
}

/** */
function rebuildRecentChangesTablePass1()
{
	$dbw = wfGetDB( DB_MASTER );

	$dbw->delete( 'recentchanges', '*' );

	print( "Loading from page and revision tables...\n" );

	global $wgRCMaxAge;

	print( '$wgRCMaxAge=' . $wgRCMaxAge );
	$days = $wgRCMaxAge / 24 / 3600;
	if ( intval($days) == $days ) {
			print( " (" . $days . " days)\n" );
	} else {
			print( " (approx. " .  intval($days) . " days)\n" );
	}

	$cutoff = time() - $wgRCMaxAge;
	$dbw->insertSelect( 'recentchanges', array( 'page', 'revision' ),
		array(
			'rc_timestamp'  => 'rev_timestamp',
			'rc_cur_time'   => 'rev_timestamp',
			'rc_user'       => 'rev_user',
			'rc_user_text'  => 'rev_user_text',
			'rc_namespace'  => 'page_namespace',
			'rc_title'      => 'page_title',
			'rc_comment'    => 'rev_comment',
			'rc_minor'      => 'rev_minor_edit',
			'rc_bot'        => 0,
			'rc_new'        => 'page_is_new',
			'rc_cur_id'     => 'page_id',
			'rc_this_oldid' => 'rev_id',
			'rc_last_oldid' => 0, // is this ok?
			'rc_type'       => $dbw->conditional( 'page_is_new != 0', RC_NEW, RC_EDIT ),
			'rc_deleted'    => 'rev_deleted'
		), array(
			'rev_timestamp > ' . $dbw->addQuotes( $dbw->timestamp( $cutoff ) ),
			'rev_page=page_id'
		), __METHOD__,
		array(), // INSERT options
		array( 'ORDER BY' => 'rev_timestamp DESC', 'LIMIT' => 5000 ) // SELECT options
	);
}

function rebuildRecentChangesTablePass2()
{
	$dbw = wfGetDB( DB_MASTER );
	list ($recentchanges, $revision) = $dbw->tableNamesN( 'recentchanges', 'revision' );

	print( "Updating links and size differences...\n" );

	# Fill in the rc_last_oldid field, which points to the previous edit
	$sql = "SELECT rc_cur_id,rc_this_oldid,rc_timestamp FROM $recentchanges " .
	  "ORDER BY rc_cur_id,rc_timestamp";
	$res = $dbw->query( $sql, DB_MASTER );

	$lastCurId = 0;
	$lastOldId = 0;
	while ( $obj = $dbw->fetchObject( $res ) ) {
		$new = 0;
		if( $obj->rc_cur_id != $lastCurId ) {
			# Switch! Look up the previous last edit, if any
			$lastCurId = intval( $obj->rc_cur_id );
			$emit = $obj->rc_timestamp;
			$sql2 = "SELECT rev_id,rev_len FROM $revision " .
				"WHERE rev_page={$lastCurId} ".
				"AND rev_timestamp<'{$emit}' ORDER BY rev_timestamp DESC LIMIT 1";
			$res2 = $dbw->query( $sql2 );
			if( $row = $dbw->fetchObject( $res2 ) ) {
				$lastOldId = intval($row->rev_id);
				# Grab the last text size if available
				$lastSize = !is_null($row->rev_len) ? intval($row->rev_len) : 'NULL';
			} else {
				# No previous edit
				$lastOldId = 0;
				$lastSize = 'NULL';
				$new = 1; // probably true
			}
			$dbw->freeResult( $res2 );
		}
		if( $lastCurId == 0 ) {
			print "Uhhh, something wrong? No curid\n";
		} else {
			# Grab the entry's text size
			$size = $dbw->selectField( 'revision', 'rev_len', array('rev_id' => $obj->rc_this_oldid ) );
			$size = !is_null($size) ? intval($size) : 'NULL';
			
			$sql3 = "UPDATE $recentchanges SET rc_last_oldid=$lastOldId,rc_new=$new,rc_type=$new," .
				"rc_old_len=$lastSize,rc_new_len=$size " .
				"WHERE rc_cur_id={$lastCurId} AND rc_this_oldid={$obj->rc_this_oldid}";
			$dbw->query( $sql3 );
			
			$lastOldId = intval( $obj->rc_this_oldid );
		}
	}
	$dbw->freeResult( $res );
}

function rebuildRecentChangesTablePass3()
{
	$dbw = wfGetDB( DB_MASTER );

	print( "Loading from user, page, and logging tables...\n" );
	
	global $wgRCMaxAge;
	// Some logs don't go in RC. This can't really detect all of those.
	// At least do the basics logs for a standard install...
	// FIXME: this needs to be maintained
	$basicRCLogs = array( 
		'block',
		'protect',
		'rights',
		'delete',
		'upload',
		'move',
		'import',
		'merge' );
	// Escape...blah blah
	$selectLogs = array();
	foreach( $basicRCLogs as $logtype ) {
		$safetype = $dbw->strencode( $logtype );
		$selectLogs[] = "'$safetype'";
	}
	
	$cutoff = time() - $wgRCMaxAge;
	$dbw->insertSelect( 'recentchanges', array( 'logging', 'page', 'user' ),
		array(
			'rc_timestamp'  => 'log_timestamp',
			'rc_cur_time'   => 'log_timestamp',
			'rc_user'       => 'log_user',
			'rc_user_text'  => 'user_name',
			'rc_namespace'  => 'log_namespace',
			'rc_title'      => 'log_title',
			'rc_comment'    => 'log_comment',
			'rc_minor'      => 0,
			'rc_bot'        => 0,
			'rc_patrolled'  => 1,
			'rc_new'        => 0,
			'rc_this_oldid' => 0,
			'rc_last_oldid' => 0,
			'rc_type'       => RC_LOG,
			'rc_cur_id'     => 'page_id',
			'rc_log_type'   => 'log_type',
			'rc_log_action' => 'log_action',
			'rc_logid'      => 'log_id',
			'rc_params'     => 'log_params',
			'rc_deleted'    => 'log_deleted'
		), array(
			'log_timestamp > ' . $dbw->addQuotes( $dbw->timestamp( $cutoff ) ),
			'log_user=user_id',
			'log_namespace=page_namespace',
			'log_title=page_title',
			'log_type IN(' . implode(',',$selectLogs) . ')'
		), __METHOD__,
		array(), // INSERT options
		array( 'ORDER BY' => 'log_timestamp DESC', 'LIMIT' => 5000 ) // SELECT options
	);
}

function rebuildRecentChangesTablePass4()
{
	global $wgGroupPermissions, $wgUseRCPatrol;
			
	$dbw = wfGetDB( DB_MASTER );
	
	list($recentchanges,$usergroups,$user) = $dbw->tableNamesN( 'recentchanges', 'user_groups', 'user' );

	$botgroups = $autopatrolgroups = array();
	foreach( $wgGroupPermissions as $group => $rights ) {
		if( isset( $rights['bot'] ) && $rights['bot'] == true ) {
			$botgroups[] = $dbw->addQuotes( $group );
		}
		if( $wgUseRCPatrol && isset( $rights['autopatrol'] ) && $rights['autopatrol'] == true ) {
			$autopatrolgroups[] = $dbw->addQuotes( $group );
		}
	}
	# Flag our recent bot edits
	if( !empty($botgroups) ) {
		$botwhere = implode(',',$botgroups);
		$botusers = array();

		print( "Flagging bot account edits...\n" );

		# Find all users that are bots
		$sql = "SELECT DISTINCT user_name FROM $usergroups, $user " .
			"WHERE ug_group IN($botwhere) AND user_id = ug_user";
		$res = $dbw->query( $sql, DB_MASTER );

		while( $obj = $dbw->fetchObject( $res ) ) {
			$botusers[] = $dbw->addQuotes( $obj->user_name );
		}
		# Fill in the rc_bot field
		if( !empty($botusers) ) {
			$botwhere = implode(',',$botusers);
			$sql2 = "UPDATE $recentchanges SET rc_bot=1 " .
				"WHERE rc_user_text IN($botwhere)";
			$dbw->query( $sql2 );
		}
	}
	global $wgMiserMode;
	# Flag our recent autopatrolled edits
	if( !$wgMiserMode && !empty($autopatrolgroups) ) {
		$patrolwhere = implode(',',$autopatrolgroups);
		$patrolusers = array();

		print( "Flagging auto-patrolled edits...\n" );

		# Find all users in RC with autopatrol rights
		$sql = "SELECT DISTINCT user_name FROM $usergroups, $user " .
			"WHERE ug_group IN($patrolwhere) AND user_id = ug_user";
		$res = $dbw->query( $sql, DB_MASTER );

		while( $obj = $dbw->fetchObject( $res ) ) {
			$patrolusers[] = $dbw->addQuotes( $obj->user_name );
		}
		
		# Fill in the rc_patrolled field
		if( !empty($patrolusers) ) {
			$patrolwhere = implode(',',$patrolusers);
			$sql2 = "UPDATE $recentchanges SET rc_patrolled=1 " .
				"WHERE rc_user_text IN($patrolwhere)";
			$dbw->query( $sql2 );
		}
	}
	
	$dbw->freeResult( $res );
}
