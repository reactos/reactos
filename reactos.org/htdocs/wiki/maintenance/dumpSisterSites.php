<?php
/**
 * Quickie page name dump script for SisterSites usage.
 * http://www.eekim.com/cgi-bin/wiki.pl?SisterSites
 *
 * Copyright (C) 2006 Brion Vibber <brion@pobox.com>
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
 * @ingroup SpecialPage
 */

require_once( 'commandLine.inc' );

$dbr = wfGetDB( DB_SLAVE );
$dbr->bufferResults( false );
$result = $dbr->select( 'page',
	array( 'page_namespace', 'page_title' ),
	array(
		'page_namespace'   => NS_MAIN,
		'page_is_redirect' => 0,
	),
	'dumpSisterSites' );

while( $row = $dbr->fetchObject( $result ) ) {
	$title = Title::makeTitle( $row->page_namespace, $row->page_title );
	$url = $title->getFullUrl();
	$text = $title->getPrefixedText();
	echo "$url $text\n";
}

$dbr->freeResult( $result );


