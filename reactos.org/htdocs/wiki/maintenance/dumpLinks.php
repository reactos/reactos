<?php
/**
 * Copyright (C) 2005 Brion Vibber <brion@pobox.com>
 * http://www.mediawiki.org/
 *
 * Quick demo hack to generate a plaintext link dump,
 * per the proposed wiki link database standard:
 * http://www.usemod.com/cgi-bin/mb.pl?LinkDatabase
 *
 * Includes all (live and broken) intra-wiki links.
 * Does not include interwiki or URL links.
 * Dumps ASCII text to stdout; command-line.
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
 * @ingroup Mainatenance
 */

require_once 'commandLine.inc';

$dbr = wfGetDB( DB_SLAVE );
$result = $dbr->select( array( 'pagelinks', 'page' ),
	array(
		'page_id',
		'page_namespace',
		'page_title',
		'pl_namespace',
		'pl_title' ),
	array( 'page_id=pl_from' ),
	'dumpLinks',
	array( 'ORDER BY' => 'page_id' ) );

$lastPage = null;
while( $row = $dbr->fetchObject( $result ) ) {
	if( $lastPage != $row->page_id ) {
		if( isset( $lastPage ) ) {
			print "\n";
		}
		$page = Title::makeTitle( $row->page_namespace, $row->page_title );
		print $page->getPrefixedUrl();
		$lastPage = $row->page_id;
	}
	$link = Title::makeTitle( $row->pl_namespace, $row->pl_title );
	print " " . $link->getPrefixedUrl();
}
if( isset( $lastPage ) )
	print "\n";


