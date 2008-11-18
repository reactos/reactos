<?php
/**
 * Rebuild recent changes table
 *
 * @file
 * @deprecated
 * @ingroup MaintenanceArchive
 * @defgroup MaintenanceArchive MaintenanceArchive
 */

/** */
function rebuildRecentChangesTable()
{
	$sql = "DROP TABLE IF EXISTS recentchanges";
	wfQuery( $sql );

	$sql = "CREATE TABLE recentchanges (
  rc_timestamp varchar(14) binary NOT NULL default '',
  rc_cur_time varchar(14) binary NOT NULL default '',
  rc_user int(10) unsigned NOT NULL default '0',
  rc_user_text varchar(255) binary NOT NULL default '',
  rc_namespace tinyint(3) unsigned NOT NULL default '0',
  rc_title varchar(255) binary NOT NULL default '',
  rc_comment varchar(255) binary NOT NULL default '',
  rc_minor tinyint(3) unsigned NOT NULL default '0',
  rc_new tinyint(3) unsigned NOT NULL default '0',
  rc_cur_id int(10) unsigned NOT NULL default '0',
  rc_this_oldid int(10) unsigned NOT NULL default '0',
  rc_last_oldid int(10) unsigned NOT NULL default '0',
  INDEX rc_cur_id (rc_cur_id),
  INDEX rc_cur_time (rc_cur_time),
  INDEX rc_timestamp (rc_timestamp),
  INDEX rc_namespace (rc_namespace),
  INDEX rc_title (rc_title)
) ENGINE=MyISAM PACK_KEYS=1;";
	wfQuery( $sql );

	print( "Loading from CUR table...\n" );

	$sql = "INSERT INTO recentchanges (rc_timestamp,rc_cur_time,rc_user," .
	  "rc_user_text,rc_namespace,rc_title,rc_comment,rc_minor,rc_new," .
	  "rc_cur_id,rc_this_oldid,rc_last_oldid) SELECT cur_timestamp," .
	  "cur_timestamp,cur_user,cur_user_text,cur_namespace,cur_title," .
	  "cur_comment,cur_minor_edit,cur_is_new,cur_id,0,0 FROM cur " .
	  "ORDER BY cur_timestamp DESC LIMIT 5000";
	wfQuery( $sql );

	print( "Loading from OLD table...\n" );

	$sql = "INSERT INTO recentchanges (rc_timestamp,rc_cur_time,rc_user," .
      "rc_user_text,rc_namespace,rc_title,rc_comment,rc_minor,rc_new," .
      "rc_cur_id,rc_this_oldid,rc_last_oldid) SELECT old_timestamp,''," .
	  "old_user,old_user_text,old_namespace,old_title,old_comment," .
	  "old_minor_edit,0,0,old_id,0 FROM old ORDER BY old_timestamp " .
	  "DESC LIMIT 5000";
	wfQuery( $sql );

	$sql = "SELECT rc_timestamp FROM recentchanges " .
	  "ORDER BY rc_timestamp DESC LIMIT 5000,1";
	$res = wfQuery( $sql );
	$obj = wfFetchObject( $res );
	$ts = $obj->rc_timestamp;

	$sql = "DELETE FROM recentchanges WHERE rc_timestamp < '{$ts}'";
	wfQuery( $sql );

	rebuildRecentChangesTablePass2();
}

function rebuildRecentChangesTablePass2()
{
	$ns = $id = $count = 0;
	$title = $ct =  "";

	print( "Updating links...\n" );

	$sql = "SELECT rc_namespace,rc_title,rc_timestamp FROM recentchanges " .
	  "ORDER BY rc_namespace,rc_title,rc_timestamp DESC";
	$res = wfQuery( $sql );

	while ( $obj = wfFetchObject( $res ) ) {
		if ( ! ( $ns == $obj->rc_namespace &&
			   0 == strcmp( $title, wfStrencode( $obj->rc_title ) ) ) ) {

			$ns = $obj->rc_namespace;
			$title = wfStrencode( $obj->rc_title );

			$sql = "SELECT cur_id,cur_timestamp FROM cur WHERE " .
			  "cur_namespace={$ns} AND cur_title='{$title}'";
			$res2 = wfQuery( $sql );
			$obj2 = wfFetchObject( $res2 );

			$id = $obj2->cur_id;
			$ct = $obj2->cur_timestamp;
		}
		$sql = "SELECT old_id FROM old WHERE old_namespace={$ns} " .
		  "AND old_title='{$title}' AND old_timestamp < '" .
		  "{$obj->rc_timestamp}' ORDER BY old_timestamp DESC LIMIT 1";
		$res2 = wfQuery( $sql );

		if ( 0 != wfNumRows( $res2 ) ) {
			$obj2 = wfFetchObject( $res2 );

			$sql = "UPDATE recentchanges SET rc_cur_id={$id},rc_cur_time=" .
			  "'{$ct}',rc_last_oldid={$obj2->old_id} WHERE " .
			  "rc_namespace={$ns} AND rc_title='{$title}' AND " .
			  "rc_timestamp='{$obj->rc_timestamp}'";
			wfQuery( $sql );
		} else {
			$sql = "UPDATE recentchanges SET rc_cur_id={$id},rc_cur_time=" .
			  "'{$ct}' WHERE rc_namespace={$ns} AND rc_title='{$title}' " .
			  "AND rc_timestamp='{$obj->rc_timestamp}'";
			wfQuery( $sql );
		}

		if ( 0 == ( ++$count % 500 ) ) {
			printf( "%d records processed.\n", $count );
		}
	}
}


?>
