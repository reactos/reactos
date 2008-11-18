<?php
/**
 * @file
 * @ingroup Maintenance
 */

if ( !defined( 'MEDIAWIKI' ) ) {
	echo "This file is not a valid entry point\n";
	exit( 1 );
}

require_once 'convertLinks.inc';
require_once 'userDupes.inc';
require_once 'deleteDefaultMessages.php';
# Extension updates
require_once( "$IP/includes/Hooks.php" );

/**
 * List of update functions to call on a MySQL-based MediaWiki installation,
 * in sequence. First item is function name, rest are parameters to pass.
 */
$wgMysqlUpdates = array(
	// 1.2
	// update_passwords obsolete
	array( 'add_field', 'ipblocks',      'ipb_id',           'patch-ipblocks.sql' ),
	array( 'add_field', 'ipblocks',      'ipb_expiry',       'patch-ipb_expiry.sql' ),
	array( 'do_interwiki_update' ),
	array( 'do_index_update' ),
	// do_linkscc_update obsolete
	array( 'add_table', 'hitcounter',                        'patch-hitcounter.sql' ),
	array( 'add_field', 'recentchanges', 'rc_type',          'patch-rc_type.sql' ),
	
	// 1.3
	array( 'add_field', 'user',          'user_real_name',   'patch-user-realname.sql' ),
	array( 'add_table', 'querycache',                        'patch-querycache.sql' ),
	array( 'add_table', 'objectcache',                       'patch-objectcache.sql' ),
	array( 'add_table', 'categorylinks',                     'patch-categorylinks.sql' ),
	// do_linkscc_1_3_update obsolete
	array( 'do_old_links_update' ),
	array( 'add_field', 'recentchanges', 'rc_ip',            'patch-rc_ip.sql' ),
	
	// 1.4
	array( 'do_image_name_unique_update' ),
	array( 'add_field', 'recentchanges', 'rc_id',            'patch-rc_id.sql' ),
	array( 'add_field', 'recentchanges', 'rc_patrolled',     'patch-rc-patrol.sql' ),
	array( 'add_table', 'logging',                           'patch-logging.sql' ),
	// do_user_rights_update obsolete
	array( 'add_field', 'user',          'user_token',       'patch-user_token.sql' ),
	// old, old_articleid, patch-remove-old-title-namespace.sql obsolete
	// user_groups, patch-userlevels.sql obsolete
	// do_group_update() obsolete
	array( 'do_watchlist_update' ),
	array( 'do_user_update' ),
	// do_copy_newtalk_to_watchlist obsolete
	
	// 1.5
	array( 'do_schema_restructuring' ),
	array( 'add_field', 'logging',       'log_params',       'patch-log_params.sql' ),
	array( 'check_bin', 'logging',       'log_title',        'patch-logging-title.sql', ),
	array( 'add_field', 'archive',       'ar_rev_id',        'patch-archive-rev_id.sql' ),
	array( 'add_field', 'page',          'page_len',         'patch-page_len.sql' ),
	array( 'do_inverse_timestamp' ),
	array( 'do_text_id' ),
	array( 'add_field', 'revision',      'rev_deleted',      'patch-rev_deleted.sql' ),
	array( 'add_field', 'image',         'img_width',        'patch-img_width.sql' ),
	array( 'add_field', 'image',         'img_metadata',     'patch-img_metadata.sql' ),
	array( 'add_field', 'user',          'user_email_token', 'patch-user_email_token.sql' ),
	array( 'add_field', 'archive',       'ar_text_id',       'patch-archive-text_id.sql' ),
	array( 'do_namespace_size' ),
	array( 'add_field', 'image',         'img_media_type',   'patch-img_media_type.sql' ),
	array( 'do_pagelinks_update' ),
	array( 'do_drop_img_type' ),
	array( 'do_user_unique_update' ),
	array( 'do_user_groups_update' ),
	array( 'add_field', 'site_stats',    'ss_total_pages',   'patch-ss_total_articles.sql' ),
	array( 'add_table', 'user_newtalk',                      'patch-usernewtalk2.sql' ),
	array( 'add_table', 'transcache',                        'patch-transcache.sql' ),
	array( 'add_field', 'interwiki',     'iw_trans',         'patch-interwiki-trans.sql' ),
	array( 'add_table', 'trackbacks',                        'patch-trackbacks.sql' ),
	
	// 1.6
	array( 'do_watchlist_null' ),
	// do_image_index_update obsolete
	array( 'do_logging_timestamp_index' ),
	array( 'add_field', 'ipblocks',        'ipb_range_start',  'patch-ipb_range_start.sql' ),
	array( 'do_page_random_update' ),
	array( 'add_field', 'user',            'user_registration','patch-user_registration.sql' ),
	array( 'do_templatelinks_update' ),
	array( 'add_table', 'externallinks',                       'patch-externallinks.sql' ),
	array( 'add_table', 'job',                                 'patch-job.sql' ),
	array( 'add_field', 'site_stats',      'ss_images',        'patch-ss_images.sql' ),
	array( 'add_table', 'langlinks',                           'patch-langlinks.sql' ),
	array( 'add_table', 'querycache_info',                     'patch-querycacheinfo.sql' ),
	array( 'add_table', 'filearchive',                         'patch-filearchive.sql' ),
	array( 'add_field', 'ipblocks',        'ipb_anon_only',    'patch-ipb_anon_only.sql' ),
	array( 'do_rc_indices_update' ),
	
	// 1.9
	array( 'add_field', 'user',          'user_newpass_time', 'patch-user_newpass_time.sql' ),
	array( 'add_table', 'redirect',                           'patch-redirect.sql' ),
	array( 'add_table', 'querycachetwo', 'patch-querycachetwo.sql' ),
	array( 'add_field', 'ipblocks',      'ipb_enable_autoblock', 'patch-ipb_optional_autoblock.sql' ),
	array( 'do_backlinking_indices_update' ),
	array( 'add_field', 'recentchanges', 'rc_old_len',        'patch-rc_len.sql' ),
	array( 'add_field', 'user',          'user_editcount',    'patch-user_editcount.sql' ),
	
	// 1.10
	array( 'do_restrictions_update' ),
	array( 'add_field', 'logging',       'log_id',           'patch-log_id.sql' ),
	array( 'add_field', 'revision',      'rev_parent_id',    'patch-rev_parent_id.sql' ),
	array( 'add_field', 'page_restrictions', 'pr_id',        'patch-page_restrictions_sortkey.sql' ),
	array( 'add_field', 'revision',      'rev_len',          'patch-rev_len.sql' ),
	array( 'add_field', 'recentchanges', 'rc_deleted',       'patch-rc_deleted.sql' ),
	array( 'add_field', 'logging',       'log_deleted',      'patch-log_deleted.sql' ),
	array( 'add_field', 'archive',       'ar_deleted',       'patch-ar_deleted.sql' ),
	array( 'add_field', 'ipblocks',      'ipb_deleted',      'patch-ipb_deleted.sql' ),
	array( 'add_field', 'filearchive',   'fa_deleted',       'patch-fa_deleted.sql' ),
	array( 'add_field', 'archive',       'ar_len',           'patch-ar_len.sql' ),
	
	// 1.11
	array( 'add_field', 'ipblocks',      'ipb_block_email',  'patch-ipb_emailban.sql' ),
	array( 'do_categorylinks_indices_update' ),
	array( 'add_field', 'oldimage',      'oi_metadata',      'patch-oi_metadata.sql'),
	array( 'do_archive_user_index' ),
	array( 'do_image_user_index' ),
	array( 'do_oldimage_user_index' ),
	array( 'add_field', 'archive',       'ar_page_id',       'patch-archive-page_id.sql'),
	array( 'add_field', 'image',         'img_sha1',         'patch-img_sha1.sql' ),

	// 1.12
	array( 'add_table', 'protected_titles',                  'patch-protected_titles.sql' ),
	
	// 1.13
	array( 'add_field', 'ipblocks',      'ipb_by_text',      'patch-ipb_by_text.sql' ),
	array( 'add_table', 'page_props',                        'patch-page_props.sql' ),
	array( 'add_table', 'updatelog',                         'patch-updatelog.sql' ),
	array( 'add_table', 'category',                          'patch-category.sql' ),
	array( 'do_category_population' ),
	array( 'add_field', 'archive',       'ar_parent_id',     'patch-ar_parent_id.sql'),
	array( 'add_field', 'user_newtalk',  'user_last_timestamp', 'patch-user_last_timestamp.sql'),
	array( 'do_populate_parent_id' ),
	array( 'check_bin', 'protected_titles', 'pt_title',      'patch-pt_title-encoding.sql', ),
	array( 'maybe_do_profiling_memory_update' ),
	array( 'do_filearchive_indices_update' ),
	array( 'update_password_format' ),
);


# For extensions only, should be populated via hooks
# $wgDBtype should be checked to specifiy the proper file
$wgExtNewTables = array(); // table, dir
$wgExtNewFields = array(); // table, column, dir
$wgExtPGNewFields = array(); // table, column attributes; for PostgreSQL
$wgExtNewIndexes = array(); // table, index, dir

# Helper function: check if the given key is present in the updatelog table.
# Obviously, only use this for updates that occur after the updatelog table was
# created!
function update_row_exists( $key ) {
	$dbr = wfGetDB( DB_SLAVE );
	$row = $dbr->selectRow(
		'updatelog',
		'1',
		array( 'ul_key' => $key ),
		__FUNCTION__
	);
	return (bool)$row;
}

function rename_table( $from, $to, $patch ) {
	global $wgDatabase;
	if ( $wgDatabase->tableExists( $from ) ) {
		if ( $wgDatabase->tableExists( $to ) ) {
			echo "...can't move table $from to $to, $to already exists.\n";
		} else {
			echo "Moving table $from to $to...";
			dbsource( archive($patch), $wgDatabase );
			echo "ok\n";
		}
	} else {
		// Source table does not exist
		// Renames are done before creations, so this is typical for a new installation
		// Ignore silently
	}
}

function add_table( $name, $patch, $fullpath=false ) {
	global $wgDatabase;
	if ( $wgDatabase->tableExists( $name ) ) {
		echo "...$name table already exists.\n";
	} else {
		echo "Creating $name table...";
		if( $fullpath ) {
			dbsource( $patch, $wgDatabase );
		} else {
			dbsource( archive($patch), $wgDatabase );
		}
		echo "ok\n";
	}
}

function add_field( $table, $field, $patch, $fullpath=false ) {
	global $wgDatabase;
	if ( !$wgDatabase->tableExists( $table ) ) {
		echo "...$table table does not exist, skipping new field patch\n";
	} elseif ( $wgDatabase->fieldExists( $table, $field ) ) {
		echo "...have $field field in $table table.\n";
	} else {
		echo "Adding $field field to table $table...";
		if( $fullpath ) {
			dbsource( $patch, $wgDatabase );
		} else {
			dbsource( archive($patch), $wgDatabase );
		}
		echo "ok\n";
	}
}

function add_index( $table, $index, $patch, $fullpath=false ) {
	global $wgDatabase;
	if( $wgDatabase->indexExists( $table, $index ) ) {
		echo "...$index key already set on $table table.\n";
	} else {
		echo "Adding $index key to table $table... ";
		if( $fullpath ) {
			dbsource( $patch, $wgDatabase );
		} else {
			dbsource( archive($patch), $wgDatabase );
		}
		echo "ok\n";
	}
}

function do_revision_updates() {
	global $wgSoftwareRevision;
	if ( $wgSoftwareRevision < 1001 ) {
		update_passwords();
	}
}

function update_passwords() {
	wfDebugDieBacktrace( "This function needs to be updated or removed.\n" );

	global $wgDatabase;
	$fname = "Update script: update_passwords()";
	print "\nIt appears that you need to update the user passwords in your\n" .
	  "database. If you have already done this (if you've run this update\n" .
	  "script once before, for example), doing so again will make all your\n" .
	  "user accounts inaccessible, so be sure you only do this once.\n" .
	  "Update user passwords? (yes/no)";

	$resp = readconsole();
    if ( ! ( "Y" == $resp{0} || "y" == $resp{0} ) ) { return; }

	$sql = "SELECT user_id,user_password FROM user";
	$source = $wgDatabase->query( $sql, $fname );

	while ( $row = $wgDatabase->fetchObject( $source ) ) {
		$id = $row->user_id;
		$oldpass = $row->user_password;
		$newpass = md5( "{$id}-{$oldpass}" );

		$sql = "UPDATE user SET user_password='{$newpass}' " .
		  "WHERE user_id={$id}";
		$wgDatabase->query( $sql, $fname );
	}
}

function do_interwiki_update() {
	# Check that interwiki table exists; if it doesn't source it
	global $wgDatabase, $IP;
	if( $wgDatabase->tableExists( "interwiki" ) ) {
		echo "...already have interwiki table\n";
		return true;
	}
	echo "Creating interwiki table: ";
	dbsource( archive("patch-interwiki.sql") );
	echo "ok\n";
	echo "Adding default interwiki definitions: ";
	dbsource( "$IP/maintenance/interwiki.sql" );
	echo "ok\n";
}

function do_index_update() {
	# Check that proper indexes are in place
	global $wgDatabase;
	$meta = $wgDatabase->fieldInfo( "recentchanges", "rc_timestamp" );
	if( !$meta->isMultipleKey() ) {
		echo "Updating indexes to 20031107: ";
		dbsource( archive("patch-indexes.sql") );
		echo "ok\n";
		return true;
	}
	echo "...indexes seem up to 20031107 standards\n";
	return false;
}

function do_image_index_update() {
	global $wgDatabase;

	$meta = $wgDatabase->fieldInfo( "image", "img_major_mime" );
	if( !$meta->isMultipleKey() ) {
		echo "Updating indexes to 20050912: ";
		dbsource( archive("patch-mimesearch-indexes.sql") );
		echo "ok\n";
		return true;
	}
	echo "...indexes seem up to 20050912 standards\n";
	return false;
}

function do_image_name_unique_update() {
	global $wgDatabase;
	if( $wgDatabase->indexExists( 'image', 'PRIMARY' ) ) {
		echo "...image primary key already set.\n";
	} else {
		echo "Making img_name the primary key... ";
		dbsource( archive("patch-image_name_primary.sql"), $wgDatabase );
		echo "ok\n";
	}
}

function do_logging_timestamp_index() {
	global $wgDatabase;
	if( $wgDatabase->indexExists( 'logging', 'times' ) ) {
		echo "...timestamp key on logging already exists.\n";
	} else {
		echo "Adding timestamp key on logging table... ";
		dbsource( archive("patch-logging-times-index.sql"), $wgDatabase );
		echo "ok\n";
	}
}

function do_archive_user_index() {
	global $wgDatabase;
	if( $wgDatabase->indexExists( 'archive', 'usertext_timestamp' ) ) {
		echo "...usertext,timestamp key on archive already exists.\n";
	} else {
		echo "Adding usertext,timestamp key on archive table... ";
		dbsource( archive("patch-archive-user-index.sql"), $wgDatabase );
		echo "ok\n";
	}
}

function do_image_user_index() {
	global $wgDatabase;
	if( $wgDatabase->indexExists( 'image', 'img_usertext_timestamp' ) ) {
		echo "...usertext,timestamp key on image already exists.\n";
	} else {
		echo "Adding usertext,timestamp key on image table... ";
		dbsource( archive("patch-image-user-index.sql"), $wgDatabase );
		echo "ok\n";
	}
}

function do_oldimage_user_index() {
	global $wgDatabase;
	if( $wgDatabase->indexExists( 'oldimage', 'oi_usertext_timestamp' ) ) {
		echo "...usertext,timestamp key on oldimage already exists.\n";
	} else {
		echo "Adding usertext,timestamp key on oldimage table... ";
		dbsource( archive("patch-oldimage-user-index.sql"), $wgDatabase );
		echo "ok\n";
	}
}

function do_watchlist_update() {
	global $wgDatabase;
	$fname = 'do_watchlist_update';
	if( $wgDatabase->fieldExists( 'watchlist', 'wl_notificationtimestamp' ) ) {
		echo "The watchlist table is already set up for email notification.\n";
	} else {
		echo "Adding wl_notificationtimestamp field for email notification management.";
		/* ALTER TABLE watchlist ADD (wl_notificationtimestamp varchar(14) binary NOT NULL default '0'); */
		dbsource( archive( 'patch-email-notification.sql' ), $wgDatabase );
		echo "ok\n";
	}
	# Check if we need to add talk page rows to the watchlist
	$talk = $wgDatabase->selectField( 'watchlist', 'count(*)', 'wl_namespace & 1', $fname );
	$nontalk = $wgDatabase->selectField( 'watchlist', 'count(*)', 'NOT (wl_namespace & 1)', $fname );
	if ( $talk != $nontalk ) {
		echo "Adding missing watchlist talk page rows... ";
		flush();

		$wgDatabase->insertSelect( 'watchlist', 'watchlist', 
			array(
				'wl_user' => 'wl_user',
				'wl_namespace' => 'wl_namespace | 1',
				'wl_title' => 'wl_title',
				'wl_notificationtimestamp' => 'wl_notificationtimestamp'
			), array( 'NOT (wl_namespace & 1)' ), $fname, 'IGNORE' );
		echo "ok\n";
	} else {
		echo "...watchlist talk page rows already present\n";
	}
}

function do_copy_newtalk_to_watchlist() {
	global $wgDatabase;
	global $wgCommandLineMode;	# this needs to be saved while getID() and getName() are called

	$res = $wgDatabase->safeQuery( 'SELECT user_id, user_ip FROM !',
		$wgDatabase->tableName( 'user_newtalk' ) );
	$num_newtalks=$wgDatabase->numRows($res);
	echo "Now converting ".$num_newtalks." user_newtalk entries to watchlist table entries ... \n";

	$user = new User();
	for ( $i = 1; $i <= $num_newtalks; $i++ ) {
		$wluser = $wgDatabase->fetchObject( $res );
		if ($wluser->user_id == 0) { # anonymous users ... have IP numbers as "names"
			if ($user->isIP($wluser->user_ip)) { # do only if it really looks like an IP number (double checked)
				$wgDatabase->replace( 'watchlist',
					array(array('wl_user','wl_namespace', 'wl_title', 'wl_notificationtimestamp' )),
					  array('wl_user' 			=> 0,
						'wl_namespace' 			=> NS_USER_TALK,
						'wl_title' 			=> $wluser->user_ip,
						'wl_notificationtimestamp' 	=> '19700101000000'
						), 'updaters.inc::do_watchlist_update2'
					);
			}
		} else { # normal users ... have user_ids
			$user->setID($wluser->user_id);
			$wgDatabase->replace( 'watchlist',
				array(array('wl_user','wl_namespace', 'wl_title', 'wl_notificationtimestamp' )),
				  array('wl_user' 			=> $user->getID(),
					'wl_namespace' 			=> NS_USER_TALK,
					'wl_title' 			=> $user->getName(),
					'wl_notificationtimestamp' 	=> '19700101000000'
					), 'updaters.inc::do_watchlist_update3'
				);
		}
	}
	echo "Done.\n";
}


function do_user_update() {
	global $wgDatabase;
	if( $wgDatabase->fieldExists( 'user', 'user_emailauthenticationtimestamp' ) ) {
		echo "User table contains old email authentication field. Dropping... ";
		dbsource( archive( 'patch-email-authentication.sql' ), $wgDatabase );
		echo "ok\n";
	} else {
		echo "...user table does not contain old email authentication field.\n";
	}
}

/**
 * 1.4 betas were missing the 'binary' marker from logging.log_title,
 * which causes a collation mismatch error on joins in MySQL 4.1.
 */
function check_bin( $table, $field, $patchFile ) {
	global $wgDatabase, $wgDBtype;
	if ($wgDBtype != 'mysql')
		return;
	$tableName = $wgDatabase->tableName( $table );
	$res = $wgDatabase->query( "SELECT $field FROM $tableName LIMIT 0" );
	$flags = explode( ' ', mysql_field_flags( $res->result, 0 ) );
	$wgDatabase->freeResult( $res );

	if( in_array( 'binary', $flags ) ) {
		echo "$table table has correct $field encoding.\n";
	} else {
		echo "Fixing $field encoding on $table table... ";
		dbsource( archive( $patchFile ), $wgDatabase );
		echo "ok\n";
	}
}

function do_schema_restructuring() {
	global $wgDatabase;
	$fname="do_schema_restructuring";
	if ( $wgDatabase->tableExists( 'page' ) ) {
		echo "...page table already exists.\n";
	} else {
		echo "...converting from cur/old to page/revision/text DB structure.\n"; flush();
		echo wfTimestamp( TS_DB );
		echo "......checking for duplicate entries.\n"; flush();

		list ($cur, $old, $page, $revision, $text) = $wgDatabase->tableNamesN( 'cur', 'old', 'page', 'revision', 'text' );

		$rows = $wgDatabase->query( "SELECT cur_title, cur_namespace, COUNT(cur_namespace) AS c
				FROM $cur GROUP BY cur_title, cur_namespace HAVING c>1", $fname );

		if ( $wgDatabase->numRows( $rows ) > 0 ) {
			echo wfTimestamp( TS_DB );
			echo "......<b>Found duplicate entries</b>\n";
			echo ( sprintf( "<b>      %-60s %3s %5s</b>\n", 'Title', 'NS', 'Count' ) );
			while ( $row = $wgDatabase->fetchObject( $rows ) ) {
				if ( ! isset( $duplicate[$row->cur_namespace] ) ) {
					$duplicate[$row->cur_namespace] = array();
				}
				$duplicate[$row->cur_namespace][] = $row->cur_title;
				echo ( sprintf( "      %-60s %3s %5s\n", $row->cur_title, $row->cur_namespace, $row->c ) );
			}
			$sql = "SELECT cur_title, cur_namespace, cur_id, cur_timestamp FROM $cur WHERE ";
			$firstCond = true;
			foreach ( $duplicate as $ns => $titles ) {
				if ( $firstCond ) {
					$firstCond = false;
				} else {
					$sql .= ' OR ';
				}
				$sql .= "( cur_namespace = {$ns} AND cur_title in (";
				$first = true;
				foreach ( $titles as $t ) {
					if ( $first ) {
						$sql .= $wgDatabase->addQuotes( $t );
						$first = false;
					} else {
						$sql .= ', ' . $wgDatabase->addQuotes( $t );
					}
				}
				$sql .= ") ) \n";
			}
			# By sorting descending, the most recent entry will be the first in the list.
			# All following entries will be deleted by the next while-loop.
			$sql .= 'ORDER BY cur_namespace, cur_title, cur_timestamp DESC';

			$rows = $wgDatabase->query( $sql, $fname );

			$prev_title = $prev_namespace = false;
			$deleteId = array();

			while ( $row = $wgDatabase->fetchObject( $rows ) ) {
				if ( $prev_title == $row->cur_title && $prev_namespace == $row->cur_namespace ) {
					$deleteId[] = $row->cur_id;
				}
				$prev_title     = $row->cur_title;
				$prev_namespace = $row->cur_namespace;
			}
			$sql = "DELETE FROM $cur WHERE cur_id IN ( " . join( ',', $deleteId ) . ')';
			$rows = $wgDatabase->query( $sql, $fname );
			echo wfTimestamp( TS_DB );
			echo "......<b>Deleted</b> ".$wgDatabase->affectedRows()." records.\n";
		}


		echo wfTimestamp( TS_DB );
		echo "......Creating tables.\n";
		$wgDatabase->query("CREATE TABLE $page (
  			page_id int(8) unsigned NOT NULL auto_increment,
  			page_namespace int NOT NULL,
  			page_title varchar(255) binary NOT NULL,
  			page_restrictions tinyblob NOT NULL,
  			page_counter bigint(20) unsigned NOT NULL default '0',
  			page_is_redirect tinyint(1) unsigned NOT NULL default '0',
  			page_is_new tinyint(1) unsigned NOT NULL default '0',
  			page_random real unsigned NOT NULL,
  			page_touched char(14) binary NOT NULL default '',
  			page_latest int(8) unsigned NOT NULL,
  			page_len int(8) unsigned NOT NULL,

  			PRIMARY KEY page_id (page_id),
  			UNIQUE INDEX name_title (page_namespace,page_title),
  			INDEX (page_random),
  			INDEX (page_len)
			) ENGINE=InnoDB", $fname );
		$wgDatabase->query("CREATE TABLE $revision (
  			rev_id int(8) unsigned NOT NULL auto_increment,
  			rev_page int(8) unsigned NOT NULL,
  			rev_comment tinyblob NOT NULL,
  			rev_user int(5) unsigned NOT NULL default '0',
  			rev_user_text varchar(255) binary NOT NULL default '',
  			rev_timestamp char(14) binary NOT NULL default '',
  			rev_minor_edit tinyint(1) unsigned NOT NULL default '0',
			rev_deleted tinyint(1) unsigned NOT NULL default '0',
			rev_len int(8) unsigned,
			rev_parent_id int(8) unsigned default NULL,
  			PRIMARY KEY rev_page_id (rev_page, rev_id),
  			UNIQUE INDEX rev_id (rev_id),
  			INDEX rev_timestamp (rev_timestamp),
  			INDEX page_timestamp (rev_page,rev_timestamp),
  			INDEX user_timestamp (rev_user,rev_timestamp),
  			INDEX usertext_timestamp (rev_user_text,rev_timestamp)
			) ENGINE=InnoDB", $fname );

		echo wfTimestamp( TS_DB );
		echo "......Locking tables.\n";
		$wgDatabase->query( "LOCK TABLES $page WRITE, $revision WRITE, $old WRITE, $cur WRITE", $fname );

		$maxold = intval( $wgDatabase->selectField( 'old', 'max(old_id)', '', $fname ) );
		echo wfTimestamp( TS_DB );
		echo "......maxold is {$maxold}\n";

		echo wfTimestamp( TS_DB );
		global $wgLegacySchemaConversion;
		if( $wgLegacySchemaConversion ) {
			// Create HistoryBlobCurStub entries.
			// Text will be pulled from the leftover 'cur' table at runtime.
			echo "......Moving metadata from cur; using blob references to text in cur table.\n";
			$cur_text = "concat('O:18:\"historyblobcurstub\":1:{s:6:\"mCurId\";i:',cur_id,';}')";
			$cur_flags = "'object'";
		} else {
			// Copy all cur text in immediately: this may take longer but avoids
			// having to keep an extra table around.
			echo "......Moving text from cur.\n";
			$cur_text = 'cur_text';
			$cur_flags = "''";
		}
		$wgDatabase->query( "INSERT INTO $old (old_namespace, old_title, old_text, old_comment, old_user, old_user_text,
    				old_timestamp, old_minor_edit, old_flags)
  			SELECT cur_namespace, cur_title, $cur_text, cur_comment, cur_user, cur_user_text, cur_timestamp, cur_minor_edit, $cur_flags
  			FROM $cur", $fname );

		echo wfTimestamp( TS_DB );
		echo "......Setting up revision table.\n";
		$wgDatabase->query( "INSERT INTO $revision (rev_id, rev_page, rev_comment, rev_user, rev_user_text, rev_timestamp,
    				rev_minor_edit)
			SELECT old_id, cur_id, old_comment, old_user, old_user_text,
    				old_timestamp, old_minor_edit
			FROM $old,$cur WHERE old_namespace=cur_namespace AND old_title=cur_title", $fname );

		echo wfTimestamp( TS_DB );
		echo "......Setting up page table.\n";
		$wgDatabase->query( "INSERT INTO $page (page_id, page_namespace, page_title, page_restrictions, page_counter,
    				page_is_redirect, page_is_new, page_random, page_touched, page_latest, page_len)
  			SELECT cur_id, cur_namespace, cur_title, cur_restrictions, cur_counter, cur_is_redirect, cur_is_new,
    				cur_random, cur_touched, rev_id, LENGTH(cur_text)
  			FROM $cur,$revision
  			WHERE cur_id=rev_page AND rev_timestamp=cur_timestamp AND rev_id > {$maxold}", $fname );

		echo wfTimestamp( TS_DB );
		echo "......Unlocking tables.\n";
		$wgDatabase->query( "UNLOCK TABLES", $fname );

		echo wfTimestamp( TS_DB );
		echo "......Renaming old.\n";
		$wgDatabase->query( "ALTER TABLE $old RENAME TO $text", $fname );

		echo wfTimestamp( TS_DB );
		echo "...done.\n";
	}
}

function do_inverse_timestamp() {
	global $wgDatabase;
	if( $wgDatabase->fieldExists( 'revision', 'inverse_timestamp' ) ) {
		echo "Removing revision.inverse_timestamp and fixing indexes... ";
		dbsource( archive( 'patch-inverse_timestamp.sql' ), $wgDatabase );
		echo "ok\n";
	} else {
		echo "revision timestamp indexes already up to 2005-03-13\n";
	}
}

function do_text_id() {
	global $wgDatabase;
	if( $wgDatabase->fieldExists( 'revision', 'rev_text_id' ) ) {
		echo "...rev_text_id already in place.\n";
	} else {
		echo "Adding rev_text_id field... ";
		dbsource( archive( 'patch-rev_text_id.sql' ), $wgDatabase );
		echo "ok\n";
	}
}

function do_namespace_size() {
	$tables = array(
		'page'          => 'page',
		'archive'       => 'ar',
		'recentchanges' => 'rc',
		'watchlist'     => 'wl',
		'querycache'    => 'qc',
		'logging'       => 'log',
	);
	foreach( $tables as $table => $prefix ) {
		do_namespace_size_on( $table, $prefix );
		flush();
	}
}

function do_namespace_size_on( $table, $prefix ) {
	global $wgDatabase, $wgDBtype;
	if ($wgDBtype != 'mysql')
		return;
	$field = $prefix . '_namespace';

	$tablename = $wgDatabase->tableName( $table );
	$result = $wgDatabase->query( "SHOW COLUMNS FROM $tablename LIKE '$field'" );
	$info = $wgDatabase->fetchObject( $result );
	$wgDatabase->freeResult( $result );

	if( substr( $info->Type, 0, 3 ) == 'int' ) {
		echo "...$field is already a full int ($info->Type).\n";
	} else {
		echo "Promoting $field from $info->Type to int... ";

		$sql = "ALTER TABLE $tablename MODIFY $field int NOT NULL";
		$wgDatabase->query( $sql );

		echo "ok\n";
	}
}

function do_pagelinks_update() {
	global $wgDatabase;
	if( $wgDatabase->tableExists( 'pagelinks' ) ) {
		echo "...already have pagelinks table.\n";
	} else {
		echo "Converting links and brokenlinks tables to pagelinks... ";
		dbsource( archive( 'patch-pagelinks.sql' ), $wgDatabase );
		echo "ok\n";
		flush();

		global $wgCanonicalNamespaceNames;
		foreach( $wgCanonicalNamespaceNames as $ns => $name ) {
			if( $ns != 0 ) {
				do_pagelinks_namespace( $ns );
			}
		}
	}
}

function do_pagelinks_namespace( $namespace ) {
	global $wgDatabase, $wgContLang;

	$ns = intval( $namespace );
	echo "Cleaning up broken links for namespace $ns... ";

	$pagelinks = $wgDatabase->tableName( 'pagelinks' );
	$name = $wgContLang->getNsText( $ns );
	$prefix = $wgDatabase->strencode( $name );
	$likeprefix = str_replace( '_', '\\_', $prefix);

	$sql = "UPDATE $pagelinks
	           SET pl_namespace=$ns,
	               pl_title=TRIM(LEADING '$prefix:' FROM pl_title)
	         WHERE pl_namespace=0
	           AND pl_title LIKE '$likeprefix:%'";

	$wgDatabase->query( $sql, 'do_pagelinks_namespace' );
	echo "ok\n";
}

function do_drop_img_type() {
	global $wgDatabase;

	if( $wgDatabase->fieldExists( 'image', 'img_type' ) ) {
		echo "Dropping unused img_type field in image table... ";
		dbsource( archive( 'patch-drop_img_type.sql' ), $wgDatabase );
		echo "ok\n";
	} else {
		echo "No img_type field in image table; Good.\n";
	}
}

function do_old_links_update() {
	global $wgDatabase;
	if( $wgDatabase->tableExists( 'pagelinks' ) ) {
		echo "Already have pagelinks; skipping old links table updates.\n";
	} else {
		convertLinks(); flush();
	}
}

function do_user_unique_update() {
	global $wgDatabase;
	$duper = new UserDupes( $wgDatabase );
	if( $duper->hasUniqueIndex() ) {
		echo "Already have unique user_name index.\n";
	} else {
		if( !$duper->clearDupes() ) {
			echo "WARNING: This next step will probably fail due to unfixed duplicates...\n";
		}
		echo "Adding unique index on user_name... ";
		dbsource( archive( 'patch-user_nameindex.sql' ), $wgDatabase );
		echo "ok\n";
	}
}

function do_user_groups_update() {
	$fname = 'do_user_groups_update';
	global $wgDatabase;

	if( $wgDatabase->tableExists( 'user_groups' ) ) {
		echo "...user_groups table already exists.\n";
		return do_user_groups_reformat();
	}

	echo "Adding user_groups table... ";
	dbsource( archive( 'patch-user_groups.sql' ), $wgDatabase );
	echo "ok\n";

	if( !$wgDatabase->tableExists( 'user_rights' ) ) {
		if( $wgDatabase->fieldExists( 'user', 'user_rights' ) ) {
			echo "Upgrading from a 1.3 or older database? Breaking out user_rights for conversion...";
			dbsource( archive( 'patch-user_rights.sql' ), $wgDatabase );
			echo "ok\n";
		} else {
			echo "*** WARNING: couldn't locate user_rights table or field for upgrade.\n";
			echo "*** You may need to manually configure some sysops by manipulating\n";
			echo "*** the user_groups table.\n";
			return;
		}
	}

	echo "Converting user_rights table to user_groups... ";
	$result = $wgDatabase->select( 'user_rights',
		array( 'ur_user', 'ur_rights' ),
		array( "ur_rights != ''" ),
		$fname );

	while( $row = $wgDatabase->fetchObject( $result ) ) {
		$groups = array_unique(
			array_map( 'trim',
				explode( ',', $row->ur_rights ) ) );

		foreach( $groups as $group ) {
			$wgDatabase->insert( 'user_groups',
				array(
					'ug_user'  => $row->ur_user,
					'ug_group' => $group ),
				$fname );
		}
	}
	$wgDatabase->freeResult( $result );
	echo "ok\n";
}

function do_user_groups_reformat() {
	# Check for bogus formats from previous 1.5 alpha code.
	global $wgDatabase;
	$info = $wgDatabase->fieldInfo( 'user_groups', 'ug_group' );

	if( $info->type() == 'int' ) {
		$oldug = $wgDatabase->tableName( 'user_groups' );
		$newug = $wgDatabase->tableName( 'user_groups_bogus' );
		echo "user_groups is in bogus intermediate format. Renaming to $newug... ";
		$wgDatabase->query( "ALTER TABLE $oldug RENAME TO $newug" );
		echo "ok\n";

		echo "Re-adding fresh user_groups table... ";
		dbsource( archive( 'patch-user_groups.sql' ), $wgDatabase );
		echo "ok\n";

		echo "***\n";
		echo "*** WARNING: You will need to manually fix up user permissions in the user_groups\n";
		echo "*** table. Old 1.5 alpha versions did some pretty funky stuff...\n";
		echo "***\n";
	} else {
		echo "...user_groups is in current format.\n";
	}

}

function do_watchlist_null() {
	# Make sure wl_notificationtimestamp can be NULL,
	# and update old broken items.
	global $wgDatabase;
	$info = $wgDatabase->fieldInfo( 'watchlist', 'wl_notificationtimestamp' );

	if( !$info->nullable() ) {
		echo "Making wl_notificationtimestamp nullable... ";
		dbsource( archive( 'patch-watchlist-null.sql' ), $wgDatabase );
		echo "ok\n";
	} else {
		echo "...wl_notificationtimestamp is already nullable.\n";
	}

}

/**
 * @bug 3946
 */
function do_page_random_update() {
	global $wgDatabase;

	echo "Setting page_random to a random value on rows where it equals 0...";

	$page = $wgDatabase->tableName( 'page' );
	$wgDatabase->query( "UPDATE $page SET page_random = RAND() WHERE page_random = 0", 'do_page_random_update' );
	$rows = $wgDatabase->affectedRows();

	echo "changed $rows rows\n";
}

function do_templatelinks_update() {
	global $wgDatabase, $wgLoadBalancer;
	$fname = 'do_templatelinks_update';

	if ( $wgDatabase->tableExists( 'templatelinks' ) ) {
		echo "...templatelinks table already exists\n";
		return;
	}
	echo "Creating templatelinks table...\n";
	dbsource( archive('patch-templatelinks.sql'), $wgDatabase );
	echo "Populating...\n";
	if ( isset( $wgLoadBalancer ) && $wgLoadBalancer->getServerCount() > 1 ) {
		// Slow, replication-friendly update
		$res = $wgDatabase->select( 'pagelinks', array( 'pl_from', 'pl_namespace', 'pl_title' ),
			array( 'pl_namespace' => NS_TEMPLATE ), $fname );
		$count = 0;
		while ( $row = $wgDatabase->fetchObject( $res ) ) {
			$count = ($count + 1) % 100;
			if ( $count == 0 ) {
				if ( function_exists( 'wfWaitForSlaves' ) ) {
					wfWaitForSlaves( 10 );
				} else {
					sleep( 1 );
				}
			}
			$wgDatabase->insert( 'templatelinks',
				array(
					'tl_from' => $row->pl_from,
					'tl_namespace' => $row->pl_namespace,
					'tl_title' => $row->pl_title,
				), $fname
			);

		}
		$wgDatabase->freeResult( $res );
	} else {
		// Fast update
		$wgDatabase->insertSelect( 'templatelinks', 'pagelinks',
			array(
				'tl_from' => 'pl_from',
				'tl_namespace' => 'pl_namespace',
				'tl_title' => 'pl_title'
			), array(
				'pl_namespace' => 10
			), $fname
		);
	}
	echo "Done. Please run maintenance/refreshLinks.php for a more thorough templatelinks update.\n";
}

// Add index on ( rc_namespace, rc_user_text ) [Jul. 2006]
// Add index on ( rc_user_text, rc_timestamp ) [Nov. 2006]
function do_rc_indices_update() {
	global $wgDatabase;
	echo( "Checking for additional recent changes indices...\n" );

	$indexes = array(
		'rc_ns_usertext' => 'patch-recentchanges-utindex.sql',
		'rc_user_text' => 'patch-rc_user_text-index.sql',
	);
	
	foreach( $indexes as $index => $patch ) {
		$info = $wgDatabase->indexInfo( 'recentchanges', $index, __METHOD__ );
		if( !$info ) {
			echo( "...index `{$index}` not found; adding..." );
			dbsource( archive( $patch ) );
			echo( "done.\n" );
		} else {
			echo( "...index `{$index}` seems ok.\n" );
		}
	}
}

function index_has_field($table, $index, $field) {
	global $wgDatabase;
	echo( "Checking if $table index $index includes field $field...\n" );
	$info = $wgDatabase->indexInfo( $table, $index, __METHOD__ );
	if( $info ) {
		foreach($info as $row) {
			if($row->Column_name == $field) {
				echo( "...index $index on table $table seems to be ok\n" );
				return true;
			}
		}
	}
	echo( "...index $index on table $table has no field $field; adding\n" );
	return false;
}

function do_backlinking_indices_update() {
	echo( "Checking for backlinking indices...\n" );
	if (!index_has_field('pagelinks', 'pl_namespace', 'pl_from') ||
		!index_has_field('templatelinks', 'tl_namespace', 'tl_from') ||
		!index_has_field('imagelinks', 'il_to', 'il_from'))
	{	
		dbsource( archive( 'patch-backlinkindexes.sql' ) );
		echo( "...backlinking indices updated\n" );
	}
}

function do_categorylinks_indices_update() {
	echo( "Checking for categorylinks indices...\n" );
	if (!index_has_field('categorylinks', 'cl_sortkey', 'cl_from'))
	{	
		dbsource( archive( 'patch-categorylinksindex.sql' ) );
		echo( "...categorylinks indices updated\n" );
	}
}

function do_filearchive_indices_update() {
	global $wgDatabase;
	echo( "Checking filearchive indices...\n" );
	$info = $wgDatabase->indexInfo( 'filearchive', 'fa_user_timestamp', __METHOD__ );
	if ( !$info )
	{	
		dbsource( archive( 'patch-filearhive-user-index.sql' ) );
		echo( "...filearchive indices updated\n" );
	}
}

function maybe_do_profiling_memory_update() {
	global $wgDatabase;
	if ( !$wgDatabase->tableExists( 'profiling' ) ) {
		// Simply ignore
	} elseif ( $wgDatabase->fieldExists( 'profiling', 'pf_memory' ) ) {
		echo "profiling table has pf_memory field.\n";
	} else {
		echo "Adding pf_memory field to table profiling...";
		dbsource( archive( 'patch-profiling-memory.sql' ), $wgDatabase );
		echo "ok\n";
	}
}

function do_stats_init() {
	// Sometimes site_stats table is not properly populated.
	global $wgDatabase;
	echo "Checking site_stats row...";
	$row = $wgDatabase->selectRow( 'site_stats', '*', array( 'ss_row_id' => 1 ), __METHOD__ );
	if( $row === false ) {
		echo "data is missing! rebuilding...\n";
		
		global $IP;
		require_once "$IP/maintenance/initStats.inc";
		wfInitStats();
	} else {
		echo "ok.\n";
	}
}

function purge_cache() {
	global $wgDatabase;
	# We can't guarantee that the user will be able to use TRUNCATE,
	# but we know that DELETE is available to us
	echo( "Purging caches..." );
	$wgDatabase->delete( 'objectcache', '*', __METHOD__ );
	echo( "done.\n" );
}

function do_all_updates( $shared = false, $purge = true ) {
	global $wgNewTables, $wgNewFields, $wgRenamedTables, $wgSharedDB, $wgSharedTables, $wgDatabase, $wgDBtype, $IP;

	wfRunHooks('LoadExtensionSchemaUpdates');

	$doUser = $shared ? $wgSharedDB && in_array('user', $wgSharedTables) : !$wgSharedDB || !in_array('user', $wgSharedTables);

	if ($wgDBtype === 'postgres') {
		do_postgres_updates();
		return;
	}
	
	# Run core updates in sequence...
	global $wgMysqlUpdates;
	foreach( $wgMysqlUpdates as $params ) {
		$func = array_shift( $params );
		call_user_func_array( $func, $params );
		flush();
	}
	
	/// @fixme clean up this mess too!
	global $wgExtNewTables, $wgExtNewFields, $wgExtNewIndexes;
	# Add missing extension tables
	foreach ( $wgExtNewTables as $tableRecord ) {
		add_table( $tableRecord[0], $tableRecord[1], true );
		flush();
	}
	# Add missing extension fields
	foreach ( $wgExtNewFields as $fieldRecord ) {
		if ( $fieldRecord[0] != 'user' || $doUser ) {
			add_field( $fieldRecord[0], $fieldRecord[1], $fieldRecord[2], true );
		}
		flush();
	}
	# Add missing extension indexes
	foreach ( $wgExtNewIndexes as $fieldRecord ) {
		add_index( $fieldRecord[0], $fieldRecord[1], $fieldRecord[2], true );
		flush();
	}


	echo "Deleting old default messages (this may take a long time!)..."; flush();
	deleteDefaultMessages();
	echo "Done\n"; flush();
	
	do_stats_init(); flush();
	
	if( $purge ) {
		purge_cache();
		flush();
	}
}

function archive($name) {
	global $wgDBtype, $IP;
	switch ($wgDBtype) {
	case "postgres":
		return "$IP/maintenance/postgres/archives/$name";
	default:
		return "$IP/maintenance/archives/$name";
	}
}

function do_restrictions_update() {
	# Adding page_restrictions table, obsoleting page.page_restrictions.
	#  Migrating old restrictions to new table
	# -- Andrew Garrett, January 2007.

	global $wgDatabase;

	$name = 'page_restrictions';
	$patch = 'patch-page_restrictions.sql';
	$patch2 = 'patch-page_restrictions_sortkey.sql';

	if ( $wgDatabase->tableExists( $name ) ) {
		echo "...$name table already exists.\n";
	} else {
		echo "Creating $name table...";
		dbsource( archive($patch), $wgDatabase );
		dbsource( archive($patch2), $wgDatabase );
		echo "ok\n";

		echo "Migrating old restrictions to new table...";

		$res = $wgDatabase->select( 'page', array( 'page_id', 'page_restrictions' ), array("page_restrictions!=''", "page_restrictions!='edit=:move='"), __METHOD__ );

		$count = 0;

		while ($row = $wgDatabase->fetchObject($res) ) {
			$count = ($count + 1) % 100;

			if ($count == 0) {
				if ( function_exists( 'wfWaitForSlaves' ) ) {
					wfWaitForSlaves( 10 );
				} else {
					sleep( 1 );
				}
			}

			# Figure out what the restrictions are..
			$id = $row->page_id;
			$flatrestrictions = explode( ':', $row->page_restrictions );

			$restrictions = array ();
			foreach( $flatrestrictions as $restriction ) {
				$thisrestriction = explode( '=', $restriction, 2 );
				if( count( $thisrestriction ) == 1 ) {
					// Compatibility with old protections from before
					// separate move protection was added.
					list( $level ) = $thisrestriction;
					if( $level ) {
						$restrictions['edit'] = $level;
						$restrictions['move'] = $level;
					}
				} else {
					list( $type, $level ) = $thisrestriction;
					if( $level ) {
						$restrictions[$type] = $level;
					}
				}

			$wgDatabase->update( 'page', array ( 'page_restrictions' => ''), array( 'page_id' => $id ), __METHOD__ );

			}
			
			foreach( $restrictions as $type => $level ) {
				$wgDatabase->insert( 'page_restrictions', array ( 'pr_page' => $id,
											'pr_type' => $type,
											'pr_level' => $level,
											'pr_cascade' => 0,
											'pr_expiry' => 'infinity' ),
											__METHOD__ );
			}
		}
		print "ok\n";
	}
}

function do_category_population() {
	if( update_row_exists( 'populate category' ) ) {
		echo "...category table already populated.\n";
		return;
	}
	require_once( 'populateCategory.inc' );
	echo "Populating category table, printing progress markers.  ".
"For large databases, you\n".
"may want to hit Ctrl-C and do this manually with maintenance/\n".
"populateCategory.php.\n";
	populateCategory( '', 10, 0, true );
	echo "Done populating category table.\n";
}

function do_populate_parent_id() {
	if( update_row_exists( 'populate rev_parent_id' ) ) {
		echo "...rev_parent_id column already populated.\n";
		return;
	}
	require_once( 'populateParentId.inc' );
	
	global $wgDatabase;
	populate_rev_parent_id( $wgDatabase );
}

function update_password_format() {
	if ( update_row_exists( 'password format' ) ) {
		echo "...password hash format already changed\n";
		return;
	}

	echo "Updating password hash format...";

	global $wgDatabase, $wgPasswordSalt;
	$user = $wgDatabase->tableName( 'user' );
	if ( $wgPasswordSalt ) {
		$sql = "UPDATE $user SET user_password=CONCAT(':B:', user_id, ':', user_password) " .
			"WHERE user_password NOT LIKE ':%'";
	} else {
		$sql = "UPDATE $user SET user_password=CONCAT(':A:', user_password) " .
			"WHERE user_password NOT LIKE ':%'";
	}
	$wgDatabase->query( $sql, __METHOD__ );
	$wgDatabase->insert( 'updatelog', array( 'ul_key' => 'password format' ), __METHOD__ );

	echo "done\n";
}

function
pg_describe_table($table)
{
global	$wgDatabase, $wgDBmwschema;
	$q = <<<END
SELECT attname, attnum FROM pg_namespace, pg_class, pg_attribute
	WHERE pg_class.relnamespace = pg_namespace.oid 
	  AND attrelid=pg_class.oid AND attnum > 0
	  AND relname=%s AND nspname=%s
END;
	$res = $wgDatabase->query(sprintf($q,
			$wgDatabase->addQuotes($table),
			$wgDatabase->addQuotes($wgDBmwschema)));
	if (!$res)
		return null;

	$cols = array();
	while ($r = $wgDatabase->fetchRow($res)) {
		$cols[] = array(	
				"name" => $r[0],
				"ord" => $r[1],
			);
	}
	return $cols;
}

function
pg_describe_index($idx)
{
global	$wgDatabase, $wgDBmwschema;

	// first fetch the key (which is a list of columns ords) and
	// the table the index applies to (an oid)
	$q = <<<END
SELECT indkey, indrelid FROM pg_namespace, pg_class, pg_index
	WHERE nspname=%s
	  AND pg_class.relnamespace = pg_namespace.oid
	  AND relname=%s
	  AND indexrelid=pg_class.oid
END;
	$res = $wgDatabase->query(sprintf($q,
			$wgDatabase->addQuotes($wgDBmwschema),
			$wgDatabase->addQuotes($idx)));
	if (!$res)
		return null;
	if (!($r = $wgDatabase->fetchRow($res))) {
		$wgDatabase->freeResult($res);
		return null;
	}

	$indkey = $r[0];
	$relid = intval($r[1]);
	$indkeys = explode(" ", $indkey);
	$wgDatabase->freeResult($res);

	$colnames = array();
	foreach ($indkeys as $rid) {
		$query = <<<END
SELECT attname FROM pg_class, pg_attribute
	WHERE attrelid=$relid
	  AND attnum=%d
	  AND attrelid=pg_class.oid
END;
		$r2 = $wgDatabase->query(sprintf($query, $rid));
		if (!$r2)
			return null;
		if (!($row2 = $wgDatabase->fetchRow($r2))) {
			$wgDatabase->freeResult($r2);
			return null;
		}
		$colnames[] = $row2[0];
		$wgDatabase->freeResult($r2);
	}

	return $colnames;
}

function
pg_index_exists($table, $index)
{
global	$wgDatabase, $wgDBmwschema;
	$exists = $wgDatabase->selectField("pg_indexes", "indexname",
			array(	"indexname" => $index,
				"tablename" => $table,
				"schemaname" => $wgDBmwschema));
	return $exists === $index;
}

function
pg_fkey_deltype($fkey)
{
global	$wgDatabase, $wgDBmwschema;
	$q = <<<END
SELECT confdeltype FROM pg_constraint, pg_namespace
	WHERE connamespace=pg_namespace.oid
	  AND nspname=%s
	  AND conname=%s;
END;
	$r = $wgDatabase->query(sprintf($q,
		$wgDatabase->addQuotes($wgDBmwschema),
		$wgDatabase->addQuotes($fkey)));
	if (!($row = $wgDatabase->fetchRow($r)))
		return null;
	return $row[0];
}

function
pg_rule_def($table, $rule)
{
global	$wgDatabase, $wgDBmwschema;
	$q = <<<END
SELECT definition FROM pg_rules
	WHERE schemaname = %s
	  AND tablename = %s
	  AND rulename = %s
END;
	$r = $wgDatabase->query(sprintf($q,
			$wgDatabase->addQuotes($wgDBmwschema),
			$wgDatabase->addQuotes($table),
			$wgDatabase->addQuotes($rule)));
	$row = $wgDatabase->fetchRow($r);
	if (!$row)
		return null;
	$d = $row[0];
	$wgDatabase->freeResult($r);
	return $d;
}

function do_postgres_updates() {
	global $wgDatabase, $wgVersion, $wgDBmwschema, $wgDBts2schema, $wgShowExceptionDetails, $wgDBuser;

	## Gather version numbers in case we need them
	$version = $wgDatabase->getServerVersion(); ## long string
	$numver = $wgDatabase->numeric_version; ## X.Y e.g. 8.3

	$wgShowExceptionDetails = 1;

	# Just in case their LocalSettings.php does not have this:
	if ( !isset( $wgDBmwschema ))
		$wgDBmwschema = 'mediawiki';

	# Verify that this user is configured correctly
	$safeuser = $wgDatabase->addQuotes($wgDBuser);
	$SQL = "SELECT array_to_string(useconfig,'*') FROM pg_catalog.pg_user WHERE usename = $safeuser";
	$config = pg_fetch_result( $wgDatabase->doQuery( $SQL ), 0, 0 );
	$conf = array();
	foreach( explode( '*', $config ) as $c ) {
		list( $x,$y ) = explode( '=', $c );
		$conf[$x] = $y;
	}
	if( !array_key_exists( 'search_path', $conf ) ) {
		$search_path = '';
	}
	else {
		$search_path = $conf['search_path'];
	}
	if( strpos( $search_path, $wgDBmwschema ) === false ) {
		echo "Adding in schema \"$wgDBmwschema\" to search_path for user \"$wgDBuser\"\n";
		$search_path = "$wgDBmwschema, $search_path";
	}
	if( strpos( $search_path, $wgDBts2schema ) === false ) {
		echo "Adding in schema \"$wgDBts2schema\" to search_path for user \"$wgDBuser\"\n";
		$search_path = "$search_path, $wgDBts2schema";
	}
	if( array_key_exists( 'search_path', $conf ) === false || $search_path != $conf['search_path'] ) {
		$wgDatabase->doQuery( "ALTER USER $wgDBuser SET search_path = $search_path" );
		$wgDatabase->doQuery( "SET search_path = $search_path" );
	}
	else {
		$path = $conf['search_path'];
		echo "... search_path for user \"$wgDBuser\" looks correct ($path)\n";
	}
	$goodconf = array(
		'client_min_messages' => 'error',
		'DateStyle'           => 'ISO, YMD',
		'TimeZone'            => 'GMT'
	);
	foreach( array_keys( $goodconf ) AS $key ) {
		$value = $goodconf[$key];
		if( !array_key_exists( $key, $conf ) or $conf[$key] !== $value ) {
			echo "Setting $key to '$value' for user \"$wgDBuser\"\n";
			$wgDatabase->doQuery( "ALTER USER $wgDBuser SET $key = '$value'" );
			$wgDatabase->doQuery( "SET $key = '$value'" );
		}
		else {
			echo "... default value of \"$key\" is correctly set to \"$value\" for user \"$wgDBuser\"\n";
		}
	}

	$newsequences = array(
		"log_log_id_seq",
		"pr_id_val",
	);

	$newtables = array(
		array("category",          "patch-category.sql"),
		array("mediawiki_version", "patch-mediawiki_version.sql"),
		array("mwuser",            "patch-mwuser.sql"),
		array("pagecontent",       "patch-pagecontent.sql"),
		array("querycachetwo",     "patch-querycachetwo.sql"),
		array("page_props",        "patch-page_props.sql"),
		array("page_restrictions", "patch-page_restrictions.sql"),
		array("profiling",         "patch-profiling.sql"),
		array("protected_titles",  "patch-protected_titles.sql"),
		array("redirect",          "patch-redirect.sql"),
		array("updatelog",         "patch-updatelog.sql"),
	);

	$newcols = array(
		array("archive",       "ar_deleted",           "SMALLINT NOT NULL DEFAULT 0"),
		array("archive",       "ar_len",               "INTEGER"),
		array("archive",       "ar_page_id",           "INTEGER"),
		array("archive",       "ar_parent_id",         "INTEGER"),
		array("image",         "img_sha1",             "TEXT NOT NULL DEFAULT ''"),
		array("ipblocks",      "ipb_anon_only",        "CHAR NOT NULL DEFAULT '0'"),
		array("ipblocks",      "ipb_by_text",          "TEXT NOT NULL DEFAULT ''"),
		array("ipblocks",      "ipb_block_email",      "CHAR NOT NULL DEFAULT '0'"),
		array("ipblocks",      "ipb_create_account",   "CHAR NOT NULL DEFAULT '1'"),
		array("ipblocks",      "ipb_deleted",          "SMALLINT NOT NULL DEFAULT 0"),
		array("ipblocks",      "ipb_enable_autoblock", "CHAR NOT NULL DEFAULT '1'"),
		array("filearchive",   "fa_deleted",           "SMALLINT NOT NULL DEFAULT 0"),
		array("logging",       "log_deleted",          "SMALLINT NOT NULL DEFAULT 0"),
		array("logging",       "log_id",               "INTEGER NOT NULL PRIMARY KEY DEFAULT nextval('log_log_id_seq')"),
		array("logging",       "log_params",           "TEXT"),
		array("mwuser",        "user_editcount",       "INTEGER"),
		array("mwuser",        "user_newpass_time",    "TIMESTAMPTZ"),
		array("oldimage",      "oi_deleted",           "SMALLINT NOT NULL DEFAULT 0"),
		array("oldimage",      "oi_metadata",          "BYTEA NOT NULL DEFAULT ''"),
		array("oldimage",      "oi_media_type",        "TEXT"),
		array("oldimage",      "oi_major_mime",        "TEXT NOT NULL DEFAULT 'unknown'"),
		array("oldimage",      "oi_minor_mime",        "TEXT NOT NULL DEFAULT 'unknown'"),
		array("oldimage",      "oi_sha1",              "TEXT NOT NULL DEFAULT ''"),
		array("page_restrictions", "pr_id",            "INTEGER NOT NULL UNIQUE DEFAULT nextval('pr_id_val')"),
		array("profiling",     "pf_memory",            "NUMERIC(18,10) NOT NULL DEFAULT 0"),
		array("recentchanges", "rc_deleted",           "SMALLINT NOT NULL DEFAULT 0"),
		array("recentchanges", "rc_log_action",        "TEXT"),
        array("recentchanges", "rc_log_type",          "TEXT"),
        array("recentchanges", "rc_logid",             "INTEGER NOT NULL DEFAULT 0"),
		array("recentchanges", "rc_new_len",           "INTEGER"),
		array("recentchanges", "rc_old_len",           "INTEGER"),
		array("recentchanges", "rc_params",            "TEXT"),
		array("revision",      "rev_len",              "INTEGER"),
		array("revision",      "rev_deleted",          "SMALLINT NOT NULL DEFAULT 0"),
		array("user_newtalk",  "user_last_timestamp",  "TIMESTAMPTZ"),
	);


	# table, column, desired type, USING clause if needed (with new default if needed)
	$typechanges = array(
		array("archive",      "ar_deleted",      "smallint", ""),
		array("archive",      "ar_minor_edit",   "smallint", "ar_minor_edit::smallint DEFAULT 0"),
		array("filearchive",  "fa_deleted",      "smallint", ""),
		array("filearchive",  "fa_height",       "integer",  ""),
		array("filearchive",  "fa_metadata",     "bytea",    "decode(fa_metadata,'escape')"),
		array("filearchive",  "fa_size",         "integer",  ""),
		array("filearchive",  "fa_width",        "integer",  ""),
		array("filearchive",  "fa_storage_group","text",     ""),
		array("filearchive",  "fa_storage_key",  "text",     ""),
		array("image",        "img_metadata",    "bytea",    "decode(img_metadata,'escape')"),
		array("image",        "img_size",        "integer",  ""),
		array("image",        "img_width",       "integer",  ""),
		array("image",        "img_height",      "integer",  ""),
		array("interwiki",    "iw_local",        "smallint", "iw_local::smallint DEFAULT 0"),
		array("interwiki",    "iw_trans",        "smallint", "iw_trans::smallint DEFAULT 0"),
		array("ipblocks",     "ipb_auto",        "smallint", "ipb_auto::smallint DEFAULT 0"),
		array("ipblocks",     "ipb_anon_only",   "smallint", "CASE WHEN ipb_anon_only=' ' THEN 0 ELSE ipb_anon_only::smallint END DEFAULT 0"),
		array("ipblocks",     "ipb_create_account", "smallint", "CASE WHEN ipb_create_account=' ' THEN 0 ELSE ipb_create_account::smallint END DEFAULT 1"),
		array("ipblocks",     "ipb_enable_autoblock", "smallint", "CASE WHEN ipb_enable_autoblock=' ' THEN 0 ELSE ipb_enable_autoblock::smallint END DEFAULT 1"),
		array("ipblocks",     "ipb_block_email", "smallint", "CASE WHEN ipb_block_email=' ' THEN 0 ELSE ipb_block_email::smallint END DEFAULT 0"),
		array("ipblocks",     "ipb_address",     "text",     "ipb_address::text"),
		array("ipblocks",     "ipb_deleted",     "smallint", "ipb_deleted::smallint"),
		array("math",         "math_inputhash",  "bytea",    "decode(math_inputhash,'escape')"),
		array("math",         "math_outputhash", "bytea",    "decode(math_outputhash,'escape')"),
		array("mwuser",       "user_token",      "text",     ""),
		array("mwuser",       "user_email_token","text",     ""),
		array("objectcache",  "keyname",         "text",     ""),
		array("oldimage",     "oi_height",       "integer",  ""),
		array("oldimage",     "oi_size",         "integer",  ""),
		array("oldimage",     "oi_width",        "integer",  ""),
		array("page",         "page_is_redirect","smallint", "page_is_redirect::smallint DEFAULT 0"),
		array("page",         "page_is_new",     "smallint", "page_is_new::smallint DEFAULT 0"),
		array("querycache",   "qc_value",        "integer",  ""),
		array("querycachetwo","qcc_value",       "integer",  ""),
		array("recentchanges","rc_bot",          "smallint", "rc_bot::smallint DEFAULT 0"),
		array("recentchanges","rc_deleted",      "smallint", ""),
		array("recentchanges","rc_minor",        "smallint", "rc_minor::smallint DEFAULT 0"),
		array("recentchanges","rc_new",          "smallint", "rc_new::smallint DEFAULT 0"),
		array("recentchanges","rc_type",         "smallint", "rc_type::smallint DEFAULT 0"),
		array("recentchanges","rc_patrolled",    "smallint", "rc_patrolled::smallint DEFAULT 0"),
		array("revision",     "rev_deleted",     "smallint", "rev_deleted::smallint DEFAULT 0"),
		array("revision",     "rev_minor_edit",  "smallint", "rev_minor_edit::smallint DEFAULT 0"),
		array("templatelinks","tl_namespace",    "smallint", "tl_namespace::smallint"),
		array("user_newtalk", "user_ip",         "text",     "host(user_ip)"),
	);

	$newindexes = array(
		array("archive",       "archive_user_text", "(ar_user_text)"),
		array("image",         "img_sha1",          "(img_sha1)"),
		array("oldimage",      "oi_sha1",           "(oi_sha1)"),
		array("revision",      "rev_text_id_idx",   "(rev_text_id)"),
	);

	$newrules = array(
	);

	foreach ($newsequences as $ns) {
		if ($wgDatabase->sequenceExists($ns)) {
			echo "... sequence \"$ns\" already exists\n";
			continue;
		}

		echo "Creating sequence \"$ns\"\n";
		$wgDatabase->query("CREATE SEQUENCE $ns");
	}

	foreach ($newtables as $nt) {
		if ($wgDatabase->tableExists($nt[0])) {
			echo "... table \"$nt[0]\" already exists\n";
			continue;
		}

		echo "Creating table \"$nt[0]\"\n";
		dbsource(archive($nt[1]));
	}

	## Needed before newcols
	if ($wgDatabase->tableExists("archive2")) {
		echo "Converting \"archive2\" back to normal archive table\n";
		if ($wgDatabase->ruleExists("archive", "archive_insert")) {
			echo "Dropping rule \"archive_insert\"\n";
			$wgDatabase->query("DROP RULE archive_insert ON archive");
		}
		if ($wgDatabase->ruleExists("archive", "archive_delete")) {
			echo "Dropping rule \"archive_delete\"\n";
			$wgDatabase->query("DROP RULE archive_delete ON archive");
		}
		dbsource(archive("patch-remove-archive2.sql"));
	}
	else
		echo "... obsolete table \"archive2\" does not exist\n";

	foreach ($newcols as $nc) {
		$fi = $wgDatabase->fieldInfo($nc[0], $nc[1]);
		if (!is_null($fi)) {
			echo "... column \"$nc[0].$nc[1]\" already exists\n";
			continue;
		}

		echo "Adding column \"$nc[0].$nc[1]\"\n";
		$wgDatabase->query("ALTER TABLE $nc[0] ADD $nc[1] $nc[2]");
	}

	foreach ($typechanges as $tc) {
		$fi = $wgDatabase->fieldInfo($tc[0], $tc[1]);
		if (is_null($fi)) {
			echo "... error: expected column $tc[0].$tc[1] to exist\n";
			exit(1);
		}

		if ($fi->type() === $tc[2])
			echo "... column \"$tc[0].$tc[1]\" is already of type \"$tc[2]\"\n";
		else {
			echo "Changing column type of \"$tc[0].$tc[1]\" from \"{$fi->type()}\" to \"$tc[2]\"\n";
			$sql = "ALTER TABLE $tc[0] ALTER $tc[1] TYPE $tc[2]";
			if (strlen($tc[3])) {
				$default = array();
				if (preg_match( '/DEFAULT (.+)/', $tc[3], $default)) {
					$sqldef = "ALTER TABLE $tc[0] ALTER $tc[1] SET DEFAULT $default[1]";
					$wgDatabase->query($sqldef);
					$tc[3] = preg_replace( '/\s*DEFAULT .+/', '', $tc[3]);
				}
				$sql .= " USING $tc[3]";
			}
			$sql .= ";\nCOMMIT;\n";
			$wgDatabase->query($sql);
		}
	}

	if ($wgDatabase->fieldInfo('oldimage','oi_deleted')->type() !== 'smallint') {
		echo "Changing \"oldimage.oi_deleted\" to type \"smallint\"\n";
		$wgDatabase->query( "ALTER TABLE oldimage ALTER oi_deleted DROP DEFAULT" );
		$wgDatabase->query( "ALTER TABLE oldimage ALTER oi_deleted TYPE SMALLINT USING (oi_deleted::smallint)" );
		$wgDatabase->query( "ALTER TABLE oldimage ALTER oi_deleted SET DEFAULT 0" );
	}
	else
		echo "... column \"oldimage.oi_deleted\" is already of type \"smallint\"\n";


	foreach ($newindexes as $ni) {
		if (pg_index_exists($ni[0], $ni[1])) {
			echo "... index \"$ni[1]\" on table \"$ni[0]\" already exists\n";
			continue;
		}
		echo "Creating index \"$ni[1]\" on table \"$ni[0]\" $ni[2]\n";
		$wgDatabase->query( "CREATE INDEX $ni[1] ON $ni[0] $ni[2]" );
	}

	foreach ($newrules as $nr) {
		if ($wgDatabase->ruleExists($nr[0], $nr[1])) {
			echo "... rule \"$nr[1]\" on table \"$nr[0]\" already exists\n";
			continue;
		}
		echo "Adding rule \"$nr[1]\" to table \"$nr[0]\"\n";
		dbsource(archive($nr[2]));
	}

	if ($wgDatabase->hasConstraint("oldimage_oi_name_fkey")) {
		echo "Making foriegn key on table \"oldimage\" (to image) a cascade delete\n";
		$wgDatabase->query( "ALTER TABLE oldimage DROP CONSTRAINT oldimage_oi_name_fkey" );
		$wgDatabase->query( "ALTER TABLE oldimage ADD CONSTRAINT oldimage_oi_name_fkey_cascade ".
			"FOREIGN KEY (oi_name) REFERENCES image(img_name) ON DELETE CASCADE" );
	}
	else
		echo "... table \"oldimage\" has correct cascade delete foreign key to image\n";

	if (!$wgDatabase->triggerExists("page", "page_deleted")) {
		echo "Adding function and trigger \"page_deleted\" to table \"page\"\n";
		dbsource(archive('patch-page_deleted.sql'));
	}
	else
		echo "... table \"page\" has \"page_deleted\" trigger\n";

	$fi = $wgDatabase->fieldInfo("recentchanges", "rc_cur_id");
	if (!$fi->nullable()) {
		echo "Removing NOT NULL constraint from \"recentchanges.rc_cur_id\"\n";
		dbsource(archive('patch-rc_cur_id-not-null.sql'));
	}
	else
		echo "... column \"recentchanges.rc_cur_id\" has a NOT NULL constraint\n";

	$pu = pg_describe_index("pagelink_unique");
	if (!is_null($pu) && ($pu[0] != "pl_from" || $pu[1] != "pl_namespace" || $pu[2] != "pl_title")) {
		echo "Dropping obsolete version of index \"pagelink_unique index\"\n";
		$wgDatabase->query("DROP INDEX pagelink_unique");
		$pu = null;
	}
	else
		echo "... obsolete version of index \"pagelink_unique index\" does not exist\n";

	if (is_null($pu)) {
		echo "Creating index \"pagelink_unique index\"\n";
		$wgDatabase->query("CREATE UNIQUE INDEX pagelink_unique ON pagelinks (pl_from,pl_namespace,pl_title)");
	}
	else
		echo "... index \"pagelink_unique_index\" already exists\n";

	if (pg_fkey_deltype("revision_rev_user_fkey") == 'r') {
		echo "... constraint \"revision_rev_user_fkey\" is ON DELETE RESTRICT\n";
	}
	else {
		echo "Changing constraint \"revision_rev_user_fkey\" to ON DELETE RESTRICT\n";
		dbsource(archive('patch-revision_rev_user_fkey.sql'));
	}

	# Fix ipb_address index
	if (pg_index_exists('ipblocks', 'ipb_address_unique' )) {
		echo "... have ipb_address_unique\n";
	} else {
		echo "Adding ipb_address_unique index\n";
		dbsource(archive('patch-ipb_address_unique.sql'));
	}

	global $wgExtNewTables, $wgExtPGNewFields, $wgExtNewIndexes;
	# Add missing extension tables
	foreach ( $wgExtNewTables as $nt ) {
		if ($wgDatabase->tableExists($nt[0])) {
			echo "... table \"$nt[0]\" already exists\n";
			continue;
		}
		echo "Creating table \"$nt[0]\"\n";
		dbsource($nt[1]);
	}
	# Add missing extension fields
	foreach ( $wgExtPGNewFields as $nc ) {
		$fi = $wgDatabase->fieldInfo($nc[0], $nc[1]);
		if (!is_null($fi)) {
			echo "... column \"$nc[0].$nc[1]\" already exists\n";
			continue;
		}
		echo "Adding column \"$nc[0].$nc[1]\"\n";
		$wgDatabase->query( "ALTER TABLE $nc[0] ADD $nc[1] $nc[2]" );
	}
	# Add missing extension indexes
	foreach ( $wgExtNewIndexes as $ni ) {
		if (pg_index_exists($ni[0], $ni[1])) {
			echo "... index \"$ni[1]\" on table \"$ni[0]\" already exists\n";
			continue;
		}
		echo "Creating index \"$ni[1]\" on table \"$ni[0]\"\n";
		dbsource($ni[2]);
	}

	# Tweak the page_title tsearch2 trigger to filter out slashes
	# This is create or replace, so harmless to call if not needed
	dbsource(archive('patch-ts2pagetitle.sql'));

	## If the server is 8.3 or higher, rewrite teh tsearch2 triggers
	## in case they have the old 'default' versions
	if ( $numver >= 8.3 )
		dbsource(archive('patch-tsearch2funcs.sql'));

	return;
}

