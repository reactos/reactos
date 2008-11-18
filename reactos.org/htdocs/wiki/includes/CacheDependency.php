<?php

/**
 * This class stores an arbitrary value along with its dependencies.
 * Users should typically only use DependencyWrapper::getFromCache(), rather
 * than instantiating one of these objects directly.
 * @ingroup Cache
 */
class DependencyWrapper {
	var $value;
	var $deps;

	/**
	 * Create an instance.
	 * @param mixed $value The user-supplied value
	 * @param mixed $deps A dependency or dependency array. All dependencies
	 *        must be objects implementing CacheDependency.
	 */
	function __construct( $value = false, $deps = array() ) {
		$this->value = $value;
		if ( !is_array( $deps ) ) {
			$deps = array( $deps );
		}
		$this->deps = $deps;
	}

	/**
	 * Returns true if any of the dependencies have expired
	 */
	function isExpired() {
		foreach ( $this->deps as $dep ) {
			if ( $dep->isExpired() ) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Initialise dependency values in preparation for storing. This must be
	 * called before serialization.
	 */
	function initialiseDeps() {
		foreach ( $this->deps as $dep ) {
			$dep->loadDependencyValues();
		}
	}

	/**
	 * Get the user-defined value
	 */
	function getValue() {
		return $this->value;
	}

	/**
	 * Store the wrapper to a cache
	 */
	function storeToCache( $cache, $key, $expiry = 0 ) {
		$this->initialiseDeps();
		$cache->set( $key, $this, $expiry );
	}

	/**
	 * Attempt to get a value from the cache. If the value is expired or missing,
	 * it will be generated with the callback function (if present), and the newly
	 * calculated value will be stored to the cache in a wrapper.
	 *
	 * @param object $cache A cache object such as $wgMemc
	 * @param string $key The cache key
	 * @param integer $expiry The expiry timestamp or interval in seconds
	 * @param mixed $callback The callback for generating the value, or false
	 * @param array $callbackParams The function parameters for the callback
	 * @param array $deps The dependencies to store on a cache miss. Note: these
	 *    are not the dependencies used on a cache hit! Cache hits use the stored
	 *    dependency array.
	 *
	 * @return mixed The value, or null if it was not present in the cache and no
	 *    callback was defined.
	 */
	static function getValueFromCache( $cache, $key, $expiry = 0, $callback = false,
		$callbackParams = array(), $deps = array() )
	{
		$obj = $cache->get( $key );
		if ( is_object( $obj ) && $obj instanceof DependencyWrapper && !$obj->isExpired() ) {
			$value = $obj->value;
		} elseif ( $callback ) {
			$value = call_user_func_array( $callback, $callbackParams );
			# Cache the newly-generated value
			$wrapper = new DependencyWrapper( $value, $deps );
			$wrapper->storeToCache( $cache, $key, $expiry );
		} else {
			$value = null;
		}
		return $value;
	}
}

/**
 * @ingroup Cache
 */
abstract class CacheDependency {
	/**
	 * Returns true if the dependency is expired, false otherwise
	 */
	abstract function isExpired();

	/**
	 * Hook to perform any expensive pre-serialize loading of dependency values.
	 */
	function loadDependencyValues() {}
}

/**
 * @ingroup Cache
 */
class FileDependency extends CacheDependency {
	var $filename, $timestamp;

	/**
	 * Create a file dependency
	 *
	 * @param string $filename The name of the file, preferably fully qualified
	 * @param mixed $timestamp The unix last modified timestamp, or false if the
	 *        file does not exist. If omitted, the timestamp will be loaded from
	 *        the file.
	 *
	 * A dependency on a nonexistent file will be triggered when the file is
	 * created. A dependency on an existing file will be triggered when the
	 * file is changed.
	 */
	function __construct( $filename, $timestamp = null ) {
		$this->filename = $filename;
		$this->timestamp = $timestamp;
	}

	function loadDependencyValues() {
		if ( is_null( $this->timestamp ) ) {
			if ( !file_exists( $this->filename ) ) {
				# Dependency on a non-existent file
				# This is a valid concept!
				$this->timestamp = false;
			} else {
				$this->timestamp = filemtime( $this->filename );
			}
		}
	}

	function isExpired() {
		if ( !file_exists( $this->filename ) ) {
			if ( $this->timestamp === false ) {
				# Still nonexistent
				return false;
			} else {
				# Deleted
				wfDebug( "Dependency triggered: {$this->filename} deleted.\n" );
				return true;
			}
		} else {
			$lastmod = filemtime( $this->filename );
			if ( $lastmod > $this->timestamp ) {
				# Modified or created
				wfDebug( "Dependency triggered: {$this->filename} changed.\n" );
				return true;
			} else {
				# Not modified
				return false;
			}
		}
	}
}

/**
 * @ingroup Cache
 */
class TitleDependency extends CacheDependency {
	var $titleObj;
	var $ns, $dbk;
	var $touched;

	/**
	 * Construct a title dependency
	 * @param Title $title
	 */
	function __construct( Title $title ) {
		$this->titleObj = $title;
		$this->ns = $title->getNamespace();
		$this->dbk = $title->getDBkey();
	}

	function loadDependencyValues() {
		$this->touched = $this->getTitle()->getTouched();
	}

	/**
	 * Get rid of bulky Title object for sleep
	 */
	function __sleep() {
		return array( 'ns', 'dbk', 'touched' );
	}

	function getTitle() {
		if ( !isset( $this->titleObj ) ) {
			$this->titleObj = Title::makeTitle( $this->ns, $this->dbk );
		}
		return $this->titleObj;
	}

	function isExpired() {
		$touched = $this->getTitle()->getTouched();
		if ( $this->touched === false ) {
			if ( $touched === false ) {
				# Still missing
				return false;
			} else {
				# Created
				return true;
			}
		} elseif ( $touched === false ) {
			# Deleted
			return true;
		} elseif ( $touched > $this->touched ) {
			# Updated
			return true;
		} else {
			# Unmodified
			return false;
		}
	}
}

/**
 * @ingroup Cache
 */
class TitleListDependency extends CacheDependency {
	var $linkBatch;
	var $timestamps;

	/**
	 * Construct a dependency on a list of titles
	 */
	function __construct( LinkBatch $linkBatch ) {
		$this->linkBatch = $linkBatch;
	}

	function calculateTimestamps() {
		# Initialise values to false
		$timestamps = array();
		foreach ( $this->getLinkBatch()->data as $ns => $dbks ) {
			if ( count( $dbks ) > 0 ) {
				$timestamps[$ns] = array();
				foreach ( $dbks as $dbk => $value ) {
					$timestamps[$ns][$dbk] = false;
				}
			}
		}

		# Do the query
		if ( count( $timestamps ) ) {
			$dbr = wfGetDB( DB_SLAVE );
			$where = $this->getLinkBatch()->constructSet( 'page', $dbr );
			$res = $dbr->select( 'page',
				array( 'page_namespace', 'page_title', 'page_touched' ),
				$where, __METHOD__ );
			while ( $row = $dbr->fetchObject( $res ) ) {
				$timestamps[$row->page_namespace][$row->page_title] = $row->page_touched;
			}
		}
		return $timestamps;
	}

	function loadDependencyValues() {
		$this->timestamps = $this->calculateTimestamps();
	}

	function __sleep() {
		return array( 'timestamps' );
	}

	function getLinkBatch() {
		if ( !isset( $this->linkBatch ) ){
			$this->linkBatch = new LinkBatch;
			$this->linkBatch->setArray( $this->timestamps );
		}
		return $this->linkBatch;
	}

	function isExpired() {
		$newTimestamps = $this->calculateTimestamps();
		foreach ( $this->timestamps as $ns => $dbks ) {
			foreach ( $dbks as $dbk => $oldTimestamp ) {
				$newTimestamp = $newTimestamps[$ns][$dbk];
				if ( $oldTimestamp === false ) {
					if ( $newTimestamp === false ) {
						# Still missing
					} else {
						# Created
						return true;
					}
				} elseif ( $newTimestamp === false ) {
					# Deleted
					return true;
				} elseif ( $newTimestamp > $oldTimestamp ) {
					# Updated
					return true;
				} else {
					# Unmodified
				}
			}
		}
		return false;
	}
}

/**
 * @ingroup Cache
 */
class GlobalDependency extends CacheDependency {
	var $name, $value;

	function __construct( $name ) {
		$this->name = $name;
		$this->value = $GLOBALS[$name];
	}

	function isExpired() {
		return $GLOBALS[$this->name] != $this->value;
	}
}

/**
 * @ingroup Cache
 */
class ConstantDependency extends CacheDependency {
	var $name, $value;

	function __construct( $name ) {
		$this->name = $name;
		$this->value = constant( $name );
	}

	function isExpired() {
		return constant( $this->name ) != $this->value;
	}
}
