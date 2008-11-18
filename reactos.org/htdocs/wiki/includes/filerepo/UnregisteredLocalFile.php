<?php

/**
 * A file object referring to either a standalone local file, or a file in a
 * local repository with no database, for example an FSRepo repository.
 *
 * Read-only.
 *
 * TODO: Currently it doesn't really work in the repository role, there are
 * lots of functions missing. It is used by the WebStore extension in the
 * standalone role.
 *
 * @ingroup FileRepo
 */
class UnregisteredLocalFile extends File {
	var $title, $path, $mime, $handler, $dims;

	static function newFromPath( $path, $mime ) {
		return new UnregisteredLocalFile( false, false, $path, $mime );
	}

	static function newFromTitle( $title, $repo ) {
		return new UnregisteredLocalFile( $title, $repo, false, false );
	}

	function __construct( $title = false, $repo = false, $path = false, $mime = false ) {
		if ( !( $title && $repo ) && !$path ) {
			throw new MWException( __METHOD__.': not enough parameters, must specify title and repo, or a full path' );
		}
		if ( $title ) {
			$this->title = $title;
			$this->name = $repo->getNameFromTitle( $title );
		} else {
			$this->name = basename( $path );
			$this->title = Title::makeTitleSafe( NS_IMAGE, $this->name );
		}
		$this->repo = $repo;
		if ( $path ) {
			$this->path = $path;
		} else {
			$this->path = $repo->getRootDirectory() . '/' . $repo->getHashPath( $this->name ) . $this->name;
		}
		if ( $mime ) {
			$this->mime = $mime;
		}
		$this->dims = array();
	}

	function getPageDimensions( $page = 1 ) {
		if ( !isset( $this->dims[$page] ) ) {
			if ( !$this->getHandler() ) {
				return false;
			}
			$this->dims[$page] = $this->handler->getPageDimensions( $this, $page );
		}
		return $this->dims[$page];
	}

	function getWidth( $page = 1 ) {
		$dim = $this->getPageDimensions( $page );
		return $dim['width'];
	}

	function getHeight( $page = 1 ) {
		$dim = $this->getPageDimensions( $page );
		return $dim['height'];
	}

	function getMimeType() {
		if ( !isset( $this->mime ) ) {
			$magic = MimeMagic::singleton();
			$this->mime = $magic->guessMimeType( $this->path );
		}
		return $this->mime;
	}

	function getImageSize( $filename ) {
		if ( !$this->getHandler() ) {
			return false;
		}
		return $this->handler->getImageSize( $this, $this->getPath() );
	}

	function getMetadata() {
		if ( !isset( $this->metadata ) ) {
			if ( !$this->getHandler() ) {
				$this->metadata = false;
			} else {
				$this->metadata = $this->handler->getMetadata( $this, $this->getPath() );
			}
		}
		return $this->metadata;
	}

	function getURL() {
		if ( $this->repo ) {
			return $this->repo->getZoneUrl( 'public' ) . '/' . $this->repo->getHashPath( $this->name ) . urlencode( $this->name );
		} else {
			return false;
		}
	}

	function getSize() {
		if ( file_exists( $this->path ) ) {
			return filesize( $this->path );
		} else {
			return false;
		}
	}
}
