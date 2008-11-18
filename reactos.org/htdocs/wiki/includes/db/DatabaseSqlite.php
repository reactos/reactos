<?php
/**
 * This script is the SQLite database abstraction layer
 *
 * See maintenance/sqlite/README for development notes and other specific information
 * @ingroup Database
 * @file
 */

/**
 * @ingroup Database
 */
class DatabaseSqlite extends Database {

	var $mAffectedRows;
	var $mLastResult;
	var $mDatabaseFile;

	/**
	 * Constructor
	 */
	function __construct($server = false, $user = false, $password = false, $dbName = false, $failFunction = false, $flags = 0) {
		global $wgOut,$wgSQLiteDataDir;
		if ("$wgSQLiteDataDir" == '') $wgSQLiteDataDir = dirname($_SERVER['DOCUMENT_ROOT']).'/data';
		if (!is_dir($wgSQLiteDataDir)) mkdir($wgSQLiteDataDir,0700);
		if (!isset($wgOut)) $wgOut = NULL; # Can't get a reference if it hasn't been set yet
		$this->mOut =& $wgOut;
		$this->mFailFunction = $failFunction;
		$this->mFlags = $flags;
		$this->mDatabaseFile = "$wgSQLiteDataDir/$dbName.sqlite";
		$this->open($server, $user, $password, $dbName);
	}

	/**
	 * todo: check if these should be true like parent class
	 */
	function implicitGroupby()   { return false; }
	function implicitOrderby()   { return false; }

	static function newFromParams($server, $user, $password, $dbName, $failFunction = false, $flags = 0) {
		return new DatabaseSqlite($server, $user, $password, $dbName, $failFunction, $flags);
	}

	/** Open an SQLite database and return a resource handle to it
	 *  NOTE: only $dbName is used, the other parameters are irrelevant for SQLite databases
	 */
	function open($server,$user,$pass,$dbName) {
		$this->mConn = false;
		if ($dbName) {
			$file = $this->mDatabaseFile;
			if ($this->mFlags & DBO_PERSISTENT) $this->mConn = new PDO("sqlite:$file",$user,$pass,array(PDO::ATTR_PERSISTENT => true));
			else $this->mConn = new PDO("sqlite:$file",$user,$pass);
			if ($this->mConn === false) wfDebug("DB connection error: $err\n");;
			$this->mOpened = $this->mConn;
			$this->mConn->setAttribute(PDO::ATTR_ERRMODE,PDO::ERRMODE_SILENT); # set error codes only, dont raise exceptions
		}
		return $this->mConn;
	}

	/**
	 * Close an SQLite database
	 */
	function close() {
		$this->mOpened = false;
		if (is_object($this->mConn)) {
			if ($this->trxLevel()) $this->immediateCommit();
			$this->mConn = null;
		}
		return true;
	}

	/**
	 * SQLite doesn't allow buffered results or data seeking etc, so we'll use fetchAll as the result
	 */
	function doQuery($sql) {
		$res = $this->mConn->query($sql);
		if ($res === false) $this->reportQueryError($this->lastError(),$this->lastErrno(),$sql,__FUNCTION__);
		else {
			$r = $res instanceof ResultWrapper ? $res->result : $res;
			$this->mAffectedRows = $r->rowCount();
			$res = new ResultWrapper($this,$r->fetchAll());
		}
		return $res;
	}

	function freeResult(&$res) {
		if ($res instanceof ResultWrapper) $res->result = NULL; else $res = NULL;
	}

	function fetchObject(&$res) {
		if ($res instanceof ResultWrapper) $r =& $res->result; else $r =& $res;
		$cur = current($r);
		if (is_array($cur)) {
			next($r);
			$obj = new stdClass;
			foreach ($cur as $k => $v) if (!is_numeric($k)) $obj->$k = $v;
			return $obj;
		}
		return false;
	}

	function fetchRow(&$res) {
		if ($res instanceof ResultWrapper) $r =& $res->result; else $r =& $res;
		$cur = current($r);
		if (is_array($cur)) {
			next($r);
			return $cur;
		}
		return false;
	}

	/**
	 * The PDO::Statement class implements the array interface so count() will work
	 */
	function numRows(&$res) {
		$r = $res instanceof ResultWrapper ? $res->result : $res;
		return count($r);
	}

	function numFields(&$res) {
		$r = $res instanceof ResultWrapper ? $res->result : $res;
		return is_array($r) ? count($r[0]) : 0;
	}

	function fieldName(&$res,$n) {
		$r = $res instanceof ResultWrapper ? $res->result : $res;
		if (is_array($r)) {
			$keys = array_keys($r[0]);
			return $keys[$n];
		}
		return  false;
	}

	/**
	 * Use MySQL's naming (accounts for prefix etc) but remove surrounding backticks
	 */
	function tableName($name) {
		return str_replace('`','',parent::tableName($name));
	}

	/**
	 * This must be called after nextSequenceVal
	 */
	function insertId() {
		return $this->mConn->lastInsertId();
	}

	function dataSeek(&$res,$row) {
		if ($res instanceof ResultWrapper) $r =& $res->result; else $r =& $res;
		reset($r);
		if ($row > 0) for ($i = 0; $i < $row; $i++) next($r);
	}

	function lastError() {
		if (!is_object($this->mConn)) return "Cannot return last error, no db connection";
		$e = $this->mConn->errorInfo();
		return isset($e[2]) ? $e[2] : '';
	}

	function lastErrno() {
		if (!is_object($this->mConn)) return "Cannot return last error, no db connection";
		return $this->mConn->errorCode();
	}

	function affectedRows() {
		return $this->mAffectedRows;
	}

	/**
	 * Returns information about an index
	 * - if errors are explicitly ignored, returns NULL on failure
	 */
	function indexInfo($table, $index, $fname = 'Database::indexExists') {
		return false;
	}

	function indexUnique($table, $index, $fname = 'Database::indexUnique') {
		return false;
	}

	/**
	 * Filter the options used in SELECT statements
	 */
	function makeSelectOptions($options) {
		foreach ($options as $k => $v) if (is_numeric($k) && $v == 'FOR UPDATE') $options[$k] = '';
		return parent::makeSelectOptions($options);
	}

	/**
	 * Based on MySQL method (parent) with some prior SQLite-sepcific adjustments
	 */
	function insert($table, $a, $fname = 'DatabaseSqlite::insert', $options = array()) {
		if (!count($a)) return true;
		if (!is_array($options)) $options = array($options);

		# SQLite uses OR IGNORE not just IGNORE
		foreach ($options as $k => $v) if ($v == 'IGNORE') $options[$k] = 'OR IGNORE';

		# SQLite can't handle multi-row inserts, so divide up into multiple single-row inserts
		if (isset($a[0]) && is_array($a[0])) {
			$ret = true;
			foreach ($a as $k => $v) if (!parent::insert($table,$v,"$fname/multi-row",$options)) $ret = false;
		}
		else $ret = parent::insert($table,$a,"$fname/single-row",$options);

		return $ret;
	}

	/**
	 * SQLite does not have a "USE INDEX" clause, so return an empty string
	 */
	function useIndexClause($index) {
		return '';
	}

	# Returns the size of a text field, or -1 for "unlimited"
	function textFieldSize($table, $field) {
		return -1;
	}

	/**
	 * No low priority option in SQLite
	 */
	function lowPriorityOption() {
		return '';
	}

	/**
	 * Returns an SQL expression for a simple conditional.
	 * - uses CASE on SQLite
	 */
	function conditional($cond, $trueVal, $falseVal) {
		return " (CASE WHEN $cond THEN $trueVal ELSE $falseVal END) ";
	}

	function wasDeadlock() {
		return $this->lastErrno() == SQLITE_BUSY;
	}

	/**
	 * @return string wikitext of a link to the server software's web site
	 */
	function getSoftwareLink() {
		return "[http://sqlite.org/ SQLite]";
	}

	/**
	 * @return string Version information from the database
	 */
	function getServerVersion() {
		global $wgContLang;
		$ver = $this->mConn->getAttribute(PDO::ATTR_SERVER_VERSION);
		$size = $wgContLang->formatSize(filesize($this->mDatabaseFile));
		$file = basename($this->mDatabaseFile);
		return $ver." ($file: $size)";
	}

	/**
	 * Query whether a given column exists in the mediawiki schema
	 */
	function fieldExists($table, $field) { return true; }

	function fieldInfo($table, $field) { return SQLiteField::fromText($this, $table, $field); }

	function begin() {
		if ($this->mTrxLevel == 1) $this->commit();
		$this->mConn->beginTransaction();
		$this->mTrxLevel = 1;
	}

	function commit() {
		if ($this->mTrxLevel == 0) return;
		$this->mConn->commit();
		$this->mTrxLevel = 0;
	}

	function rollback() {
		if ($this->mTrxLevel == 0) return;
		$this->mConn->rollBack();
		$this->mTrxLevel = 0;
	}

	function limitResultForUpdate($sql, $num) {
		return $sql;
	}

	function strencode($s) {
		return substr($this->addQuotes($s),1,-1);
	}

	function encodeBlob($b) {
		return new Blob( $b );
	}

	function decodeBlob($b) {
		if ($b instanceof Blob) {
			$b = $b->fetch();
		}
		return $b;
	}

	function addQuotes($s) {
		if ( $s instanceof Blob ) {
			return "x'" . bin2hex( $s->fetch() ) . "'";
		} else {
			return $this->mConn->quote($s);
		}
	}

	function quote_ident($s) { return $s; }

	/**
	 * For now, does nothing
	 */
	function selectDB($db) { return true; }

	/**
	 * not done
	 */
	public function setTimeout($timeout) { return; }

	function ping() {
		wfDebug("Function ping() not written for SQLite yet");
		return true;
	}

	/**
	 * How lagged is this slave?
	 */
	public function getLag() {
		return 0;
	}

	/**
	 * Called by the installer script (when modified according to the MediaWikiLite installation instructions)
	 * - this is the same way PostgreSQL works, MySQL reads in tables.sql and interwiki.sql using dbsource (which calls db->sourceFile)
	 */
	public function setup_database() {
		global $IP,$wgSQLiteDataDir,$wgDBTableOptions;
		$wgDBTableOptions = '';
		$mysql_tmpl  = "$IP/maintenance/tables.sql";
		$mysql_iw    = "$IP/maintenance/interwiki.sql";
		$sqlite_tmpl = "$IP/maintenance/sqlite/tables.sql";

		# Make an SQLite template file if it doesn't exist (based on the same one MySQL uses to create a new wiki db)
		if (!file_exists($sqlite_tmpl)) {
			$sql = file_get_contents($mysql_tmpl);
			$sql = preg_replace('/^\s*--.*?$/m','',$sql); # strip comments
			$sql = preg_replace('/^\s*(UNIQUE)?\s*(PRIMARY)?\s*KEY.+?$/m','',$sql);
			$sql = preg_replace('/^\s*(UNIQUE )?INDEX.+?$/m','',$sql); # These indexes should be created with a CREATE INDEX query
			$sql = preg_replace('/^\s*FULLTEXT.+?$/m','',$sql);        # Full text indexes
			$sql = preg_replace('/ENUM\(.+?\)/','TEXT',$sql); # Make ENUM's into TEXT's
			$sql = preg_replace('/binary\(\d+\)/','BLOB',$sql);
			$sql = preg_replace('/(TYPE|MAX_ROWS|AVG_ROW_LENGTH)=\w+/','',$sql);
			$sql = preg_replace('/,\s*\)/s',')',$sql); # removing previous items may leave a trailing comma
			$sql = str_replace('binary','',$sql);
			$sql = str_replace('auto_increment','PRIMARY KEY AUTOINCREMENT',$sql);
			$sql = str_replace(' unsigned','',$sql);
			$sql = str_replace(' int ',' INTEGER ',$sql);
			$sql = str_replace('NOT NULL','',$sql);

			# Tidy up and write file
			$sql = preg_replace('/^\s*^/m','',$sql); # Remove empty lines
			$sql = preg_replace('/;$/m',";\n",$sql); # Separate each statement with an empty line
			file_put_contents($sqlite_tmpl,$sql);
		}

		# Parse the SQLite template replacing inline variables such as /*$wgDBprefix*/
		$err = $this->sourceFile($sqlite_tmpl);
		if ($err !== true) $this->reportQueryError($err,0,$sql,__FUNCTION__);

		# Use DatabasePostgres's code to populate interwiki from MySQL template
		$f = fopen($mysql_iw,'r');
		if ($f == false) dieout("<li>Could not find the interwiki.sql file");
		$sql = "INSERT INTO interwiki(iw_prefix,iw_url,iw_local) VALUES ";
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
class SQLiteField extends MySQLField {

	function __construct() {
	}

	static function fromText($db, $table, $field) {
		$n = new SQLiteField;
		$n->name = $field;
		$n->tablename = $table;
		return $n;
	}

} // end DatabaseSqlite class

