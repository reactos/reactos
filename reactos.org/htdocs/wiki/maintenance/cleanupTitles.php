<?php
/*
 * Script to clean up broken, unparseable titles.
 *
 * Usage: php cleanupTitles.php [--fix]
 * Options:
 *   --fix  Actually clean up titles; otherwise just checks for them
 *
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
 * @author Brion Vibber <brion at pobox.com>
 * @ingroup Maintenance
 */

require_once( 'commandLine.inc' );
require_once( 'cleanupTable.inc' );

/**
 * @ingroup Maintenance
 */
class TitleCleanup extends TableCleanup {
	function __construct( $dryrun = false ) {
		parent::__construct( 'page', $dryrun );
	}

	function processPage( $row ) {
		$current = Title::makeTitle( $row->page_namespace, $row->page_title );
		$display = $current->getPrefixedText();

		$verified = UtfNormal::cleanUp( $display );

		$title = Title::newFromText( $verified );

		if( is_null( $title ) ) {
			$this->log( "page $row->page_id ($display) is illegal." );
			$this->moveIllegalPage( $row );
			return $this->progress( 1 );
		}

		if( !$title->equals( $current ) ) {
			$this->log( "page $row->page_id ($display) doesn't match self." );
			$this->moveInconsistentPage( $row, $title );
			return $this->progress( 1 );
		}

		$this->progress( 0 );
	}

	function moveIllegalPage( $row ) {
		$legal = 'A-Za-z0-9_/\\\\-';
		$legalized = preg_replace_callback( "!([^$legal])!",
			array( &$this, 'hexChar' ),
			$row->page_title );
		if( $legalized == '.' ) $legalized = '(dot)';
		if( $legalized == '_' ) $legalized = '(space)';
		$legalized = 'Broken/' . $legalized;

		$title = Title::newFromText( $legalized );
		if( is_null( $title ) ) {
			$clean = 'Broken/id:' . $row->page_id;
			$this->log( "Couldn't legalize; form '$legalized' still invalid; using '$clean'" );
			$title = Title::newFromText( $clean );
		} elseif( $title->exists() ) {
			$clean = 'Broken/id:' . $row->page_id;
			$this->log( "Legalized for '$legalized' exists; using '$clean'" );
			$title = Title::newFromText( $clean );
		}

		$dest = $title->getDBkey();
		if( $this->dryrun ) {
			$this->log( "DRY RUN: would rename $row->page_id ($row->page_namespace,'$row->page_title') to ($row->page_namespace,'$dest')" );
		} else {
			$this->log( "renaming $row->page_id ($row->page_namespace,'$row->page_title') to ($row->page_namespace,'$dest')" );
			$dbw = wfGetDB( DB_MASTER );
			$dbw->update( 'page',
				array( 'page_title' => $dest ),
				array( 'page_id' => $row->page_id ),
				'cleanupTitles::moveInconsistentPage' );
		}
	}

	function moveInconsistentPage( $row, $title ) {
		if( $title->exists() || $title->getInterwiki() ) {
			if( $title->getInterwiki() ) {
				$prior = $title->getPrefixedDbKey();
			} else {
				$prior = $title->getDBkey();
			}
			$clean = 'Broken/' . $prior;
			$verified = Title::makeTitleSafe( $row->page_namespace, $clean );
			if( $verified->exists() ) {
				$blah = "Broken/id:" . $row->page_id;
				$this->log( "Couldn't legalize; form '$clean' exists; using '$blah'" );
				$verified = Title::makeTitleSafe( $row->page_namespace, $blah );
			}
			$title = $verified;
		}
		if( is_null( $title ) ) {
			wfDie( "Something awry; empty title.\n" );
		}
		$ns = $title->getNamespace();
		$dest = $title->getDBkey();
		if( $this->dryrun ) {
			$this->log( "DRY RUN: would rename $row->page_id ($row->page_namespace,'$row->page_title') to ($row->page_namespace,'$dest')" );
		} else {
			$this->log( "renaming $row->page_id ($row->page_namespace,'$row->page_title') to ($ns,'$dest')" );
			$dbw = wfGetDB( DB_MASTER );
			$dbw->update( 'page',
				array(
					'page_namespace' => $ns,
					'page_title' => $dest
				),
				array( 'page_id' => $row->page_id ),
				'cleanupTitles::moveInconsistentPage' );
			$linkCache = LinkCache::singleton();
			$linkCache->clear();
		}
	}
}

$wgUser->setName( 'Conversion script' );
$caps = new TitleCleanup( !isset( $options['fix'] ) );
$caps->cleanup();


