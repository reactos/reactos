<?php
# Copyright (C) 2005 Brion Vibber <brion@pobox.com>
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
 * Look for 'orphan' revisions hooked to pages which don't exist
 * And 'childless' pages with no revisions.
 * Then, kill the poor widows and orphans.
 * Man this is depressing.
 *
 * @file
 * @author <brion@pobox.com>
 * @ingroup Maintenance
 */

$options = array( 'fix' );

/** */
require_once( 'commandLine.inc' );
$wgTitle = Title::newFromText( 'Orphan revision cleanup script' );

checkOrphans( isset( $options['fix'] ) );
checkSeparation( isset( $options['fix'] ) );
#checkWidows( isset( $options['fix'] ) );

# ------

function checkOrphans( $fix ) {
	$dbw = wfGetDB( DB_MASTER );
	$page = $dbw->tableName( 'page' );
	$revision = $dbw->tableName( 'revision' );

	if( $fix ) {
		$dbw->query( "LOCK TABLES $page WRITE, $revision WRITE" );
	}

	echo "Checking for orphan revision table entries... (this may take a while on a large wiki)\n";
	$result = $dbw->query( "
		SELECT *
		FROM $revision LEFT OUTER JOIN $page ON rev_page=page_id
		WHERE page_id IS NULL
	");
	$orphans = $dbw->numRows( $result );
	if( $orphans > 0 ) {
		global $wgContLang;
		echo "$orphans orphan revisions...\n";
		printf( "%10s %10s %14s %20s %s\n", 'rev_id', 'rev_page', 'rev_timestamp', 'rev_user_text', 'rev_comment' );
		while( $row = $dbw->fetchObject( $result ) ) {
			$comment = ( $row->rev_comment == '' )
				? ''
				: '(' . $wgContLang->truncate( $row->rev_comment, 40, '...' ) . ')';
			printf( "%10d %10d %14s %20s %s\n",
				$row->rev_id,
				$row->rev_page,
				$row->rev_timestamp,
				$wgContLang->truncate( $row->rev_user_text, 17, '...' ),
				$comment );
			if( $fix ) {
				$dbw->delete( 'revision', array( 'rev_id' => $row->rev_id ) );
			}
		}
		if( !$fix ) {
			echo "Run again with --fix to remove these entries automatically.\n";
		}
	} else {
		echo "No orphans! Yay!\n";
	}

	if( $fix ) {
		$dbw->query( "UNLOCK TABLES" );
	}
}

/**
 * @todo DON'T USE THIS YET! It will remove entries which have children,
 *       but which aren't properly attached (eg if page_latest is bogus
 *       but valid revisions do exist)
 */
function checkWidows( $fix ) {
	$dbw = wfGetDB( DB_MASTER );
	$page = $dbw->tableName( 'page' );
	$revision = $dbw->tableName( 'revision' );

	if( $fix ) {
		$dbw->query( "LOCK TABLES $page WRITE, $revision WRITE" );
	}

	echo "\nChecking for childless page table entries... (this may take a while on a large wiki)\n";
	$result = $dbw->query( "
		SELECT *
		FROM $page LEFT OUTER JOIN $revision ON page_latest=rev_id
		WHERE rev_id IS NULL
	");
	$widows = $dbw->numRows( $result );
	if( $widows > 0 ) {
		global $wgContLang;
		echo "$widows childless pages...\n";
		printf( "%10s %11s %2s %s\n", 'page_id', 'page_latest', 'ns', 'page_title' );
		while( $row = $dbw->fetchObject( $result ) ) {
			printf( "%10d %11d %2d %s\n",
				$row->page_id,
				$row->page_latest,
				$row->page_namespace,
				$row->page_title );
			if( $fix ) {
				$dbw->delete( 'page', array( 'page_id' => $row->page_id ) );
			}
		}
		if( !$fix ) {
			echo "Run again with --fix to remove these entries automatically.\n";
		}
	} else {
		echo "No childless pages! Yay!\n";
	}

	if( $fix ) {
		$dbw->query( "UNLOCK TABLES" );
	}
}


function checkSeparation( $fix ) {
	$dbw = wfGetDB( DB_MASTER );
	$page     = $dbw->tableName( 'page' );
	$revision = $dbw->tableName( 'revision' );
	$text     = $dbw->tableName( 'text' );

	if( $fix ) {
		$dbw->query( "LOCK TABLES $page WRITE, $revision WRITE, $text WRITE" );
	}

	echo "\nChecking for pages whose page_latest links are incorrect... (this may take a while on a large wiki)\n";
	$result = $dbw->query( "
		SELECT *
		FROM $page LEFT OUTER JOIN $revision ON page_latest=rev_id
	");
	$found = 0;
	while( $row = $dbw->fetchObject( $result ) ) {
		$result2 = $dbw->query( "
			SELECT MAX(rev_timestamp) as max_timestamp
			FROM $revision
			WHERE rev_page=$row->page_id
		" );
		$row2 = $dbw->fetchObject( $result2 );
		$dbw->freeResult( $result2 );
		if( $row2 ) {
			if( $row->rev_timestamp != $row2->max_timestamp ) {
				if( $found == 0 ) {
					printf( "%10s %10s %14s %14s\n",
						'page_id', 'rev_id', 'timestamp', 'max timestamp' );
				}
				++$found;
				printf( "%10d %10d %14s %14s\n",
					$row->page_id,
					$row->page_latest,
					$row->rev_timestamp,
					$row2->max_timestamp );
				if( $fix ) {
					# ...
					$maxId = $dbw->selectField(
						'revision',
						'rev_id',
						array(
							'rev_page'      => $row->page_id,
							'rev_timestamp' => $row2->max_timestamp ) );
					echo "... updating to revision $maxId\n";
					$maxRev = Revision::newFromId( $maxId );
					$title = Title::makeTitle( $row->page_namespace, $row->page_title );
					$article = new Article( $title );
					$article->updateRevisionOn( $dbw, $maxRev );
				}
			}
		} else {
			echo "wtf\n";
		}
	}

	if( $found ) {
		echo "Found $found pages with incorrect latest revision.\n";
	} else {
		echo "No pages with incorrect latest revision. Yay!\n";
	}
	if( !$fix && $found > 0 ) {
		echo "Run again with --fix to remove these entries automatically.\n";
	}

	if( $fix ) {
		$dbw->query( "UNLOCK TABLES" );
	}
}

