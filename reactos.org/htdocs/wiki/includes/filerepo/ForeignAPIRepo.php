<?php

/**
 * A foreign repository with a remote MediaWiki with an API thingy
 * Very hacky and inefficient
 * do not use except for testing :D
 *
 * Example config:
 *
 * $wgForeignFileRepos[] = array(
 *   'class'                  => 'ForeignAPIRepo',
 *   'name'                   => 'shared',
 *   'apibase'                => 'http://en.wikipedia.org/w/api.php',
 *   'fetchDescription'       => true, // Optional
 *   'descriptionCacheExpiry' => 3600,
 * );
 *
 * @ingroup FileRepo
 */
class ForeignAPIRepo extends FileRepo {
	var $fileFactory = array( 'ForeignAPIFile', 'newFromTitle' );
	protected $mQueryCache = array();
	
	function __construct( $info ) {
		parent::__construct( $info );
		$this->mApiBase = $info['apibase']; // http://commons.wikimedia.org/w/api.php
		if( !$this->scriptDirUrl ) {
			// hack for description fetches
			$this->scriptDirUrl = dirname( $this->mApiBase );
		}
	}

	function storeBatch( $triplets, $flags = 0 ) {
		return false;
	}

	function storeTemp( $originalName, $srcPath ) {
		return false;
	}
	function publishBatch( $triplets, $flags = 0 ) {
		return false;
	}
	function deleteBatch( $sourceDestPairs ) {
		return false;
	}
	function getFileProps( $virtualUrl ) {
		return false;
	}
	
	protected function queryImage( $query ) {
		$data = $this->fetchImageQuery( $query );
		
		if( isset( $data['query']['pages'] ) ) {
			foreach( $data['query']['pages'] as $pageid => $info ) {
				if( isset( $info['imageinfo'][0] ) ) {
					return $info['imageinfo'][0];
				}
			}
		}
		return false;
	}
	
	protected function fetchImageQuery( $query ) {
		global $wgMemc;
		
		$url = $this->mApiBase .
			'?' .
			wfArrayToCgi(
				array_merge( $query,
					array(
						'format' => 'json',
						'action' => 'query',
						'prop' => 'imageinfo' ) ) );
		
		if( !isset( $this->mQueryCache[$url] ) ) {
			$key = wfMemcKey( 'ForeignAPIRepo', $url );
			$data = $wgMemc->get( $key );
			if( !$data ) {
				$data = Http::get( $url );
				$wgMemc->set( $key, $data, 3600 );
			}

			if( count( $this->mQueryCache ) > 100 ) {
				// Keep the cache from growing infinitely
				$this->mQueryCache = array();
			}
			$this->mQueryCache[$url] = $data;
		}
		return json_decode( $this->mQueryCache[$url], true );
	}
	
	function getImageInfo( $title, $time = false ) {
		return $this->queryImage( array(
			'titles' => 'Image:' . $title->getText(),
			'iiprop' => 'timestamp|user|comment|url|size|sha1|metadata|mime' ) );
	}
	
	function getThumbUrl( $name, $width=-1, $height=-1 ) {
		$info = $this->queryImage( array(
			'titles' => 'Image:' . $name,
			'iiprop' => 'url',
			'iiurlwidth' => $width,
			'iiurlheight' => $height ) );
		if( $info ) {
			return $info['thumburl'];
		} else {
			return false;
		}
	}
}
