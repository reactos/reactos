<?php
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

$originalDir = getcwd();

require_once( 'commandLine.inc' );
require_once( 'backup.inc' );

/**
 * Stream wrapper around 7za filter program.
 * Required since we can't pass an open file resource to XMLReader->open()
 * which is used for the text prefetch.
 *
 * @ingroup Maintenance
 */
class SevenZipStream {
	var $stream;
	
	private function stripPath( $path ) {
		$prefix = 'mediawiki.compress.7z://';
		return substr( $path, strlen( $prefix ) );
	}
	
	function stream_open( $path, $mode, $options, &$opened_path ) {
		if( $mode{0} == 'r' ) {
			$options = 'e -bd -so';
		} elseif( $mode{0} == 'w' ) {
			$options = 'a -bd -si';
		} else {
			return false;
		}
		$arg = wfEscapeShellArg( $this->stripPath( $path ) );
		$command = "7za $options $arg";
		if( !wfIsWindows() ) {
			// Suppress the stupid messages on stderr
			$command .= ' 2>/dev/null';
		}
		$this->stream = popen( $command, $mode );
		return ($this->stream !== false);
	}
	
	function url_stat( $path, $flags ) {
		return stat( $this->stripPath( $path ) );
	}
	
	// This is all so lame; there should be a default class we can extend
	
	function stream_close() {
		return fclose( $this->stream );
	}
	
	function stream_flush() {
		return fflush( $this->stream );
	}
	
	function stream_read( $count ) {
		return fread( $this->stream, $count );
	}
	
	function stream_write( $data ) {
		return fwrite( $this->stream, $data );
	}
	
	function stream_tell() {
		return ftell( $this->stream );
	}
	
	function stream_eof() {
		return feof( $this->stream );
	}
	
	function stream_seek( $offset, $whence ) {
		return fseek( $this->stream, $offset, $whence );
	}
}
stream_wrapper_register( 'mediawiki.compress.7z', 'SevenZipStream' );

/**
 * @ingroup Maintenance
 */
class TextPassDumper extends BackupDumper {
	var $prefetch = null;
	var $input = "php://stdin";
	var $history = WikiExporter::FULL;
	var $fetchCount = 0;
	var $prefetchCount = 0;
	
	var $failures = 0;
	var $maxFailures = 200;
	var $failureTimeout = 5; // Seconds to sleep after db failure
	
	var $php = "php";
	var $spawn = false;
	var $spawnProc = false;
	var $spawnWrite = false;
	var $spawnRead = false;
	var $spawnErr = false;

	function dump() {
		# This shouldn't happen if on console... ;)
		header( 'Content-type: text/html; charset=UTF-8' );

		# Notice messages will foul up your XML output even if they're
		# relatively harmless.
		if( ini_get( 'display_errors' ) )
			ini_set( 'display_errors', 'stderr' );

		$this->initProgress( $this->history );

		$this->db = $this->backupDb();

		$this->egress = new ExportProgressFilter( $this->sink, $this );

		$input = fopen( $this->input, "rt" );
		$result = $this->readDump( $input );

		if( WikiError::isError( $result ) ) {
			wfDie( $result->getMessage() );
		}
		
		if( $this->spawnProc ) {
			$this->closeSpawn();
		}

		$this->report( true );
	}

	function processOption( $opt, $val, $param ) {
		$url = $this->processFileOpt( $val, $param );
		
		switch( $opt ) {
		case 'prefetch':
			global $IP;
			require_once "$IP/maintenance/backupPrefetch.inc";
			$this->prefetch = new BaseDump( $url );
			break;
		case 'stub':
			$this->input = $url;
			break;
		case 'current':
			$this->history = WikiExporter::CURRENT;
			break;
		case 'full':
			$this->history = WikiExporter::FULL;
			break;
		case 'spawn':
			$this->spawn = true;
			if( $val ) {
				$this->php = $val;
			}
			break;
		}
	}
	
	function processFileOpt( $val, $param ) {
		switch( $val ) {
		case "file":
			return $param;
		case "gzip":
			return "compress.zlib://$param";
		case "bzip2":
			return "compress.bzip2://$param";
		case "7zip":
			return "mediawiki.compress.7z://$param";
		default:
			return $val;
		}
	}

	/**
	 * Overridden to include prefetch ratio if enabled.
	 */
	function showReport() {
		if( !$this->prefetch ) {
			return parent::showReport();
		}
		
		if( $this->reporting ) {
			$delta = wfTime() - $this->startTime;
			$now = wfTimestamp( TS_DB );
			if( $delta ) {
				$rate = $this->pageCount / $delta;
				$revrate = $this->revCount / $delta;
				$portion = $this->revCount / $this->maxCount;
				$eta = $this->startTime + $delta / $portion;
				$etats = wfTimestamp( TS_DB, intval( $eta ) );
				$fetchrate = 100.0 * $this->prefetchCount / $this->fetchCount;
			} else {
				$rate = '-';
				$revrate = '-';
				$etats = '-';
				$fetchrate = '-';
			}
			$this->progress( sprintf( "%s: %s %d pages (%0.3f/sec), %d revs (%0.3f/sec), %0.1f%% prefetched, ETA %s [max %d]",
				$now, wfWikiID(), $this->pageCount, $rate, $this->revCount, $revrate, $fetchrate, $etats, $this->maxCount ) );
		}
	}

	function readDump( $input ) {
		$this->buffer = "";
		$this->openElement = false;
		$this->atStart = true;
		$this->state = "";
		$this->lastName = "";
		$this->thisPage = 0;
		$this->thisRev = 0;

		$parser = xml_parser_create( "UTF-8" );
		xml_parser_set_option( $parser, XML_OPTION_CASE_FOLDING, false );

		xml_set_element_handler( $parser, array( &$this, 'startElement' ), array( &$this, 'endElement' ) );
		xml_set_character_data_handler( $parser, array( &$this, 'characterData' ) );

		$offset = 0; // for context extraction on error reporting
		$bufferSize = 512 * 1024;
		do {
			$chunk = fread( $input, $bufferSize );
			if( !xml_parse( $parser, $chunk, feof( $input ) ) ) {
				wfDebug( "TextDumpPass::readDump encountered XML parsing error\n" );
				return new WikiXmlError( $parser, 'XML import parse failure', $chunk, $offset );
			}
			$offset += strlen( $chunk );
		} while( $chunk !== false && !feof( $input ) );
		xml_parser_free( $parser );
		
		return true;
	}

	function getText( $id ) {
		$this->fetchCount++;
		if( isset( $this->prefetch ) ) {
			$text = $this->prefetch->prefetch( $this->thisPage, $this->thisRev );
			if( $text === null ) {
				// Entry missing from prefetch dump
			} elseif( $text === "" ) {
				// Blank entries may indicate that the prior dump was broken.
				// To be safe, reload it.
			} else {
				$this->prefetchCount++;
				return $text;
			}
		}
		return $this->doGetText( $id );
	}
	
	private function doGetText( $id ) {
		if( $this->spawn ) {
			return $this->getTextSpawned( $id );
		} else {
			return $this->getTextDbSafe( $id );
		}
	}
	
	/**
	 * Fetch a text revision from the database, retrying in case of failure.
	 * This may survive some transitory errors by reconnecting, but
	 * may not survive a long-term server outage.
	 */
	private function getTextDbSafe( $id ) {
		while( true ) {
			try {
				$text = $this->getTextDb( $id );
				$ex = new MWException("Graceful storage failure");
			} catch (DBQueryError $ex) {
				$text = false;
			}
			if( $text === false ) {
				$this->failures++;
				if( $this->failures > $this->maxFailures ) {
					throw $ex;
				} else {
					$this->progress( "Database failure $this->failures " .
						"of allowed $this->maxFailures for revision $id! " .
						"Pausing $this->failureTimeout seconds..." );
					sleep( $this->failureTimeout );
				}
			} else {
				return $text;
			}
		}
	}
	
	/**
	 * May throw a database error if, say, the server dies during query.
	 */
	private function getTextDb( $id ) {
		$id = intval( $id );
		$row = $this->db->selectRow( 'text',
			array( 'old_text', 'old_flags' ),
			array( 'old_id' => $id ),
			'TextPassDumper::getText' );
		$text = Revision::getRevisionText( $row );
		if( $text === false ) {
			return false;
		}
		$stripped = str_replace( "\r", "", $text );
		$normalized = UtfNormal::cleanUp( $stripped );
		return $normalized;
	}
	
	private function getTextSpawned( $id ) {
		wfSuppressWarnings();
		if( !$this->spawnProc ) {
			// First time?
			$this->openSpawn();
		}
		while( true ) {
			
			$text = $this->getTextSpawnedOnce( $id );
			if( !is_string( $text ) ) {
				$this->progress("Database subprocess failed. Respawning...");
				
				$this->closeSpawn();
				sleep( $this->failureTimeout );
				$this->openSpawn();
				
				continue;
			}
			wfRestoreWarnings();
			return $text;
		}
	}
	
	function openSpawn() {
		global $IP, $wgDBname;
		
		$cmd = implode( " ",
			array_map( 'wfEscapeShellArg',
				array(
					$this->php,
					"$IP/maintenance/fetchText.php",
					$wgDBname ) ) );
		$spec = array(
			0 => array( "pipe", "r" ),
			1 => array( "pipe", "w" ),
			2 => array( "file", "/dev/null", "a" ) );
		$pipes = array();
		
		$this->progress( "Spawning database subprocess: $cmd" );
		$this->spawnProc = proc_open( $cmd, $spec, $pipes );
		if( !$this->spawnProc ) {
			// shit
			$this->progress( "Subprocess spawn failed." );
			return false;
		}
		list(
			$this->spawnWrite, // -> stdin
			$this->spawnRead,  // <- stdout
		) = $pipes;
		
		return true;
	}
	
	private function closeSpawn() {
		wfSuppressWarnings();
		if( $this->spawnRead )
			fclose( $this->spawnRead );
		$this->spawnRead = false;
		if( $this->spawnWrite )
			fclose( $this->spawnWrite );
		$this->spawnWrite = false;
		if( $this->spawnErr )
			fclose( $this->spawnErr );
		$this->spawnErr = false;
		if( $this->spawnProc )
			pclose( $this->spawnProc );
		$this->spawnProc = false;
		wfRestoreWarnings();
	}
	
	private function getTextSpawnedOnce( $id ) {
		$ok = fwrite( $this->spawnWrite, "$id\n" );
		//$this->progress( ">> $id" );
		if( !$ok ) return false;
		
		$ok = fflush( $this->spawnWrite );
		//$this->progress( ">> [flush]" );
		if( !$ok ) return false;
		
		$len = fgets( $this->spawnRead );
		//$this->progress( "<< " . trim( $len ) );
		if( $len === false ) return false;
		
		$nbytes = intval( $len );
		$text = "";
		
		// Subprocess may not send everything at once, we have to loop.
		while( $nbytes > strlen( $text ) ) {
			$buffer = fread( $this->spawnRead, $nbytes - strlen( $text ) );
			if( $text === false ) break;
			$text .= $buffer;
		}
		
		$gotbytes = strlen( $text );
		if( $gotbytes != $nbytes ) {
			$this->progress( "Expected $nbytes bytes from database subprocess, got $gotbytes ");
			return false;
		}
		
		// Do normalization in the dump thread...
		$stripped = str_replace( "\r", "", $text );
		$normalized = UtfNormal::cleanUp( $stripped );
		return $normalized;
	}

	function startElement( $parser, $name, $attribs ) {
		$this->clearOpenElement( null );
		$this->lastName = $name;

		if( $name == 'revision' ) {
			$this->state = $name;
			$this->egress->writeOpenPage( null, $this->buffer );
			$this->buffer = "";
		} elseif( $name == 'page' ) {
			$this->state = $name;
			if( $this->atStart ) {
				$this->egress->writeOpenStream( $this->buffer );
				$this->buffer = "";
				$this->atStart = false;
			}
		}

		if( $name == "text" && isset( $attribs['id'] ) ) {
			$text = $this->getText( $attribs['id'] );
			$this->openElement = array( $name, array( 'xml:space' => 'preserve' ) );
			if( strlen( $text ) > 0 ) {
				$this->characterData( $parser, $text );
			}
		} else {
			$this->openElement = array( $name, $attribs );
		}
	}

	function endElement( $parser, $name ) {
		if( $this->openElement ) {
			$this->clearOpenElement( "" );
		} else {
			$this->buffer .= "</$name>";
		}

		if( $name == 'revision' ) {
			$this->egress->writeRevision( null, $this->buffer );
			$this->buffer = "";
			$this->thisRev = "";
		} elseif( $name == 'page' ) {
			$this->egress->writeClosePage( $this->buffer );
			$this->buffer = "";
			$this->thisPage = "";
		} elseif( $name == 'mediawiki' ) {
			$this->egress->writeCloseStream( $this->buffer );
			$this->buffer = "";
		}
	}

	function characterData( $parser, $data ) {
		$this->clearOpenElement( null );
		if( $this->lastName == "id" ) {
			if( $this->state == "revision" ) {
				$this->thisRev .= $data;
			} elseif( $this->state == "page" ) {
				$this->thisPage .= $data;
			}
		}
		$this->buffer .= htmlspecialchars( $data );
	}

	function clearOpenElement( $style ) {
		if( $this->openElement ) {
			$this->buffer .= wfElement( $this->openElement[0], $this->openElement[1], $style );
			$this->openElement = false;
		}
	}
}


$dumper = new TextPassDumper( $argv );

if( true ) {
	$dumper->dump();
} else {
	$dumper->progress( <<<ENDS
This script postprocesses XML dumps from dumpBackup.php to add
page text which was stubbed out (using --stub).

XML input is accepted on stdin.
XML output is sent to stdout; progress reports are sent to stderr.

Usage: php dumpTextPass.php [<options>]
Options:
  --stub=<type>:<file> To load a compressed stub dump instead of stdin
  --prefetch=<type>:<file> Use a prior dump file as a text source, to save
              pressure on the database.
              (Requires PHP 5.0+ and the XMLReader PECL extension)
  --quiet     Don't dump status reports to stderr.
  --report=n  Report position and speed after every n pages processed.
              (Default: 100)
  --server=h  Force reading from MySQL server h
  --current   Base ETA on number of pages in database instead of all revisions
  --spawn     Spawn a subprocess for loading text records
ENDS
);
}


