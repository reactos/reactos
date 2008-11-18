<?php
/**
 * @defgroup Database Database
 *
 * @file
 * @ingroup Database
 * This file deals with MySQL interface functions
 * and query specifics/optimisations
 */

/** Number of times to re-try an operation in case of deadlock */
define( 'DEADLOCK_TRIES', 4 );
/** Minimum time to wait before retry, in microseconds */
define( 'DEADLOCK_DELAY_MIN', 500000 );
/** Maximum time to wait before retry */
define( 'DEADLOCK_DELAY_MAX', 1500000 );

/**
 * Database abstraction object
 * @ingroup Database
 */
class Database {

#------------------------------------------------------------------------------
# Variables
#------------------------------------------------------------------------------

	protected $mLastQuery = '';
	protected $mPHPError = false;

	protected $mServer, $mUser, $mPassword, $mConn = null, $mDBname;
	protected $mOpened = false;

	protected $mFailFunction;
	protected $mTablePrefix;
	protected $mFlags;
	protected $mTrxLevel = 0;
	protected $mErrorCount = 0;
	protected $mLBInfo = array();
	protected $mFakeSlaveLag = null, $mFakeMaster = false;

#------------------------------------------------------------------------------
# Accessors
#------------------------------------------------------------------------------
	# These optionally set a variable and return the previous state

	/**
	 * Fail function, takes a Database as a parameter
	 * Set to false for default, 1 for ignore errors
	 */
	function failFunction( $function = NULL ) {
		return wfSetVar( $this->mFailFunction, $function );
	}

	/**
	 * Output page, used for reporting errors
	 * FALSE means discard output
	 */
	function setOutputPage( $out ) {
		wfDeprecated( __METHOD__ );
	}

	/**
	 * Boolean, controls output of large amounts of debug information
	 */
	function debug( $debug = NULL ) {
		return wfSetBit( $this->mFlags, DBO_DEBUG, $debug );
	}

	/**
	 * Turns buffering of SQL result sets on (true) or off (false).
	 * Default is "on" and it should not be changed without good reasons.
	 */
	function bufferResults( $buffer = NULL ) {
		if ( is_null( $buffer ) ) {
			return !(bool)( $this->mFlags & DBO_NOBUFFER );
		} else {
			return !wfSetBit( $this->mFlags, DBO_NOBUFFER, !$buffer );
		}
	}

	/**
	 * Turns on (false) or off (true) the automatic generation and sending
	 * of a "we're sorry, but there has been a database error" page on
	 * database errors. Default is on (false). When turned off, the
	 * code should use lastErrno() and lastError() to handle the
	 * situation as appropriate.
	 */
	function ignoreErrors( $ignoreErrors = NULL ) {
		return wfSetBit( $this->mFlags, DBO_IGNORE, $ignoreErrors );
	}

	/**
	 * The current depth of nested transactions
	 * @param $level Integer: , default NULL.
	 */
	function trxLevel( $level = NULL ) {
		return wfSetVar( $this->mTrxLevel, $level );
	}

	/**
	 * Number of errors logged, only useful when errors are ignored
	 */
	function errorCount( $count = NULL ) {
		return wfSetVar( $this->mErrorCount, $count );
	}

	function tablePrefix( $prefix = null ) {
		return wfSetVar( $this->mTablePrefix, $prefix );
	}

	/**
	 * Properties passed down from the server info array of the load balancer
	 */
	function getLBInfo( $name = NULL ) {
		if ( is_null( $name ) ) {
			return $this->mLBInfo;
		} else {
			if ( array_key_exists( $name, $this->mLBInfo ) ) {
				return $this->mLBInfo[$name];
			} else {
				return NULL;
			}
		}
	}

	function setLBInfo( $name, $value = NULL ) {
		if ( is_null( $value ) ) {
			$this->mLBInfo = $name;
		} else {
			$this->mLBInfo[$name] = $value;
		}
	}

	/**
	 * Set lag time in seconds for a fake slave
	 */
	function setFakeSlaveLag( $lag ) {
		$this->mFakeSlaveLag = $lag;
	}

	/**
	 * Make this connection a fake master
	 */
	function setFakeMaster( $enabled = true ) {
		$this->mFakeMaster = $enabled;
	}

	/**
	 * Returns true if this database supports (and uses) cascading deletes
	 */
	function cascadingDeletes() {
		return false;
	}

	/**
	 * Returns true if this database supports (and uses) triggers (e.g. on the page table)
	 */
	function cleanupTriggers() {
		return false;
	}

	/**
	 * Returns true if this database is strict about what can be put into an IP field.
	 * Specifically, it uses a NULL value instead of an empty string.
	 */
	function strictIPs() {
		return false;
	}

	/**
	 * Returns true if this database uses timestamps rather than integers
	*/
	function realTimestamps() {
		return false;
	}

	/**
	 * Returns true if this database does an implicit sort when doing GROUP BY
	 */
	function implicitGroupby() {
		return true;
	}

	/**
	 * Returns true if this database does an implicit order by when the column has an index
	 * For example: SELECT page_title FROM page LIMIT 1
	 */
	function implicitOrderby() {
		return true;
	}

	/**
	 * Returns true if this database can do a native search on IP columns
	 * e.g. this works as expected: .. WHERE rc_ip = '127.42.12.102/32';
	 */
	function searchableIPs() {
		return false;
	}

	/**
	 * Returns true if this database can use functional indexes
	 */
	function functionalIndexes() {
		return false;
	}

	/**#@+
	 * Get function
	 */
	function lastQuery() { return $this->mLastQuery; }
	function isOpen() { return $this->mOpened; }
	/**#@-*/

	function setFlag( $flag ) {
		$this->mFlags |= $flag;
	}

	function clearFlag( $flag ) {
		$this->mFlags &= ~$flag;
	}

	function getFlag( $flag ) {
		return !!($this->mFlags & $flag);
	}

	/**
	 * General read-only accessor
	 */
	function getProperty( $name ) {
		return $this->$name;
	}

	function getWikiID() {
		if( $this->mTablePrefix ) {
			return "{$this->mDBname}-{$this->mTablePrefix}";
		} else {
			return $this->mDBname;
		}
	}

#------------------------------------------------------------------------------
# Other functions
#------------------------------------------------------------------------------

	/**@{{
	 * Constructor.
	 * @param string $server database server host
	 * @param string $user database user name
	 * @param string $password database user password
	 * @param string $dbname database name
	 * @param failFunction
	 * @param $flags
	 * @param $tablePrefix String: database table prefixes. By default use the prefix gave in LocalSettings.php
	 */
	function __construct( $server = false, $user = false, $password = false, $dbName = false,
		$failFunction = false, $flags = 0, $tablePrefix = 'get from global' ) {

		global $wgOut, $wgDBprefix, $wgCommandLineMode;
		# Can't get a reference if it hasn't been set yet
		if ( !isset( $wgOut ) ) {
			$wgOut = NULL;
		}

		$this->mFailFunction = $failFunction;
		$this->mFlags = $flags;

		if ( $this->mFlags & DBO_DEFAULT ) {
			if ( $wgCommandLineMode ) {
				$this->mFlags &= ~DBO_TRX;
			} else {
				$this->mFlags |= DBO_TRX;
			}
		}

		/*
		// Faster read-only access
		if ( wfReadOnly() ) {
			$this->mFlags |= DBO_PERSISTENT;
			$this->mFlags &= ~DBO_TRX;
		}*/

		/** Get the default table prefix*/
		if ( $tablePrefix == 'get from global' ) {
			$this->mTablePrefix = $wgDBprefix;
		} else {
			$this->mTablePrefix = $tablePrefix;
		}

		if ( $server ) {
			$this->open( $server, $user, $password, $dbName );
		}
	}

	/**
	 * @static
	 * @param failFunction
	 * @param $flags
	 */
	static function newFromParams( $server, $user, $password, $dbName, $failFunction = false, $flags = 0 )
	{
		return new Database( $server, $user, $password, $dbName, $failFunction, $flags );
	}

	/**
	 * Usually aborts on failure
	 * If the failFunction is set to a non-zero integer, returns success
	 */
	function open( $server, $user, $password, $dbName ) {
		global $wguname, $wgAllDBsAreLocalhost;
		wfProfileIn( __METHOD__ );

		# Test for missing mysql.so
		# First try to load it
		if (!@extension_loaded('mysql')) {
			@dl('mysql.so');
		}

		# Fail now
		# Otherwise we get a suppressed fatal error, which is very hard to track down
		if ( !function_exists( 'mysql_connect' ) ) {
			throw new DBConnectionError( $this, "MySQL functions missing, have you compiled PHP with the --with-mysql option?\n" );
		}

		# Debugging hack -- fake cluster
		if ( $wgAllDBsAreLocalhost ) {
			$realServer = 'localhost';
		} else {
			$realServer = $server;
		}
		$this->close();
		$this->mServer = $server;
		$this->mUser = $user;
		$this->mPassword = $password;
		$this->mDBname = $dbName;

		$success = false;

		wfProfileIn("dbconnect-$server");

		# Try to connect up to three times
		# The kernel's default SYN retransmission period is far too slow for us,
		# so we use a short timeout plus a manual retry.
		$this->mConn = false;
		$max = 3;
		$this->installErrorHandler();
		for ( $i = 0; $i < $max && !$this->mConn; $i++ ) {
			if ( $i > 1 ) {
				usleep( 1000 );
			}
			if ( $this->mFlags & DBO_PERSISTENT ) {
				$this->mConn = mysql_pconnect( $realServer, $user, $password );
			} else {
				# Create a new connection...
				$this->mConn = mysql_connect( $realServer, $user, $password, true );
			}
			if ($this->mConn === false) {
				#$iplus = $i + 1;
				#wfLogDBError("Connect loop error $iplus of $max ($server): " . mysql_errno() . " - " . mysql_error()."\n"); 
			}
		}
		$phpError = $this->restoreErrorHandler();
		
		wfProfileOut("dbconnect-$server");

		if ( $dbName != '' ) {
			if ( $this->mConn !== false ) {
				$success = @/**/mysql_select_db( $dbName, $this->mConn );
				if ( !$success ) {
					$error = "Error selecting database $dbName on server {$this->mServer} " .
						"from client host {$wguname['nodename']}\n";
					wfLogDBError(" Error selecting database $dbName on server {$this->mServer} \n");
					wfDebug( $error );
				}
			} else {
				wfDebug( "DB connection error\n" );
				wfDebug( "Server: $server, User: $user, Password: " .
					substr( $password, 0, 3 ) . "..., error: " . mysql_error() . "\n" );
				$success = false;
			}
		} else {
			# Delay USE query
			$success = (bool)$this->mConn;
		}

		if ( $success ) {
			$version = $this->getServerVersion();
			if ( version_compare( $version, '4.1' ) >= 0 ) {
				// Tell the server we're communicating with it in UTF-8.
				// This may engage various charset conversions.
				global $wgDBmysql5;
				if( $wgDBmysql5 ) {
					$this->query( 'SET NAMES utf8', __METHOD__ );
				}
				// Turn off strict mode
				$this->query( "SET sql_mode = ''", __METHOD__ );
			}

			// Turn off strict mode if it is on
		} else {
			$this->reportConnectionError( $phpError );
		}

		$this->mOpened = $success;
		wfProfileOut( __METHOD__ );
		return $success;
	}
	/**@}}*/

	protected function installErrorHandler() {
		$this->mPHPError = false;
		set_error_handler( array( $this, 'connectionErrorHandler' ) );
	}

	protected function restoreErrorHandler() {
		restore_error_handler();
		return $this->mPHPError;
	}

	protected function connectionErrorHandler( $errno,  $errstr ) {
		$this->mPHPError = $errstr;
	}

	/**
	 * Closes a database connection.
	 * if it is open : commits any open transactions
	 *
	 * @return bool operation success. true if already closed.
	 */
	function close()
	{
		$this->mOpened = false;
		if ( $this->mConn ) {
			if ( $this->trxLevel() ) {
				$this->immediateCommit();
			}
			return mysql_close( $this->mConn );
		} else {
			return true;
		}
	}

	/**
	 * @param string $error fallback error message, used if none is given by MySQL
	 */
	function reportConnectionError( $error = 'Unknown error' ) {
		$myError = $this->lastError();
		if ( $myError ) {
			$error = $myError;
		}

		if ( $this->mFailFunction ) {
			# Legacy error handling method
			if ( !is_int( $this->mFailFunction ) ) {
				$ff = $this->mFailFunction;
				$ff( $this, $error );
			}
		} else {
			# New method
			wfLogDBError( "Connection error: $error\n" );
			throw new DBConnectionError( $this, $error );
		}
	}

	/**
	 * Usually aborts on failure.  If errors are explicitly ignored, returns success.
	 *
	 * @param  $sql        String: SQL query
	 * @param  $fname      String: Name of the calling function, for profiling/SHOW PROCESSLIST 
	 *     comment (you can use __METHOD__ or add some extra info)
	 * @param  $tempIgnore Bool:   Whether to avoid throwing an exception on errors... 
	 *     maybe best to catch the exception instead?
	 * @return true for a successful write query, ResultWrapper object for a successful read query, 
	 *     or false on failure if $tempIgnore set
	 * @throws DBQueryError Thrown when the database returns an error of any kind
	 */
	public function query( $sql, $fname = '', $tempIgnore = false ) {
		global $wgProfiler;

		$isMaster = !is_null( $this->getLBInfo( 'master' ) );
		if ( isset( $wgProfiler ) ) {
			# generalizeSQL will probably cut down the query to reasonable
			# logging size most of the time. The substr is really just a sanity check.

			# Who's been wasting my precious column space? -- TS
			#$profName = 'query: ' . $fname . ' ' . substr( Database::generalizeSQL( $sql ), 0, 255 );

			if ( $isMaster ) {
				$queryProf = 'query-m: ' . substr( Database::generalizeSQL( $sql ), 0, 255 );
				$totalProf = 'Database::query-master';
			} else {
				$queryProf = 'query: ' . substr( Database::generalizeSQL( $sql ), 0, 255 );
				$totalProf = 'Database::query';
			}
			wfProfileIn( $totalProf );
			wfProfileIn( $queryProf );
		}

		$this->mLastQuery = $sql;

		# Add a comment for easy SHOW PROCESSLIST interpretation
		#if ( $fname ) {
			global $wgUser;
			if ( is_object( $wgUser ) && !($wgUser instanceof StubObject) ) {
				$userName = $wgUser->getName();
				if ( mb_strlen( $userName ) > 15 ) {
					$userName = mb_substr( $userName, 0, 15 ) . '...';
				}
				$userName = str_replace( '/', '', $userName );
			} else {
				$userName = '';
			}
			$commentedSql = preg_replace('/\s/', " /* $fname $userName */ ", $sql, 1);
		#} else {
		#	$commentedSql = $sql;
		#}

		# If DBO_TRX is set, start a transaction
		if ( ( $this->mFlags & DBO_TRX ) && !$this->trxLevel() && 
			$sql != 'BEGIN' && $sql != 'COMMIT' && $sql != 'ROLLBACK') {
			// avoid establishing transactions for SHOW and SET statements too -
			// that would delay transaction initializations to once connection 
			// is really used by application
			$sqlstart = substr($sql,0,10); // very much worth it, benchmark certified(tm)
			if (strpos($sqlstart,"SHOW ")!==0 and strpos($sqlstart,"SET ")!==0) 
				$this->begin(); 
		}

		if ( $this->debug() ) {
			$sqlx = substr( $commentedSql, 0, 500 );
			$sqlx = strtr( $sqlx, "\t\n", '  ' );
			if ( $isMaster ) {
				wfDebug( "SQL-master: $sqlx\n" );
			} else {
				wfDebug( "SQL: $sqlx\n" );
			}
		}

		# Do the query and handle errors
		$ret = $this->doQuery( $commentedSql );

		# Try reconnecting if the connection was lost
		if ( false === $ret && ( $this->lastErrno() == 2013 || $this->lastErrno() == 2006 ) ) {
			# Transaction is gone, like it or not
			$this->mTrxLevel = 0;
			wfDebug( "Connection lost, reconnecting...\n" );
			if ( $this->ping() ) {
				wfDebug( "Reconnected\n" );
				$sqlx = substr( $commentedSql, 0, 500 );
				$sqlx = strtr( $sqlx, "\t\n", '  ' );
				global $wgRequestTime;
				$elapsed = round( microtime(true) - $wgRequestTime, 3 );
				wfLogDBError( "Connection lost and reconnected after {$elapsed}s, query: $sqlx\n" );
				$ret = $this->doQuery( $commentedSql );
			} else {
				wfDebug( "Failed\n" );
			}
		}

		if ( false === $ret ) {
			$this->reportQueryError( $this->lastError(), $this->lastErrno(), $sql, $fname, $tempIgnore );
		}

		if ( isset( $wgProfiler ) ) {
			wfProfileOut( $queryProf );
			wfProfileOut( $totalProf );
		}
		return $this->resultObject( $ret );
	}

	/**
	 * The DBMS-dependent part of query()
	 * @param  $sql String: SQL query.
	 * @return Result object to feed to fetchObject, fetchRow, ...; or false on failure
	 * @access private
	 */
	/*private*/ function doQuery( $sql ) {
		if( $this->bufferResults() ) {
			$ret = mysql_query( $sql, $this->mConn );
		} else {
			$ret = mysql_unbuffered_query( $sql, $this->mConn );
		}
		return $ret;
	}

	/**
	 * @param $error
	 * @param $errno
	 * @param $sql
	 * @param string $fname
	 * @param bool $tempIgnore
	 */
	function reportQueryError( $error, $errno, $sql, $fname, $tempIgnore = false ) {
		global $wgCommandLineMode;
		# Ignore errors during error handling to avoid infinite recursion
		$ignore = $this->ignoreErrors( true );
		++$this->mErrorCount;

		if( $ignore || $tempIgnore ) {
			wfDebug("SQL ERROR (ignored): $error\n");
			$this->ignoreErrors( $ignore );
		} else {
			$sql1line = str_replace( "\n", "\\n", $sql );
			wfLogDBError("$fname\t{$this->mServer}\t$errno\t$error\t$sql1line\n");
			wfDebug("SQL ERROR: " . $error . "\n");
			throw new DBQueryError( $this, $error, $errno, $sql, $fname );
		}
	}


	/**
	 * Intended to be compatible with the PEAR::DB wrapper functions.
	 * http://pear.php.net/manual/en/package.database.db.intro-execute.php
	 *
	 * ? = scalar value, quoted as necessary
	 * ! = raw SQL bit (a function for instance)
	 * & = filename; reads the file and inserts as a blob
	 *     (we don't use this though...)
	 */
	function prepare( $sql, $func = 'Database::prepare' ) {
		/* MySQL doesn't support prepared statements (yet), so just
		   pack up the query for reference. We'll manually replace
		   the bits later. */
		return array( 'query' => $sql, 'func' => $func );
	}

	function freePrepared( $prepared ) {
		/* No-op for MySQL */
	}

	/**
	 * Execute a prepared query with the various arguments
	 * @param string $prepared the prepared sql
	 * @param mixed $args Either an array here, or put scalars as varargs
	 */
	function execute( $prepared, $args = null ) {
		if( !is_array( $args ) ) {
			# Pull the var args
			$args = func_get_args();
			array_shift( $args );
		}
		$sql = $this->fillPrepared( $prepared['query'], $args );
		return $this->query( $sql, $prepared['func'] );
	}

	/**
	 * Prepare & execute an SQL statement, quoting and inserting arguments
	 * in the appropriate places.
	 * @param string $query
	 * @param string $args ...
	 */
	function safeQuery( $query, $args = null ) {
		$prepared = $this->prepare( $query, 'Database::safeQuery' );
		if( !is_array( $args ) ) {
			# Pull the var args
			$args = func_get_args();
			array_shift( $args );
		}
		$retval = $this->execute( $prepared, $args );
		$this->freePrepared( $prepared );
		return $retval;
	}

	/**
	 * For faking prepared SQL statements on DBs that don't support
	 * it directly.
	 * @param string $preparedSql - a 'preparable' SQL statement
	 * @param array $args - array of arguments to fill it with
	 * @return string executable SQL
	 */
	function fillPrepared( $preparedQuery, $args ) {
		reset( $args );
		$this->preparedArgs =& $args;
		return preg_replace_callback( '/(\\\\[?!&]|[?!&])/',
			array( &$this, 'fillPreparedArg' ), $preparedQuery );
	}

	/**
	 * preg_callback func for fillPrepared()
	 * The arguments should be in $this->preparedArgs and must not be touched
	 * while we're doing this.
	 *
	 * @param array $matches
	 * @return string
	 * @private
	 */
	function fillPreparedArg( $matches ) {
		switch( $matches[1] ) {
			case '\\?': return '?';
			case '\\!': return '!';
			case '\\&': return '&';
		}
		list( /* $n */ , $arg ) = each( $this->preparedArgs );
		switch( $matches[1] ) {
			case '?': return $this->addQuotes( $arg );
			case '!': return $arg;
			case '&':
				# return $this->addQuotes( file_get_contents( $arg ) );
				throw new DBUnexpectedError( $this, '& mode is not implemented. If it\'s really needed, uncomment the line above.' );
			default:
				throw new DBUnexpectedError( $this, 'Received invalid match. This should never happen!' );
		}
	}

	/**#@+
	 * @param mixed $res A SQL result
	 */
	/**
	 * Free a result object
	 */
	function freeResult( $res ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		if ( !@/**/mysql_free_result( $res ) ) {
			throw new DBUnexpectedError( $this, "Unable to free MySQL result" );
		}
	}

	/**
	 * Fetch the next row from the given result object, in object form.
	 * Fields can be retrieved with $row->fieldname, with fields acting like
	 * member variables.
	 *
	 * @param $res SQL result object as returned from Database::query(), etc.
	 * @return MySQL row object
	 * @throws DBUnexpectedError Thrown if the database returns an error
	 */
	function fetchObject( $res ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		@/**/$row = mysql_fetch_object( $res );
		if( $this->lastErrno() ) {
			throw new DBUnexpectedError( $this, 'Error in fetchObject(): ' . htmlspecialchars( $this->lastError() ) );
		}
		return $row;
	}

	/**
	 * Fetch the next row from the given result object, in associative array
	 * form.  Fields are retrieved with $row['fieldname'].
	 *
	 * @param $res SQL result object as returned from Database::query(), etc.
	 * @return MySQL row object
	 * @throws DBUnexpectedError Thrown if the database returns an error
	 */
 	function fetchRow( $res ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		@/**/$row = mysql_fetch_array( $res );
		if ( $this->lastErrno() ) {
			throw new DBUnexpectedError( $this, 'Error in fetchRow(): ' . htmlspecialchars( $this->lastError() ) );
		}
		return $row;
	}

	/**
	 * Get the number of rows in a result object
	 */
	function numRows( $res ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		@/**/$n = mysql_num_rows( $res );
		if( $this->lastErrno() ) {
			throw new DBUnexpectedError( $this, 'Error in numRows(): ' . htmlspecialchars( $this->lastError() ) );
		}
		return $n;
	}

	/**
	 * Get the number of fields in a result object
	 * See documentation for mysql_num_fields()
	 */
	function numFields( $res ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		return mysql_num_fields( $res );
	}

	/**
	 * Get a field name in a result object
	 * See documentation for mysql_field_name():
	 * http://www.php.net/mysql_field_name
	 */
	function fieldName( $res, $n ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		return mysql_field_name( $res, $n );
	}

	/**
	 * Get the inserted value of an auto-increment row
	 *
	 * The value inserted should be fetched from nextSequenceValue()
	 *
	 * Example:
	 * $id = $dbw->nextSequenceValue('page_page_id_seq');
	 * $dbw->insert('page',array('page_id' => $id));
	 * $id = $dbw->insertId();
	 */
	function insertId() { return mysql_insert_id( $this->mConn ); }

	/**
	 * Change the position of the cursor in a result object
	 * See mysql_data_seek()
	 */
	function dataSeek( $res, $row ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		return mysql_data_seek( $res, $row );
	}

	/**
	 * Get the last error number
	 * See mysql_errno()
	 */
	function lastErrno() {
		if ( $this->mConn ) {
			return mysql_errno( $this->mConn );
		} else {
			return mysql_errno();
		}
	}

	/**
	 * Get a description of the last error
	 * See mysql_error() for more details
	 */
	function lastError() {
		if ( $this->mConn ) {
			# Even if it's non-zero, it can still be invalid
			wfSuppressWarnings();
			$error = mysql_error( $this->mConn );
			if ( !$error ) {
				$error = mysql_error();
			}
			wfRestoreWarnings();
		} else {
			$error = mysql_error();
		}
		if( $error ) {
			$error .= ' (' . $this->mServer . ')';
		}
		return $error;
	}
	/**
	 * Get the number of rows affected by the last write query
	 * See mysql_affected_rows() for more details
	 */
	function affectedRows() { return mysql_affected_rows( $this->mConn ); }
	/**#@-*/ // end of template : @param $result

	/**
	 * Simple UPDATE wrapper
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns success
	 *
	 * This function exists for historical reasons, Database::update() has a more standard
	 * calling convention and feature set
	 */
	function set( $table, $var, $value, $cond, $fname = 'Database::set' )
	{
		$table = $this->tableName( $table );
		$sql = "UPDATE $table SET $var = '" .
		  $this->strencode( $value ) . "' WHERE ($cond)";
		return (bool)$this->query( $sql, $fname );
	}

	/**
	 * Simple SELECT wrapper, returns a single field, input must be encoded
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns FALSE on failure
	 */
	function selectField( $table, $var, $cond='', $fname = 'Database::selectField', $options = array() ) {
		if ( !is_array( $options ) ) {
			$options = array( $options );
		}
		$options['LIMIT'] = 1;

		$res = $this->select( $table, $var, $cond, $fname, $options );
		if ( $res === false || !$this->numRows( $res ) ) {
			return false;
		}
		$row = $this->fetchRow( $res );
		if ( $row !== false ) {
			$this->freeResult( $res );
			return $row[0];
		} else {
			return false;
		}
	}

	/**
	 * Returns an optional USE INDEX clause to go after the table, and a
	 * string to go at the end of the query
	 *
	 * @private
	 *
	 * @param array $options an associative array of options to be turned into
	 *              an SQL query, valid keys are listed in the function.
	 * @return array
	 */
	function makeSelectOptions( $options ) {
		$preLimitTail = $postLimitTail = '';
		$startOpts = '';

		$noKeyOptions = array();
		foreach ( $options as $key => $option ) {
			if ( is_numeric( $key ) ) {
				$noKeyOptions[$option] = true;
			}
		}

		if ( isset( $options['GROUP BY'] ) ) $preLimitTail .= " GROUP BY {$options['GROUP BY']}";
		if ( isset( $options['HAVING'] ) ) $preLimitTail .= " HAVING {$options['HAVING']}";
		if ( isset( $options['ORDER BY'] ) ) $preLimitTail .= " ORDER BY {$options['ORDER BY']}";
		
		//if (isset($options['LIMIT'])) {
		//	$tailOpts .= $this->limitResult('', $options['LIMIT'],
		//		isset($options['OFFSET']) ? $options['OFFSET'] 
		//		: false);
		//}

		if ( isset( $noKeyOptions['FOR UPDATE'] ) ) $postLimitTail .= ' FOR UPDATE';
		if ( isset( $noKeyOptions['LOCK IN SHARE MODE'] ) ) $postLimitTail .= ' LOCK IN SHARE MODE';
		if ( isset( $noKeyOptions['DISTINCT'] ) || isset( $noKeyOptions['DISTINCTROW'] ) ) $startOpts .= 'DISTINCT';

		# Various MySQL extensions
		if ( isset( $noKeyOptions['STRAIGHT_JOIN'] ) ) $startOpts .= ' /*! STRAIGHT_JOIN */';
		if ( isset( $noKeyOptions['HIGH_PRIORITY'] ) ) $startOpts .= ' HIGH_PRIORITY';
		if ( isset( $noKeyOptions['SQL_BIG_RESULT'] ) ) $startOpts .= ' SQL_BIG_RESULT';
		if ( isset( $noKeyOptions['SQL_BUFFER_RESULT'] ) ) $startOpts .= ' SQL_BUFFER_RESULT';
		if ( isset( $noKeyOptions['SQL_SMALL_RESULT'] ) ) $startOpts .= ' SQL_SMALL_RESULT';
		if ( isset( $noKeyOptions['SQL_CALC_FOUND_ROWS'] ) ) $startOpts .= ' SQL_CALC_FOUND_ROWS';
		if ( isset( $noKeyOptions['SQL_CACHE'] ) ) $startOpts .= ' SQL_CACHE';
		if ( isset( $noKeyOptions['SQL_NO_CACHE'] ) ) $startOpts .= ' SQL_NO_CACHE';

		if ( isset( $options['USE INDEX'] ) && ! is_array( $options['USE INDEX'] ) ) {
			$useIndex = $this->useIndexClause( $options['USE INDEX'] );
		} else {
			$useIndex = '';
		}
		
		return array( $startOpts, $useIndex, $preLimitTail, $postLimitTail );
	}

	/**
	 * SELECT wrapper
	 *
	 * @param mixed  $table   Array or string, table name(s) (prefix auto-added)
	 * @param mixed  $vars    Array or string, field name(s) to be retrieved
	 * @param mixed  $conds   Array or string, condition(s) for WHERE
	 * @param string $fname   Calling function name (use __METHOD__) for logs/profiling
	 * @param array  $options Associative array of options (e.g. array('GROUP BY' => 'page_title')),
	 *                        see Database::makeSelectOptions code for list of supported stuff
	 * @param array $join_conds Associative array of table join conditions (optional)
	 *                        (e.g. array( 'page' => array('LEFT JOIN','page_latest=rev_id') )
	 * @return mixed Database result resource (feed to Database::fetchObject or whatever), or false on failure
	 */
	function select( $table, $vars, $conds='', $fname = 'Database::select', $options = array(), $join_conds = array() )
	{
		$sql = $this->selectSQLText( $table, $vars, $conds, $fname, $options, $join_conds );
		return $this->query( $sql, $fname );
	}
	
	/**
	 * SELECT wrapper
	 *
	 * @param mixed  $table   Array or string, table name(s) (prefix auto-added)
	 * @param mixed  $vars    Array or string, field name(s) to be retrieved
	 * @param mixed  $conds   Array or string, condition(s) for WHERE
	 * @param string $fname   Calling function name (use __METHOD__) for logs/profiling
	 * @param array  $options Associative array of options (e.g. array('GROUP BY' => 'page_title')),
	 *                        see Database::makeSelectOptions code for list of supported stuff
	 * @param array $join_conds Associative array of table join conditions (optional)
	 *                        (e.g. array( 'page' => array('LEFT JOIN','page_latest=rev_id') )
	 * @return string, the SQL text
	 */
	function selectSQLText( $table, $vars, $conds='', $fname = 'Database::select', $options = array(), $join_conds = array() ) {
		if( is_array( $vars ) ) {
			$vars = implode( ',', $vars );
		}
		if( !is_array( $options ) ) {
			$options = array( $options );
		}
		if( is_array( $table ) ) {
			if ( !empty($join_conds) || ( isset( $options['USE INDEX'] ) && is_array( @$options['USE INDEX'] ) ) )
				$from = ' FROM ' . $this->tableNamesWithUseIndexOrJOIN( $table, @$options['USE INDEX'], $join_conds );
			else
				$from = ' FROM ' . implode( ',', array_map( array( &$this, 'tableName' ), $table ) );
		} elseif ($table!='') {
			if ($table{0}==' ') {
				$from = ' FROM ' . $table;
			} else {
				$from = ' FROM ' . $this->tableName( $table );
			}
		} else {
			$from = '';
		}

		list( $startOpts, $useIndex, $preLimitTail, $postLimitTail ) = $this->makeSelectOptions( $options );

		if( !empty( $conds ) ) {
			if ( is_array( $conds ) ) {
				$conds = $this->makeList( $conds, LIST_AND );
			}
			$sql = "SELECT $startOpts $vars $from $useIndex WHERE $conds $preLimitTail";
		} else {
			$sql = "SELECT $startOpts $vars $from $useIndex $preLimitTail";
		}

		if (isset($options['LIMIT']))
			$sql = $this->limitResult($sql, $options['LIMIT'],
				isset($options['OFFSET']) ? $options['OFFSET'] : false);
		$sql = "$sql $postLimitTail";
		
		if (isset($options['EXPLAIN'])) {
			$sql = 'EXPLAIN ' . $sql;
		}
		return $sql;
	}

	/**
	 * Single row SELECT wrapper
	 * Aborts or returns FALSE on error
	 *
	 * $vars: the selected variables
	 * $conds: a condition map, terms are ANDed together.
	 *   Items with numeric keys are taken to be literal conditions
	 * Takes an array of selected variables, and a condition map, which is ANDed
	 * e.g: selectRow( "page", array( "page_id" ), array( "page_namespace" =>
	 * NS_MAIN, "page_title" => "Astronomy" ) )   would return an object where
	 * $obj- >page_id is the ID of the Astronomy article
	 *
	 * @todo migrate documentation to phpdocumentor format
	 */
	function selectRow( $table, $vars, $conds, $fname = 'Database::selectRow', $options = array(), $join_conds = array() ) {
		$options['LIMIT'] = 1;
		$res = $this->select( $table, $vars, $conds, $fname, $options, $join_conds );
		if ( $res === false )
			return false;
		if ( !$this->numRows($res) ) {
			$this->freeResult($res);
			return false;
		}
		$obj = $this->fetchObject( $res );
		$this->freeResult( $res );
		return $obj;

	}
	
	/**
	 * Estimate rows in dataset
	 * Returns estimated count, based on EXPLAIN output
	 * Takes same arguments as Database::select()
	 */
	
	function estimateRowCount( $table, $vars='*', $conds='', $fname = 'Database::estimateRowCount', $options = array() ) {
		$options['EXPLAIN']=true;
		$res = $this->select ($table, $vars, $conds, $fname, $options );
		if ( $res === false )
			return false;
		if (!$this->numRows($res)) {
			$this->freeResult($res);
			return 0;
		}
		
		$rows=1;
	
		while( $plan = $this->fetchObject( $res ) ) {
			$rows *= ($plan->rows > 0)?$plan->rows:1; // avoid resetting to zero
		}
		
		$this->freeResult($res);
		return $rows;		
	}
	

	/**
	 * Removes most variables from an SQL query and replaces them with X or N for numbers.
	 * It's only slightly flawed. Don't use for anything important.
	 *
	 * @param string $sql A SQL Query
	 * @static
	 */
	static function generalizeSQL( $sql ) {
		# This does the same as the regexp below would do, but in such a way
		# as to avoid crashing php on some large strings.
		# $sql = preg_replace ( "/'([^\\\\']|\\\\.)*'|\"([^\\\\\"]|\\\\.)*\"/", "'X'", $sql);

		$sql = str_replace ( "\\\\", '', $sql);
		$sql = str_replace ( "\\'", '', $sql);
		$sql = str_replace ( "\\\"", '', $sql);
		$sql = preg_replace ("/'.*'/s", "'X'", $sql);
		$sql = preg_replace ('/".*"/s', "'X'", $sql);

		# All newlines, tabs, etc replaced by single space
		$sql = preg_replace ( '/\s+/', ' ', $sql);

		# All numbers => N
		$sql = preg_replace ('/-?[0-9]+/s', 'N', $sql);

		return $sql;
	}

	/**
	 * Determines whether a field exists in a table
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns NULL on failure
	 */
	function fieldExists( $table, $field, $fname = 'Database::fieldExists' ) {
		$table = $this->tableName( $table );
		$res = $this->query( 'DESCRIBE '.$table, $fname );
		if ( !$res ) {
			return NULL;
		}

		$found = false;

		while ( $row = $this->fetchObject( $res ) ) {
			if ( $row->Field == $field ) {
				$found = true;
				break;
			}
		}
		return $found;
	}

	/**
	 * Determines whether an index exists
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns NULL on failure
	 */
	function indexExists( $table, $index, $fname = 'Database::indexExists' ) {
		$info = $this->indexInfo( $table, $index, $fname );
		if ( is_null( $info ) ) {
			return NULL;
		} else {
			return $info !== false;
		}
	}


	/**
	 * Get information about an index into an object
	 * Returns false if the index does not exist
	 */
	function indexInfo( $table, $index, $fname = 'Database::indexInfo' ) {
		# SHOW INDEX works in MySQL 3.23.58, but SHOW INDEXES does not.
		# SHOW INDEX should work for 3.x and up:
		# http://dev.mysql.com/doc/mysql/en/SHOW_INDEX.html
		$table = $this->tableName( $table );
		$sql = 'SHOW INDEX FROM '.$table;
		$res = $this->query( $sql, $fname );
		if ( !$res ) {
			return NULL;
		}

		$result = array();
		while ( $row = $this->fetchObject( $res ) ) {
			if ( $row->Key_name == $index ) {
				$result[] = $row;
			}
		}
		$this->freeResult($res);
		
		return empty($result) ? false : $result;
	}

	/**
	 * Query whether a given table exists
	 */
	function tableExists( $table ) {
		$table = $this->tableName( $table );
		$old = $this->ignoreErrors( true );
		$res = $this->query( "SELECT 1 FROM $table LIMIT 1" );
		$this->ignoreErrors( $old );
		if( $res ) {
			$this->freeResult( $res );
			return true;
		} else {
			return false;
		}
	}

	/**
	 * mysql_fetch_field() wrapper
	 * Returns false if the field doesn't exist
	 *
	 * @param $table
	 * @param $field
	 */
	function fieldInfo( $table, $field ) {
		$table = $this->tableName( $table );
		$res = $this->query( "SELECT * FROM $table LIMIT 1" );
		$n = mysql_num_fields( $res->result );
		for( $i = 0; $i < $n; $i++ ) {
			$meta = mysql_fetch_field( $res->result, $i );
			if( $field == $meta->name ) {
				return new MySQLField($meta);
			}
		}
		return false;
	}

	/**
	 * mysql_field_type() wrapper
	 */
	function fieldType( $res, $index ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		return mysql_field_type( $res, $index );
	}

	/**
	 * Determines if a given index is unique
	 */
	function indexUnique( $table, $index ) {
		$indexInfo = $this->indexInfo( $table, $index );
		if ( !$indexInfo ) {
			return NULL;
		}
		return !$indexInfo[0]->Non_unique;
	}

	/**
	 * INSERT wrapper, inserts an array into a table
	 *
	 * $a may be a single associative array, or an array of these with numeric keys, for
	 * multi-row insert.
	 *
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns success
	 */
	function insert( $table, $a, $fname = 'Database::insert', $options = array() ) {
		# No rows to insert, easy just return now
		if ( !count( $a ) ) {
			return true;
		}

		$table = $this->tableName( $table );
		if ( !is_array( $options ) ) {
			$options = array( $options );
		}
		if ( isset( $a[0] ) && is_array( $a[0] ) ) {
			$multi = true;
			$keys = array_keys( $a[0] );
		} else {
			$multi = false;
			$keys = array_keys( $a );
		}

		$sql = 'INSERT ' . implode( ' ', $options ) .
			" INTO $table (" . implode( ',', $keys ) . ') VALUES ';

		if ( $multi ) {
			$first = true;
			foreach ( $a as $row ) {
				if ( $first ) {
					$first = false;
				} else {
					$sql .= ',';
				}
				$sql .= '(' . $this->makeList( $row ) . ')';
			}
		} else {
			$sql .= '(' . $this->makeList( $a ) . ')';
		}
		return (bool)$this->query( $sql, $fname );
	}

	/**
	 * Make UPDATE options for the Database::update function
	 *
	 * @private
	 * @param array $options The options passed to Database::update
	 * @return string
	 */
	function makeUpdateOptions( $options ) {
		if( !is_array( $options ) ) {
			$options = array( $options );
		}
		$opts = array();
		if ( in_array( 'LOW_PRIORITY', $options ) )
			$opts[] = $this->lowPriorityOption();
		if ( in_array( 'IGNORE', $options ) )
			$opts[] = 'IGNORE';
		return implode(' ', $opts);
	}

	/**
	 * UPDATE wrapper, takes a condition array and a SET array
	 *
	 * @param string $table  The table to UPDATE
	 * @param array  $values An array of values to SET
	 * @param array  $conds  An array of conditions (WHERE). Use '*' to update all rows.
	 * @param string $fname  The Class::Function calling this function
	 *                       (for the log)
	 * @param array  $options An array of UPDATE options, can be one or
	 *                        more of IGNORE, LOW_PRIORITY
	 * @return bool
	 */
	function update( $table, $values, $conds, $fname = 'Database::update', $options = array() ) {
		$table = $this->tableName( $table );
		$opts = $this->makeUpdateOptions( $options );
		$sql = "UPDATE $opts $table SET " . $this->makeList( $values, LIST_SET );
		if ( $conds != '*' ) {
			$sql .= " WHERE " . $this->makeList( $conds, LIST_AND );
		}
		return $this->query( $sql, $fname );
	}

	/**
	 * Makes an encoded list of strings from an array
	 * $mode:
	 *        LIST_COMMA         - comma separated, no field names
	 *        LIST_AND           - ANDed WHERE clause (without the WHERE)
	 *        LIST_OR            - ORed WHERE clause (without the WHERE)
	 *        LIST_SET           - comma separated with field names, like a SET clause
	 *        LIST_NAMES         - comma separated field names
	 */
	function makeList( $a, $mode = LIST_COMMA ) {
		if ( !is_array( $a ) ) {
			throw new DBUnexpectedError( $this, 'Database::makeList called with incorrect parameters' );
		}

		$first = true;
		$list = '';
		foreach ( $a as $field => $value ) {
			if ( !$first ) {
				if ( $mode == LIST_AND ) {
					$list .= ' AND ';
				} elseif($mode == LIST_OR) {
					$list .= ' OR ';
				} else {
					$list .= ',';
				}
			} else {
				$first = false;
			}
			if ( ($mode == LIST_AND || $mode == LIST_OR) && is_numeric( $field ) ) {
				$list .= "($value)";
			} elseif ( ($mode == LIST_SET) && is_numeric( $field ) ) {
				$list .= "$value";
			} elseif ( ($mode == LIST_AND || $mode == LIST_OR) && is_array($value) ) {
				if( count( $value ) == 0 ) {
					throw new MWException( __METHOD__.': empty input' );
				} elseif( count( $value ) == 1 ) {
					// Special-case single values, as IN isn't terribly efficient
					// Don't necessarily assume the single key is 0; we don't
					// enforce linear numeric ordering on other arrays here.
					$value = array_values( $value );
					$list .= $field." = ".$this->addQuotes( $value[0] );
				} else {
					$list .= $field." IN (".$this->makeList($value).") ";
				}
			} elseif( is_null($value) ) {
				if ( $mode == LIST_AND || $mode == LIST_OR ) {
					$list .= "$field IS ";
				} elseif ( $mode == LIST_SET ) {
					$list .= "$field = ";
				}
				$list .= 'NULL';
			} else {
				if ( $mode == LIST_AND || $mode == LIST_OR || $mode == LIST_SET ) {
					$list .= "$field = ";
				}
				$list .= $mode == LIST_NAMES ? $value : $this->addQuotes( $value );
			}
		}
		return $list;
	}

	/**
	 * Change the current database
	 */
	function selectDB( $db ) {
		$this->mDBname = $db;
		return mysql_select_db( $db, $this->mConn );
	}

	/**
	 * Get the current DB name
	 */
	function getDBname() {
		return $this->mDBname;
	}

	/**
	 * Get the server hostname or IP address
	 */
	function getServer() {
		return $this->mServer;
	}

	/**
	 * Format a table name ready for use in constructing an SQL query
	 *
	 * This does two important things: it quotes the table names to clean them up,
	 * and it adds a table prefix if only given a table name with no quotes.
	 *
	 * All functions of this object which require a table name call this function
	 * themselves. Pass the canonical name to such functions. This is only needed
	 * when calling query() directly.
	 *
	 * @param string $name database table name
	 * @return string full database name
	 */
	function tableName( $name ) {
		global $wgSharedDB, $wgSharedPrefix, $wgSharedTables;
		# Skip the entire process when we have a string quoted on both ends.
		# Note that we check the end so that we will still quote any use of
		# use of `database`.table. But won't break things if someone wants
		# to query a database table with a dot in the name.
		if ( $name[0] == '`' && substr( $name, -1, 1 ) == '`' ) return $name;
		
		# Lets test for any bits of text that should never show up in a table
		# name. Basically anything like JOIN or ON which are actually part of
		# SQL queries, but may end up inside of the table value to combine
		# sql. Such as how the API is doing.
		# Note that we use a whitespace test rather than a \b test to avoid
		# any remote case where a word like on may be inside of a table name
		# surrounded by symbols which may be considered word breaks.
		if( preg_match( '/(^|\s)(DISTINCT|JOIN|ON|AS)(\s|$)/i', $name ) !== 0 ) return $name;
		
		# Split database and table into proper variables.
		# We reverse the explode so that database.table and table both output
		# the correct table.
		$dbDetails = array_reverse( explode( '.', $name, 2 ) );
		if( isset( $dbDetails[1] ) ) @list( $table, $database ) = $dbDetails;
		else                         @list( $table ) = $dbDetails;
		$prefix = $this->mTablePrefix; # Default prefix
		
		# A database name has been specified in input. Quote the table name
		# because we don't want any prefixes added.
		if( isset($database) ) $table = ( $table[0] == '`' ? $table : "`{$table}`" );
		
		# Note that we use the long format because php will complain in in_array if
		# the input is not an array, and will complain in is_array if it is not set.
		if( !isset( $database ) # Don't use shared database if pre selected.
		 && isset( $wgSharedDB ) # We have a shared database
		 && $table[0] != '`' # Paranoia check to prevent shared tables listing '`table`'
		 && isset( $wgSharedTables )
		 && is_array( $wgSharedTables )
		 && in_array( $table, $wgSharedTables ) ) { # A shared table is selected
			$database = $wgSharedDB;
			$prefix   = isset( $wgSharedPrefix ) ? $wgSharedPrefix : $prefix;
		}
		
		# Quote the $database and $table and apply the prefix if not quoted.
		if( isset($database) ) $database = ( $database[0] == '`' ? $database : "`{$database}`" );
		$table = ( $table[0] == '`' ? $table : "`{$prefix}{$table}`" );
		
		# Merge our database and table into our final table name.
		$tableName = ( isset($database) ? "{$database}.{$table}" : "{$table}" );
		
		# We're finished, return.
		return $tableName;
	}

	/**
	 * Fetch a number of table names into an array
	 * This is handy when you need to construct SQL for joins
	 *
	 * Example:
	 * extract($dbr->tableNames('user','watchlist'));
	 * $sql = "SELECT wl_namespace,wl_title FROM $watchlist,$user
	 *         WHERE wl_user=user_id AND wl_user=$nameWithQuotes";
	 */
	public function tableNames() {
		$inArray = func_get_args();
		$retVal = array();
		foreach ( $inArray as $name ) {
			$retVal[$name] = $this->tableName( $name );
		}
		return $retVal;
	}
	
	/**
	 * Fetch a number of table names into an zero-indexed numerical array
	 * This is handy when you need to construct SQL for joins
	 *
	 * Example:
	 * list( $user, $watchlist ) = $dbr->tableNamesN('user','watchlist');
	 * $sql = "SELECT wl_namespace,wl_title FROM $watchlist,$user
	 *         WHERE wl_user=user_id AND wl_user=$nameWithQuotes";
	 */
	public function tableNamesN() {
		$inArray = func_get_args();
		$retVal = array();
		foreach ( $inArray as $name ) {
			$retVal[] = $this->tableName( $name );
		}
		return $retVal;
	}

	/**
	 * @private
	 */
	function tableNamesWithUseIndexOrJOIN( $tables, $use_index = array(), $join_conds = array() ) {
		$ret = array();
		$retJOIN = array();
		$use_index_safe = is_array($use_index) ? $use_index : array();
		$join_conds_safe = is_array($join_conds) ? $join_conds : array();
		foreach ( $tables as $table ) {
			// Is there a JOIN and INDEX clause for this table?
			if ( isset($join_conds_safe[$table]) && isset($use_index_safe[$table]) ) {
				$tableClause = $join_conds_safe[$table][0] . ' ' . $this->tableName( $table );
				$tableClause .= ' ' . $this->useIndexClause( implode( ',', (array)$use_index_safe[$table] ) );
				$tableClause .= ' ON (' . $this->makeList((array)$join_conds_safe[$table][1], LIST_AND) . ')';
				$retJOIN[] = $tableClause;
			// Is there an INDEX clause?
			} else if ( isset($use_index_safe[$table]) ) {
				$tableClause = $this->tableName( $table );
				$tableClause .= ' ' . $this->useIndexClause( implode( ',', (array)$use_index_safe[$table] ) );
				$ret[] = $tableClause;
			// Is there a JOIN clause?
			} else if ( isset($join_conds_safe[$table]) ) {
				$tableClause = $join_conds_safe[$table][0] . ' ' . $this->tableName( $table );
				$tableClause .= ' ON (' . $this->makeList((array)$join_conds_safe[$table][1], LIST_AND) . ')';
				$retJOIN[] = $tableClause;
			} else {
				$tableClause = $this->tableName( $table );
				$ret[] = $tableClause;
			}
		}
		// We can't separate explicit JOIN clauses with ',', use ' ' for those
		$straightJoins = !empty($ret) ? implode( ',', $ret ) : "";
		$otherJoins = !empty($retJOIN) ? implode( ' ', $retJOIN ) : "";
		// Compile our final table clause
		return implode(' ',array($straightJoins,$otherJoins) );
	}

	/**
	 * Wrapper for addslashes()
	 * @param string $s String to be slashed.
	 * @return string slashed string.
	 */
	function strencode( $s ) {
		return mysql_real_escape_string( $s, $this->mConn );
	}

	/**
	 * If it's a string, adds quotes and backslashes
	 * Otherwise returns as-is
	 */
	function addQuotes( $s ) {
		if ( is_null( $s ) ) {
			return 'NULL';
		} else {
			# This will also quote numeric values. This should be harmless,
			# and protects against weird problems that occur when they really
			# _are_ strings such as article titles and string->number->string
			# conversion is not 1:1.
			return "'" . $this->strencode( $s ) . "'";
		}
	}

	/**
	 * Escape string for safe LIKE usage
	 */
	function escapeLike( $s ) {
		$s=$this->strencode( $s );
		$s=str_replace(array('%','_'),array('\%','\_'),$s);
		return $s;
	}

	/**
	 * Returns an appropriately quoted sequence value for inserting a new row.
	 * MySQL has autoincrement fields, so this is just NULL. But the PostgreSQL
	 * subclass will return an integer, and save the value for insertId()
	 */
	function nextSequenceValue( $seqName ) {
		return NULL;
	}

	/**
	 * USE INDEX clause
	 * PostgreSQL doesn't have them and returns ""
	 */
	function useIndexClause( $index ) {
		return "FORCE INDEX ($index)";
	}

	/**
	 * REPLACE query wrapper
	 * PostgreSQL simulates this with a DELETE followed by INSERT
	 * $row is the row to insert, an associative array
	 * $uniqueIndexes is an array of indexes. Each element may be either a
	 * field name or an array of field names
	 *
	 * It may be more efficient to leave off unique indexes which are unlikely to collide.
	 * However if you do this, you run the risk of encountering errors which wouldn't have
	 * occurred in MySQL
	 *
	 * @todo migrate comment to phodocumentor format
	 */
	function replace( $table, $uniqueIndexes, $rows, $fname = 'Database::replace' ) {
		$table = $this->tableName( $table );

		# Single row case
		if ( !is_array( reset( $rows ) ) ) {
			$rows = array( $rows );
		}

		$sql = "REPLACE INTO $table (" . implode( ',', array_keys( $rows[0] ) ) .') VALUES ';
		$first = true;
		foreach ( $rows as $row ) {
			if ( $first ) {
				$first = false;
			} else {
				$sql .= ',';
			}
			$sql .= '(' . $this->makeList( $row ) . ')';
		}
		return $this->query( $sql, $fname );
	}

	/**
	 * DELETE where the condition is a join
	 * MySQL does this with a multi-table DELETE syntax, PostgreSQL does it with sub-selects
	 *
	 * For safety, an empty $conds will not delete everything. If you want to delete all rows where the
	 * join condition matches, set $conds='*'
	 *
	 * DO NOT put the join condition in $conds
	 *
	 * @param string $delTable The table to delete from.
	 * @param string $joinTable The other table.
	 * @param string $delVar The variable to join on, in the first table.
	 * @param string $joinVar The variable to join on, in the second table.
	 * @param array $conds Condition array of field names mapped to variables, ANDed together in the WHERE clause
	 */
	function deleteJoin( $delTable, $joinTable, $delVar, $joinVar, $conds, $fname = 'Database::deleteJoin' ) {
		if ( !$conds ) {
			throw new DBUnexpectedError( $this, 'Database::deleteJoin() called with empty $conds' );
		}

		$delTable = $this->tableName( $delTable );
		$joinTable = $this->tableName( $joinTable );
		$sql = "DELETE $delTable FROM $delTable, $joinTable WHERE $delVar=$joinVar ";
		if ( $conds != '*' ) {
			$sql .= ' AND ' . $this->makeList( $conds, LIST_AND );
		}

		return $this->query( $sql, $fname );
	}

	/**
	 * Returns the size of a text field, or -1 for "unlimited"
	 */
	function textFieldSize( $table, $field ) {
		$table = $this->tableName( $table );
		$sql = "SHOW COLUMNS FROM $table LIKE \"$field\";";
		$res = $this->query( $sql, 'Database::textFieldSize' );
		$row = $this->fetchObject( $res );
		$this->freeResult( $res );

		$m = array();
		if ( preg_match( '/\((.*)\)/', $row->Type, $m ) ) {
			$size = $m[1];
		} else {
			$size = -1;
		}
		return $size;
	}

	/**
	 * @return string Returns the text of the low priority option if it is supported, or a blank string otherwise
	 */
	function lowPriorityOption() {
		return 'LOW_PRIORITY';
	}

	/**
	 * DELETE query wrapper
	 *
	 * Use $conds == "*" to delete all rows
	 */
	function delete( $table, $conds, $fname = 'Database::delete' ) {
		if ( !$conds ) {
			throw new DBUnexpectedError( $this, 'Database::delete() called with no conditions' );
		}
		$table = $this->tableName( $table );
		$sql = "DELETE FROM $table";
		if ( $conds != '*' ) {
			$sql .= ' WHERE ' . $this->makeList( $conds, LIST_AND );
		}
		return $this->query( $sql, $fname );
	}

	/**
	 * INSERT SELECT wrapper
	 * $varMap must be an associative array of the form array( 'dest1' => 'source1', ...)
	 * Source items may be literals rather than field names, but strings should be quoted with Database::addQuotes()
	 * $conds may be "*" to copy the whole table
	 * srcTable may be an array of tables.
	 */
	function insertSelect( $destTable, $srcTable, $varMap, $conds, $fname = 'Database::insertSelect',
		$insertOptions = array(), $selectOptions = array() )
	{
		$destTable = $this->tableName( $destTable );
		if ( is_array( $insertOptions ) ) {
			$insertOptions = implode( ' ', $insertOptions );
		}
		if( !is_array( $selectOptions ) ) {
			$selectOptions = array( $selectOptions );
		}
		list( $startOpts, $useIndex, $tailOpts ) = $this->makeSelectOptions( $selectOptions );
		if( is_array( $srcTable ) ) {
			$srcTable =  implode( ',', array_map( array( &$this, 'tableName' ), $srcTable ) );
		} else {
			$srcTable = $this->tableName( $srcTable );
		}
		$sql = "INSERT $insertOptions INTO $destTable (" . implode( ',', array_keys( $varMap ) ) . ')' .
			" SELECT $startOpts " . implode( ',', $varMap ) .
			" FROM $srcTable $useIndex ";
		if ( $conds != '*' ) {
			$sql .= ' WHERE ' . $this->makeList( $conds, LIST_AND );
		}
		$sql .= " $tailOpts";
		return $this->query( $sql, $fname );
	}

	/**
	 * Construct a LIMIT query with optional offset
	 * This is used for query pages
	 * $sql string SQL query we will append the limit too
	 * $limit integer the SQL limit
	 * $offset integer the SQL offset (default false)
	 */
	function limitResult($sql, $limit, $offset=false) {
		if( !is_numeric($limit) ) {
			throw new DBUnexpectedError( $this, "Invalid non-numeric limit passed to limitResult()\n" );
		}
		return "$sql LIMIT "
				. ( (is_numeric($offset) && $offset != 0) ? "{$offset}," : "" )
				. "{$limit} ";
	}
	function limitResultForUpdate($sql, $num) {
		return $this->limitResult($sql, $num, 0);
	}

	/**
	 * Returns an SQL expression for a simple conditional.
	 * Uses IF on MySQL.
	 *
	 * @param string $cond SQL expression which will result in a boolean value
	 * @param string $trueVal SQL expression to return if true
	 * @param string $falseVal SQL expression to return if false
	 * @return string SQL fragment
	 */
	function conditional( $cond, $trueVal, $falseVal ) {
		return " IF($cond, $trueVal, $falseVal) ";
	}

	/**
	 * Returns a comand for str_replace function in SQL query.
	 * Uses REPLACE() in MySQL
	 *
	 * @param string $orig String or column to modify
	 * @param string $old String or column to seek
	 * @param string $new String or column to replace with
	 */
	function strreplace( $orig, $old, $new ) {
		return "REPLACE({$orig}, {$old}, {$new})";
	}

	/**
	 * Determines if the last failure was due to a deadlock
	 */
	function wasDeadlock() {
		return $this->lastErrno() == 1213;
	}

	/**
	 * Perform a deadlock-prone transaction.
	 *
	 * This function invokes a callback function to perform a set of write
	 * queries. If a deadlock occurs during the processing, the transaction
	 * will be rolled back and the callback function will be called again.
	 *
	 * Usage:
	 *   $dbw->deadlockLoop( callback, ... );
	 *
	 * Extra arguments are passed through to the specified callback function.
	 *
	 * Returns whatever the callback function returned on its successful,
	 * iteration, or false on error, for example if the retry limit was
	 * reached.
	 */
	function deadlockLoop() {
		$myFname = 'Database::deadlockLoop';

		$this->begin();
		$args = func_get_args();
		$function = array_shift( $args );
		$oldIgnore = $this->ignoreErrors( true );
		$tries = DEADLOCK_TRIES;
		if ( is_array( $function ) ) {
			$fname = $function[0];
		} else {
			$fname = $function;
		}
		do {
			$retVal = call_user_func_array( $function, $args );
			$error = $this->lastError();
			$errno = $this->lastErrno();
			$sql = $this->lastQuery();

			if ( $errno ) {
				if ( $this->wasDeadlock() ) {
					# Retry
					usleep( mt_rand( DEADLOCK_DELAY_MIN, DEADLOCK_DELAY_MAX ) );
				} else {
					$this->reportQueryError( $error, $errno, $sql, $fname );
				}
			}
		} while( $this->wasDeadlock() && --$tries > 0 );
		$this->ignoreErrors( $oldIgnore );
		if ( $tries <= 0 ) {
			$this->query( 'ROLLBACK', $myFname );
			$this->reportQueryError( $error, $errno, $sql, $fname );
			return false;
		} else {
			$this->query( 'COMMIT', $myFname );
			return $retVal;
		}
	}

	/**
	 * Do a SELECT MASTER_POS_WAIT()
	 *
	 * @param string $file the binlog file
	 * @param string $pos the binlog position
	 * @param integer $timeout the maximum number of seconds to wait for synchronisation
	 */
	function masterPosWait( MySQLMasterPos $pos, $timeout ) {
		$fname = 'Database::masterPosWait';
		wfProfileIn( $fname );

		# Commit any open transactions
		if ( $this->mTrxLevel ) {
			$this->immediateCommit();
		}

		if ( !is_null( $this->mFakeSlaveLag ) ) {
			$wait = intval( ( $pos->pos - microtime(true) + $this->mFakeSlaveLag ) * 1e6 );
			if ( $wait > $timeout * 1e6 ) {
				wfDebug( "Fake slave timed out waiting for $pos ($wait us)\n" );
				wfProfileOut( $fname );
				return -1;
			} elseif ( $wait > 0 ) {
				wfDebug( "Fake slave waiting $wait us\n" );
				usleep( $wait );
				wfProfileOut( $fname );
				return 1;
			} else {
				wfDebug( "Fake slave up to date ($wait us)\n" );
				wfProfileOut( $fname );
				return 0;
			}
		}

		# Call doQuery() directly, to avoid opening a transaction if DBO_TRX is set
		$encFile = $this->addQuotes( $pos->file );
		$encPos = intval( $pos->pos );
		$sql = "SELECT MASTER_POS_WAIT($encFile, $encPos, $timeout)";
		$res = $this->doQuery( $sql );
		if ( $res && $row = $this->fetchRow( $res ) ) {
			$this->freeResult( $res );
			wfProfileOut( $fname );
			return $row[0];
		} else {
			wfProfileOut( $fname );
			return false;
		}
	}

	/**
	 * Get the position of the master from SHOW SLAVE STATUS
	 */
	function getSlavePos() {
		if ( !is_null( $this->mFakeSlaveLag ) ) {
			$pos = new MySQLMasterPos( 'fake', microtime(true) - $this->mFakeSlaveLag );
			wfDebug( __METHOD__.": fake slave pos = $pos\n" );
			return $pos;
		}
		$res = $this->query( 'SHOW SLAVE STATUS', 'Database::getSlavePos' );
		$row = $this->fetchObject( $res );
		if ( $row ) {
			return new MySQLMasterPos( $row->Master_Log_File, $row->Read_Master_Log_Pos );
		} else {
			return false;
		}
	}

	/**
	 * Get the position of the master from SHOW MASTER STATUS
	 */
	function getMasterPos() {
		if ( $this->mFakeMaster ) {
			return new MySQLMasterPos( 'fake', microtime( true ) );
		}
		$res = $this->query( 'SHOW MASTER STATUS', 'Database::getMasterPos' );
		$row = $this->fetchObject( $res );
		if ( $row ) {
			return new MySQLMasterPos( $row->File, $row->Position );
		} else {
			return false;
		}
	}

	/**
	 * Begin a transaction, committing any previously open transaction
	 */
	function begin( $fname = 'Database::begin' ) {
		$this->query( 'BEGIN', $fname );
		$this->mTrxLevel = 1;
	}

	/**
	 * End a transaction
	 */
	function commit( $fname = 'Database::commit' ) {
		$this->query( 'COMMIT', $fname );
		$this->mTrxLevel = 0;
	}

	/**
	 * Rollback a transaction.
	 * No-op on non-transactional databases.
	 */
	function rollback( $fname = 'Database::rollback' ) {
		$this->query( 'ROLLBACK', $fname, true );
		$this->mTrxLevel = 0;
	}

	/**
	 * Begin a transaction, committing any previously open transaction
	 * @deprecated use begin()
	 */
	function immediateBegin( $fname = 'Database::immediateBegin' ) {
		$this->begin();
	}

	/**
	 * Commit transaction, if one is open
	 * @deprecated use commit()
	 */
	function immediateCommit( $fname = 'Database::immediateCommit' ) {
		$this->commit();
	}

	/**
	 * Return MW-style timestamp used for MySQL schema
	 */
	function timestamp( $ts=0 ) {
		return wfTimestamp(TS_MW,$ts);
	}

	/**
	 * Local database timestamp format or null
	 */
	function timestampOrNull( $ts = null ) {
		if( is_null( $ts ) ) {
			return null;
		} else {
			return $this->timestamp( $ts );
		}
	}

	/**
	 * @todo document
	 */
	function resultObject( $result ) {
		if( empty( $result ) ) {
			return false;
		} elseif ( $result instanceof ResultWrapper ) {
			return $result;
		} elseif ( $result === true ) {
			// Successful write query
			return $result;
		} else {
			return new ResultWrapper( $this, $result );
		}
	}

	/**
	 * Return aggregated value alias
	 */
	function aggregateValue ($valuedata,$valuename='value') {
		return $valuename;
	}

	/**
	 * @return string wikitext of a link to the server software's web site
	 */
	function getSoftwareLink() {
		return "[http://www.mysql.com/ MySQL]";
	}

	/**
	 * @return string Version information from the database
	 */
	function getServerVersion() {
		return mysql_get_server_info( $this->mConn );
	}

	/**
	 * Ping the server and try to reconnect if it there is no connection
	 */
	function ping() {
		if( !function_exists( 'mysql_ping' ) ) {
			wfDebug( "Tried to call mysql_ping but this is ancient PHP version. Faking it!\n" );
			return true;
		}
		$ping = mysql_ping( $this->mConn );
		if ( $ping ) {
			return true;
		}

		// Need to reconnect manually in MySQL client 5.0.13+
		if ( version_compare( mysql_get_client_info(), '5.0.13', '>=' ) ) {
			mysql_close( $this->mConn );
			$this->mOpened = false;
			$this->mConn = false;
			$this->open( $this->mServer, $this->mUser, $this->mPassword, $this->mDBname );
			return true;
		}
		return false;
	}

	/**
	 * Get slave lag.
	 * At the moment, this will only work if the DB user has the PROCESS privilege
	 */
	function getLag() {
		if ( !is_null( $this->mFakeSlaveLag ) ) {
			wfDebug( "getLag: fake slave lagged {$this->mFakeSlaveLag} seconds\n" );
			return $this->mFakeSlaveLag;
		}
		$res = $this->query( 'SHOW PROCESSLIST' );
		# Find slave SQL thread
		while ( $row = $this->fetchObject( $res ) ) {
			/* This should work for most situations - when default db 
			 * for thread is not specified, it had no events executed, 
			 * and therefore it doesn't know yet how lagged it is.
			 *
			 * Relay log I/O thread does not select databases.
			 */
			if ( $row->User == 'system user' && 
				$row->State != 'Waiting for master to send event' &&
				$row->State != 'Connecting to master' && 
				$row->State != 'Queueing master event to the relay log' &&
				$row->State != 'Waiting for master update' &&
				$row->State != 'Requesting binlog dump'
				) {
				# This is it, return the time (except -ve)
				if ( $row->Time > 0x7fffffff ) {
					return false;
				} else {
					return $row->Time;
				}
			}
		}
		return false;
	}

	/**
	 * Get status information from SHOW STATUS in an associative array
	 */
	function getStatus($which="%") {
		$res = $this->query( "SHOW STATUS LIKE '{$which}'" );
		$status = array();
		while ( $row = $this->fetchObject( $res ) ) {
			$status[$row->Variable_name] = $row->Value;
		}
		return $status;
	}

	/**
	 * Return the maximum number of items allowed in a list, or 0 for unlimited.
	 */
	function maxListLen() {
		return 0;
	}

	function encodeBlob($b) {
		return $b;
	}

	function decodeBlob($b) {
		return $b;
	}

	/**
	 * Override database's default connection timeout.
	 * May be useful for very long batch queries such as
	 * full-wiki dumps, where a single query reads out
	 * over hours or days.
	 * @param int $timeout in seconds
	 */
	public function setTimeout( $timeout ) {
		$this->query( "SET net_read_timeout=$timeout" );
		$this->query( "SET net_write_timeout=$timeout" );
	}

	/**
	 * Read and execute SQL commands from a file.
	 * Returns true on success, error string or exception on failure (depending on object's error ignore settings)
	 * @param string $filename File name to open
	 * @param callback $lineCallback Optional function called before reading each line
	 * @param callback $resultCallback Optional function called for each MySQL result
	 */
	function sourceFile( $filename, $lineCallback = false, $resultCallback = false ) {
		$fp = fopen( $filename, 'r' );
		if ( false === $fp ) {
			throw new MWException( "Could not open \"{$filename}\".\n" );
		}
		$error = $this->sourceStream( $fp, $lineCallback, $resultCallback );
		fclose( $fp );
		return $error;
	}

	/**
	 * Read and execute commands from an open file handle
	 * Returns true on success, error string or exception on failure (depending on object's error ignore settings)
	 * @param string $fp File handle
	 * @param callback $lineCallback Optional function called before reading each line
	 * @param callback $resultCallback Optional function called for each MySQL result
	 */
	function sourceStream( $fp, $lineCallback = false, $resultCallback = false ) {
		$cmd = "";
		$done = false;
		$dollarquote = false;

		while ( ! feof( $fp ) ) {
			if ( $lineCallback ) {
				call_user_func( $lineCallback );
			}
			$line = trim( fgets( $fp, 1024 ) );
			$sl = strlen( $line ) - 1;

			if ( $sl < 0 ) { continue; }
			if ( '-' == $line{0} && '-' == $line{1} ) { continue; }

			## Allow dollar quoting for function declarations
			if (substr($line,0,4) == '$mw$') {
				if ($dollarquote) {
					$dollarquote = false;
					$done = true;
				}
				else {
					$dollarquote = true;
				}
			}
			else if (!$dollarquote) {
				if ( ';' == $line{$sl} && ($sl < 2 || ';' != $line{$sl - 1})) {
					$done = true;
					$line = substr( $line, 0, $sl );
				}
			}

			if ( '' != $cmd ) { $cmd .= ' '; }
			$cmd .= "$line\n";

			if ( $done ) {
				$cmd = str_replace(';;', ";", $cmd);
				$cmd = $this->replaceVars( $cmd );
				$res = $this->query( $cmd, __METHOD__ );
				if ( $resultCallback ) {
					call_user_func( $resultCallback, $res );
				}

				if ( false === $res ) {
					$err = $this->lastError();
					return "Query \"{$cmd}\" failed with error code \"$err\".\n";
				}

				$cmd = '';
				$done = false;
			}
		}
		return true;
	}


	/**
	 * Replace variables in sourced SQL
	 */
	protected function replaceVars( $ins ) {
		$varnames = array(
			'wgDBserver', 'wgDBname', 'wgDBintlname', 'wgDBuser',
			'wgDBpassword', 'wgDBsqluser', 'wgDBsqlpassword',
			'wgDBadminuser', 'wgDBadminpassword', 'wgDBTableOptions',
		);

		// Ordinary variables
		foreach ( $varnames as $var ) {
			if( isset( $GLOBALS[$var] ) ) {
				$val = addslashes( $GLOBALS[$var] ); // FIXME: safety check?
				$ins = str_replace( '{$' . $var . '}', $val, $ins );
				$ins = str_replace( '/*$' . $var . '*/`', '`' . $val, $ins );
				$ins = str_replace( '/*$' . $var . '*/', $val, $ins );
			}
		}

		// Table prefixes
		$ins = preg_replace_callback( '/\/\*(?:\$wgDBprefix|_)\*\/([a-zA-Z_0-9]*)/',
			array( &$this, 'tableNameCallback' ), $ins );
		return $ins;
	}

	/**
	 * Table name callback
	 * @private
	 */
	protected function tableNameCallback( $matches ) {
		return $this->tableName( $matches[1] );
	}

	/*
	 * Build a concatenation list to feed into a SQL query
	*/
	function buildConcat( $stringList ) {
		return 'CONCAT(' . implode( ',', $stringList ) . ')';
	}
	
	/**
	 * Acquire a lock
	 * 
	 * Abstracted from Filestore::lock() so child classes can implement for
	 * their own needs.
	 * 
	 * @param string $lockName Name of lock to aquire
	 * @param string $method Name of method calling us
	 * @return bool
	 */
	public function lock( $lockName, $method ) {
		$lockName = $this->addQuotes( $lockName );
		$result = $this->query( "SELECT GET_LOCK($lockName, 5) AS lockstatus", $method );
		$row = $this->fetchObject( $result );
		$this->freeResult( $result );

		if( $row->lockstatus == 1 ) {
			return true;
		} else {
			wfDebug( __METHOD__." failed to acquire lock\n" );
			return false;
		}
	}
	/**
	 * Release a lock.
	 * 
	 * @todo fixme - Figure out a way to return a bool
	 * based on successful lock release.
	 * 
	 * @param string $lockName Name of lock to release
	 * @param string $method Name of method calling us
	 */
	public function unlock( $lockName, $method ) {
		$lockName = $this->addQuotes( $lockName );
		$result = $this->query( "SELECT RELEASE_LOCK($lockName)", $method );
		$this->freeResult( $result );
	}
}

/**
 * Database abstraction object for mySQL
 * Inherit all methods and properties of Database::Database()
 *
 * @ingroup Database
 * @see Database
 */
class DatabaseMysql extends Database {
	# Inherit all
}

/******************************************************************************
 * Utility classes
 *****************************************************************************/

/**
 * Utility class.
 * @ingroup Database
 */
class DBObject {
	public $mData;

	function DBObject($data) {
		$this->mData = $data;
	}

	function isLOB() {
		return false;
	}

	function data() {
		return $this->mData;
	}
}

/**
 * Utility class
 * @ingroup Database
 *
 * This allows us to distinguish a blob from a normal string and an array of strings
 */
class Blob {
	private $mData;
	function __construct($data) {
		$this->mData = $data;
	}
	function fetch() {
		return $this->mData;
	}
}

/**
 * Utility class.
 * @ingroup Database
 */
class MySQLField {
	private $name, $tablename, $default, $max_length, $nullable,
		$is_pk, $is_unique, $is_multiple, $is_key, $type;
	function __construct ($info) {
		$this->name = $info->name;
		$this->tablename = $info->table;
		$this->default = $info->def;
		$this->max_length = $info->max_length;
		$this->nullable = !$info->not_null;
		$this->is_pk = $info->primary_key;
		$this->is_unique = $info->unique_key;
		$this->is_multiple = $info->multiple_key;
		$this->is_key = ($this->is_pk || $this->is_unique || $this->is_multiple);
		$this->type = $info->type;
	}

	function name() {
		return $this->name;
	}

	function tableName() {
		return $this->tableName;
	}

	function defaultValue() {
		return $this->default;
	}

	function maxLength() {
		return $this->max_length;
	}

	function nullable() {
		return $this->nullable;
	}

	function isKey() {
		return $this->is_key;
	}

	function isMultipleKey() {
		return $this->is_multiple;
	}

	function type() {
		return $this->type;
	}
}

/******************************************************************************
 * Error classes
 *****************************************************************************/

/**
 * Database error base class
 * @ingroup Database
 */
class DBError extends MWException {
	public $db;

	/**
	 * Construct a database error
	 * @param Database $db The database object which threw the error
	 * @param string $error A simple error message to be used for debugging
	 */
	function __construct( Database &$db, $error ) {
		$this->db =& $db;
		parent::__construct( $error );
	}
}

/**
 * @ingroup Database
 */
class DBConnectionError extends DBError {
	public $error;
	
	function __construct( Database &$db, $error = 'unknown error' ) {
		$msg = 'DB connection error';
		if ( trim( $error ) != '' ) {
			$msg .= ": $error";
		}
		$this->error = $error;
		parent::__construct( $db, $msg );
	}

	function useOutputPage() {
		// Not likely to work
		return false;
	}

	function useMessageCache() {
		// Not likely to work
		return false;
	}
	
	function getText() {
		return $this->getMessage() . "\n";
	}

	function getLogMessage() {
		# Don't send to the exception log
		return false;
	}

	function getPageTitle() {
		global $wgSitename;
		return "$wgSitename has a problem";
	}

	function getHTML() {
		global $wgTitle, $wgUseFileCache, $title, $wgInputEncoding;
		global $wgSitename, $wgServer, $wgMessageCache;

		# I give up, Brion is right. Getting the message cache to work when there is no DB is tricky.
		# Hard coding strings instead.

		$noconnect = "<p><strong>Sorry! This site is experiencing technical difficulties.</strong></p><p>Try waiting a few minutes and reloading.</p><p><small>(Can't contact the database server: $1)</small></p>";
		$mainpage = 'Main Page';
		$searchdisabled = <<<EOT
<p style="margin: 1.5em 2em 1em">$wgSitename search is disabled for performance reasons. You can search via Google in the meantime.
<span style="font-size: 89%; display: block; margin-left: .2em">Note that their indexes of $wgSitename content may be out of date.</span></p>',
EOT;

		$googlesearch = "
<!-- SiteSearch Google -->
<FORM method=GET action=\"http://www.google.com/search\">
<TABLE bgcolor=\"#FFFFFF\"><tr><td>
<A HREF=\"http://www.google.com/\">
<IMG SRC=\"http://www.google.com/logos/Logo_40wht.gif\"
border=\"0\" ALT=\"Google\"></A>
</td>
<td>
<INPUT TYPE=text name=q size=31 maxlength=255 value=\"$1\">
<INPUT type=submit name=btnG VALUE=\"Google Search\">
<font size=-1>
<input type=hidden name=domains value=\"$wgServer\"><br /><input type=radio name=sitesearch value=\"\"> WWW <input type=radio name=sitesearch value=\"$wgServer\" checked> $wgServer <br />
<input type='hidden' name='ie' value='$2'>
<input type='hidden' name='oe' value='$2'>
</font>
</td></tr></TABLE>
</FORM>
<!-- SiteSearch Google -->";
		$cachederror = "The following is a cached copy of the requested page, and may not be up to date. ";

		# No database access
		if ( is_object( $wgMessageCache ) ) {
			$wgMessageCache->disable();
		}

		if ( trim( $this->error ) == '' ) {
			$this->error = $this->db->getProperty('mServer');
		}

		$text = str_replace( '$1', $this->error, $noconnect );
		$text .= wfGetSiteNotice();

		if($wgUseFileCache) {
			if($wgTitle) {
				$t =& $wgTitle;
			} else {
				if($title) {
					$t = Title::newFromURL( $title );
				} elseif (@/**/$_REQUEST['search']) {
					$search = $_REQUEST['search'];
					return $searchdisabled .
					  str_replace( array( '$1', '$2' ), array( htmlspecialchars( $search ),
					  $wgInputEncoding ), $googlesearch );
				} else {
					$t = Title::newFromText( $mainpage );
				}
			}

			$cache = new HTMLFileCache( $t );
			if( $cache->isFileCached() ) {
				// @todo, FIXME: $msg is not defined on the next line.
				$msg = '<p style="color: red"><b>'.$msg."<br />\n" .
					$cachederror . "</b></p>\n";

				$tag = '<div id="article">';
				$text = str_replace(
					$tag,
					$tag . $msg,
					$cache->fetchPageText() );
			}
		}

		return $text;
	}
}

/**
 * @ingroup Database
 */
class DBQueryError extends DBError {
	public $error, $errno, $sql, $fname;
	
	function __construct( Database &$db, $error, $errno, $sql, $fname ) {
		$message = "A database error has occurred\n" .
		  "Query: $sql\n" .
		  "Function: $fname\n" .
		  "Error: $errno $error\n";

		parent::__construct( $db, $message );
		$this->error = $error;
		$this->errno = $errno;
		$this->sql = $sql;
		$this->fname = $fname;
	}

	function getText() {
		if ( $this->useMessageCache() ) {
			return wfMsg( 'dberrortextcl', htmlspecialchars( $this->getSQL() ),
			  htmlspecialchars( $this->fname ), $this->errno, htmlspecialchars( $this->error ) ) . "\n";
		} else {
			return $this->getMessage();
		}
	}
	
	function getSQL() {
		global $wgShowSQLErrors;
		if( !$wgShowSQLErrors ) {
			return $this->msg( 'sqlhidden', 'SQL hidden' );
		} else {
			return $this->sql;
		}
	}
	
	function getLogMessage() {
		# Don't send to the exception log
		return false;
	}

	function getPageTitle() {
		return $this->msg( 'databaseerror', 'Database error' );
	}

	function getHTML() {
		if ( $this->useMessageCache() ) {
			return wfMsgNoDB( 'dberrortext', htmlspecialchars( $this->getSQL() ),
			  htmlspecialchars( $this->fname ), $this->errno, htmlspecialchars( $this->error ) );
		} else {
			return nl2br( htmlspecialchars( $this->getMessage() ) );
		}
	}
}

/**
 * @ingroup Database
 */
class DBUnexpectedError extends DBError {}


/**
 * Result wrapper for grabbing data queried by someone else
 * @ingroup Database
 */
class ResultWrapper implements Iterator {
	var $db, $result, $pos = 0, $currentRow = null;

	/**
	 * Create a new result object from a result resource and a Database object
	 */
	function ResultWrapper( $database, $result ) {
		$this->db = $database;
		if ( $result instanceof ResultWrapper ) {
			$this->result = $result->result;
		} else {
			$this->result = $result;
		}
	}

	/**
	 * Get the number of rows in a result object
	 */
	function numRows() {
		return $this->db->numRows( $this->result );
	}

	/**
	 * Fetch the next row from the given result object, in object form.
	 * Fields can be retrieved with $row->fieldname, with fields acting like
	 * member variables.
	 *
	 * @param $res SQL result object as returned from Database::query(), etc.
	 * @return MySQL row object
	 * @throws DBUnexpectedError Thrown if the database returns an error
	 */
	function fetchObject() {
		return $this->db->fetchObject( $this->result );
	}

	/**
	 * Fetch the next row from the given result object, in associative array
	 * form.  Fields are retrieved with $row['fieldname'].
	 *
	 * @param $res SQL result object as returned from Database::query(), etc.
	 * @return MySQL row object
	 * @throws DBUnexpectedError Thrown if the database returns an error
	 */
	function fetchRow() {
		return $this->db->fetchRow( $this->result );
	}

	/**
	 * Free a result object
	 */
	function free() {
		$this->db->freeResult( $this->result );
		unset( $this->result );
		unset( $this->db );
	}

	/**
	 * Change the position of the cursor in a result object
	 * See mysql_data_seek()
	 */
	function seek( $row ) {
		$this->db->dataSeek( $this->result, $row );
	}

	/*********************
	 * Iterator functions
	 * Note that using these in combination with the non-iterator functions
	 * above may cause rows to be skipped or repeated.
	 */

	function rewind() {
		if ($this->numRows()) {
			$this->db->dataSeek($this->result, 0);
		}
		$this->pos = 0;
		$this->currentRow = null;
	}

	function current() {
		if ( is_null( $this->currentRow ) ) {
			$this->next();
		}
		return $this->currentRow;
	}

	function key() {
		return $this->pos;
	}

	function next() {
		$this->pos++;
		$this->currentRow = $this->fetchObject();
		return $this->currentRow;
	}

	function valid() {
		return $this->current() !== false;
	}
}

class MySQLMasterPos {
	var $file, $pos;

	function __construct( $file, $pos ) {
		$this->file = $file;
		$this->pos = $pos;
	}

	function __toString() {
		return "{$this->file}/{$this->pos}";
	}
}
