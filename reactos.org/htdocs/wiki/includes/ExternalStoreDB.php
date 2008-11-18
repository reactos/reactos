<?php

/**
 * External database storage will use one (or more) separate connection pools
 * from what the main wiki uses. If we load many revisions, such as when doing
 * bulk backups or maintenance, we want to keep them around over the lifetime
 * of the script.
 *
 * Associative array of LoadBalancer objects, indexed by cluster name.
 */
global $wgExternalLoadBalancers;
$wgExternalLoadBalancers = array();

/**
 * One-step cache variable to hold base blobs; operations that
 * pull multiple revisions may often pull multiple times from
 * the same blob. By keeping the last-used one open, we avoid
 * redundant unserialization and decompression overhead.
 */
global $wgExternalBlobCache;
$wgExternalBlobCache = array();

/**
 * DB accessable external objects
 * @ingroup ExternalStorage
 */
class ExternalStoreDB {

	/** @todo Document.*/
	function &getLoadBalancer( $cluster ) {
		return wfGetLBFactory()->getExternalLB( $cluster );
	}

	/** @todo Document.*/
	function &getSlave( $cluster ) {
		$lb =& $this->getLoadBalancer( $cluster );
		return $lb->getConnection( DB_SLAVE );
	}

	/** @todo Document.*/
	function &getMaster( $cluster ) {
		$lb =& $this->getLoadBalancer( $cluster );
		return $lb->getConnection( DB_MASTER );
	}

	/** @todo Document.*/
	function getTable( &$db ) {
		$table = $db->getLBInfo( 'blobs table' );
		if ( is_null( $table ) ) {
			$table = 'blobs';
		}
		return $table;
	}

	/**
	 * Fetch data from given URL
	 * @param string $url An url of the form DB://cluster/id or DB://cluster/id/itemid for concatened storage.
	 */
	function fetchFromURL($url) {
		$path = explode( '/', $url );
		$cluster  = $path[2];
		$id	  = $path[3];
		if ( isset( $path[4] ) ) {
			$itemID = $path[4];
		} else {
			$itemID = false;
		}

		$ret =& $this->fetchBlob( $cluster, $id, $itemID );

		if ( $itemID !== false && $ret !== false ) {
			return $ret->getItem( $itemID );
		}
		return $ret;
	}

	/**
	 * Fetch a blob item out of the database; a cache of the last-loaded
	 * blob will be kept so that multiple loads out of a multi-item blob
	 * can avoid redundant database access and decompression.
	 * @param $cluster
	 * @param $id
	 * @param $itemID
	 * @return mixed
	 * @private
	 */
	function &fetchBlob( $cluster, $id, $itemID ) {
		global $wgExternalBlobCache;
		$cacheID = ( $itemID === false ) ? "$cluster/$id" : "$cluster/$id/";
		if( isset( $wgExternalBlobCache[$cacheID] ) ) {
			wfDebug( "ExternalStoreDB::fetchBlob cache hit on $cacheID\n" );
			return $wgExternalBlobCache[$cacheID];
		}

		wfDebug( "ExternalStoreDB::fetchBlob cache miss on $cacheID\n" );

		$dbr =& $this->getSlave( $cluster );
		$ret = $dbr->selectField( $this->getTable( $dbr ), 'blob_text', array( 'blob_id' => $id ) );
		if ( $ret === false ) {
			wfDebugLog( 'ExternalStoreDB', "ExternalStoreDB::fetchBlob master fallback on $cacheID\n" );
			// Try the master
			$dbw =& $this->getMaster( $cluster );
			$ret = $dbw->selectField( $this->getTable( $dbw ), 'blob_text', array( 'blob_id' => $id ) );
			if( $ret === false) {
				wfDebugLog( 'ExternalStoreDB', "ExternalStoreDB::fetchBlob master failed to find $cacheID\n" );
			}
		}
		if( $itemID !== false && $ret !== false ) {
			// Unserialise object; caller extracts item
			$ret = unserialize( $ret );
		}

		$wgExternalBlobCache = array( $cacheID => &$ret );
		return $ret;
	}

	/**
	 * Insert a data item into a given cluster
	 *
	 * @param $cluster String: the cluster name
	 * @param $data String: the data item
	 * @return string URL
	 */
	function store( $cluster, $data ) {
		$fname = 'ExternalStoreDB::store';

		$dbw =& $this->getMaster( $cluster );

		$id = $dbw->nextSequenceValue( 'blob_blob_id_seq' );
		$dbw->insert( $this->getTable( $dbw ), array( 'blob_id' => $id, 'blob_text' => $data ), $fname );
		$id = $dbw->insertId();
		if ( $dbw->getFlag( DBO_TRX ) ) {
			$dbw->immediateCommit();
		}
		return "DB://$cluster/$id";
	}
}
