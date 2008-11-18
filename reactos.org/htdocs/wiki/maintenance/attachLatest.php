<?php
// quick hackjob to fix damages imports on wikisource
// page records have page_latest wrong

/**
 * Copyright (C) 2005 Brion Vibber <brion@pobox.com>
 * http://www.mediawiki.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @file
 * @ingroup Maintenance
 */

require_once( 'commandLine.inc' );

$fixit = isset( $options['fix'] );
$fname = 'attachLatest';

echo "Looking for pages with page_latest set to 0...\n";
$dbw = wfGetDB( DB_MASTER );
$result = $dbw->select( 'page',
	array( 'page_id', 'page_namespace', 'page_title' ),
	array( 'page_latest' => 0 ),
	$fname );

$n = 0;
while( $row = $dbw->fetchObject( $result ) ) {
	$pageId = intval( $row->page_id );
	$title = Title::makeTitle( $row->page_namespace, $row->page_title );
	$name = $title->getPrefixedText();
	$latestTime = $dbw->selectField( 'revision',
		'MAX(rev_timestamp)',
		array( 'rev_page' => $pageId ),
		$fname );
	if( !$latestTime ) {
		echo wfWikiID()." $pageId [[$name]] can't find latest rev time?!\n";
		continue;
	}

	$revision = Revision::loadFromTimestamp( $dbw, $title, $latestTime );
	if( is_null( $revision ) ) {
		echo wfWikiID()." $pageId [[$name]] latest time $latestTime, can't find revision id\n";
		continue;
	}
	$id = $revision->getId();
	echo wfWikiID()." $pageId [[$name]] latest time $latestTime, rev id $id\n";
	if( $fixit ) {
		$article = new Article( $title );
		$article->updateRevisionOn( $dbw, $revision );
	}
	$n++;
}
$dbw->freeResult( $result );
echo "Done! Processed $n pages.\n";
if( !$fixit ) {
	echo "This was a dry run; rerun with --fix to update page_latest.\n";
}


