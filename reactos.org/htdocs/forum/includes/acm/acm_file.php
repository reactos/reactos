<?php
/**
*
* @package acm
* @version $Id: acm_file.php 8479 2008-03-29 00:22:48Z naderman $
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

/**
* ACM File Based Caching
* @package acm
*/
class acm
{
	var $vars = array();
	var $var_expires = array();
	var $is_modified = false;

	var $sql_rowset = array();
	var $sql_row_pointer = array();
	var $cache_dir = '';

	/**
	* Set cache path
	*/
	function acm()
	{
		global $phpbb_root_path;
		$this->cache_dir = $phpbb_root_path . 'cache/';
	}

	/**
	* Load global cache
	*/
	function load()
	{
		global $phpEx;
		if (file_exists($this->cache_dir . 'data_global.' . $phpEx))
		{
			@include($this->cache_dir . 'data_global.' . $phpEx);
		}
		else
		{
			return false;
		}

		return true;
	}

	/**
	* Unload cache object
	*/
	function unload()
	{
		$this->save();
		unset($this->vars);
		unset($this->var_expires);
		unset($this->sql_rowset);
		unset($this->sql_row_pointer);

		$this->vars = array();
		$this->var_expires = array();
		$this->sql_rowset = array();
		$this->sql_row_pointer = array();
	}

	/**
	* Save modified objects
	*/
	function save()
	{
		if (!$this->is_modified)
		{
			return;
		}

		global $phpEx;

		if ($fp = @fopen($this->cache_dir . 'data_global.' . $phpEx, 'wb'))
		{
			@flock($fp, LOCK_EX);
			fwrite($fp, "<?php\n\$this->vars = " . var_export($this->vars, true) . ";\n\n\$this->var_expires = " . var_export($this->var_expires, true) . "\n?>");
			@flock($fp, LOCK_UN);
			fclose($fp);

			@chmod($this->cache_dir . 'data_global.' . $phpEx, 0666);
		}
		else
		{
			// Now, this occurred how often? ... phew, just tell the user then...
			if (!@is_writable($this->cache_dir))
			{
				trigger_error($this->cache_dir . ' is NOT writable.', E_USER_ERROR);
			}

			trigger_error('Not able to open ' . $this->cache_dir . 'data_global.' . $phpEx, E_USER_ERROR);
		}

		$this->is_modified = false;
	}

	/**
	* Tidy cache
	*/
	function tidy()
	{
		global $phpEx;

		$dir = @opendir($this->cache_dir);

		if (!$dir)
		{
			return;
		}

		while (($entry = readdir($dir)) !== false)
		{
			if (!preg_match('/^(sql_|data_(?!global))/', $entry))
			{
				continue;
			}

			$expired = true;
			@include($this->cache_dir . $entry);
			if ($expired)
			{
				$this->remove_file($this->cache_dir . $entry);
			}
		}
		closedir($dir);

		if (file_exists($this->cache_dir . 'data_global.' . $phpEx))
		{
			if (!sizeof($this->vars))
			{
				$this->load();
			}

			foreach ($this->var_expires as $var_name => $expires)
			{
				if (time() > $expires)
				{
					$this->destroy($var_name);
				}
			}
		}
		
		set_config('cache_last_gc', time(), true);
	}

	/**
	* Get saved cache object
	*/
	function get($var_name)
	{
		if ($var_name[0] == '_')
		{
			global $phpEx;

			if (!$this->_exists($var_name))
			{
				return false;
			}

			@include($this->cache_dir . "data{$var_name}.$phpEx");
			return (isset($data)) ? $data : false;
		}
		else
		{
			return ($this->_exists($var_name)) ? $this->vars[$var_name] : false;
		}
	}

	/**
	* Put data into cache
	*/
	function put($var_name, $var, $ttl = 31536000)
	{
		if ($var_name[0] == '_')
		{
			global $phpEx;

			if ($fp = @fopen($this->cache_dir . "data{$var_name}.$phpEx", 'wb'))
			{
				@flock($fp, LOCK_EX);
				fwrite($fp, "<?php\n\$expired = (time() > " . (time() + $ttl) . ") ? true : false;\nif (\$expired) { return; }\n\n\$data = " . var_export($var, true) . ";\n?>");
				@flock($fp, LOCK_UN);
				fclose($fp);

				@chmod($this->cache_dir . "data{$var_name}.$phpEx", 0666);
			}
		}
		else
		{
			$this->vars[$var_name] = $var;
			$this->var_expires[$var_name] = time() + $ttl;
			$this->is_modified = true;
		}
	}

	/**
	* Purge cache data
	*/
	function purge()
	{
		// Purge all phpbb cache files
		$dir = @opendir($this->cache_dir);

		if (!$dir)
		{
			return;
		}

		while (($entry = readdir($dir)) !== false)
		{
			if (strpos($entry, 'sql_') !== 0 && strpos($entry, 'data_') !== 0 && strpos($entry, 'ctpl_') !== 0 && strpos($entry, 'tpl_') !== 0)
			{
				continue;
			}

			$this->remove_file($this->cache_dir . $entry);
		}
		closedir($dir);

		unset($this->vars);
		unset($this->var_expires);
		unset($this->sql_rowset);
		unset($this->sql_row_pointer);

		$this->vars = array();
		$this->var_expires = array();
		$this->sql_rowset = array();
		$this->sql_row_pointer = array();

		$this->is_modified = false;
	}

	/**
	* Destroy cache data
	*/
	function destroy($var_name, $table = '')
	{
		global $phpEx;

		if ($var_name == 'sql' && !empty($table))
		{
			if (!is_array($table))
			{
				$table = array($table);
			}

			$dir = @opendir($this->cache_dir);

			if (!$dir)
			{
				return;
			}

			while (($entry = readdir($dir)) !== false)
			{
				if (strpos($entry, 'sql_') !== 0)
				{
					continue;
				}

				// The following method is more failproof than simply assuming the query is on line 3 (which it should be)
				$check_line = @file_get_contents($this->cache_dir . $entry);

				if (empty($check_line))
				{
					continue;
				}

				// Now get the contents between /* and */
				$check_line = substr($check_line, strpos($check_line, '/* ') + 3, strpos($check_line, ' */') - strpos($check_line, '/* ') - 3);

				$found = false;
				foreach ($table as $check_table)
				{
					// Better catch partial table names than no table names. ;)
					if (strpos($check_line, $check_table) !== false)
					{
						$found = true;
						break;
					}
				}

				if ($found)
				{
					$this->remove_file($this->cache_dir . $entry);
				}
			}
			closedir($dir);

			return;
		}

		if (!$this->_exists($var_name))
		{
			return;
		}

		if ($var_name[0] == '_')
		{
			$this->remove_file($this->cache_dir . 'data' . $var_name . ".$phpEx", true);
		}
		else if (isset($this->vars[$var_name]))
		{
			$this->is_modified = true;
			unset($this->vars[$var_name]);
			unset($this->var_expires[$var_name]);

			// We save here to let the following cache hits succeed
			$this->save();
		}
	}

	/**
	* Check if a given cache entry exist
	*/
	function _exists($var_name)
	{
		if ($var_name[0] == '_')
		{
			global $phpEx;
			return file_exists($this->cache_dir . 'data' . $var_name . ".$phpEx");
		}
		else
		{
			if (!sizeof($this->vars))
			{
				$this->load();
			}

			if (!isset($this->var_expires[$var_name]))
			{
				return false;
			}

			return (time() > $this->var_expires[$var_name]) ? false : isset($this->vars[$var_name]);
		}
	}

	/**
	* Load cached sql query
	*/
	function sql_load($query)
	{
		global $phpEx;

		// Remove extra spaces and tabs
		$query = preg_replace('/[\n\r\s\t]+/', ' ', $query);
		$query_id = sizeof($this->sql_rowset);

		if (!file_exists($this->cache_dir . 'sql_' . md5($query) . ".$phpEx"))
		{
			return false;
		}

		@include($this->cache_dir . 'sql_' . md5($query) . ".$phpEx");

		if (!isset($expired))
		{
			return false;
		}
		else if ($expired)
		{
			$this->remove_file($this->cache_dir . 'sql_' . md5($query) . ".$phpEx", true);
			return false;
		}

		$this->sql_row_pointer[$query_id] = 0;

		return $query_id;
	}

	/**
	* Save sql query
	*/
	function sql_save($query, &$query_result, $ttl)
	{
		global $db, $phpEx;

		// Remove extra spaces and tabs
		$query = preg_replace('/[\n\r\s\t]+/', ' ', $query);
		$filename = $this->cache_dir . 'sql_' . md5($query) . '.' . $phpEx;

		if ($fp = @fopen($filename, 'wb'))
		{
			@flock($fp, LOCK_EX);

			$query_id = sizeof($this->sql_rowset);
			$this->sql_rowset[$query_id] = array();
			$this->sql_row_pointer[$query_id] = 0;

			while ($row = $db->sql_fetchrow($query_result))
			{
				$this->sql_rowset[$query_id][] = $row;
			}
			$db->sql_freeresult($query_result);

			$file = "<?php\n\n/* " . str_replace('*/', '*\/', $query) . " */\n";
			$file .= "\n\$expired = (time() > " . (time() + $ttl) . ") ? true : false;\nif (\$expired) { return; }\n";

			fwrite($fp, $file . "\n\$this->sql_rowset[\$query_id] = " . var_export($this->sql_rowset[$query_id], true) . ";\n?>");
			@flock($fp, LOCK_UN);
			fclose($fp);

			@chmod($filename, 0666);

			$query_result = $query_id;
		}
	}

	/**
	* Ceck if a given sql query exist in cache
	*/
	function sql_exists($query_id)
	{
		return isset($this->sql_rowset[$query_id]);
	}

	/**
	* Fetch row from cache (database)
	*/
	function sql_fetchrow($query_id)
	{
		if ($this->sql_row_pointer[$query_id] < sizeof($this->sql_rowset[$query_id]))
		{
			return $this->sql_rowset[$query_id][$this->sql_row_pointer[$query_id]++];
		}

		return false;
	}

	/**
	* Fetch a field from the current row of a cached database result (database)
	*/
	function sql_fetchfield($query_id, $field)
	{
		if ($this->sql_row_pointer[$query_id] < sizeof($this->sql_rowset[$query_id]))
		{
			return (isset($this->sql_rowset[$query_id][$this->sql_row_pointer[$query_id]][$field])) ? $this->sql_rowset[$query_id][$this->sql_row_pointer[$query_id]][$field] : false;
		}

		return false;
	}

	/**
	* Seek a specific row in an a cached database result (database)
	*/
	function sql_rowseek($rownum, $query_id)
	{
		if ($rownum >= sizeof($this->sql_rowset[$query_id]))
		{
			return false;
		}

		$this->sql_row_pointer[$query_id] = $rownum;
		return true;
	}

	/**
	* Free memory used for a cached database result (database)
	*/
	function sql_freeresult($query_id)
	{
		if (!isset($this->sql_rowset[$query_id]))
		{
			return false;
		}

		unset($this->sql_rowset[$query_id]);
		unset($this->sql_row_pointer[$query_id]);

		return true;
	}

	/**
	* Removes/unlinks file
	*/
	function remove_file($filename, $check = false)
	{
		if ($check && !@is_writeable($this->cache_dir))
		{
			// E_USER_ERROR - not using language entry - intended.
			trigger_error('Unable to remove files within ' . $this->cache_dir . '. Please check directory permissions.', E_USER_ERROR);
		}

		return @unlink($filename);
	}
}

?>