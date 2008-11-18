<?php

/*
 * Makes the required database updates for rev_parent_id
 * to be of any use. It can be used for some simple tracking
 * and to find new page edits by users.
 */

require_once 'commandLine.inc';
require_once 'populateParentId.inc';
	
$db =& wfGetDB( DB_MASTER );
if ( !$db->tableExists( 'revision' ) ) {
	echo "revision table does not exist\n";
	exit( 1 );
}

populate_rev_parent_id( $db );
