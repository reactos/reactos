<?php
/**
 * @defgroup ExternalStorage ExternalStorage
 */

/**
 * Constructor class for data kept in external repositories
 *
 * External repositories might be populated by maintenance/async
 * scripts, thus partial moving of data may be possible, as well
 * as possibility to have any storage format (i.e. for archives)
 *
 * @ingroup ExternalStorage
 */
class ExternalStore {
	/* Fetch data from given URL */
	static function fetchFromURL($url) {
		global $wgExternalStores;

		if( !$wgExternalStores )
			return false;

		@list( $proto, $path ) = explode( '://', $url, 2 );
		/* Bad URL */
		if( $path == '' )
			return false;

		$store = self::getStoreObject( $proto );
		if ( $store === false )
			return false;
		return $store->fetchFromURL( $url );
	}

	/**
	 * Get an external store object of the given type
	 */
	static function getStoreObject( $proto ) {
		global $wgExternalStores;
		if( !$wgExternalStores )
			return false;
		/* Protocol not enabled */
		if( !in_array( $proto, $wgExternalStores ) )
			return false;

		$class = 'ExternalStore' . ucfirst( $proto );
		/* Any custom modules should be added to $wgAutoLoadClasses for on-demand loading */
		if( !class_exists( $class ) ){
			return false;
		}

		return new $class();
	}

	/**
	 * Store a data item to an external store, identified by a partial URL
	 * The protocol part is used to identify the class, the rest is passed to the
	 * class itself as a parameter.
	 * Returns the URL of the stored data item, or false on error
	 */
	static function insert( $url, $data ) {
		list( $proto, $params ) = explode( '://', $url, 2 );
		$store = self::getStoreObject( $proto );
		if ( $store === false ) {
			return false;
		} else {
			return $store->store( $params, $data );
		}
	}
}
