<?php
# Copyright (C) 2004 Brion Vibber <brion@pobox.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * If on the old non-unique indexes, check the cur table for duplicate
 * entries and remove them...
 *
 * @file
 * @ingroup Maintenance
 */

function fixDupes( $fixthem = false) {
	$dbw = wfGetDB( DB_MASTER );
	$cur = $dbw->tableName( 'cur' );
	$old = $dbw->tableName( 'old' );
	$dbw->query( "LOCK TABLES $cur WRITE, $old WRITE" );
	echo "Checking for duplicate cur table entries... (this may take a while on a large wiki)\n";
	$res = $dbw->query( <<<END
SELECT cur_namespace,cur_title,count(*) as c,min(cur_id) as id
  FROM $cur
 GROUP BY cur_namespace,cur_title
HAVING c > 1
END
	);
	$n = $dbw->numRows( $res );
	echo "Found $n titles with duplicate entries.\n";
	if( $n > 0 ) {
		if( $fixthem ) {
			echo "Correcting...\n";
		} else {
			echo "Just a demo...\n";
		}
		while( $row = $dbw->fetchObject( $res ) ) {
			$ns = intval( $row->cur_namespace );
			$title = $dbw->addQuotes( $row->cur_title );

			# Get the first responding ID; that'll be the one we keep.
			$id = $dbw->selectField( 'cur', 'cur_id', array(
				'cur_namespace' => $row->cur_namespace,
				'cur_title'     => $row->cur_title ) );

			echo "$ns:$row->cur_title (canonical ID $id)\n";
			if( $id != $row->id ) {
				echo "  ** minimum ID $row->id; ";
				$timeMin = $dbw->selectField( 'cur', 'cur_timestamp', array(
					'cur_id' => $row->id ) );
				$timeFirst = $dbw->selectField( 'cur', 'cur_timestamp', array(
					'cur_id' => $id ) );
				if( $timeMin == $timeFirst ) {
					echo "timestamps match at $timeFirst; ok\n";
				} else {
					echo "timestamps don't match! min: $timeMin, first: $timeFirst; ";
					if( $timeMin > $timeFirst ) {
						$id = $row->id;
						echo "keeping minimum: $id\n";
					} else {
						echo "keeping first: $id\n";
					}
				}
			}

			if( $fixthem ) {
				$dbw->query( <<<END
INSERT
  INTO $old
      (old_namespace, old_title,      old_text,
       old_comment,   old_user,       old_user_text,
       old_timestamp, old_minor_edit, old_flags,
       inverse_timestamp)
SELECT cur_namespace, cur_title,      cur_text,
       cur_comment,   cur_user,       cur_user_text,
       cur_timestamp, cur_minor_edit, '',
       inverse_timestamp
  FROM $cur
 WHERE cur_namespace=$ns
   AND cur_title=$title
   AND cur_id != $id
END
				);
				$dbw->query( <<<END
DELETE
  FROM $cur
 WHERE cur_namespace=$ns
   AND cur_title=$title
   AND cur_id != $id
END
				);
			}
		}
	}
	$dbw->query( 'UNLOCK TABLES' );
	if( $fixthem ) {
		echo "Done.\n";
	} else {
		echo "Run again with --fix option to delete the duplicates.\n";
	}
}

function checkDupes( $fixthem = false, $indexonly = false ) {
	$dbw = wfGetDB( DB_MASTER );
	if( $dbw->indexExists( 'cur', 'name_title' ) &&
	    $dbw->indexUnique( 'cur', 'name_title' ) ) {
		echo wfWikiID().": cur table has the current unique index; no duplicate entries.\n";
	} elseif( $dbw->indexExists( 'cur', 'name_title_dup_prevention' ) ) {
		echo wfWikiID().": cur table has a temporary name_title_dup_prevention unique index; no duplicate entries.\n";
	} else {
		echo wfWikiID().": cur table has the old non-unique index and may have duplicate entries.\n";
		if( !$indexonly ) {
			fixDupes( $fixthem );
		}
	}
}
