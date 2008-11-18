<?php
/**
 * Contain the HTMLFileCache class
 * @file
 * @ingroup Cache
 */

/**
 * Handles talking to the file cache, putting stuff in and taking it back out.
 * Mostly called from Article.php, also from DatabaseFunctions.php for the
 * emergency abort/fallback to cache.
 *
 * Global options that affect this module:
 * - $wgCachePages
 * - $wgCacheEpoch
 * - $wgUseFileCache
 * - $wgFileCacheDirectory
 * - $wgUseGzip
 *
 * @ingroup Cache
 */
class HTMLFileCache {
	var $mTitle, $mFileCache;

	function HTMLFileCache( &$title ) {
		$this->mTitle =& $title;
		$this->mFileCache = '';
	}

	function fileCacheName() {
		global $wgFileCacheDirectory;
		if( !$this->mFileCache ) {
			$key = $this->mTitle->getPrefixedDbkey();
			$hash = md5( $key );
			$key = str_replace( '.', '%2E', urlencode( $key ) );

			$hash1 = substr( $hash, 0, 1 );
			$hash2 = substr( $hash, 0, 2 );
			$this->mFileCache = "{$wgFileCacheDirectory}/{$hash1}/{$hash2}/{$key}.html";

			if($this->useGzip())
				$this->mFileCache .= '.gz';

			wfDebug( " fileCacheName() - {$this->mFileCache}\n" );
		}
		return $this->mFileCache;
	}

	function isFileCached() {
		return file_exists( $this->fileCacheName() );
	}

	function fileCacheTime() {
		return wfTimestamp( TS_MW, filemtime( $this->fileCacheName() ) );
	}

	function isFileCacheGood( $timestamp ) {
		global $wgCacheEpoch;

		if( !$this->isFileCached() ) return false;

		$cachetime = $this->fileCacheTime();
		$good = (( $timestamp <= $cachetime ) &&
			 ( $wgCacheEpoch <= $cachetime ));

		wfDebug(" isFileCacheGood() - cachetime $cachetime, touched {$timestamp} epoch {$wgCacheEpoch}, good $good\n");
		return $good;
	}

	function useGzip() {
		global $wgUseGzip;
		return $wgUseGzip;
	}

	/* In handy string packages */
	function fetchRawText() {
		return file_get_contents( $this->fileCacheName() );
	}

	function fetchPageText() {
		if( $this->useGzip() ) {
			/* Why is there no gzfile_get_contents() or gzdecode()? */
			return implode( '', gzfile( $this->fileCacheName() ) );
		} else {
			return $this->fetchRawText();
		}
	}

	/* Working directory to/from output */
	function loadFromFileCache() {
		global $wgOut, $wgMimeType, $wgOutputEncoding, $wgContLanguageCode;
		wfDebug(" loadFromFileCache()\n");

		$filename=$this->fileCacheName();
		$wgOut->sendCacheControl();

		header( "Content-type: $wgMimeType; charset={$wgOutputEncoding}" );
		header( "Content-language: $wgContLanguageCode" );

		if( $this->useGzip() ) {
			if( wfClientAcceptsGzip() ) {
				header( 'Content-Encoding: gzip' );
			} else {
				/* Send uncompressed */
				readgzfile( $filename );
				return;
			}
		}
		readfile( $filename );
	}

	function checkCacheDirs() {
		$filename = $this->fileCacheName();
		$mydir2=substr($filename,0,strrpos($filename,'/')); # subdirectory level 2
		$mydir1=substr($mydir2,0,strrpos($mydir2,'/')); # subdirectory level 1

		if(!file_exists($mydir1)) { mkdir($mydir1,0775); } # create if necessary
		if(!file_exists($mydir2)) { mkdir($mydir2,0775); }
	}

	function saveToFileCache( $origtext ) {
		$text = $origtext;
		if(strcmp($text,'') == 0) return '';

		wfDebug(" saveToFileCache()\n", false);

		$this->checkCacheDirs();

		$f = fopen( $this->fileCacheName(), 'w' );
		if($f) {
			$now = wfTimestampNow();
			if( $this->useGzip() ) {
				$rawtext = str_replace( '</html>',
					'<!-- Cached/compressed '.$now." -->\n</html>",
					$text );
				$text = gzencode( $rawtext );
			} else {
				$text = str_replace( '</html>',
					'<!-- Cached '.$now." -->\n</html>",
					$text );
			}
			fwrite( $f, $text );
			fclose( $f );
			if( $this->useGzip() ) {
				if( wfClientAcceptsGzip() ) {
					header( 'Content-Encoding: gzip' );
					return $text;
				} else {
					return $rawtext;
				}
			} else {
				return $text;
			}
		}
		return $text;
	}

}
