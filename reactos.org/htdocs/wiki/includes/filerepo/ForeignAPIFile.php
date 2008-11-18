<?php

/** 
 * Very hacky and inefficient
 * do not use :D
 *
 * @ingroup FileRepo
 */
class ForeignAPIFile extends File {
	function __construct( $title, $repo, $info ) {
		parent::__construct( $title, $repo );
		$this->mInfo = $info;
	}
	
	static function newFromTitle( $title, $repo ) {
		$info = $repo->getImageInfo( $title );
		if( $info ) {
			return new ForeignAPIFile( $title, $repo, $info );
		} else {
			return null;
		}
	}
	
	// Dummy functions...
	public function exists() {
		return true;
	}
	
	public function getPath() {
		return false;
	}

	function transform( $params, $flags = 0 ) {
		$thumbUrl = $this->repo->getThumbUrl(
			$this->getName(),
			isset( $params['width'] ) ? $params['width'] : -1,
			isset( $params['height'] ) ? $params['height'] : -1 );
		if( $thumbUrl ) {
			wfDebug( __METHOD__ . " got remote thumb $thumbUrl\n" );
			return $this->handler->getTransform( $this, 'bogus', $thumbUrl, $params );;
		}
		return false;
	}

	// Info we can get from API...
	public function getWidth( $page = 1 ) {
		return intval( @$this->mInfo['width'] );
	}
	
	public function getHeight( $page = 1 ) {
		return intval( @$this->mInfo['height'] );
	}
	
	public function getMetadata() {
		return serialize( (array)@$this->mInfo['metadata'] );
	}
	
	public function getSize() {
		return intval( @$this->mInfo['size'] );
	}
	
	public function getUrl() {
		return strval( @$this->mInfo['url'] );
	}

	public function getUser( $method='text' ) {
		return strval( @$this->mInfo['user'] );
	}
	
	public function getDescription() {
		return strval( @$this->mInfo['comment'] );
	}

	function getSha1() {
		return wfBaseConvert( strval( @$this->mInfo['sha1'] ), 16, 36, 31 );
	}
	
	function getTimestamp() {
		return wfTimestamp( TS_MW, strval( @$this->mInfo['timestamp'] ) );
	}
	
	function getMimeType() {
		if( empty( $info['mime'] ) ) {
			$magic = MimeMagic::singleton();
			$info['mime'] = $magic->guessTypesForExtension( $this->getExtension() );
		}
		return $info['mime'];
	}
	
	/// @fixme May guess wrong on file types that can be eg audio or video
	function getMediaType() {
		$magic = MimeMagic::singleton();
		return $magic->getMediaType( null, $this->getMimeType() );
	}
	
	function getDescriptionUrl() {
		return isset( $this->mInfo['descriptionurl'] )
			? $this->mInfo['descriptionurl']
			: false;
	}
}
