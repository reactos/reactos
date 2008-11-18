<?php
/**
*
* @package dbal
* @version $Id: mysql.php 8479 2008-03-29 00:22:48Z naderman $
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
* MySQL4 Database Abstraction Layer
* Compatible with:
* MySQL 3.23+
* MySQL 4.0+
* MySQL 4.1+
* MySQL 5.0+
* @package dbal
*/
class dbal_mysql extends dbal
{
	var $mysql_version;
	var $multi_insert = true;

	/**
	* Connect to server
	* @access public
	*/
	function sql_connect($sqlserver, $sqluser, $sqlpassword, $database, $port = false, $persistency = false, $new_link = false)
	{
		$this->persistency = $persistency;
		$this->user = $sqluser;
		$this->server = $sqlserver . (($port) ? ':' . $port : '');
		$this->dbname = $database;

		$this->sql_layer = 'mysql4';

		$this->db_connect_id = ($this->persistency) ? @mysql_pconnect($this->server, $this->user, $sqlpassword, $new_link) : @mysql_connect($this->server, $this->user, $sqlpassword, $new_link);

		if ($this->db_connect_id && $this->dbname != '')
		{
			if (@mysql_select_db($this->dbname, $this->db_connect_id))
			{
				// Determine what version we are using and if it natively supports UNICODE
				$this->mysql_version = mysql_get_server_info($this->db_connect_id);

				if (version_compare($this->mysql_version, '4.1.3', '>='))
				{
					@mysql_query("SET NAMES 'utf8'", $this->db_connect_id);
					// enforce strict mode on databases that support it
					if (version_compare($this->mysql_version, '5.0.2', '>='))
					{
						$result = @mysql_query('SELECT @@session.sql_mode AS sql_mode', $this->db_connect_id);
						$row = @mysql_fetch_assoc($result);
						@mysql_free_result($result);
						$modes = array_map('trim', explode(',', $row['sql_mode']));

						// TRADITIONAL includes STRICT_ALL_TABLES and STRICT_TRANS_TABLES
						if (!in_array('TRADITIONAL', $modes))
						{
							if (!in_array('STRICT_ALL_TABLES', $modes))
							{
								$modes[] = 'STRICT_ALL_TABLES';
							}

							if (!in_array('STRICT_TRANS_TABLES', $modes))
							{
								$modes[] = 'STRICT_TRANS_TABLES';
							}
						}

						$mode = implode(',', $modes);
						@mysql_query("SET SESSION sql_mode='{$mode}'", $this->db_connect_id);
					}
				}
				else if (version_compare($this->mysql_version, '4.0.0', '<'))
				{
					$this->sql_layer = 'mysql';
				}

				return $this->db_connect_id;
			}
		}

		return $this->sql_error('');
	}

	/**
	* Version information about used database
	*/
	function sql_server_info()
	{
		return 'MySQL ' . $this->mysql_version;
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
				return @mysql_query('BEGIN', $this->db_connect_id);
			break;

			case 'commit':
				return @mysql_query('COMMIT', $this->db_connect_id);
			break;

			case 'rollback':
				return @mysql_query('ROLLBACK', $this->db_connect_id);
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
				if (($this->query_result = @mysql_query($query, $this->db_connect_id)) === false)
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
			// Having a value of -1 was always a bug
			$total = '18446744073709551615';
		}

		$query .= "\n LIMIT " . ((!empty($offset)) ? $offset . ', ' . $total : $total);

		return $this->sql_query($query, $cache_ttl);
	}

	/**
	* Return number of affected rows
	*/
	function sql_affectedrows()
	{
		return ($this->db_connect_id) ? @mysql_affected_rows($this->db_connect_id) : false;
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

		return ($query_id !== false) ? @mysql_fetch_assoc($query_id) : false;
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

		return ($query_id !== false) ? @mysql_data_seek($query_id, $rownum) : false;
	}

	/**
	* Get last inserted id after insert statement
	*/
	function sql_nextid()
	{
		return ($this->db_connect_id) ? @mysql_insert_id($this->db_connect_id) : false;
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

		if (isset($this->open_queries[(int) $query_id]))
		{
			unset($this->open_queries[(int) $query_id]);
			return @mysql_free_result($query_id);
		}

		return false;
	}

	/**
	* Escape string used in sql query
	*/
	function sql_escape($msg)
	{
		if (!$this->db_connect_id)
		{
			return @mysql_real_escape_string($msg);
		}

		return @mysql_real_escape_string($msg, $this->db_connect_id);
	}

	/**
	* Build LIKE expression
	* @access private
	*/
	function _sql_like_expression($expression)
	{
		return $expression;
	}

	/**
	* Build db-specific query data
	* @access private
	*/
	function _sql_custom_build($stage, $data)
	{
		switch ($stage)
		{
			case 'FROM':
				$data = '(' . $data . ')';
			break;
		}

		return $data;
	}
	
	/**
	* return sql error array
	* @access private
	*/
	function _sql_error()
	{
		if (!$this->db_connect_id)
		{
			return array(
				'message'	=> @mysql_error(),
				'code'		=> @mysql_errno()
			);
		}

		return array(
			'message'	=> @mysql_error($this->db_connect_id),
			'code'		=> @mysql_errno($this->db_connect_id)
		);
	}

	/**
	* Close sql connection
	* @access private
	*/
	function _sql_close()
	{
		return @mysql_close($this->db_connect_id);
	}

	/**
	* Build db-specific report
	* @access private
	*/
	function _sql_report($mode, $query = '')
	{
		static $test_prof;

		// current detection method, might just switch to see the existance of INFORMATION_SCHEMA.PROFILING
		if ($test_prof === null)
		{
			$test_prof = false;
			if (strpos($this->mysql_version, 'community') !== false)
			{
				$ver = substr($this->mysql_version, 0, strpos($this->mysql_version, '-'));
				if (version_compare($ver, '5.0.37', '>=') && version_compare($ver, '5.1', '<'))
				{
					$test_prof = true;
				}
			}
		}

		switch ($mode)
		{
			case 'start':

				$explain_query = $query;
				if (preg_match('/UPDATE ([a-z0-9_]+).*?WHERE(.*)/s', $query, $m))
				{
					$explain_query = 'SELECT * FROM ' . $m[1] . ' WHERE ' . $m[2];
				}
				else if (preg_match('/DELETE FROM ([a-z0-9_]+).*?WHERE(.*)/s', $query, $m))
				{
					$explain_query = 'SELECT * FROM ' . $m[1] . ' WHERE ' . $m[2];
				}

				if (preg_match('/^SELECT/', $explain_query))
				{
					$html_table = false;

					// begin profiling
					if ($test_prof)
					{
						@mysql_query('SET profiling = 1;', $this->db_connect_id);
					}

					if ($result = @mysql_query("EXPLAIN $explain_query", $this->db_connect_id))
					{
						while ($row = @mysql_fetch_assoc($result))
						{
							$html_table = $this->sql_report('add_select_row', $query, $html_table, $row);
						}
					}
					@mysql_free_result($result);

					if ($html_table)
					{
						$this->html_hold .= '</table>';
					}

					if ($test_prof)
					{
						$html_table = false;

						// get the last profile
						if ($result = @mysql_query('SHOW PROFILE ALL;', $this->db_connect_id))
						{
							$this->html_hold .= '<br />';
							while ($row = @mysql_fetch_assoc($result))
							{
								// make <unknown> HTML safe
								if (!empty($row['Source_function']))
								{
									$row['Source_function'] = str_replace(array('<', '>'), array('&lt;', '&gt;'), $row['Source_function']);
								}

								// remove unsupported features
								foreach ($row as $key => $val)
								{
									if ($val === null)
									{
										unset($row[$key]);
									}
								}
								$html_table = $this->sql_report('add_select_row', $query, $html_table, $row);
							}
						}
						@mysql_free_result($result);

						if ($html_table)
						{
							$this->html_hold .= '</table>';
						}

						@mysql_query('SET profiling = 0;', $this->db_connect_id);
					}
				}

			break;

			case 'fromcache':
				$endtime = explode(' ', microtime());
				$endtime = $endtime[0] + $endtime[1];

				$result = @mysql_query($query, $this->db_connect_id);
				while ($void = @mysql_fetch_assoc($result))
				{
					// Take the time spent on parsing rows into account
				}
				@mysql_free_result($result);

				$splittime = explode(' ', microtime());
				$splittime = $splittime[0] + $splittime[1];

				$this->sql_report('record_fromcache', $query, $endtime, $splittime);

			break;
		}
	}
}

?>