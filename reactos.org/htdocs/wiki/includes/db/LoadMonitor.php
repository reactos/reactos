<?php

/**
 * An interface for database load monitoring
 */

interface LoadMonitor {
	/**
	 * Construct a new LoadMonitor with a given LoadBalancer parent
	 */
	function __construct( $parent );
	
	/**
	 * Perform pre-connection load ratio adjustment.
	 * @param array $loads
	 * @param string $group The selected query group
	 * @param string $wiki
	 */
	function scaleLoads( &$loads, $group = false, $wiki = false );

	/**
	 * Perform post-connection backoff.
	 *
	 * If the connection is in overload, this should return a backoff factor 
	 * which will be used to control polling time. The number of threads 
	 * connected is a good measure.
	 *
	 * If there is no overload, zero can be returned.
	 *
	 * A threshold thread count is given, the concrete class may compare this 
	 * to the running thread count. The threshold may be false, which indicates
	 * that the sysadmin has not configured this feature.
	 *
	 * @param Database $conn
	 * @param float $threshold
	 */
	function postConnectionBackoff( $conn, $threshold );

	/**
	 * Return an estimate of replication lag for each server
	 */
	function getLagTimes( $serverIndexes, $wiki );
}


/**
 * Basic MySQL load monitor with no external dependencies
 * Uses memcached to cache the replication lag for a short time
 */

class LoadMonitor_MySQL implements LoadMonitor {
	var $parent; // LoadBalancer

	function __construct( $parent ) {
		$this->parent = $parent;
	}

	function scaleLoads( &$loads, $group = false, $wiki = false ) {
	}

	function getLagTimes( $serverIndexes, $wiki ) {
		wfProfileIn( __METHOD__ );
		$expiry = 5;
		$requestRate = 10;

		global $wgMemc;
		$masterName = $this->parent->getServerName( 0 );
		$memcKey = wfMemcKey( 'lag_times', $masterName );
		$times = $wgMemc->get( $memcKey );
		if ( $times ) {
			# Randomly recache with probability rising over $expiry
			$elapsed = time() - $times['timestamp'];
			$chance = max( 0, ( $expiry - $elapsed ) * $requestRate );
			if ( mt_rand( 0, $chance ) != 0 ) {
				unset( $times['timestamp'] );
				wfProfileOut( __METHOD__ );
				return $times;
			}
			wfIncrStats( 'lag_cache_miss_expired' );
		} else {
			wfIncrStats( 'lag_cache_miss_absent' );
		}

		# Cache key missing or expired

		$times = array();
		foreach ( $serverIndexes as $i ) {
			if ($i == 0) { # Master
				$times[$i] = 0;
			} elseif ( false !== ( $conn = $this->parent->getAnyOpenConnection( $i ) ) ) {
				$times[$i] = $conn->getLag();
			} elseif ( false !== ( $conn = $this->parent->openConnection( $i, $wiki ) ) ) {
				$times[$i] = $conn->getLag();
			}
		}

		# Add a timestamp key so we know when it was cached
		$times['timestamp'] = time();
		$wgMemc->set( $memcKey, $times, $expiry );

		# But don't give the timestamp to the caller
		unset($times['timestamp']);
		$lagTimes = $times;

		wfProfileOut( __METHOD__ );
		return $lagTimes;
	}

	function postConnectionBackoff( $conn, $threshold ) {
		if ( !$threshold ) {
			return 0;
		}
		$status = $conn->getStatus("Thread%");
		if ( $status['Threads_running'] > $threshold ) {
			return $status['Threads_connected'];
		} else {
			return 0;
		}
	}
}

