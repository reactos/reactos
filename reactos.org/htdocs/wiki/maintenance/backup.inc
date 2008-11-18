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
 * @ingroup Dump Maintenance
 */

/**
 * @ingroup Dump Maintenance
 */
class DumpDBZip2Output extends DumpPipeOutput {
	function DumpDBZip2Output( $file ) {
		parent::DumpPipeOutput( "dbzip2", $file );
	}
}

/**
 * @ingroup Dump Maintenance
 */
class BackupDumper {
	var $reportingInterval = 100;
	var $reporting = true;
	var $pageCount = 0;
	var $revCount  = 0;
	var $server    = null; // use default
	var $pages     = null; // all pages
	var $skipHeader = false; // don't output <mediawiki> and <siteinfo>
	var $skipFooter = false; // don't output </mediawiki>
	var $startId    = 0;
	var $endId      = 0;
	var $sink       = null; // Output filters
	var $stubText   = false; // include rev_text_id instead of text; for 2-pass dump
	var $dumpUploads = false;

	function BackupDumper( $args ) {
		$this->stderr = fopen( "php://stderr", "wt" );

		// Built-in output and filter plugins
		$this->registerOutput( 'file', 'DumpFileOutput' );
		$this->registerOutput( 'gzip', 'DumpGZipOutput' );
		$this->registerOutput( 'bzip2', 'DumpBZip2Output' );
		$this->registerOutput( 'dbzip2', 'DumpDBZip2Output' );
		$this->registerOutput( '7zip', 'Dump7ZipOutput' );

		$this->registerFilter( 'latest', 'DumpLatestFilter' );
		$this->registerFilter( 'notalk', 'DumpNotalkFilter' );
		$this->registerFilter( 'namespace', 'DumpNamespaceFilter' );

		$this->sink = $this->processArgs( $args );
	}

	/**
	 * @param string $name
	 * @param string $class name of output filter plugin class
	 */
	function registerOutput( $name, $class ) {
		$this->outputTypes[$name] = $class;
	}

	/**
	 * @param string $name
	 * @param string $class name of filter plugin class
	 */
	function registerFilter( $name, $class ) {
		$this->filterTypes[$name] = $class;
	}

	/**
	 * Load a plugin and register it
	 * @param string $class Name of plugin class; must have a static 'register'
	 *                      method that takes a BackupDumper as a parameter.
	 * @param string $file Full or relative path to the PHP file to load, or empty
	 */
	function loadPlugin( $class, $file ) {
		if( $file != '' ) {
			require_once( $file );
		}
		$register = array( $class, 'register' );
		call_user_func_array( $register, array( &$this ) );
	}

	/**
	 * @param array $args
	 * @return array
	 * @static
	 */
	function processArgs( $args ) {
		$sink = null;
		$sinks = array();
		foreach( $args as $arg ) {
			$matches = array();
			if( preg_match( '/^--(.+?)(?:=(.+?)(?::(.+?))?)?$/', $arg, $matches ) ) {
				@list( /* $full */ , $opt, $val, $param ) = $matches;
				switch( $opt ) {
				case "plugin":
					$this->loadPlugin( $val, $param );
					break;
				case "output":
					if( !is_null( $sink ) ) {
						$sinks[] = $sink;
					}
					if( !isset( $this->outputTypes[$val] ) ) {
						wfDie( "Unrecognized output sink type '$val'\n" );
					}
					$type = $this->outputTypes[$val];
					$sink = new $type( $param );
					break;
				case "filter":
					if( is_null( $sink ) ) {
						$this->progress( "Warning: assuming stdout for filter output\n" );
						$sink = new DumpOutput();
					}
					if( !isset( $this->filterTypes[$val] ) ) {
						wfDie( "Unrecognized filter type '$val'\n" );
					}
					$type = $this->filterTypes[$val];
					$filter = new $type( $sink, $param );

					// references are lame in php...
					unset( $sink );
					$sink = $filter;

					break;
				case "report":
					$this->reportingInterval = intval( $val );
					break;
				case "server":
					$this->server = $val;
					break;
				case "force-normal":
					if( !function_exists( 'utf8_normalize' ) ) {
						dl( "php_utfnormal.so" );
						if( !function_exists( 'utf8_normalize' ) ) {
							wfDie( "Failed to load UTF-8 normalization extension. " .
								"Install or remove --force-normal parameter to use slower code.\n" );
						}
					}
					break;
				default:
					$this->processOption( $opt, $val, $param );
				}
			}
		}

		if( is_null( $sink ) ) {
			$sink = new DumpOutput();
		}
		$sinks[] = $sink;

		if( count( $sinks ) > 1 ) {
			return new DumpMultiWriter( $sinks );
		} else {
			return $sink;
		}
	}

	function processOption( $opt, $val, $param ) {
		// extension point for subclasses to add options
	}

	function dump( $history, $text = MW_EXPORT_TEXT ) {
		# Notice messages will foul up your XML output even if they're
		# relatively harmless.
		if( ini_get( 'display_errors' ) )
			ini_set( 'display_errors', 'stderr' );

		$this->initProgress( $history );

		$db = $this->backupDb();
		$exporter = new WikiExporter( $db, $history, WikiExporter::STREAM, $text );
		$exporter->dumpUploads = $this->dumpUploads;

		$wrapper = new ExportProgressFilter( $this->sink, $this );
		$exporter->setOutputSink( $wrapper );

		if( !$this->skipHeader )
			$exporter->openStream();

		if( is_null( $this->pages ) ) {
			if( $this->startId || $this->endId ) {
				$exporter->pagesByRange( $this->startId, $this->endId );
			} else {
				$exporter->allPages();
			}
		} else {
			$exporter->pagesByName( $this->pages );
		}

		if( !$this->skipFooter )
			$exporter->closeStream();

		$this->report( true );
	}
	
	/**
	 * Initialise starting time and maximum revision count.
	 * We'll make ETA calculations based an progress, assuming relatively
	 * constant per-revision rate.
	 * @param int $history WikiExporter::CURRENT or WikiExporter::FULL
	 */
	function initProgress( $history = WikiExporter::FULL ) {
		$table = ($history == WikiExporter::CURRENT) ? 'page' : 'revision';
		$field = ($history == WikiExporter::CURRENT) ? 'page_id' : 'rev_id';
		
		$dbr = wfGetDB( DB_SLAVE );
		$this->maxCount = $dbr->selectField( $table, "MAX($field)", '', 'BackupDumper::dump' );
		$this->startTime = wfTime();
	}

	function backupDb() {
		global $wgDBadminuser, $wgDBadminpassword;
		global $wgDBname, $wgDebugDumpSql, $wgDBtype;
		$flags = ($wgDebugDumpSql ? DBO_DEBUG : 0) | DBO_DEFAULT; // god-damn hack

		$class = 'Database' . ucfirst($wgDBtype);
		$db = new $class( $this->backupServer(), $wgDBadminuser, $wgDBadminpassword, $wgDBname, false, $flags );
		
		// Discourage the server from disconnecting us if it takes a long time
		// to read out the big ol' batch query.
		$db->setTimeout( 3600 * 24 );
		
		return $db;
	}

	function backupServer() {
		global $wgDBserver;
		return $this->server
			? $this->server
			: $wgDBserver;
	}

	function reportPage() {
		$this->pageCount++;
	}

	function revCount() {
		$this->revCount++;
		$this->report();
	}

	function report( $final = false ) {
		if( $final xor ( $this->revCount % $this->reportingInterval == 0 ) ) {
			$this->showReport();
		}
	}

	function showReport() {
		if( $this->reporting ) {
			$delta = wfTime() - $this->startTime;
			$now = wfTimestamp( TS_DB );
			if( $delta ) {
				$rate = $this->pageCount / $delta;
				$revrate = $this->revCount / $delta;
				$portion = $this->revCount / $this->maxCount;
				$eta = $this->startTime + $delta / $portion;
				$etats = wfTimestamp( TS_DB, intval( $eta ) );
			} else {
				$rate = '-';
				$revrate = '-';
				$etats = '-';
			}
			$this->progress( sprintf( "%s: %s %d pages (%0.3f/sec), %d revs (%0.3f/sec), ETA %s [max %d]",
				$now, wfWikiID(), $this->pageCount, $rate, $this->revCount, $revrate, $etats, $this->maxCount ) );
		}
	}

	function progress( $string ) {
		fwrite( $this->stderr, $string . "\n" );
	}
}

class ExportProgressFilter extends DumpFilter {
	function ExportProgressFilter( &$sink, &$progress ) {
		parent::DumpFilter( $sink );
		$this->progress = $progress;
	}

	function writeClosePage( $string ) {
		parent::writeClosePage( $string );
		$this->progress->reportPage();
	}

	function writeRevision( $rev, $string ) {
		parent::writeRevision( $rev, $string );
		$this->progress->revCount();
	}
}
