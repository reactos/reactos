<?php
/**
 * @ingroup Database
 * @file
 */

/**
 * This is the Oracle database abstraction layer.
 * @ingroup Database
 */
class ORABlob {
	var $mData;

	function __construct($data) {
		$this->mData = $data;
	}

	function getData() {
		return $this->mData;
	}
}

/**
 * The oci8 extension is fairly weak and doesn't support oci_num_rows, among
 * other things.  We use a wrapper class to handle that and other
 * Oracle-specific bits, like converting column names back to lowercase.
 * @ingroup Database
 */
class ORAResult {
	private $rows;
	private $cursor;
	private $stmt;
	private $nrows;
	private $db;

	function __construct(&$db, $stmt) {
		$this->db =& $db;
		if (($this->nrows = oci_fetch_all($stmt, $this->rows, 0, -1, OCI_FETCHSTATEMENT_BY_ROW | OCI_NUM)) === false) {
			$e = oci_error($stmt);
			$db->reportQueryError($e['message'], $e['code'], '', __FUNCTION__);
			return;
		}

		$this->cursor = 0;
		$this->stmt = $stmt;
	}

	function free() {
		oci_free_statement($this->stmt);
	}

	function seek($row) {
		$this->cursor = min($row, $this->nrows);
	}

	function numRows() {
		return $this->nrows;
	}

	function numFields() {
		return oci_num_fields($this->stmt);
	}

	function fetchObject() {
		if ($this->cursor >= $this->nrows)
			return false;

		$row = $this->rows[$this->cursor++];
		$ret = new stdClass();
		foreach ($row as $k => $v) {
			$lc = strtolower(oci_field_name($this->stmt, $k + 1));
			$ret->$lc = $v;
		}

		return $ret;
	}

	function fetchAssoc() {
		if ($this->cursor >= $this->nrows)
			return false;

		$row = $this->rows[$this->cursor++];
		$ret = array();
		foreach ($row as $k => $v) {
			$lc = strtolower(oci_field_name($this->stmt, $k + 1));
			$ret[$lc] = $v;
			$ret[$k] = $v;
		}
		return $ret;
	}
}

/**
 * @ingroup Database
 */
class DatabaseOracle extends Database {
	var $mInsertId = NULL;
	var $mLastResult = NULL;
	var $numeric_version = NULL;
	var $lastResult = null;
	var $cursor = 0;
	var $mAffectedRows;

	function DatabaseOracle($server = false, $user = false, $password = false, $dbName = false,
		$failFunction = false, $flags = 0 )
	{

		global $wgOut;
		# Can't get a reference if it hasn't been set yet
		if ( !isset( $wgOut ) ) {
			$wgOut = NULL;
		}
		$this->mOut =& $wgOut;
		$this->mFailFunction = $failFunction;
		$this->mFlags = $flags;
		$this->open( $server, $user, $password, $dbName);

	}

	function cascadingDeletes() {
		return true;
	}
	function cleanupTriggers() {
		return true;
	}
	function strictIPs() {
		return true;
	}
	function realTimestamps() {
		return true;
	}
	function implicitGroupby() {
		return false;
	}
	function implicitOrderby() {
		return false;
	}
	function searchableIPs() {
		return true;
	}

	static function newFromParams( $server = false, $user = false, $password = false, $dbName = false,
		$failFunction = false, $flags = 0)
	{
		return new DatabaseOracle( $server, $user, $password, $dbName, $failFunction, $flags );
	}

	/**
	 * Usually aborts on failure
	 * If the failFunction is set to a non-zero integer, returns success
	 */
	function open( $server, $user, $password, $dbName ) {
		if ( !function_exists( 'oci_connect' ) ) {
			throw new DBConnectionError( $this, "Oracle functions missing, have you compiled PHP with the --with-oci8 option?\n (Note: if you recently installed PHP, you may need to restart your webserver and database)\n" );
		}

		# Needed for proper UTF-8 functionality
		putenv("NLS_LANG=AMERICAN_AMERICA.AL32UTF8");

		$this->close();
		$this->mServer = $server;
		$this->mUser = $user;
		$this->mPassword = $password;
		$this->mDBname = $dbName;

		if (!strlen($user)) { ## e.g. the class is being loaded
			return;
		}

		error_reporting( E_ALL );
		$this->mConn = oci_connect($user, $password, $dbName);

		if ($this->mConn == false) {
			wfDebug("DB connection error\n");
			wfDebug("Server: $server, Database: $dbName, User: $user, Password: " . substr( $password, 0, 3 ) . "...\n");
			wfDebug($this->lastError()."\n");
			return false;
		}

		$this->mOpened = true;
		return $this->mConn;
	}

	/**
	 * Closes a database connection, if it is open
	 * Returns success, true if already closed
	 */
	function close() {
		$this->mOpened = false;
		if ( $this->mConn ) {
			return oci_close( $this->mConn );
		} else {
			return true;
		}
	}

	function execFlags() {
		return $this->mTrxLevel ? OCI_DEFAULT : OCI_COMMIT_ON_SUCCESS;
	}

	function doQuery($sql) {
		wfDebug("SQL: [$sql]\n");
		if (!mb_check_encoding($sql)) {
			throw new MWException("SQL encoding is invalid");
		}

		if (($this->mLastResult = $stmt = oci_parse($this->mConn, $sql)) === false) {
			$e = oci_error($this->mConn);
			$this->reportQueryError($e['message'], $e['code'], $sql, __FUNCTION__);
		}

		if (oci_execute($stmt, $this->execFlags()) == false) {
			$e = oci_error($stmt);
			$this->reportQueryError($e['message'], $e['code'], $sql, __FUNCTION__);
		}
		if (oci_statement_type($stmt) == "SELECT")
			return new ORAResult($this, $stmt);
		else {
			$this->mAffectedRows = oci_num_rows($stmt);
			return true;
		}
	}

	function queryIgnore($sql, $fname = '') {
		return $this->query($sql, $fname, true);
	}

	function freeResult($res) {
		$res->free();
	}

	function fetchObject($res) {
		return $res->fetchObject();
	}

	function fetchRow($res) {
		return $res->fetchAssoc();
	}

	function numRows($res) {
		return $res->numRows();
	}

	function numFields($res) {
		return $res->numFields();
	}

	function fieldName($stmt, $n) {
		return pg_field_name($stmt, $n);
	}

	/**
	 * This must be called after nextSequenceVal
	 */
	function insertId() {
		return $this->mInsertId;
	}

	function dataSeek($res, $row) {
		$res->seek($row);
	}

	function lastError() {
		if ($this->mConn === false)
			$e = oci_error();
		else
			$e = oci_error($this->mConn);
		return $e['message'];
	}

	function lastErrno() {
		if ($this->mConn === false)
			$e = oci_error();
		else
			$e = oci_error($this->mConn);
		return $e['code'];
	}

	function affectedRows() {
		return $this->mAffectedRows;
	}

	/**
	 * Returns information about an index
	 * If errors are explicitly ignored, returns NULL on failure
	 */
	function indexInfo( $table, $index, $fname = 'Database::indexExists' ) {
		return false;
	}

	function indexUnique ($table, $index, $fname = 'Database::indexUnique' ) {
		return false;
	}

	function insert( $table, $a, $fname = 'Database::insert', $options = array() ) {
		if (!is_array($options))
			$options = array($options);

		#if (in_array('IGNORE', $options))
		#	$oldIgnore = $this->ignoreErrors(true);

		# IGNORE is performed using single-row inserts, ignoring errors in each
		# FIXME: need some way to distiguish between key collision and other types of error
		//$oldIgnore = $this->ignoreErrors(true);
		if (!is_array(reset($a))) {
			$a = array($a);
		}
		foreach ($a as $row) {
			$this->insertOneRow($table, $row, $fname);
		}
		//$this->ignoreErrors($oldIgnore);
		$retVal = true;

		//if (in_array('IGNORE', $options))
		//	$this->ignoreErrors($oldIgnore);

		return $retVal;
	}

	function insertOneRow($table, $row, $fname) {
		// "INSERT INTO tables (a, b, c)"
		$sql = "INSERT INTO " . $this->tableName($table) . " (" . join(',', array_keys($row)) . ')';
		$sql .= " VALUES (";

		// for each value, append ":key"
		$first = true;
		$returning = '';
		foreach ($row as $col => $val) {
			if (is_object($val)) {
				$what = "EMPTY_BLOB()";
				assert($returning === '');
				$returning = " RETURNING $col INTO :bval";
				$blobcol = $col;
			} else
				$what = ":$col";

			if ($first)
				$sql .= "$what";
			else
				$sql.= ", $what";
			$first = false;
		}
		$sql .= ") $returning";

		$stmt = oci_parse($this->mConn, $sql);
		foreach ($row as $col => $val) {
			if (!is_object($val)) {
				if (oci_bind_by_name($stmt, ":$col", $row[$col]) === false)
					$this->reportQueryError($this->lastErrno(), $this->lastError(), $sql, __METHOD__);
			}
		}

		if (($bval = oci_new_descriptor($this->mConn, OCI_D_LOB)) === false) {
			$e = oci_error($stmt);
			throw new DBUnexpectedError($this, "Cannot create LOB descriptor: " . $e['message']);
		}

		if (strlen($returning))
			oci_bind_by_name($stmt, ":bval", $bval, -1, SQLT_BLOB);

		if (oci_execute($stmt, OCI_DEFAULT) === false) {
			$e = oci_error($stmt);
			$this->reportQueryError($e['message'], $e['code'], $sql, __METHOD__);
		}
		if (strlen($returning)) {
			$bval->save($row[$blobcol]->getData());
			$bval->free();
		}
		if (!$this->mTrxLevel)
			oci_commit($this->mConn);

		oci_free_statement($stmt);
	}

	function tableName( $name ) {
		# Replace reserved words with better ones
		switch( $name ) {
			case 'user':
				return 'mwuser';
			case 'text':
				return 'pagecontent';
			default:
				return $name;
		}
	}

	/**
	 * Return the next in a sequence, save the value for retrieval via insertId()
	 */
	function nextSequenceValue($seqName) {
		$res = $this->query("SELECT $seqName.nextval FROM dual");
		$row = $this->fetchRow($res);
		$this->mInsertId = $row[0];
		$this->freeResult($res);
		return $this->mInsertId;
	}

	/**
	 * Oracle does not have a "USE INDEX" clause, so return an empty string
	 */
	function useIndexClause($index) {
		return '';
	}

	# REPLACE query wrapper
	# Oracle simulates this with a DELETE followed by INSERT
	# $row is the row to insert, an associative array
	# $uniqueIndexes is an array of indexes. Each element may be either a
	# field name or an array of field names
	#
	# It may be more efficient to leave off unique indexes which are unlikely to collide.
	# However if you do this, you run the risk of encountering errors which wouldn't have
	# occurred in MySQL
	function replace( $table, $uniqueIndexes, $rows, $fname = 'Database::replace' ) {
		$table = $this->tableName($table);

		if (count($rows)==0) {
			return;
		}

		# Single row case
		if (!is_array(reset($rows))) {
			$rows = array($rows);
		}

		foreach( $rows as $row ) {
			# Delete rows which collide
			if ( $uniqueIndexes ) {
				$sql = "DELETE FROM $table WHERE ";
				$first = true;
				foreach ( $uniqueIndexes as $index ) {
					if ( $first ) {
						$first = false;
						$sql .= "(";
					} else {
						$sql .= ') OR (';
					}
					if ( is_array( $index ) ) {
						$first2 = true;
						foreach ( $index as $col ) {
							if ( $first2 ) {
								$first2 = false;
							} else {
								$sql .= ' AND ';
							}
							$sql .= $col.'=' . $this->addQuotes( $row[$col] );
						}
					} else {
						$sql .= $index.'=' . $this->addQuotes( $row[$index] );
					}
				}
				$sql .= ')';
				$this->query( $sql, $fname );
			}

			# Now insert the row
			$sql = "INSERT INTO $table (" . $this->makeList( array_keys( $row ), LIST_NAMES ) .') VALUES (' .
				$this->makeList( $row, LIST_COMMA ) . ')';
			$this->query($sql, $fname);
		}
	}

	# DELETE where the condition is a join
	function deleteJoin( $delTable, $joinTable, $delVar, $joinVar, $conds, $fname = "Database::deleteJoin" ) {
		if ( !$conds ) {
			throw new DBUnexpectedError($this,  'Database::deleteJoin() called with empty $conds' );
		}

		$delTable = $this->tableName( $delTable );
		$joinTable = $this->tableName( $joinTable );
		$sql = "DELETE FROM $delTable WHERE $delVar IN (SELECT $joinVar FROM $joinTable ";
		if ( $conds != '*' ) {
			$sql .= 'WHERE ' . $this->makeList( $conds, LIST_AND );
		}
		$sql .= ')';

		$this->query( $sql, $fname );
	}

	# Returns the size of a text field, or -1 for "unlimited"
	function textFieldSize( $table, $field ) {
		$table = $this->tableName( $table );
		$sql = "SELECT t.typname as ftype,a.atttypmod as size
			FROM pg_class c, pg_attribute a, pg_type t
			WHERE relname='$table' AND a.attrelid=c.oid AND
				a.atttypid=t.oid and a.attname='$field'";
		$res =$this->query($sql);
		$row=$this->fetchObject($res);
		if ($row->ftype=="varchar") {
			$size=$row->size-4;
		} else {
			$size=$row->size;
		}
		$this->freeResult( $res );
		return $size;
	}

	function lowPriorityOption() {
		return '';
	}

	function limitResult($sql, $limit, $offset) {
		if ($offset === false)
			$offset = 0;
		return "SELECT * FROM ($sql) WHERE rownum >= (1 + $offset) AND rownum < 1 + $limit + $offset";
	}

	/**
	 * Returns an SQL expression for a simple conditional.
	 * Uses CASE on Oracle
	 *
	 * @param string $cond SQL expression which will result in a boolean value
	 * @param string $trueVal SQL expression to return if true
	 * @param string $falseVal SQL expression to return if false
	 * @return string SQL fragment
	 */
	function conditional( $cond, $trueVal, $falseVal ) {
		return " (CASE WHEN $cond THEN $trueVal ELSE $falseVal END) ";
	}

	function wasDeadlock() {
		return $this->lastErrno() == 'OCI-00060';
	}

	function timestamp($ts = 0) {
		return wfTimestamp(TS_ORACLE, $ts);
	}

	/**
	 * Return aggregated value function call
	 */
	function aggregateValue ($valuedata,$valuename='value') {
		return $valuedata;
	}

	function reportQueryError($error, $errno, $sql, $fname, $tempIgnore = false) {
		# Ignore errors during error handling to avoid infinite
		# recursion
		$ignore = $this->ignoreErrors(true);
		++$this->mErrorCount;

		if ($ignore || $tempIgnore) {
echo "error ignored! query = [$sql]\n";
			wfDebug("SQL ERROR (ignored): $error\n");
			$this->ignoreErrors( $ignore );
		}
		else {
echo "error!\n";
			$message = "A database error has occurred\n" .
				"Query: $sql\n" .
				"Function: $fname\n" .
				"Error: $errno $error\n";
			throw new DBUnexpectedError($this, $message);
		}
	}

	/**
	 * @return string wikitext of a link to the server software's web site
	 */
	function getSoftwareLink() {
		return "[http://www.oracle.com/ Oracle]";
	}

	/**
	 * @return string Version information from the database
	 */
	function getServerVersion() {
		return oci_server_version($this->mConn);
	}

	/**
	 * Query whether a given table exists (in the given schema, or the default mw one if not given)
	 */
	function tableExists($table) {
		$etable= $this->addQuotes($table);
		$SQL = "SELECT 1 FROM user_tables WHERE table_name='$etable'";
		$res = $this->query($SQL);
		$count = $res ? oci_num_rows($res) : 0;
		if ($res)
			$this->freeResult($res);
		return $count;
	}

	/**
	 * Query whether a given column exists in the mediawiki schema
	 */
	function fieldExists( $table, $field ) {
		return true; // XXX
	}

	function fieldInfo( $table, $field ) {
		return false; // XXX
	}

	function begin( $fname = '' ) {
		$this->mTrxLevel = 1;
	}
	function immediateCommit( $fname = '' ) {
		return true;
	}
	function commit( $fname = '' ) {
		oci_commit($this->mConn);
		$this->mTrxLevel = 0;
	}

	/* Not even sure why this is used in the main codebase... */
	function limitResultForUpdate($sql, $num) {
		return $sql;
	}

	function strencode($s) {
		return str_replace("'", "''", $s);
	}

	function encodeBlob($b) {
		return new ORABlob($b);
	}
	function decodeBlob($b) {
		return $b; //return $b->load();
	}

	function addQuotes( $s ) {
	global	$wgLang;
		$s = $wgLang->checkTitleEncoding($s);
		return "'" . $this->strencode($s) . "'";
	}

	function quote_ident( $s ) {
		return $s;
	}

	/* For now, does nothing */
	function selectDB( $db ) {
		return true;
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
		if ( isset( $options['ORDER BY'] ) ) $preLimitTail .= " ORDER BY {$options['ORDER BY']}";

		if (isset($options['LIMIT'])) {
		//	$tailOpts .= $this->limitResult('', $options['LIMIT'],
		//		isset($options['OFFSET']) ? $options['OFFSET']
		//		: false);
		}

		#if ( isset( $noKeyOptions['FOR UPDATE'] ) ) $tailOpts .= ' FOR UPDATE';
		#if ( isset( $noKeyOptions['LOCK IN SHARE MODE'] ) ) $tailOpts .= ' LOCK IN SHARE MODE';
		if ( isset( $noKeyOptions['DISTINCT'] ) || isset( $noKeyOptions['DISTINCTROW'] ) ) $startOpts .= 'DISTINCT';

		if ( isset( $options['USE INDEX'] ) && ! is_array( $options['USE INDEX'] ) ) {
			$useIndex = $this->useIndexClause( $options['USE INDEX'] );
		} else {
			$useIndex = '';
		}

		return array( $startOpts, $useIndex, $preLimitTail, $postLimitTail );
	}

	public function setTimeout( $timeout ) {
		// @todo fixme no-op
	}

	function ping() {
		wfDebug( "Function ping() not written for DatabaseOracle.php yet");
		return true;
	}

	/**
	 * How lagged is this slave?
	 *
	 * @return int
	 */
	public function getLag() {
		# Not implemented for Oracle
		return 0;
	}

	function setFakeSlaveLag( $lag ) {}
	function setFakeMaster( $enabled = true ) {}

	function getDBname() {
		return $this->mDBname;
	}

	function getServer() {
		return $this->mServer;
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

} // end DatabaseOracle class
