<?php
/**
*
* @package dbal
* @version $Id: sqlite.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

include_once($phpbb_root_path . 'includes/db/dbal.' . $phpEx);

/**
* Sqlite Database Abstraction Layer
* Minimum Requirement: 2.8.2+
* @package dbal
*/
class dbal_sqlite extends dbal
{
	/**
	* Connect to server
	*/
	function sql_connect($sqlserver, $sqluser, $sqlpassword, $database, $port = false, $persistency = false, $new_link = false)
	{
		$this->persistency = $persistency;
		$this->user = $sqluser;
		$this->server = $sqlserver . (($port) ? ':' . $port : '');
		$this->dbname = $database;

		$error = '';
		$this->db_connect_id = ($this->persistency) ? @sqlite_popen($this->server, 0666, $error) : @sqlite_open($this->server, 0666, $error);

		if ($this->db_connect_id)
		{
			@sqlite_query('PRAGMA short_column_names = 1', $this->db_connect_id);
		}

		
		return ($this->db_connect_id) ? true : array('message' => $error);
	}

	/**
	* Version information about used database
	*/
	function sql_server_info()
	{
		return 'SQLite ' . @sqlite_libversion();
	}

	/**
	* SQL Transaction
	* @access private
	*/
	function _sql_transaction($status = 'begin')
	{
		switch ($status)
		{
			case 'begin':
				return @sqlite_query('BEGIN', $this->db_connect_id);
			break;

			case 'commit':
				return @sqlite_query('COMMIT', $this->db_connect_id);
			break;

			case 'rollback':
				return @sqlite_query('ROLLBACK', $this->db_connect_id);
			break;
		}

		return true;
	}

	/**
	* Base query method
	*
	* @param	string	$query		Contains the SQL query which shall be executed
	* @param	int		$cache_ttl	Either 0 to avoid caching or the time in seconds which the result shall be kept in cache
	* @return	mixed				When casted to bool the returned value returns true on success and false on failure
	*
	* @access	public
	*/
	function sql_query($query = '', $cache_ttl = 0)
	{
		if ($query != '')
		{
			global $cache;

			// EXPLAIN only in extra debug mode
			if (defined('DEBUG_EXTRA'))
			{
				$this->sql_report('start', $query);
			}

			$this->query_result = ($cache_ttl && method_exists($cache, 'sql_load')) ? $cache->sql_load($query) : false;
			$this->sql_add_num_queries($this->query_result);

			if ($this->query_result === false)
			{
				if (($this->query_result = @sqlite_query($query, $this->db_connect_id)) === false)
				{
					$this->sql_error($query);
				}

				if (defined('DEBUG_EXTRA'))
				{
					$this->sql_report('stop', $query);
				}

				if ($cache_ttl && method_exists($cache, 'sql_save'))
				{
					$this->open_queries[(int) $this->query_result] = $this->query_result;
					$cache->sql_save($query, $this->query_result, $cache_ttl);
				}
				else if (strpos($query, 'SELECT') === 0 && $this->query_result)
				{
					$this->open_queries[(int) $this->query_result] = $this->query_result;
				}
			}
			else if (defined('DEBUG_EXTRA'))
			{
				$this->sql_report('fromcache', $query);
			}
		}
		else
		{
			return false;
		}

		return ($this->query_result) ? $this->query_result : false;
	}

	/**
	* Build LIMIT query
	*/
	function _sql_query_limit($query, $total, $offset = 0, $cache_ttl = 0)
	{
		$this->query_result = false;

		// if $total is set to 0 we do not want to limit the number of rows
		if ($total == 0)
		{
			$total = -1;
		}

		$query .= "\n LIMIT " . ((!empty($offset)) ? $offset . ', ' . $total : $total);

		return $this->sql_query($query, $cache_ttl);
	}

	/**
	* Return number of affected rows
	*/
	function sql_affectedrows()
	{
		return ($this->db_connect_id) ? @sqlite_changes($this->db_connect_id) : false;
	}

	/**
	* Fetch current row
	*/
	function sql_fetchrow($query_id = false)
	{
		global $cache;

		if ($query_id === false)
		{
			$query_id = $this->query_result;
		}

		if (isset($cache->sql_rowset[$query_id]))
		{
			return $cache->sql_fetchrow($query_id);
		}

		return ($query_id !== false) ? @sqlite_fetch_array($query_id, SQLITE_ASSOC) : false;
	}

	/**
	* Seek to given row number
	* rownum is zero-based
	*/
	function sql_rowseek($rownum, &$query_id)
	{
		global $cache;

		if ($query_id === false)
		{
			$query_id = $this->query_result;
		}

		if (isset($cache->sql_rowset[$query_id]))
		{
			return $cache->sql_rowseek($rownum, $query_id);
		}

		return ($query_id !== false) ? @sqlite_seek($query_id, $rownum) : false;
	}

	/**
	* Get last inserted id after insert statement
	*/
	function sql_nextid()
	{
		return ($this->db_connect_id) ? @sqlite_last_insert_rowid($this->db_connect_id) : false;
	}

	/**
	* Free sql result
	*/
	function sql_freeresult($query_id = false)
	{
		global $cache;

		if ($query_id === false)
		{
			$query_id = $this->query_result;
		}

		if (isset($cache->sql_rowset[$query_id]))
		{
			return $cache->sql_freeresult($query_id);
		}

		return true;
	}

	/**
	* Escape string used in sql query
	*/
	function sql_escape($msg)
	{
		return @sqlite_escape_string($msg);
	}

	/**
	* Correctly adjust LIKE expression for special characters
	* For SQLite an underscore is a not-known character... this may change with SQLite3
	*/
	function sql_like_expression($expression)
	{
		// Unlike LIKE, GLOB is case sensitive (unfortunatly). SQLite users need to live with it!
		// We only catch * and ? here, not the character map possible on file globbing.
		$expression = str_replace(array(chr(0) . '_', chr(0) . '%'), array(chr(0) . '?', chr(0) . '*'), $expression);

		$expression = str_replace(array('?', '*'), array("\?", "\*"), $expression);
		$expression = str_replace(array(chr(0) . "\?", chr(0) . "\*"), array('?', '*'), $expression);

		return 'GLOB \'' . $this->sql_escape($expression) . '\'';
	}

	/**
	* return sql error array
	* @access private
	*/
	function _sql_error()
	{
		return array(
			'message'	=> @sqlite_error_string(@sqlite_last_error($this->db_connect_id)),
			'code'		=> @sqlite_last_error($this->db_connect_id)
		);
	}

	/**
	* Build db-specific query data
	* @access private
	*/
	function _sql_custom_build($stage, $data)
	{
		return $data;
	}

	/**
	* Close sql connection
	* @access private
	*/
	function _sql_close()
	{
		return @sqlite_close($this->db_connect_id);
	}

	/**
	* Build db-specific report
	* @access private
	*/
	function _sql_report($mode, $query = '')
	{
		switch ($mode)
		{
			case 'start':
			break;

			case 'fromcache':
				$endtime = explode(' ', microtime());
				$endtime = $endtime[0] + $endtime[1];

				$result = @sqlite_query($query, $this->db_connect_id);
				while ($void = @sqlite_fetch_array($result, SQLITE_ASSOC))
				{
					// Take the time spent on parsing rows into account
				}

				$splittime = explode(' ', microtime());
				$splittime = $splittime[0] + $splittime[1];

				$this->sql_report('record_fromcache', $query, $endtime, $splittime);

			break;
		}
	}
}

?>