<?php
# Copyright (C) 2005-2007 Brion Vibber <brion@pobox.com>
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
 * @file
 * @ingroup Maintenance
 */

$options = array( 'fix', 'suffix', 'help' );

/** */
require_once( 'commandLine.inc' );

if(isset( $options['help'] ) ) {
print <<<ENDS
usage: namespaceDupes.php [--fix] [--suffix=<text>] [--help]
    --help          : this help message
    --fix           : attempt to automatically fix errors
    --suffix=<text> : dupes will be renamed with correct namespace with <text>
                      appended after the article name.
    --prefix=<text> : Do an explicit check for the given title prefix
                      in place of the standard namespace list.
    --verbose       : Display output for checked namespaces without conflicts

ENDS;
die;
}

class NamespaceConflictChecker {
	function NamespaceConflictChecker( $db, $verbose=false ) {
		$this->db = $db;
		$this->verbose = $verbose;
	}

	function checkAll( $fix, $suffix = '' ) {
		global $wgContLang, $wgNamespaceAliases, $wgCanonicalNamespaceNames;
		global $wgCapitalLinks;
		
		$spaces = array();
		
		// List interwikis first, so they'll be overridden
		// by any conflicting local namespaces.
		foreach( $this->getInterwikiList() as $prefix ) {
			$name = $wgContLang->ucfirst( $prefix );
			$spaces[$name] = 0;
		}

		// Now pull in all canonical and alias namespaces...
		foreach( $wgCanonicalNamespaceNames as $ns => $name ) {
			// This includes $wgExtraNamespaces
			if( $name !== '' ) {
				$spaces[$name] = $ns;
			}
		}
		foreach( $wgContLang->getNamespaces() as $ns => $name ) {
			if( $name !== '' ) {
				$spaces[$name] = $ns;
			}
		}
		foreach( $wgNamespaceAliases as $name => $ns ) {
			$spaces[$name] = $ns;
		}
		foreach( $wgContLang->namespaceAliases as $name => $ns ) {
			$spaces[$name] = $ns;
		}
		
		// We'll need to check for lowercase keys as well,
		// since we're doing case-sensitive searches in the db.
		foreach( $spaces as $name => $ns ) {
			$moreNames = array();
			$moreNames[] = $wgContLang->uc( $name );
			$moreNames[] = $wgContLang->ucfirst( $wgContLang->lc( $name ) );
			$moreNames[] = $wgContLang->ucwords( $name );
			$moreNames[] = $wgContLang->ucwords( $wgContLang->lc( $name ) );
			$moreNames[] = $wgContLang->ucwordbreaks( $name );
			$moreNames[] = $wgContLang->ucwordbreaks( $wgContLang->lc( $name ) );
			if( !$wgCapitalLinks ) {
				foreach( $moreNames as $altName ) {
					$moreNames[] = $wgContLang->lcfirst( $altName );
				}
				$moreNames[] = $wgContLang->lcfirst( $name );
			}
			foreach( array_unique( $moreNames ) as $altName ) {
				if( $altName !== $name ) {
					$spaces[$altName] = $ns;
				}
			}
		}
		
		ksort( $spaces );
		asort( $spaces );
		
		$ok = true;
		foreach( $spaces as $name => $ns ) {
			$ok = $this->checkNamespace( $ns, $name, $fix, $suffix ) && $ok;
		}
		return $ok;
	}
	
	private function getInterwikiList() {
		$result = $this->db->select( 'interwiki', array( 'iw_prefix' ) );
		while( $row = $this->db->fetchObject( $result ) ) {
			$prefixes[] = $row->iw_prefix;
		}
		$this->db->freeResult( $result );
		return $prefixes;
	}

	function checkNamespace( $ns, $name, $fix, $suffix = '' ) {
		if( $ns == 0 ) {
			$header = "Checking interwiki prefix: \"$name\"\n";
		} else {
			$header = "Checking namespace $ns: \"$name\"\n";
		}

		$conflicts = $this->getConflicts( $ns, $name );
		$count = count( $conflicts );
		if( $count == 0 ) {
			if( $this->verbose ) {
				echo $header;
				echo "... no conflicts detected!\n";
			}
			return true;
		}

		echo $header;
		echo "... $count conflicts detected:\n";
		$ok = true;
		foreach( $conflicts as $row ) {
			$resolvable = $this->reportConflict( $row, $suffix );
			$ok = $ok && $resolvable;
			if( $fix && ( $resolvable || $suffix != '' ) ) {
				$ok = $this->resolveConflict( $row, $resolvable, $suffix ) && $ok;
			}
		}
		return $ok;
	}
	
	/**
	 * @todo: do this for reals
	 */
	function checkPrefix( $key, $prefix, $fix, $suffix = '' ) {
		echo "Checking prefix \"$prefix\" vs namespace $key\n";
		return $this->checkNamespace( $key, $prefix, $fix, $suffix );
	}

	function getConflicts( $ns, $name ) {
		$page  = 'page';
		$table = $this->db->tableName( $page );

		$prefix     = $this->db->strencode( $name );
		$likeprefix = str_replace( '_', '\\_', $prefix);
		$encNamespace = $this->db->addQuotes( $ns );

		$titleSql = "TRIM(LEADING '$prefix:' FROM {$page}_title)";
		if( $ns == 0 ) {
			// An interwiki; try an alternate encoding with '-' for ':'
			$titleSql = "CONCAT('$prefix-',$titleSql)";
		}
                                     
		$sql = "SELECT {$page}_id    AS id,
		               {$page}_title AS oldtitle,
		               $encNamespace AS namespace,
		               $titleSql     AS title
		          FROM {$table}
		         WHERE {$page}_namespace=0
		           AND {$page}_title LIKE '$likeprefix:%'";

		$result = $this->db->query( $sql, 'NamespaceConflictChecker::getConflicts' );

		$set = array();
		while( $row = $this->db->fetchObject( $result ) ) {
			$set[] = $row;
		}
		$this->db->freeResult( $result );

		return $set;
	}

	function reportConflict( $row, $suffix ) {
		$newTitle = Title::makeTitleSafe( $row->namespace, $row->title );
		if( !$newTitle ) {
			// Title is also an illegal title...
			// For the moment we'll let these slide to cleanupTitles or whoever.
			printf( "... %d (0,\"%s\")\n",
				$row->id,
				$row->oldtitle );
			echo "...  *** cannot resolve automatically; illegal title ***\n";
			return false;
		}
		
		printf( "... %d (0,\"%s\") -> (%d,\"%s\") [[%s]]\n",
			$row->id,
			$row->oldtitle,
			$newTitle->getNamespace(),
			$newTitle->getDBkey(),
			$newTitle->getPrefixedText() );

		$id = $newTitle->getArticleId();
		if( $id ) {
			echo "...  *** cannot resolve automatically; page exists with ID $id ***\n";
			return false;
		} else {
			return true;
		}
	}

	function resolveConflict( $row, $resolvable, $suffix ) {
		if( !$resolvable ) {
			echo "...  *** old title {$row->title}\n";
			$row->title .= $suffix;
			echo "...  *** new title {$row->title}\n";
			$title = Title::makeTitleSafe( $row->namespace, $row->title );
			if ( ! $title ) {
				echo "... !!! invalid title\n";
				return false;
			}
			echo "...  *** using suffixed form [[" . $title->getPrefixedText() . "]] ***\n";
		}
		$tables = array( 'page' );
		foreach( $tables as $table ) {
			$this->resolveConflictOn( $row, $table );
		}
		return true;
	}

	function resolveConflictOn( $row, $table ) {
		echo "... resolving on $table... ";
		$newTitle = Title::makeTitleSafe( $row->namespace, $row->title );
		$this->db->update( $table,
			array(
				"{$table}_namespace" => $newTitle->getNamespace(),
				"{$table}_title"     => $newTitle->getDBkey(),
			),
			array(
				"{$table}_namespace" => 0,
				"{$table}_title"     => $row->oldtitle,
			),
			__METHOD__ );
		echo "ok.\n";
		return true;
	}
}




$wgTitle = Title::newFromText( 'Namespace title conflict cleanup script' );

$verbose = isset( $options['verbose'] );
$fix = isset( $options['fix'] );
$suffix = isset( $options['suffix'] ) ? $options['suffix'] : '';
$prefix = isset( $options['prefix'] ) ? $options['prefix'] : '';
$key = isset( $options['key'] ) ? intval( $options['key'] ) : 0;

$dbw = wfGetDB( DB_MASTER );
$duper = new NamespaceConflictChecker( $dbw, $verbose );

if( $prefix ) {
	$retval = $duper->checkPrefix( $key, $prefix, $fix, $suffix );
} else {
	$retval = $duper->checkAll( $fix, $suffix );
}

if( $retval ) {
	echo "\nLooks good!\n";
	exit( 0 );
} else {
	echo "\nOh noeees\n";
	exit( -1 );
}


