<?php
/**
 * @file
 * @deprecated
 * @ingroup MaintenanceArchive
 */

/** */
print "This script is obsolete!";
print "It is retained in the source here in case some of its
code might be useful for ad-hoc conversion tasks, but it is
not maintained and probably won't even work as is.";
exit();

# Convert watchlists to new format

global $IP;
require_once( "../LocalSettings.php" );
require_once( "$IP/Setup.php" );

$wgTitle = Title::newFromText( "Rebuild links script" );
set_time_limit(0);

$wgDBuser			= "wikiadmin";
$wgDBpassword		= $wgDBadminpassword;

$sql = "DROP TABLE IF EXISTS watchlist";
wfQuery( $sql, DB_MASTER );
$sql = "CREATE TABLE watchlist (
  wl_user int(5) unsigned NOT NULL,
  wl_page int(8) unsigned NOT NULL,
  UNIQUE KEY (wl_user, wl_page)
) ENGINE=MyISAM PACK_KEYS=1";
wfQuery( $sql, DB_MASTER );

$lc = new LinkCache;

# Now, convert!
$sql = "SELECT user_id,user_watch FROM user";
$res = wfQuery( $sql, DB_SLAVE );
$nu = wfNumRows( $res );
$sql = "INSERT into watchlist (wl_user,wl_page) VALUES ";
$i = $n = 0;
while( $row = wfFetchObject( $res ) ) {
	$list = explode( "\n", $row->user_watch );
	$bits = array();
	foreach( $list as $title ) {
		if( $id = $lc->addLink( $title ) and ! $bits[$id]++) {
			$sql .= ($i++ ? "," : "") . "({$row->user_id},{$id})";
		}
	}
	if( ($n++ % 100) == 0 ) echo "$n of $nu users done...\n";
}
echo "$n users done.\n";
if( $i ) {
	wfQuery( $sql, DB_MASTER );
}


# Add index
# is this necessary?
$sql = "ALTER TABLE watchlist
  ADD INDEX wl_user (wl_user),
  ADD INDEX wl_page (wl_page)";
#wfQuery( $sql, DB_MASTER );


