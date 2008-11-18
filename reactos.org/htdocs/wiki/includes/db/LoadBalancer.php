<?php
/**
 * @file
 * @ingroup Database
 */

/**
 * Database load balancing object
 *
 * @todo document
 * @ingroup Database
 */
class LoadBalancer {
	/* private */ var $mServers, $mConns, $mLoads, $mGroupLoads;
	/* private */ var $mFailFunction, $mErrorConnection;
	/* private */ var $mReadIndex, $mLastIndex, $mAllowLagged;
	/* private */ var $mWaitForPos, $mWaitTimeout;
	/* private */ var $mLaggedSlaveMode, $mLastError = 'Unknown error';
	/* private */ var $mParentInfo, $mLagTimes;
	/* private */ var $mLoadMonitorClass, $mLoadMonitor;

	/**
	 * @param array $params Array with keys:
	 *    servers           Required. Array of server info structures.
	 *    failFunction	    Deprecated, use exceptions instead.
	 *    masterWaitTimeout Replication lag wait timeout
	 *    loadMonitor       Name of a class used to fetch server lag and load.
	 */
	function __construct( $params )
	{
		if ( !isset( $params['servers'] ) ) {
			throw new MWException( __CLASS__.': missing servers parameter' );
		}
		$this->mServers = $params['servers'];

		if ( isset( $params['failFunction'] ) ) {
			$this->mFailFunction = $params['failFunction'];
		} else {
			$this->mFailFunction = false;
		}
		if ( isset( $params['waitTimeout'] ) ) {
			$this->mWaitTimeout = $params['waitTimeout'];
		} else {
			$this->mWaitTimeout = 10;
		}

		$this->mReadIndex = -1;
		$this->mWriteIndex = -1;
		$this->mConns = array(
			'local' => array(),
			'foreignUsed' => array(),
			'foreignFree' => array() );
		$this->mLastIndex = -1;
		$this->mLoads = array();
		$this->mWaitForPos = false;
		$this->mLaggedSlaveMode = false;
		$this->mErrorConnection = false;
		$this->mAllowLag = false;
		$this->mLoadMonitorClass = isset( $params['loadMonitor'] ) 
			? $params['loadMonitor'] : 'LoadMonitor_MySQL';

		foreach( $params['servers'] as $i => $server ) {
			$this->mLoads[$i] = $server['load'];
			if ( isset( $server['groupLoads'] ) ) {
				foreach ( $server['groupLoads'] as $group => $ratio ) {
					if ( !isset( $this->mGroupLoads[$group] ) ) {
						$this->mGroupLoads[$group] = array();
					}
					$this->mGroupLoads[$group][$i] = $ratio;
				}
			}
		}
	}

	static function newFromParams( $servers, $failFunction = false, $waitTimeout = 10 )
	{
		return new LoadBalancer( $servers, $failFunction, $waitTimeout );
	}

	/**
	 * Get a LoadMonitor instance
	 */
	function getLoadMonitor() {
		if ( !isset( $this->mLoadMonitor ) ) {
			$class = $this->mLoadMonitorClass;
			$this->mLoadMonitor = new $class( $this );
		}
		return $this->mLoadMonitor;
	}

	/**
	 * Get or set arbitrary data used by the parent object, usually an LBFactory
	 */
	function parentInfo( $x = null ) {
		return wfSetVar( $this->mParentInfo, $x );
	}

	/**
	 * Given an array of non-normalised probabilities, this function will select
	 * an element and return the appropriate key
	 */
	function pickRandom( $weights )
	{
		if ( !is_array( $weights ) || count( $weights ) == 0 ) {
			return false;
		}

		$sum = array_sum( $weights );
		if ( $sum == 0 ) {
			# No loads on any of them
			# In previous versions, this triggered an unweighted random selection,
			# but this feature has been removed as of April 2006 to allow for strict
			# separation of query groups.
			return false;
		}
		$max = mt_getrandmax();
		$rand = mt_rand(0, $max) / $max * $sum;

		$sum = 0;
		foreach ( $weights as $i => $w ) {
			$sum += $w;
			if ( $sum >= $rand ) {
				break;
			}
		}
		return $i;
	}

	function getRandomNonLagged( $loads, $wiki = false ) {
		# Unset excessively lagged servers
		$lags = $this->getLagTimes( $wiki );
		foreach ( $lags as $i => $lag ) {
			if ( $i != 0 && isset( $this->mServers[$i]['max lag'] ) ) {
				if ( $lag === false ) {
					wfDebug( "Server #$i is not replicating\n" );
					unset( $loads[$i] );
				} elseif ( $lag > $this->mServers[$i]['max lag'] ) {
					wfDebug( "Server #$i is excessively lagged ($lag seconds)\n" );
					unset( $loads[$i] );
				}
			}
		}

		# Find out if all the slaves with non-zero load are lagged
		$sum = 0;
		foreach ( $loads as $load ) {
			$sum += $load;
		}
		if ( $sum == 0 ) {
			# No appropriate DB servers except maybe the master and some slaves with zero load
			# Do NOT use the master
			# Instead, this function will return false, triggering read-only mode,
			# and a lagged slave will be used instead.
			return false;
		}

		if ( count( $loads ) == 0 ) {
			return false;
		}

		#wfDebugLog( 'connect', var_export( $loads, true ) );

		# Return a random representative of the remainder
		return $this->pickRandom( $loads );
	}

	/**
	 * Get the index of the reader connection, which may be a slave
	 * This takes into account load ratios and lag times. It should
	 * always return a consistent index during a given invocation
	 *
	 * Side effect: opens connections to databases
	 */
	function getReaderIndex( $group = false, $wiki = false ) {
		global $wgReadOnly, $wgDBClusterTimeout, $wgDBAvgStatusPoll, $wgDBtype;

		# FIXME: For now, only go through all this for mysql databases
		if ($wgDBtype != 'mysql') {
			return $this->getWriterIndex();
		}

		if ( count( $this->mServers ) == 1 )  {
			# Skip the load balancing if there's only one server
			return 0;
		} elseif ( $group === false and $this->mReadIndex >= 0 ) {
			# Shortcut if generic reader exists already
			return $this->mReadIndex;
		}

		wfProfileIn( __METHOD__ );

		$totalElapsed = 0;

		# convert from seconds to microseconds
		$timeout = $wgDBClusterTimeout * 1e6;

		# Find the relevant load array
		if ( $group !== false ) {
			if ( isset( $this->mGroupLoads[$group] ) ) {
				$nonErrorLoads = $this->mGroupLoads[$group];
			} else {
				# No loads for this group, return false and the caller can use some other group
				wfDebug( __METHOD__.": no loads for group $group\n" );
				wfProfileOut( __METHOD__ );
				return false;
			}
		} else {
			$nonErrorLoads = $this->mLoads;
		}

		if ( !$nonErrorLoads ) {
			throw new MWException( "Empty server array given to LoadBalancer" );
		}

		# Scale the configured load ratios according to the dynamic load (if the load monitor supports it)
		$this->getLoadMonitor()->scaleLoads( $nonErrorLoads, $group, $wiki );

		$i = false;
		$found = false;
		$laggedSlaveMode = false;

		# First try quickly looking through the available servers for a server that
		# meets our criteria
		do {
			$totalThreadsConnected = 0;
			$overloadedServers = 0;
			$currentLoads = $nonErrorLoads;
			while ( count( $currentLoads ) ) {
				if ( $wgReadOnly || $this->mAllowLagged || $laggedSlaveMode ) {
					$i = $this->pickRandom( $currentLoads );
				} else {
					$i = $this->getRandomNonLagged( $currentLoads, $wiki );
					if ( $i === false && count( $currentLoads ) != 0 )  {
						# All slaves lagged. Switch to read-only mode
						$wgReadOnly = wfMsgNoDBForContent( 'readonly_lag' );
						$i = $this->pickRandom( $currentLoads );
						$laggedSlaveMode = true;
					}
				}

				if ( $i === false ) {
					# pickRandom() returned false
					# This is permanent and means the configuration or the load monitor 
					# wants us to return false.
					wfDebugLog( 'connect', __METHOD__.": pickRandom() returned false\n" );
					wfProfileOut( __METHOD__ );
					return false;
				}

				wfDebugLog( 'connect', __METHOD__.": Using reader #$i: {$this->mServers[$i]['host']}...\n" );
				$conn = $this->openConnection( $i, $wiki );

				if ( !$conn ) {
					wfDebugLog( 'connect', __METHOD__.": Failed connecting to $i/$wiki\n" );
					unset( $nonErrorLoads[$i] );
					unset( $currentLoads[$i] );
					continue;
				}

				// Perform post-connection backoff
				$threshold = isset( $this->mServers[$i]['max threads'] ) 
					? $this->mServers[$i]['max threads'] : false;
				$backoff = $this->getLoadMonitor()->postConnectionBackoff( $conn, $threshold );

				// Decrement reference counter, we are finished with this connection.
				// It will be incremented for the caller later.
				if ( $wiki !== false ) {
					$this->reuseConnection( $conn );
				}
				
				if ( $backoff ) {
					# Post-connection overload, don't use this server for now
					$totalThreadsConnected += $backoff;
					$overloadedServers++;
					unset( $currentLoads[$i] );
				} else {
					# Return this server
					break 2;
				}
			}

			# No server found yet
			$i = false;

			# If all servers were down, quit now
			if ( !count( $nonErrorLoads ) ) {
				wfDebugLog( 'connect', "All servers down\n" );
				break;
			}

			# Some servers must have been overloaded
			if ( $overloadedServers == 0 ) {
				throw new MWException( __METHOD__.": unexpectedly found no overloaded servers" );
			}
			# Back off for a while
			# Scale the sleep time by the number of connected threads, to produce a
			# roughly constant global poll rate
			$avgThreads = $totalThreadsConnected / $overloadedServers;
			$totalElapsed += $this->sleep( $wgDBAvgStatusPoll * $avgThreads );
		} while ( $totalElapsed < $timeout );

		if ( $totalElapsed >= $timeout ) {
			wfDebugLog( 'connect', "All servers busy\n" );
			$this->mErrorConnection = false;
			$this->mLastError = 'All servers busy';
		}

		if ( $i !== false ) {
			# Slave connection successful
			# Wait for the session master pos for a short time
			if ( $this->mWaitForPos && $i > 0 ) {
				if ( !$this->doWait( $i ) ) {
					$this->mServers[$i]['slave pos'] = $conn->getSlavePos();
				}
			}
			if ( $this->mReadIndex <=0 && $this->mLoads[$i]>0 && $i !== false ) {
				$this->mReadIndex = $i;
			}
		}
		wfProfileOut( __METHOD__ );
		return $i;
	}

	/**
	 * Wait for a specified number of microseconds, and return the period waited
	 */
	function sleep( $t ) {
		wfProfileIn( __METHOD__ );
		wfDebug( __METHOD__.": waiting $t us\n" );
		usleep( $t );
		wfProfileOut( __METHOD__ );
		return $t;
	}

	/**
	 * Get a random server to use in a query group
	 * @deprecated use getReaderIndex
	 */
	function getGroupIndex( $group ) {
		return $this->getReaderIndex( $group );
	}

	/**
	 * Set the master wait position
	 * If a DB_SLAVE connection has been opened already, waits
	 * Otherwise sets a variable telling it to wait if such a connection is opened
	 */
	public function waitFor( $pos ) {
		wfProfileIn( __METHOD__ );
		$this->mWaitForPos = $pos;
		$i = $this->mReadIndex;

		if ( $i > 0 ) {
			if ( !$this->doWait( $i ) ) {
				$this->mServers[$i]['slave pos'] = $this->getAnyOpenConnection( $i )->getSlavePos();
				$this->mLaggedSlaveMode = true;
			}
		}
		wfProfileOut( __METHOD__ );
	}

	/**
	 * Get any open connection to a given server index, local or foreign
	 * Returns false if there is no connection open
	 */
	function getAnyOpenConnection( $i ) {
		foreach ( $this->mConns as $type => $conns ) {
			if ( !empty( $conns[$i] ) ) {
				return reset( $conns[$i] );
			}
		}
		return false;
	}

	/**
	 * Wait for a given slave to catch up to the master pos stored in $this
	 */
	function doWait( $index ) {
		# Find a connection to wait on
		$conn = $this->getAnyOpenConnection( $index );
		if ( !$conn ) {
			wfDebug( __METHOD__ . ": no connection open\n" );
			return false;
		}

		wfDebug( __METHOD__.": Waiting for slave #$index to catch up...\n" );
		$result = $conn->masterPosWait( $this->mWaitForPos, $this->mWaitTimeout );

		if ( $result == -1 || is_null( $result ) ) {
			# Timed out waiting for slave, use master instead
			wfDebug( __METHOD__.": Timed out waiting for slave #$index pos {$this->mWaitForPos}\n" );
			return false;
		} else {
			wfDebug( __METHOD__.": Done\n" );
			return true;
		}
	}

	/**
	 * Get a connection by index
	 * This is the main entry point for this class.
	 */
	public function &getConnection( $i, $groups = array(), $wiki = false ) {
		global $wgDBtype;
		wfProfileIn( __METHOD__ );

		if ( $wiki === wfWikiID() ) {
			$wiki = false;
		}

		# Query groups
		if ( $i == DB_MASTER ) {
			$i = $this->getWriterIndex();
		} elseif ( !is_array( $groups ) ) {
			$groupIndex = $this->getReaderIndex( $groups, $wiki );
			if ( $groupIndex !== false ) {
				$serverName = $this->getServerName( $groupIndex );
				wfDebug( __METHOD__.": using server $serverName for group $groups\n" );
				$i = $groupIndex;
			}
		} else {
			foreach ( $groups as $group ) {
				$groupIndex = $this->getReaderIndex( $group, $wiki );
				if ( $groupIndex !== false ) {
					$serverName = $this->getServerName( $groupIndex );
					wfDebug( __METHOD__.": using server $serverName for group $group\n" );
					$i = $groupIndex;
					break;
				}
			}
		}

		# Operation-based index
		if ( $i == DB_SLAVE ) {
			$i = $this->getReaderIndex( false, $wiki );
		} elseif ( $i == DB_LAST ) {
			# Just use $this->mLastIndex, which should already be set
			$i = $this->mLastIndex;
			if ( $i === -1 ) {
				# Oh dear, not set, best to use the writer for safety
				wfDebug( "Warning: DB_LAST used when there was no previous index\n" );
				$i = $this->getWriterIndex();
			}
		}
		# Couldn't find a working server in getReaderIndex()?
		if ( $i === false ) {
			$this->reportConnectionError( $this->mErrorConnection );
		}

		# Now we have an explicit index into the servers array
		$conn = $this->openConnection( $i, $wiki );
		if ( !$conn ) {
			$this->reportConnectionError( $this->mErrorConnection );
		}

		wfProfileOut( __METHOD__ );
		return $conn;
	}

	/**
	 * Mark a foreign connection as being available for reuse under a different
	 * DB name or prefix. This mechanism is reference-counted, and must be called
	 * the same number of times as getConnection() to work.
	 */
	public function reuseConnection( $conn ) {
		$serverIndex = $conn->getLBInfo('serverIndex');
		$refCount = $conn->getLBInfo('foreignPoolRefCount');
		$dbName = $conn->getDBname();
		$prefix = $conn->tablePrefix();
		if ( strval( $prefix ) !== '' ) {
			$wiki = "$dbName-$prefix";
		} else {
			$wiki = $dbName;
		}
		if ( $serverIndex === null || $refCount === null ) {
			wfDebug( __METHOD__.": this connection was not opened as a foreign connection\n" );
			/**
			 * This can happen in code like:
			 *   foreach ( $dbs as $db ) {
			 *     $conn = $lb->getConnection( DB_SLAVE, array(), $db );
			 *     ...
			 *     $lb->reuseConnection( $conn );
			 *   }
			 * When a connection to the local DB is opened in this way, reuseConnection()
			 * should be ignored
			 */
			return;
		}
		if ( $this->mConns['foreignUsed'][$serverIndex][$wiki] !== $conn ) {
			throw new MWException( __METHOD__.": connection not found, has the connection been freed already?" );
		}
		$conn->setLBInfo( 'foreignPoolRefCount', --$refCount );
		if ( $refCount <= 0 ) {
			$this->mConns['foreignFree'][$serverIndex][$wiki] = $conn;
			unset( $this->mConns['foreignUsed'][$serverIndex][$wiki] );
			wfDebug( __METHOD__.": freed connection $serverIndex/$wiki\n" );
		} else {
			wfDebug( __METHOD__.": reference count for $serverIndex/$wiki reduced to $refCount\n" );
		}
	}

	/**
	 * Open a connection to the server given by the specified index
	 * Index must be an actual index into the array.
	 * If the server is already open, returns it.
	 *
	 * On error, returns false, and the connection which caused the
	 * error will be available via $this->mErrorConnection.
	 *
	 * @param integer $i Server index
	 * @param string $wiki Wiki ID to open
	 * @return Database
	 *
	 * @access private
	 */
	function openConnection( $i, $wiki = false ) {
		wfProfileIn( __METHOD__ );
		if ( $wiki !== false ) {
			$conn = $this->openForeignConnection( $i, $wiki );
			wfProfileOut( __METHOD__);
			return $conn;
		}
		if ( isset( $this->mConns['local'][$i][0] ) ) {
			$conn = $this->mConns['local'][$i][0];
		} else {
			$server = $this->mServers[$i];
			$server['serverIndex'] = $i;
			$conn = $this->reallyOpenConnection( $server );
			if ( $conn->isOpen() ) {
				$this->mConns['local'][$i][0] = $conn;
			} else {
				wfDebug( "Failed to connect to database $i at {$this->mServers[$i]['host']}\n" );
				$this->mErrorConnection = $conn;
				$conn = false;
			}
		}
		$this->mLastIndex = $i;
		wfProfileOut( __METHOD__ );
		return $conn;
	}

	/**
	 * Open a connection to a foreign DB, or return one if it is already open.
	 *
	 * Increments a reference count on the returned connection which locks the
	 * connection to the requested wiki. This reference count can be
	 * decremented by calling reuseConnection().
	 *
	 * If a connection is open to the appropriate server already, but with the wrong
	 * database, it will be switched to the right database and returned, as long as
	 * it has been freed first with reuseConnection().
	 *
	 * On error, returns false, and the connection which caused the
	 * error will be available via $this->mErrorConnection.
	 *
	 * @param integer $i Server index
	 * @param string $wiki Wiki ID to open
	 * @return Database
	 */
	function openForeignConnection( $i, $wiki ) {
		wfProfileIn(__METHOD__);
		list( $dbName, $prefix ) = wfSplitWikiID( $wiki );
		if ( isset( $this->mConns['foreignUsed'][$i][$wiki] ) ) {
			// Reuse an already-used connection
			$conn = $this->mConns['foreignUsed'][$i][$wiki];
			wfDebug( __METHOD__.": reusing connection $i/$wiki\n" );
		} elseif ( isset( $this->mConns['foreignFree'][$i][$wiki] ) ) {
			// Reuse a free connection for the same wiki
			$conn = $this->mConns['foreignFree'][$i][$wiki];
			unset( $this->mConns['foreignFree'][$i][$wiki] );
			$this->mConns['foreignUsed'][$i][$wiki] = $conn;
			wfDebug( __METHOD__.": reusing free connection $i/$wiki\n" );
		} elseif ( !empty( $this->mConns['foreignFree'][$i] ) ) {
			// Reuse a connection from another wiki
			$conn = reset( $this->mConns['foreignFree'][$i] );
			$oldWiki = key( $this->mConns['foreignFree'][$i] );

			if ( !$conn->selectDB( $dbName ) ) {
				global $wguname;
				$this->mLastError = "Error selecting database $dbName on server " .
					$conn->getServer() . " from client host {$wguname['nodename']}\n";
				$this->mErrorConnection = $conn;
				$conn = false;
			} else {
				$conn->tablePrefix( $prefix );
				unset( $this->mConns['foreignFree'][$i][$oldWiki] );
				$this->mConns['foreignUsed'][$i][$wiki] = $conn;
				wfDebug( __METHOD__.": reusing free connection from $oldWiki for $wiki\n" );
			}
		} else {
			// Open a new connection
			$server = $this->mServers[$i];
			$server['serverIndex'] = $i;
			$server['foreignPoolRefCount'] = 0;
			$conn = $this->reallyOpenConnection( $server, $dbName );
			if ( !$conn->isOpen() ) {
				wfDebug( __METHOD__.": error opening connection for $i/$wiki\n" );
				$this->mErrorConnection = $conn;
				$conn = false;
			} else {
				$this->mConns['foreignUsed'][$i][$wiki] = $conn;
				wfDebug( __METHOD__.": opened new connection for $i/$wiki\n" );
			}
		}

		// Increment reference count
		if ( $conn ) {
			$refCount = $conn->getLBInfo( 'foreignPoolRefCount' );
			$conn->setLBInfo( 'foreignPoolRefCount', $refCount + 1 );
		}
		wfProfileOut(__METHOD__);
		return $conn;
	}

	/**
	 * Test if the specified index represents an open connection
	 * @access private
	 */
	function isOpen( $index ) {
		if( !is_integer( $index ) ) {
			return false;
		}
		return (bool)$this->getAnyOpenConnection( $index );
	}

	/**
	 * Really opens a connection. Uncached.
	 * Returns a Database object whether or not the connection was successful.
	 * @access private
	 */
	function reallyOpenConnection( $server, $dbNameOverride = false ) {
		if( !is_array( $server ) ) {
			throw new MWException( 'You must update your load-balancing configuration. See DefaultSettings.php entry for $wgDBservers.' );
		}

		extract( $server );
		if ( $dbNameOverride !== false ) {
			$dbname = $dbNameOverride;
		}

		# Get class for this database type
		$class = 'Database' . ucfirst( $type );

		# Create object
		wfDebug( "Connecting to $host $dbname...\n" );
		$db = new $class( $host, $user, $password, $dbname, 1, $flags );
		if ( $db->isOpen() ) {
			wfDebug( "Connected\n" );
		} else {
			wfDebug( "Failed\n" );
		}
		$db->setLBInfo( $server );
		if ( isset( $server['fakeSlaveLag'] ) ) {
			$db->setFakeSlaveLag( $server['fakeSlaveLag'] );
		}
		if ( isset( $server['fakeMaster'] ) ) {
			$db->setFakeMaster( true );
		}
		return $db;
	}

	function reportConnectionError( &$conn ) {
		wfProfileIn( __METHOD__ );
		# Prevent infinite recursion

		static $reporting = false;
		if ( !$reporting ) {
			$reporting = true;
			if ( !is_object( $conn ) ) {
				// No last connection, probably due to all servers being too busy
				$conn = new Database;
				if ( $this->mFailFunction ) {
					$conn->failFunction( $this->mFailFunction );
					$conn->reportConnectionError( $this->mLastError );
				} else {
					// If all servers were busy, mLastError will contain something sensible
					throw new DBConnectionError( $conn, $this->mLastError );
				}
			} else {
				if ( $this->mFailFunction ) {
					$conn->failFunction( $this->mFailFunction );
				} else {
					$conn->failFunction( false );
				}
				$server = $conn->getProperty( 'mServer' );
				$conn->reportConnectionError( "{$this->mLastError} ({$server})" );
			}
			$reporting = false;
		}
		wfProfileOut( __METHOD__ );
	}

	function getWriterIndex() {
		return 0;
	}

	/**
	 * Returns true if the specified index is a valid server index
	 */
	function haveIndex( $i ) {
		return array_key_exists( $i, $this->mServers );
	}

	/**
	 * Returns true if the specified index is valid and has non-zero load
	 */
	function isNonZeroLoad( $i ) {
		return array_key_exists( $i, $this->mServers ) && $this->mLoads[$i] != 0;
	}

	/**
	 * Get the number of defined servers (not the number of open connections)
	 */
	function getServerCount() {
		return count( $this->mServers );
	}

	/**
	 * Get the host name or IP address of the server with the specified index
	 * Prefer a readable name if available.
	 */
	function getServerName( $i ) {
		if ( isset( $this->mServers[$i]['hostName'] ) ) {
			return $this->mServers[$i]['hostName'];
		} elseif ( isset( $this->mServers[$i]['host'] ) ) {
			return $this->mServers[$i]['host'];
		} else {
			return '';
		}
	}

	/**
	 * Return the server info structure for a given index, or false if the index is invalid.
	 */
	function getServerInfo( $i ) {
		if ( isset( $this->mServers[$i] ) ) {
			return $this->mServers[$i];
		} else {
			return false;
		}
	}

	/**
	 * Get the current master position for chronology control purposes
	 * @return mixed
	 */
	function getMasterPos() {
		# If this entire request was served from a slave without opening a connection to the
		# master (however unlikely that may be), then we can fetch the position from the slave.
		$masterConn = $this->getAnyOpenConnection( 0 );
		if ( !$masterConn ) {
			for ( $i = 1; $i < count( $this->mServers ); $i++ ) {
				$conn = $this->getAnyOpenConnection( $i );
				if ( $conn ) {
					wfDebug( "Master pos fetched from slave\n" );
					return $conn->getSlavePos();
				}
			}
		} else {
			wfDebug( "Master pos fetched from master\n" );
			return $masterConn->getMasterPos();
		}
		return false;
	}

	/**
	 * Close all open connections
	 */
	function closeAll() {
		foreach ( $this->mConns as $conns2 ) {
			foreach  ( $conns2 as $conns3 ) {
				foreach ( $conns3 as $conn ) {
					$conn->close();
				}
			}
		}
		$this->mConns = array(
			'local' => array(),
			'foreignFree' => array(),
			'foreignUsed' => array(),
		);
	}

	/**
	 * Close a connection
	 * Using this function makes sure the LoadBalancer knows the connection is closed.
	 * If you use $conn->close() directly, the load balancer won't update its state.
	 */
	function closeConnecton( $conn ) {
		$done = false;
		foreach ( $this->mConns as $i1 => $conns2 ) {
			foreach ( $conns2 as $i2 => $conns3 ) {
				foreach ( $conns3 as $i3 => $candidateConn ) {
					if ( $conn === $candidateConn ) {
						$conn->close();
						unset( $this->mConns[$i1][$i2][$i3] );
						$done = true;
						break;
					}
				}
			}
		}
		if ( !$done ) {
			$conn->close();
		}
	}

	/**
	 * Commit transactions on all open connections
	 */
	function commitAll() {
		foreach ( $this->mConns as $conns2 ) {
			foreach ( $conns2 as $conns3 ) {
				foreach ( $conns3 as $conn ) {
					$conn->immediateCommit();
				}
			}
		}
	}

	/* Issue COMMIT only on master, only if queries were done on connection */
	function commitMasterChanges() {
		// Always 0, but who knows.. :)
		$masterIndex = $this->getWriterIndex();
		foreach ( $this->mConns as $type => $conns2 ) {
			if ( empty( $conns2[$masterIndex] ) ) {
				continue;
			}
			foreach ( $conns2[$masterIndex] as $conn ) {
				if ( $conn->lastQuery() != '' ) {
					$conn->commit();
				}
			}
		}
	}

	function waitTimeout( $value = NULL ) {
		return wfSetVar( $this->mWaitTimeout, $value );
	}

	function getLaggedSlaveMode() {
		return $this->mLaggedSlaveMode;
	}

	/* Disables/enables lag checks */
	function allowLagged($mode=null) {
		if ($mode===null)
			return $this->mAllowLagged;
		$this->mAllowLagged=$mode;
	}

	function pingAll() {
		$success = true;
		foreach ( $this->mConns as $conns2 ) {
			foreach ( $conns2 as $conns3 ) {
				foreach ( $conns3 as $conn ) {
					if ( !$conn->ping() ) {
						$success = false;
					}
				}
			}
		}
		return $success;
	}

	/**
	 * Call a function with each open connection object
	 */
	function forEachOpenConnection( $callback, $params = array() ) {
		foreach ( $this->mConns as $conns2 ) {
			foreach ( $conns2 as $conns3 ) {
				foreach ( $conns3 as $conn ) {
					$mergedParams = array_merge( array( $conn ), $params );
					call_user_func_array( $callback, $mergedParams );
				}
			}
		}
	}

	/**
	 * Get the hostname and lag time of the most-lagged slave.
	 * This is useful for maintenance scripts that need to throttle their updates.
	 * May attempt to open connections to slaves on the default DB.
	 */
	function getMaxLag() {
		$maxLag = -1;
		$host = '';
		foreach ( $this->mServers as $i => $conn ) {
			$conn = $this->getAnyOpenConnection( $i );
			if ( !$conn ) {
				$conn = $this->openConnection( $i );
			}
			if ( !$conn ) {
				continue;
			}
			$lag = $conn->getLag();
			if ( $lag > $maxLag ) {
				$maxLag = $lag;
				$host = $this->mServers[$i]['host'];
			}
		}
		return array( $host, $maxLag );
	}

	/**
	 * Get lag time for each server
	 * Results are cached for a short time in memcached, and indefinitely in the process cache
	 */
	function getLagTimes( $wiki = false ) {
		# Try process cache
		if ( isset( $this->mLagTimes ) ) {
			return $this->mLagTimes;
		}
		# No, send the request to the load monitor
		$this->mLagTimes = $this->getLoadMonitor()->getLagTimes( array_keys( $this->mServers ), $wiki );
		return $this->mLagTimes;
	}
}
