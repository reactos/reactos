<?php
/*
 * Script to clean up broken, unparseable upload filenames.
 *
 * Usage: php cleanupImages.php [--fix]
 * Options:
 *   --fix  Actually clean up titles; otherwise just checks for them
 *
 * Copyright (C) 2005-2006 Brion Vibber <brion@pobox.com>
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
class ImageCleanup extends TableCleanup {
	function __construct( $dryrun = false ) {
		parent::__construct( 'image', $dryrun );
	}

	function processPage( $row ) {
		global $wgContLang;

		$source = $row->img_name;
		if( $source == '' ) {
			// Ye olde empty rows. Just kill them.
			$this->killRow( $source );
			return $this->progress( 1 );
		}
		
		$cleaned = $source;
		
		// About half of old bad image names have percent-codes
		$cleaned = rawurldecode( $cleaned );
		
		// Some are old latin-1
		$cleaned = $wgContLang->checkTitleEncoding( $cleaned );
		
		// Many of remainder look like non-normalized unicode
		$cleaned = UtfNormal::cleanUp( $cleaned );
		
		$title = Title::makeTitleSafe( NS_IMAGE, $cleaned );
		
		if( is_null( $title ) ) {
			$this->log( "page $source ($cleaned) is illegal." );
			$safe = $this->buildSafeTitle( $cleaned );
			$this->pokeFile( $source, $safe );
			return $this->progress( 1 );
		}

		if( $title->getDBkey() !== $source ) {
			$munged = $title->getDBkey();
			$this->log( "page $source ($munged) doesn't match self." );
			$this->pokeFile( $source, $munged );
			return $this->progress( 1 );
		}

		$this->progress( 0 );
	}
	
	function killRow( $name ) {
		if( $this->dryrun ) {
			$this->log( "DRY RUN: would delete bogus row '$name'" );
		} else {
			$this->log( "deleting bogus row '$name'" );
			$db = wfGetDB( DB_MASTER );
			$db->delete( 'image',
				array( 'img_name' => $name ),
				__METHOD__ );
		}
	}
	
	function filePath( $name ) {
		if ( !isset( $this->repo ) ) {
			$this->repo = RepoGroup::singleton()->getLocalRepo();
		}
		return $this->repo->getRootDirectory() . '/' . $this->repo->getHashPath( $name ) . $name;
	}
	
	function pokeFile( $orig, $new ) {
		$path = $this->filePath( $orig );
		if( !file_exists( $path ) ) {
			$this->log( "missing file: $path" );
			return $this->killRow( $orig );
		}
		
		$db = wfGetDB( DB_MASTER );
		$version = 0;
		$final = $new;
		
		while( $db->selectField( 'image', 'img_name',
			array( 'img_name' => $final ), __METHOD__ ) ) {
			$this->log( "Rename conflicts with '$final'..." );
			$version++;
			$final = $this->appendTitle( $new, "_$version" );
		}
		
		$finalPath = $this->filePath( $final );
		
		if( $this->dryrun ) {
			$this->log( "DRY RUN: would rename $path to $finalPath" );
		} else {
			$this->log( "renaming $path to $finalPath" );
			$db->begin();
			$db->update( 'image',
				array( 'img_name' => $final ),
				array( 'img_name' => $orig ),
				__METHOD__ );
			$dir = dirname( $finalPath );
			if( !file_exists( $dir ) ) {
				if( !mkdir( $dir, 0777, true ) ) {
					$this->log( "RENAME FAILED, COULD NOT CREATE $dir" );
					$db->rollback();
					return;
				}
			}
			if( rename( $path, $finalPath ) ) {
				$db->commit();
			} else {
				$this->log( "RENAME FAILED" );
				$db->rollback();
			}
		}
	}
	
	function appendTitle( $name, $suffix ) {
		return preg_replace( '/^(.*)(\..*?)$/',
			"\\1$suffix\\2", $name );
	}
	
	function buildSafeTitle( $name ) {
		global $wgLegalTitleChars;
		$x = preg_replace_callback(
			"/([^$wgLegalTitleChars])/",
			array( $this, 'hexChar' ),
			$name );
		
		$test = Title::makeTitleSafe( NS_IMAGE, $x );
		if( is_null( $test ) || $test->getDBkey() !== $x ) {
			$this->log( "Unable to generate safe title from '$name', got '$x'" );
			return false;
		}
		
		return $x;
	}
}

$wgUser->setName( 'Conversion script' );
$caps = new ImageCleanup( !isset( $options['fix'] ) );
$caps->cleanup();


