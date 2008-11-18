<?php
/**
 * This script is the MSSQL Server database abstraction layer
 *
 * See maintenance/mssql/README for development notes and other specific information
 * @ingroup Database
 * @file
 */

/**
 * @ingroup Database
 */
class DatabaseMssql extends Database {

	var $mAffectedRows;
	var $mLastResult;
	var $mLastError;
	var $mLastErrorNo;
	var $mDatabaseFile;

	/**
	 * Constructor
	 */
	function __construct($server = false, $user = false, $password = false, $dbName = false,
			$failFunction = false, $flags = 0, $tablePrefix = 'get from global') {

		global $wgOut, $wgDBprefix, $wgCommandLineMode;
		if (!isset($wgOut)) $wgOut = NULL; # Can't get a reference if it hasn't been set yet
		$this->mOut =& $wgOut;
		$this->mFailFunction = $failFunction;
		$this->mFlags = $flags;

		if ( $this->mFlags & DBO_DEFAULT ) {
			if ( $wgCommandLineMode ) {
				$this->mFlags &= ~DBO_TRX;
			} else {
				$this->mFlags |= DBO_TRX;
			}
		}

		/** Get the default table prefix*/
		$this->mTablePrefix = $tablePrefix == 'get from global' ? $wgDBprefix : $tablePrefix;

		if ($server) $this->open($server, $user, $password, $dbName);

	}

	/**
	 * todo: check if these should be true like parent class
	 */
	function implicitGroupby()   { return false; }
	function implicitOrderby()   { return false; }

	static function newFromParams($server, $user, $password, $dbName, $failFunction = false, $flags = 0) {
		return new DatabaseMssql($server, $user, $password, $dbName, $failFunction, $flags);
	}

	/** Open an MSSQL database and return a resource handle to it
	 *  NOTE: only $dbName is used, the other parameters are irrelevant for MSSQL databases
	 */
	function open($server,$user,$password,$dbName) {
		wfProfileIn(__METHOD__);

		# Test for missing mysql.so
		# First try to load it
		if (!@extension_loaded('mssql')) {
			@dl('mssql.so');
		}

		# Fail now
		# Otherwise we get a suppressed fatal error, which is very hard to track down
		if (!function_exists( 'mssql_connect')) {
			throw new DBConnectionError( $this, "MSSQL functions missing, have you compiled PHP with the --with-mssql option?\n" );
		}

		$this->close();
		$this->mServer   = $server;
		$this->mUser     = $user;
		$this->mPassword = $password;
		$this->mDBname   = $dbName;

		wfProfileIn("dbconnect-$server");

		# Try to connect up to three times
		# The kernel's default SYN retransmission period is far too slow for us,
		# so we use a short timeout plus a manual retry.
		$this->mConn = false;
		$max = 3;
		for ( $i = 0; $i < $max && !$this->mConn; $i++ ) {
			if ( $i > 1 ) {
				usleep( 1000 );
			}
			if ($this->mFlags & DBO_PERSISTENT) {
				@/**/$this->mConn = mssql_pconnect($server, $user, $password);
			} else {
				# Create a new connection...
				@/**/$this->mConn = mssql_connect($server, $user, $password, true);
			}
		}
		
		wfProfileOut("dbconnect-$server");

		if ($dbName != '') {
			if ($this->mConn !== false) {
				$success = @/**/mssql_select_db($dbName, $this->mConn);
				if (!$success) {
					$error = "Error selecting database $dbName on server {$this->mServer} " .
						"from client host {$wguname['nodename']}\n";
					wfLogDBError(" Error selecting database $dbName on server {$this->mServer} \n");
					wfDebug( $error );
				}
			} else {
				wfDebug("DB connection error\n");
				wfDebug("Server: $server, User: $user, Password: ".substr($password, 0, 3)."...\n");
				$success = false;
			}
		} else {
			# Delay USE query
			$success = (bool)$this->mConn;
		}

		if (!$success) $this->reportConnectionError();
		$this->mOpened = $success;
		wfProfileOut(__METHOD__);
		return $success;
	}

	/**
	 * Close an MSSQL database
	 */
	function close() {
		$this->mOpened = false;
		if ($this->mConn) {
			if ($this->trxLevel()) $this->immediateCommit();
			return mssql_close($this->mConn);
		} else return true;
	}

	/**
	 * - MSSQL doesn't seem to do buffered results
	 * - the trasnaction syntax is modified here to avoid having to replicate
	 *   Database::query which uses BEGIN, COMMIT, ROLLBACK
	 */
	function doQuery($sql) {
		if ($sql == 'BEGIN' || $sql == 'COMMIT' || $sql == 'ROLLBACK') return true; # $sql .= ' TRANSACTION';
		$sql = preg_replace('|[^\x07-\x7e]|','?',$sql); # TODO: need to fix unicode - just removing it here while testing
		$ret = mssql_query($sql, $this->mConn);
		if ($ret === false) {
			$err = mssql_get_last_message();
			if ($err) $this->mlastError = $err;
			$row = mssql_fetch_row(mssql_query('select @@ERROR'));
			if ($row[0]) $this->mlastErrorNo = $row[0];
		} else $this->mlastErrorNo = false;
		return $ret;
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
		if ( !@/**/mssql_free_result( $res ) ) {
			throw new DBUnexpectedError( $this, "Unable to free MSSQL result" );
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
		@/**/$row = mssql_fetch_object( $res );
		if ( $this->lastErrno() ) {
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
		@/**/$row = mssql_fetch_array( $res );
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
		@/**/$n = mssql_num_rows( $res );
		if ( $this->lastErrno() ) {
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
		return mssql_num_fields( $res );
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
		return mssql_field_name( $res, $n );
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
	function insertId() {
		$row = mssql_fetch_row(mssql_query('select @@IDENTITY'));
		return $row[0];
	}

	/**
	 * Change the position of the cursor in a result object
	 * See mysql_data_seek()
	 */
	function dataSeek( $res, $row ) {
		if ( $res instanceof ResultWrapper ) {
			$res = $res->result;
		}
		return mssql_data_seek( $res, $row );
	}

	/**
	 * Get the last error number
	 */
	function lastErrno() {
		return $this->mlastErrorNo;
	}

	/**
	 * Get a description of the last error
	 */
	function lastError() {
		return $this->mlastError;
	}

	/**
	 * Get the number of rows affected by the last write query
	 */
	function affectedRows() {
		return mssql_rows_affected( $this->mConn );
	}

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
		if ($value == "NULL") $value = "''"; # see comments in makeListWithoutNulls()
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
	 * @return mixed Database result resource (feed to Database::fetchObject or whatever), or false on failure
	 */
	function select( $table, $vars, $conds='', $fname = 'Database::select', $options = array() )
	{
		if( is_array( $vars ) ) {
			$vars = implode( ',', $vars );
		}
		if( !is_array( $options ) ) {
			$options = array( $options );
		}
		if( is_array( $table ) ) {
			if ( isset( $options['USE INDEX'] ) && is_array( $options['USE INDEX'] ) )
				$from = ' FROM ' . $this->tableNamesWithUseIndex( $table, $options['USE INDEX'] );
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
		return $this->query( $sql, $fname );
	}

	/**
	 * Estimate rows in dataset
	 * Returns estimated count, based on EXPLAIN output
	 * Takes same arguments as Database::select()
	 */
	function estimateRowCount( $table, $vars='*', $conds='', $fname = 'Database::estimateRowCount', $options = array() ) {
		$rows = 0;
		$res = $this->select ($table, 'COUNT(*)', $conds, $fname, $options );
		if ($res) {
			$row = $this->fetchObject($res);
			$rows = $row[0];
		}
		$this->freeResult($res);
		return $rows;
	}
	
	/**
	 * Determines whether a field exists in a table
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns NULL on failure
	 */
	function fieldExists( $table, $field, $fname = 'Database::fieldExists' ) {
		$table = $this->tableName( $table );
		$sql = "SELECT TOP 1 * FROM $table";
		$res = $this->query( $sql, 'Database::fieldExists' );

		$found = false;
		while ( $row = $this->fetchArray( $res ) ) {
			if ( isset($row[$field]) ) {
				$found = true;
				break;
			}
		}

		$this->freeResult( $res );
		return $found;
	}

	/**
	 * Get information about an index into an object
	 * Returns false if the index does not exist
	 */
	function indexInfo( $table, $index, $fname = 'Database::indexInfo' ) {

		throw new DBUnexpectedError( $this, 'Database::indexInfo called which is not supported yet' );
		return NULL;

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
		$res = $this->query( "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = '$table'" );
		$exist = ($res->numRows() > 0);
		$this->freeResult($res);
		return $exist;
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
		$res = $this->query( "SELECT TOP 1 * FROM $table" );
		$n = mssql_num_fields( $res->result );
		for( $i = 0; $i < $n; $i++ ) {
			$meta = mssql_fetch_field( $res->result, $i );
			if( $field == $meta->name ) {
				return new MSSQLField($meta);
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
		return mssql_field_type( $res, $index );
	}

	/**
	 * INSERT wrapper, inserts an array into a table
	 *
	 * $a may be a single associative array, or an array of these with numeric keys, for
	 * multi-row insert.
	 *
	 * Usually aborts on failure
	 * If errors are explicitly ignored, returns success
	 * 
	 * Same as parent class implementation except that it removes primary key from column lists
	 * because MSSQL doesn't support writing nulls to IDENTITY (AUTO_INCREMENT) columns
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
		
		# todo: need to record primary keys at table create time, and remove NULL assignments to them
		if ( isset( $a[0] ) && is_array( $a[0] ) ) {
			$multi = true;
			$keys = array_keys( $a[0] );
#			if (ereg('_id$',$keys[0])) {
				foreach ($a as $i) {
					if (is_null($i[$keys[0]])) unset($i[$keys[0]]); # remove primary-key column from multiple insert lists if empty value
				}
#			}
			$keys = array_keys( $a[0] );
		} else {
			$multi = false;
			$keys = array_keys( $a );
#			if (ereg('_id$',$keys[0]) && empty($a[$keys[0]])) unset($a[$keys[0]]); # remove primary-key column from insert list if empty value
			if (is_null($a[$keys[0]])) unset($a[$keys[0]]); # remove primary-key column from insert list if empty value
			$keys = array_keys( $a );
		}

		# handle IGNORE option
		# example:
		#   MySQL: INSERT IGNORE INTO user_groups (ug_user,ug_group) VALUES ('1','sysop')
		#   MSSQL: IF NOT EXISTS (SELECT * FROM user_groups WHERE ug_user = '1') INSERT INTO user_groups (ug_user,ug_group) VALUES ('1','sysop')
		$ignore = in_array('IGNORE',$options);

		# remove IGNORE from options list
		if ($ignore) {
			$oldoptions = $options;
			$options = array();
			foreach ($oldoptions as $o) if ($o != 'IGNORE') $options[] = $o;
		}

		$keylist = implode(',', $keys);
		$sql = 'INSERT '.implode(' ', $options)." INTO $table (".implode(',', $keys).') VALUES ';
		if ($multi) {
			if ($ignore) {
				# If multiple and ignore, then do each row as a separate conditional insert
				foreach ($a as $row) {
					$prival = $row[$keys[0]];
					$sql = "IF NOT EXISTS (SELECT * FROM $table WHERE $keys[0] = '$prival') $sql";
					if (!$this->query("$sql (".$this->makeListWithoutNulls($row).')', $fname)) return false;
				}
				return true;
			} else {
				$first = true;
				foreach ($a as $row) {
					if ($first) $first = false; else $sql .= ',';
					$sql .= '('.$this->makeListWithoutNulls($row).')';
				}
			}
		} else {
			if ($ignore) {
				$prival = $a[$keys[0]];
				$sql = "IF NOT EXISTS (SELECT * FROM $table WHERE $keys[0] = '$prival') $sql";
			}
			$sql .= '('.$this->makeListWithoutNulls($a).')';
		}
		return (bool)$this->query( $sql, $fname );
	}

	/**
	 * MSSQL doesn't allow implicit casting of NULL's into non-null values for NOT NULL columns
	 *   for now I've just converted the NULL's in the lists for updates and inserts into empty strings
	 *   which get implicitly casted to 0 for numeric columns
	 * NOTE: the set() method above converts NULL to empty string as well but not via this method
	 */
	function makeListWithoutNulls($a, $mode = LIST_COMMA) {
		return str_replace("NULL","''",$this->makeList($a,$mode));
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
		$sql = "UPDATE $opts $table SET " . $this->makeListWithoutNulls( $values, LIST_SET );
		if ( $conds != '*' ) {
			$sql .= " WHERE " . $this->makeList( $conds, LIST_AND );
		}
		return $this->query( $sql, $fname );
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
	 * Change the current database
	 */
	function selectDB( $db ) {
		$this->mDBname = $db;
		return mssql_select_db( $db, $this->mConn );
	}

	/**
	 * MSSQL has a problem with the backtick quoting, so all this does is ensure the prefix is added exactly once
	 */
	function tableName($name) {
		return strpos($name, $this->mTablePrefix) === 0 ? $name : "{$this->mTablePrefix}$name";
	}

	/**
	 * MSSQL doubles quotes instead of escaping them
	 * @param string $s String to be slashed.
	 * @return string slashed string.
	 */
	function strencode($s) {
		return str_replace("'","''",$s);
	}

	/**
	 * USE INDEX clause
	 */
	function useIndexClause( $index ) {
		return "";
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
		$sql = "SELECT TOP 1 * FROM $table;";
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
	 * $sql string SQL query we will append the limit to
	 * $limit integer the SQL limit
	 * $offset integer the SQL offset (default false)
	 */
	function limitResult($sql, $limit, $offset=false) {
		if( !is_numeric($limit) ) {
			throw new DBUnexpectedError( $this, "Invalid non-numeric limit passed to limitResult()\n" );
		}
		if ($offset) {
			throw new DBUnexpectedError( $this, 'Database::limitResult called with non-zero offset which is not supported yet' );
		} else {
			$sql = ereg_replace("^SELECT", "SELECT TOP $limit", $sql);
		}
		return $sql;
	}

	/**
	 * Returns an SQL expression for a simple conditional.
	 *
	 * @param string $cond SQL expression which will result in a boolean value
	 * @param string $trueVal SQL expression to return if true
	 * @param string $falseVal SQL expression to return if false
	 * @return string SQL fragment
	 */
	function conditional( $cond, $trueVal, $falseVal ) {
		return " (CASE WHEN $cond THEN $trueVal ELSE $falseVal END) ";
	}

	/**
	 * Should determine if the last failure was due to a deadlock
	 * @return bool
	 */
	function wasDeadlock() {
		return $this->lastErrno() == 1205;
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
	 * @return string wikitext of a link to the server software's web site
	 */
	function getSoftwareLink() {
		return "[http://www.microsoft.com/sql/default.mspx Microsoft SQL Server 2005 Home]";
	}

	/**
	 * @return string Version information from the database
	 */
	function getServerVersion() {
		$row = mssql_fetch_row(mssql_query('select @@VERSION'));
		return ereg("^(.+[0-9]+\\.[0-9]+\\.[0-9]+) ",$row[0],$m) ? $m[1] : $row[0];
	}

	function limitResultForUpdate($sql, $num) {
		return $sql;
	}

	/**
	 * not done
	 */
	public function setTimeout($timeout) { return; }

	function ping() {
		wfDebug("Function ping() not written for MSSQL yet");
		return true;
	}

	/**
	 * How lagged is this slave?
	 */
	public function getLag() {
		return 0;
	}

	/**
	 * Called by the installer script
	 * - this is the same way as DatabasePostgresql.php, MySQL reads in tables.sql and interwiki.sql using dbsource (which calls db->sourceFile)
	 */
	public function setup_database() {
		global $IP,$wgDBTableOptions;
		$wgDBTableOptions = '';
		$mysql_tmpl = "$IP/maintenance/tables.sql";
		$mysql_iw   = "$IP/maintenance/interwiki.sql";
		$mssql_tmpl = "$IP/maintenance/mssql/tables.sql";

		# Make an MSSQL template file if it doesn't exist (based on the same one MySQL uses to create a new wiki db)
		if (!file_exists($mssql_tmpl)) { # todo: make this conditional again
			$sql = file_get_contents($mysql_tmpl);
			$sql = preg_replace('/^\s*--.*?$/m','',$sql); # strip comments
			$sql = preg_replace('/^\s*(UNIQUE )?(INDEX|KEY|FULLTEXT).+?$/m', '', $sql); # These indexes should be created with a CREATE INDEX query
			$sql = preg_replace('/(\sKEY) [^\(]+\(/is', '$1 (', $sql); # "KEY foo (foo)" should just be "KEY (foo)"
			$sql = preg_replace('/(varchar\([0-9]+\))\s+binary/i', '$1', $sql); # "varchar(n) binary" cannot be followed by "binary"
			$sql = preg_replace('/(var)?binary\(([0-9]+)\)/ie', '"varchar(".strlen(pow(2,$2)).")"', $sql); # use varchar(chars) not binary(bits)
			$sql = preg_replace('/ (var)?binary/i', ' varchar', $sql); # use varchar not binary
			$sql = preg_replace('/(varchar\([0-9]+\)(?! N))/', '$1 NULL', $sql); # MSSQL complains if NULL is put into a varchar
			#$sql = preg_replace('/ binary/i',' varchar',$sql); # MSSQL binary's can't be assigned with strings, so use varchar's instead
			#$sql = preg_replace('/(binary\([0-9]+\) (NOT NULL )?default) [\'"].*?[\'"]/i','$1 0',$sql); # binary default cannot be string
			$sql = preg_replace('/[a-z]*(blob|text)([ ,])/i', 'text$2', $sql); # no BLOB types in MSSQL
			$sql = preg_replace('/\).+?;/',');', $sql); # remove all table options
			$sql = preg_replace('/ (un)?signed/i', '', $sql);
			$sql = preg_replace('/ENUM\(.+?\)/','TEXT',$sql); # Make ENUM's into TEXT's
			$sql = str_replace(' bool ', ' bit ', $sql);
			$sql = str_replace('auto_increment', 'IDENTITY(1,1)', $sql);
			#$sql = preg_replace('/NOT NULL(?! IDENTITY)/', 'NULL', $sql); # Allow NULL's for non IDENTITY columns

			# Tidy up and write file
			$sql = preg_replace('/,\s*\)/s', "\n)", $sql); # Remove spurious commas left after INDEX removals
			$sql = preg_replace('/^\s*^/m', '', $sql); # Remove empty lines
			$sql = preg_replace('/;$/m', ";\n", $sql); # Separate each statement with an empty line
			file_put_contents($mssql_tmpl, $sql);
		}

		# Parse the MSSQL template replacing inline variables such as /*$wgDBprefix*/
		$err = $this->sourceFile($mssql_tmpl);
		if ($err !== true) $this->reportQueryError($err,0,$sql,__FUNCTION__);

		# Use DatabasePostgres's code to populate interwiki from MySQL template
		$f = fopen($mysql_iw,'r');
		if ($f == false) dieout("<li>Could not find the interwiki.sql file");
		$sql = "INSERT INTO {$this->mTablePrefix}interwiki(iw_prefix,iw_url,iw_local) VALUES ";
		while (!feof($f)) {
			$line = fgets($f,1024);
			$matches = array();
			if (!preg_match('/^\s*(\(.+?),(\d)\)/', $line, $matches)) continue;
			$this->query("$sql $matches[1],$matches[2])");
		}
	}
	
	/** 
	 * No-op lock functions
	 */
	public function lock( $lockName, $method ) {
		return true;
	}
	public function unlock( $lockName, $method ) {
		return true;
	}

}

/**
 * @ingroup Database
 */
class MSSQLField extends MySQLField {

	function __construct() {
	}

	static function fromText($db, $table, $field) {
		$n = new MSSQLField;
		$n->name = $field;
		$n->tablename = $table;
		return $n;
	}

} // end DatabaseMssql class

