<?php

/**
 * @ingroup FileRepo
 */
class ForeignDBFile extends LocalFile {
	static function newFromTitle( $title, $repo, $unused = null ) {
		return new self( $title, $repo );
	}

	/**
	 * Create a ForeignDBFile from a title
	 * Do not call this except from inside a repo class.
	 */
	static function newFromRow( $row, $repo ) {
		$title = Title::makeTitle( NS_IMAGE, $row->img_name );
		$file = new self( $title, $repo );
		$file->loadFromRow( $row );
		return $file;
	}

	function getCacheKey() {
		if ( $this->repo->hasSharedCache ) {
			$hashedName = md5($this->name);
			return wfForeignMemcKey( $this->repo->dbName, $this->repo->tablePrefix,
				'file', $hashedName );
		} else {
			return false;
		}
	}

	function publish( $srcPath, $flags = 0 ) {
		$this->readOnlyError();
	}

	function recordUpload( $oldver, $desc, $license = '', $copyStatus = '', $source = '',
		$watch = false, $timestamp = false ) {
		$this->readOnlyError();
	}
	function restore( $versions = array(), $unsuppress = false ) {
		$this->readOnlyError();
	}
	function delete( $reason, $suppress = false ) {
		$this->readOnlyError();
	}
	function move( $target ) {
		$this->readOnlyError();
	}
	
	function getDescriptionUrl() {
		// Restore remote behaviour
		return File::getDescriptionUrl();
	}

	function getDescriptionText() {
		// Restore remote behaviour
		return File::getDescriptionText();
	}
}
