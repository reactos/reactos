<?php
/**
*
* @package dbal
* @version $Id: mssql.php 8479 2008-03-29 00:22:48Z naderman $
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
* MSSQL Database Abstraction Layer
* Minimum Requirement is MSSQL 2000+
* @package dbal
*/
class dbal_mssql extends dbal
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

		@ini_set('mssql.charset', 'UTF-8');
		@ini_set('mssql.textlimit', 2147483647);
		@ini_set('mssql.textsize', 2147483647);

		if (version_compare(PHP_VERSION, '5.1.0', '>=') || (version_compare(PHP_VERSION, '5.0.0-dev', '<=') && version_compare(PHP_VERSION, '4.4.1', '>=')))
		{
			$this->db_connect_id = ($this->persistency) ? @mssql_pconnect($this->server, $this->user, $sqlpassword, $new_link) : @mssql_connect($this->server, $this->user, $sqlpassword, $new_link);
		}
		else
		{
			$this->db_connect_id = ($this->persistency) ? @mssql_pconnect($this->server, $this->user, $sqlpassword) : @mssql_connect($this->server, $this->user, $sqlpassword);
		}

		if ($this->db_connect_id && $this->dbname != '')
		{
			if (!@mssql_select_db($this->dbname, $this->db_connect_id))
			{
				@mssql_close($this->db_connect_id);
				return false;
			}
		}

		return ($this->db_connect_id) ? $this->db_connect_id : $this->sql_error('');
	}

	/**
	* Version information about used database
	*/
	function sql_server_info()
	{
		$result_id = @mssql_query("SELECT SERVERPROPERTY('productversion'), SERVERPROPERTY('productlevel'), SERVERPROPERTY('edition')", $this->db_connect_id);

		$row = false;
		if ($result_id)
		{
			$row = @mssql_fetch_assoc($result_id);
			@mssql_free_result($result_id);
		}

		if ($row)
		{
			return 'MSSQL<br />' . implode(' ', $row);
		}

		return 'MSSQL';
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
				return @mssql_query('BEGIN TRANSACTION', $this->db_connect_id);
			break;

			case 'commit':
				return @mssql_query('COMMIT TRANSACTION', $this->db_connect_id);
			break;

			case 'rollback':
				return @mssql_query('ROLLBACK TRANSACTION', $this->db_connect_id);
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
				if (($this->query_result = @mssql_query($query, $this->db_connect_id)) === false)
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

		// Since TOP is only returning a set number of rows we won't need it if total is set to 0 (return all rows)
		if ($total)
		{
			// We need to grab the total number of rows + the offset number of rows to get the correct result
			if (strpos($query, 'SELECT DISTINCT') === 0)
			{
				$query = 'SELECT DISTINCT TOP ' . ($total + $offset) . ' ' . substr($query, 15);
			}
			else
			{
				$query = 'SELECT TOP ' . ($total + $offset) . ' ' . substr($query, 6);
			}
		}

		$result = $this->sql_query($query, $cache_ttl);

		// Seek by $offset rows
		if ($offset)
		{
			$this->sql_rowseek($offset, $result);
		}

		return $result;
	}

	/**
	* Return number of affected rows
	*/
	function sql_affectedrows()
	{
		return ($this->db_connect_id) ? @mssql_rows_affected($this->db_connect_id) : false;
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

		if ($query_id === false)
		{
			return false;
		}

		$row = @mssql_fetch_assoc($query_id);

		// I hope i am able to remove this later... hopefully only a PHP or MSSQL bug
		if ($row)
		{
			foreach ($row as $key => $value)
			{
				$row[$key] = ($value === ' ' || $value === NULL) ? '' : $value;
			}
		}

		return $row;
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

		return ($query_id !== false) ? @mssql_data_seek($query_id, $rownum) : false;
	}

	/**
	* Get last inserted id after insert statement
	*/
	function sql_nextid()
	{
		$result_id = @mssql_query('SELECT SCOPE_IDENTITY()', $this->db_connect_id);
		if ($result_id)
		{
			if ($row = @mssql_fetch_assoc($result_id))
			{
				@mssql_free_result($result_id);
				return $row['computed'];
			}
			@mssql_free_result($result_id);
		}

		return false;
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

		if (isset($this->open_queries[$query_id]))
		{
			unset($this->open_queries[$query_id]);
			return @mssql_free_result($query_id);
		}

		return false;
	}

	/**
	* Escape string used in sql query
	*/
	function sql_escape($msg)
	{
		return str_replace("'", "''", $msg);
	}

	/**
	* Build LIKE expression
	* @access private
	*/
	function _sql_like_expression($expression)
	{
		return $expression . " ESCAPE '\\'";
	}

	/**
	* return sql error array
	* @access private
	*/
	function _sql_error()
	{
		$error = array(
			'message'	=> @mssql_get_last_message(),
			'code'		=> ''
		);

		// Get error code number
		$result_id = @mssql_query('SELECT @@ERROR as code', $this->db_connect_id);
		if ($result_id)
		{
			$row = @mssql_fetch_assoc($result_id);
			$error['code'] = $row['code'];
			@mssql_free_result($result_id);
		}

		// Get full error message if possible
		$sql = 'SELECT CAST(description as varchar(255)) as message
			FROM master.dbo.sysmessages
			WHERE error = ' . $error['code'];
		$result_id = @mssql_query($sql);
		
		if ($result_id)
		{
			$row = @mssql_fetch_assoc($result_id);
			if (!empty($row['message']))
			{
				$error['message'] .= '<br />' . $row['message'];
			}
			@mssql_free_result($result_id);
		}

		return $error;
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
		return @mssql_close($this->db_connect_id);
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
				$html_table = false;
				@mssql_query('SET SHOWPLAN_TEXT ON;', $this->db_connect_id);
				if ($result = @mssql_query($query, $this->db_connect_id))
				{
					@mssql_next_result($result);
					while ($row = @mssql_fetch_row($result))
					{
						$html_table = $this->sql_report('add_select_row', $query, $html_table, $row);
					}
				}
				@mssql_query('SET SHOWPLAN_TEXT OFF;', $this->db_connect_id);
				@mssql_free_result($result);

				if ($html_table)
				{
					$this->html_hold .= '</table>';
				}
			break;

			case 'fromcache':
				$endtime = explode(' ', microtime());
				$endtime = $endtime[0] + $endtime[1];

				$result = @mssql_query($query, $this->db_connect_id);
				while ($void = @mssql_fetch_assoc($result))
				{
					// Take the time spent on parsing rows into account
				}
				@mssql_free_result($result);

				$splittime = explode(' ', microtime());
				$splittime = $splittime[0] + $splittime[1];

				$this->sql_report('record_fromcache', $query, $endtime, $splittime);

			break;
		}
	}
}

?>