<?php

/**
 * A foreign repository with a MediaWiki database accessible via the configured LBFactory
 * @ingroup FileRepo
 */
class ForeignDBViaLBRepo extends LocalRepo {
	var $wiki, $dbName, $tablePrefix;
	var $fileFactory = array( 'ForeignDBFile', 'newFromTitle' );
	var $fileFromRowFactory = array( 'ForeignDBFile', 'newFromRow' );

	function __construct( $info ) {
		parent::__construct( $info );
		$this->wiki = $info['wiki'];
		list( $this->dbName, $this->tablePrefix ) = wfSplitWikiID( $this->wiki );
		$this->hasSharedCache = $info['hasSharedCache'];
	}

	function getMasterDB() {
		return wfGetDB( DB_MASTER, array(), $this->wiki );
	}

	function getSlaveDB() {
		return wfGetDB( DB_SLAVE, array(), $this->wiki );
	}
	function hasSharedCache() {
		return $this->hasSharedCache;
	}

	function store( $srcPath, $dstZone, $dstRel, $flags = 0 ) {
		throw new MWException( get_class($this) . ': write operations are not supported' );
	}
	function publish( $srcPath, $dstRel, $archiveRel, $flags = 0 ) {
		throw new MWException( get_class($this) . ': write operations are not supported' );
	}
	function deleteBatch( $fileMap ) {
		throw new MWException( get_class($this) . ': write operations are not supported' );
	}
}
