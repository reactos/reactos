<?php
#
# Copyright (C) 2003-2004 Brion Vibber <brion@pobox.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * @defgroup Cache Cache
 *
 * @file
 * @ingroup Cache
 */

/**
 * interface is intended to be more or less compatible with
 * the PHP memcached client.
 *
 * backends for local hash array and SQL table included:
 * <code>
 *   $bag = new HashBagOStuff();
 *   $bag = new MediaWikiBagOStuff($tablename); # connect to db first
 * </code>
 *
 * @ingroup Cache
 */
class BagOStuff {
	var $debugmode;

	function __construct() {
		$this->set_debug( false );
	}

	function set_debug($bool) {
		$this->debugmode = $bool;
	}

	/* *** THE GUTS OF THE OPERATION *** */
	/* Override these with functional things in subclasses */

	function get($key) {
		/* stub */
		return false;
	}

	function set($key, $value, $exptime=0) {
		/* stub */
		return false;
	}

	function delete($key, $time=0) {
		/* stub */
		return false;
	}

	function lock($key, $timeout = 0) {
		/* stub */
		return true;
	}

	function unlock($key) {
		/* stub */
		return true;
	}

	function keys() {
		/* stub */
		return array();
	}

	/* *** Emulated functions *** */
	/* Better performance can likely be got with custom written versions */
	function get_multi($keys) {
		$out = array();
		foreach($keys as $key)
			$out[$key] = $this->get($key);
		return $out;
	}

	function set_multi($hash, $exptime=0) {
		foreach($hash as $key => $value)
			$this->set($key, $value, $exptime);
	}

	function add($key, $value, $exptime=0) {
		if( $this->get($key) == false ) {
			$this->set($key, $value, $exptime);
			return true;
		}
	}

	function add_multi($hash, $exptime=0) {
		foreach($hash as $key => $value)
			$this->add($key, $value, $exptime);
	}

	function delete_multi($keys, $time=0) {
		foreach($keys as $key)
			$this->delete($key, $time);
	}

	function replace($key, $value, $exptime=0) {
		if( $this->get($key) !== false )
			$this->set($key, $value, $exptime);
	}

	function incr($key, $value=1) {
		if ( !$this->lock($key) ) {
			return false;
		}
		$value = intval($value);
		if($value < 0) $value = 0;

		$n = false;
		if( ($n = $this->get($key)) !== false ) {
			$n += $value;
			$this->set($key, $n); // exptime?
		}
		$this->unlock($key);
		return $n;
	}

	function decr($key, $value=1) {
		if ( !$this->lock($key) ) {
			return false;
		}
		$value = intval($value);
		if($value < 0) $value = 0;

		$m = false;
		if( ($n = $this->get($key)) !== false ) {
			$m = $n - $value;
			if($m < 0) $m = 0;
			$this->set($key, $m); // exptime?
		}
		$this->unlock($key);
		return $m;
	}

	function _debug($text) {
		if($this->debugmode)
			wfDebug("BagOStuff debug: $text\n");
	}

	/**
	 * Convert an optionally relative time to an absolute time
	 */
	static function convertExpiry( $exptime ) {
		if(($exptime != 0) && ($exptime < 3600*24*30)) {
			return time() + $exptime;
		} else {
			return $exptime;
		}
	}
}


/**
 * Functional versions!
 * This is a test of the interface, mainly. It stores things in an associative
 * array, which is not going to persist between program runs.
 *
 * @ingroup Cache
 */
class HashBagOStuff extends BagOStuff {
	var $bag;

	function __construct() {
		$this->bag = array();
	}

	function _expire($key) {
		$et = $this->bag[$key][1];
		if(($et == 0) || ($et > time()))
			return false;
		$this->delete($key);
		return true;
	}

	function get($key) {
		if(!$this->bag[$key])
			return false;
		if($this->_expire($key))
			return false;
		return $this->bag[$key][0];
	}

	function set($key,$value,$exptime=0) {
		$this->bag[$key] = array( $value, BagOStuff::convertExpiry( $exptime ) );
	}

	function delete($key,$time=0) {
		if(!$this->bag[$key])
			return false;
		unset($this->bag[$key]);
		return true;
	}

	function keys() {
		return array_keys( $this->bag );
	}
}

/**
 * Generic class to store objects in a database
 *
 * @ingroup Cache
 */
abstract class SqlBagOStuff extends BagOStuff {
	var $table;
	var $lastexpireall = 0;

	/**
	 * Constructor
	 *
	 * @param $tablename String: name of the table to use
	 */
	function __construct($tablename = 'objectcache') {
		$this->table = $tablename;
	}

	function get($key) {
		/* expire old entries if any */
		$this->garbageCollect();

		$res = $this->_query(
			"SELECT value,exptime FROM $0 WHERE keyname='$1'", $key);
		if(!$res) {
			$this->_debug("get: ** error: " . $this->_dberror($res) . " **");
			return false;
		}
		if($row=$this->_fetchobject($res)) {
			$this->_debug("get: retrieved data; exp time is " . $row->exptime);
			if ( $row->exptime != $this->_maxdatetime() &&
			  wfTimestamp( TS_UNIX, $row->exptime ) < time() )
			{
				$this->_debug("get: key has expired, deleting");
				$this->delete($key);
				return false;
			}
			return $this->_unserialize($this->_blobdecode($row->value));
		} else {
			$this->_debug('get: no matching rows');
		}
		return false;
	}

	function set($key,$value,$exptime=0) {
		if ( $this->_readonly() ) {
			return false;
		}
		$exptime = intval($exptime);
		if($exptime < 0) $exptime = 0;
		if($exptime == 0) {
			$exp = $this->_maxdatetime();
		} else {
			if($exptime < 3.16e8) # ~10 years
				$exptime += time();
			$exp = $this->_fromunixtime($exptime);
		}
		$this->_begin();
		$this->_query(
			"DELETE FROM $0 WHERE keyname='$1'", $key );
		$this->_doinsert($this->getTableName(), array(
					'keyname' => $key,
					'value' => $this->_blobencode($this->_serialize($value)),
					'exptime' => $exp
				));
		$this->_commit();
		return true; /* ? */
	}

	function delete($key,$time=0) {
		if ( $this->_readonly() ) {
			return false;
		}
		$this->_begin();
		$this->_query(
			"DELETE FROM $0 WHERE keyname='$1'", $key );
		$this->_commit();
		return true; /* ? */
	}

	function keys() {
		$res = $this->_query( "SELECT keyname FROM $0" );
		if(!$res) {
			$this->_debug("keys: ** error: " . $this->_dberror($res) . " **");
			return array();
		}
		$result = array();
		while( $row = $this->_fetchobject($res) ) {
			$result[] = $row->keyname;
		}
		return $result;
	}

	function getTableName() {
		return $this->table;
	}

	function _query($sql) {
		$reps = func_get_args();
		$reps[0] = $this->getTableName();
		// ewwww
		for($i=0;$i<count($reps);$i++) {
			$sql = str_replace(
				'$' . $i,
				$i > 0 ? $this->_strencode($reps[$i]) : $reps[$i],
				$sql);
		}
		$res = $this->_doquery($sql);
		if($res == false) {
			$this->_debug('query failed: ' . $this->_dberror($res));
		}
		return $res;
	}

	function _strencode($str) {
		/* Protect strings in SQL */
		return str_replace( "'", "''", $str );
	}
	function _blobencode($str) {
		return $str;
	}
	function _blobdecode($str) {
		return $str;
	}

	abstract function _doinsert($table, $vals);
	abstract function _doquery($sql);

	abstract function _readonly();

	function _begin() {}
	function _commit() {}

	function _freeresult($result) {
		/* stub */
		return false;
	}

	function _dberror($result) {
		/* stub */
		return 'unknown error';
	}

	abstract function _maxdatetime();
	abstract function _fromunixtime($ts);

	function garbageCollect() {
		/* Ignore 99% of requests */
		if ( !mt_rand( 0, 100 ) ) {
			$nowtime = time();
			/* Avoid repeating the delete within a few seconds */
			if ( $nowtime > ($this->lastexpireall + 1) ) {
				$this->lastexpireall = $nowtime;
				$this->expireall();
			}
		}
	}

	function expireall() {
		/* Remove any items that have expired */
		if ( $this->_readonly() ) {
			return false;
		}
		$now = $this->_fromunixtime( time() );
		$this->_begin();
		$this->_query( "DELETE FROM $0 WHERE exptime < '$now'" );
		$this->_commit();
	}

	function deleteall() {
		/* Clear *all* items from cache table */
		if ( $this->_readonly() ) {
			return false;
		}
		$this->_begin();
		$this->_query( "DELETE FROM $0" );
		$this->_commit();
	}

	/**
	 * Serialize an object and, if possible, compress the representation.
	 * On typical message and page data, this can provide a 3X decrease
	 * in storage requirements.
	 *
	 * @param $data mixed
	 * @return string
	 */
	function _serialize( &$data ) {
		$serial = serialize( $data );
		if( function_exists( 'gzdeflate' ) ) {
			return gzdeflate( $serial );
		} else {
			return $serial;
		}
	}

	/**
	 * Unserialize and, if necessary, decompress an object.
	 * @param $serial string
	 * @return mixed
	 */
	function _unserialize( $serial ) {
		if( function_exists( 'gzinflate' ) ) {
			$decomp = @gzinflate( $serial );
			if( false !== $decomp ) {
				$serial = $decomp;
			}
		}
		$ret = unserialize( $serial );
		return $ret;
	}
}

/**
 * Stores objects in the main database of the wiki
 *
 * @ingroup Cache
 */
class MediaWikiBagOStuff extends SqlBagOStuff {
	var $tableInitialised = false;
	var $lb, $db;

	function _getDB(){
		if ( !isset( $this->lb ) ) {
			$this->lb = wfGetLBFactory()->newMainLB();
			$this->db = $this->lb->getConnection( DB_MASTER );
			$this->db->clearFlag( DBO_TRX );
		}
		return $this->db;
	}
	function _begin() {
		$this->_getDB()->begin();
	}
	function _commit() {
		$this->_getDB()->commit();
	}
	function _doquery($sql) {
		return $this->_getDB()->query( $sql, __METHOD__ );
	}
	function _doinsert($t, $v) {
		return $this->_getDB()->insert($t, $v, __METHOD__, array( 'IGNORE' ) );
	}
	function _fetchobject($result) {
		return $this->_getDB()->fetchObject($result);
	}
	function _freeresult($result) {
		return $this->_getDB()->freeResult($result);
	}
	function _dberror($result) {
		return $this->_getDB()->lastError();
	}
	function _maxdatetime() {
		if ( time() > 0x7fffffff ) {
			return $this->_fromunixtime( 1<<62 );
		} else {
			return $this->_fromunixtime( 0x7fffffff );
		}
	}
	function _fromunixtime($ts) {
		return $this->_getDB()->timestamp($ts);
	}
	function _readonly(){
		return wfReadOnly();
	}
	function _strencode($s) {
		return $this->_getDB()->strencode($s);
	}
	function _blobencode($s) {
		return $this->_getDB()->encodeBlob($s);
	}
	function _blobdecode($s) {
		return $this->_getDB()->decodeBlob($s);
	}
	function getTableName() {
		if ( !$this->tableInitialised ) {
			$dbw = $this->_getDB();
			/* This is actually a hack, we should be able
			   to use Language classes here... or not */
			if (!$dbw)
				throw new MWException("Could not connect to database");
			$this->table = $dbw->tableName( $this->table );
			$this->tableInitialised = true;
		}
		return $this->table;
	}
}

/**
 * This is a wrapper for Turck MMCache's shared memory functions.
 *
 * You can store objects with mmcache_put() and mmcache_get(), but Turck seems
 * to use a weird custom serializer that randomly segfaults. So we wrap calls
 * with serialize()/unserialize().
 *
 * The thing I noticed about the Turck serialized data was that unlike ordinary
 * serialize(), it contained the names of methods, and judging by the amount of
 * binary data, perhaps even the bytecode of the methods themselves. It may be
 * that Turck's serializer is faster, so a possible future extension would be
 * to use it for arrays but not for objects.
 *
 * @ingroup Cache
 */
class TurckBagOStuff extends BagOStuff {
	function get($key) {
		$val = mmcache_get( $key );
		if ( is_string( $val ) ) {
			$val = unserialize( $val );
		}
		return $val;
	}

	function set($key, $value, $exptime=0) {
		mmcache_put( $key, serialize( $value ), $exptime );
		return true;
	}

	function delete($key, $time=0) {
		mmcache_rm( $key );
		return true;
	}

	function lock($key, $waitTimeout = 0 ) {
		mmcache_lock( $key );
		return true;
	}

	function unlock($key) {
		mmcache_unlock( $key );
		return true;
	}
}

/**
 * This is a wrapper for APC's shared memory functions
 *
 * @ingroup Cache
 */
class APCBagOStuff extends BagOStuff {
	function get($key) {
		$val = apc_fetch($key);
		if ( is_string( $val ) ) {
			$val = unserialize( $val );
		}
		return $val;
	}

	function set($key, $value, $exptime=0) {
		apc_store($key, serialize($value), $exptime);
		return true;
	}

	function delete($key, $time=0) {
		apc_delete($key);
		return true;
	}
}


/**
 * This is a wrapper for eAccelerator's shared memory functions.
 *
 * This is basically identical to the Turck MMCache version,
 * mostly because eAccelerator is based on Turck MMCache.
 *
 * @ingroup Cache
 */
class eAccelBagOStuff extends BagOStuff {
	function get($key) {
		$val = eaccelerator_get( $key );
		if ( is_string( $val ) ) {
			$val = unserialize( $val );
		}
		return $val;
	}

	function set($key, $value, $exptime=0) {
		eaccelerator_put( $key, serialize( $value ), $exptime );
		return true;
	}

	function delete($key, $time=0) {
		eaccelerator_rm( $key );
		return true;
	}

	function lock($key, $waitTimeout = 0 ) {
		eaccelerator_lock( $key );
		return true;
	}

	function unlock($key) {
		eaccelerator_unlock( $key );
		return true;
	}
}

/**
 * Wrapper for XCache object caching functions; identical interface
 * to the APC wrapper
 *
 * @ingroup Cache
 */
class XCacheBagOStuff extends BagOStuff {

	/**
	 * Get a value from the XCache object cache
	 *
	 * @param $key String: cache key
	 * @return mixed
	 */
	public function get( $key ) {
		$val = xcache_get( $key );
		if( is_string( $val ) )
			$val = unserialize( $val );
		return $val;
	}

	/**
	 * Store a value in the XCache object cache
	 *
	 * @param $key String: cache key
	 * @param $value Mixed: object to store
	 * @param $expire Int: expiration time
	 * @return bool
	 */
	public function set( $key, $value, $expire = 0 ) {
		xcache_set( $key, serialize( $value ), $expire );
		return true;
	}

	/**
	 * Remove a value from the XCache object cache
	 *
	 * @param $key String: cache key
	 * @param $time Int: not used in this implementation
	 * @return bool
	 */
	public function delete( $key, $time = 0 ) {
		xcache_unset( $key );
		return true;
	}

}

/**
 * @todo document
 * @ingroup Cache
 */
class DBABagOStuff extends BagOStuff {
	var $mHandler, $mFile, $mReader, $mWriter, $mDisabled;

	function __construct( $handler = 'db3', $dir = false ) {
		if ( $dir === false ) {
			global $wgTmpDirectory;
			$dir = $wgTmpDirectory;
		}
		$this->mFile = "$dir/mw-cache-" . wfWikiID();
		$this->mFile .= '.db';
		wfDebug( __CLASS__.": using cache file {$this->mFile}\n" );
		$this->mHandler = $handler;
	}

	/**
	 * Encode value and expiry for storage
	 */
	function encode( $value, $expiry ) {
		# Convert to absolute time
		$expiry = BagOStuff::convertExpiry( $expiry );
		return sprintf( '%010u', intval( $expiry ) ) . ' ' . serialize( $value );
	}

	/**
	 * @return list containing value first and expiry second
	 */
	function decode( $blob ) {
		if ( !is_string( $blob ) ) {
			return array( null, 0 );
		} else {
			return array(
				unserialize( substr( $blob, 11 ) ),
				intval( substr( $blob, 0, 10 ) )
		   	);
		}
	}

	function getReader() {
		if ( file_exists( $this->mFile ) ) {
			$handle = dba_open( $this->mFile, 'rl', $this->mHandler );
		} else {
			$handle = $this->getWriter();
		}
		if ( !$handle ) {
			wfDebug( "Unable to open DBA cache file {$this->mFile}\n" );
		}
		return $handle;
	}

	function getWriter() {
		$handle = dba_open( $this->mFile, 'cl', $this->mHandler );
		if ( !$handle ) {
			wfDebug( "Unable to open DBA cache file {$this->mFile}\n" );
		}
		return $handle;
	}

	function get( $key ) {
		wfProfileIn( __METHOD__ );
		wfDebug( __METHOD__."($key)\n" );
		$handle = $this->getReader();
		if ( !$handle ) {
			return null;
		}
		$val = dba_fetch( $key, $handle );
		list( $val, $expiry ) = $this->decode( $val );
		# Must close ASAP because locks are held
		dba_close( $handle );

		if ( !is_null( $val ) && $expiry && $expiry < time() ) {
			# Key is expired, delete it
			$handle = $this->getWriter();
			dba_delete( $key, $handle );
			dba_close( $handle );
			wfDebug( __METHOD__.": $key expired\n" );
			$val = null;
		}
		wfProfileOut( __METHOD__ );
		return $val;
	}

	function set( $key, $value, $exptime=0 ) {
		wfProfileIn( __METHOD__ );
		wfDebug( __METHOD__."($key)\n" );
		$blob = $this->encode( $value, $exptime );
		$handle = $this->getWriter();
		if ( !$handle ) {
			return false;
		}
		$ret = dba_replace( $key, $blob, $handle );
		dba_close( $handle );
		wfProfileOut( __METHOD__ );
		return $ret;
	}

	function delete( $key, $time = 0 ) {
		wfProfileIn( __METHOD__ );
		wfDebug( __METHOD__."($key)\n" );
		$handle = $this->getWriter();
		if ( !$handle ) {
			return false;
		}
		$ret = dba_delete( $key, $handle );
		dba_close( $handle );
		wfProfileOut( __METHOD__ );
		return $ret;
	}

	function add( $key, $value, $exptime = 0 ) {
		wfProfileIn( __METHOD__ );
		$blob = $this->encode( $value, $exptime );
		$handle = $this->getWriter();
		if ( !$handle ) {
			return false;
		}
		$ret = dba_insert( $key, $blob, $handle );
		# Insert failed, check to see if it failed due to an expired key
		if ( !$ret ) {
			list( $value, $expiry ) = $this->decode( dba_fetch( $key, $handle ) );
			if ( $expiry < time() ) {
				# Yes expired, delete and try again
				dba_delete( $key, $handle );
				$ret = dba_insert( $key, $blob, $handle );
				# This time if it failed then it will be handled by the caller like any other race
			}
		}

		dba_close( $handle );
		wfProfileOut( __METHOD__ );
		return $ret;
	}

	function keys() {
		$reader = $this->getReader();
		$k1 = dba_firstkey( $reader );
		if( !$k1 ) {
			return array();
		}
		$result[] = $k1;
		while( $key = dba_nextkey( $reader ) ) {
			$result[] = $key;
		}
		return $result;
	}
}
