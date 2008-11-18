<?php
/**
*
* @package install
* @version $Id: functions_install.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2006 phpBB Group
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
* Determine if we are able to load a specified PHP module and do so if possible
*/
function can_load_dll($dll)
{
	return ((@ini_get('enable_dl') || strtolower(@ini_get('enable_dl')) == 'on') && (!@ini_get('safe_mode') || strtolower(@ini_get('safe_mode')) == 'off') && @dl($dll . '.' . PHP_SHLIB_SUFFIX)) ? true : false;
}

/**
* Returns an array of available DBMS with some data, if a DBMS is specified it will only
* return data for that DBMS and will load its extension if necessary.
*/
function get_available_dbms($dbms = false, $return_unavailable = false, $only_20x_options = false)
{
	global $lang;
	$available_dbms = array(
		'firebird'	=> array(
			'LABEL'			=> 'FireBird',
			'SCHEMA'		=> 'firebird',
			'MODULE'		=> 'interbase',
			'DELIM'			=> ';;',
			'COMMENTS'		=> 'remove_remarks',
			'DRIVER'		=> 'firebird',
			'AVAILABLE'		=> true,
			'2.0.x'			=> false,
		),
		'mysqli'	=> array(
			'LABEL'			=> 'MySQL with MySQLi Extension',
			'SCHEMA'		=> 'mysql_41',
			'MODULE'		=> 'mysqli',
			'DELIM'			=> ';',
			'COMMENTS'		=> 'remove_remarks',
			'DRIVER'		=> 'mysqli',
			'AVAILABLE'		=> true,
			'2.0.x'			=> true,
		),
		'mysql'		=> array(
			'LABEL'			=> 'MySQL',
			'SCHEMA'		=> 'mysql',
			'MODULE'		=> 'mysql',
			'DELIM'			=> ';',
			'COMMENTS'		=> 'remove_remarks',
			'DRIVER'		=> 'mysql',
			'AVAILABLE'		=> true,
			'2.0.x'			=> true,
		),
		'mssql'		=> array(
			'LABEL'			=> 'MS SQL Server 2000+',
			'SCHEMA'		=> 'mssql',
			'MODULE'		=> 'mssql',
			'DELIM'			=> 'GO',
			'COMMENTS'		=> 'remove_comments',
			'DRIVER'		=> 'mssql',
			'AVAILABLE'		=> true,
			'2.0.x'			=> true,
		),
		'mssql_odbc'=>	array(
			'LABEL'			=> 'MS SQL Server [ ODBC ]',
			'SCHEMA'		=> 'mssql',
			'MODULE'		=> 'odbc',
			'DELIM'			=> 'GO',
			'COMMENTS'		=> 'remove_comments',
			'DRIVER'		=> 'mssql_odbc',
			'AVAILABLE'		=> true,
			'2.0.x'			=> true,
		),
		'oracle'	=>	array(
			'LABEL'			=> 'Oracle',
			'SCHEMA'		=> 'oracle',
			'MODULE'		=> 'oci8',
			'DELIM'			=> '/',
			'COMMENTS'		=> 'remove_comments',
			'DRIVER'		=> 'oracle',
			'AVAILABLE'		=> true,
			'2.0.x'			=> false,
		),
		'postgres' => array(
			'LABEL'			=> 'PostgreSQL 7.x/8.x',
			'SCHEMA'		=> 'postgres',
			'MODULE'		=> 'pgsql',
			'DELIM'			=> ';',
			'COMMENTS'		=> 'remove_comments',
			'DRIVER'		=> 'postgres',
			'AVAILABLE'		=> true,
			'2.0.x'			=> true,
		),
		'sqlite'		=> array(
			'LABEL'			=> 'SQLite',
			'SCHEMA'		=> 'sqlite',
			'MODULE'		=> 'sqlite',
			'DELIM'			=> ';',
			'COMMENTS'		=> 'remove_remarks',
			'DRIVER'		=> 'sqlite',
			'AVAILABLE'		=> true,
			'2.0.x'			=> false,
		),
	);

	if ($dbms)
	{
		if (isset($available_dbms[$dbms]))
		{
			$available_dbms = array($dbms => $available_dbms[$dbms]);
		}
		else
		{
			return array();
		}
	}

	// now perform some checks whether they are really available
	foreach ($available_dbms as $db_name => $db_ary)
	{
		if ($only_20x_options && !$db_ary['2.0.x'])
		{
			if ($return_unavailable)
			{
				$available_dbms[$db_name]['AVAILABLE'] = false;
			}
			else
			{
				unset($available_dbms[$db_name]);
			}
			continue;
		}

		$dll = $db_ary['MODULE'];

		if (!@extension_loaded($dll))
		{
			if (!can_load_dll($dll))
			{
				if ($return_unavailable)
				{
					$available_dbms[$db_name]['AVAILABLE'] = false;
				}
				else
				{
					unset($available_dbms[$db_name]);
				}
				continue;
			}
		}
		$any_db_support = true;
	}

	if ($return_unavailable)
	{
		$available_dbms['ANY_DB_SUPPORT'] = $any_db_support;
	}
	return $available_dbms;
}

/**
* Generate the drop down of available database options
*/
function dbms_select($default = '', $only_20x_options = false)
{
	global $lang;
	
	$available_dbms = get_available_dbms(false, false, $only_20x_options);
	$dbms_options = '';
	foreach ($available_dbms as $dbms_name => $details)
	{
		$selected = ($dbms_name == $default) ? ' selected="selected"' : '';
		$dbms_options .= '<option value="' . $dbms_name . '"' . $selected .'>' . $lang['DLL_' . strtoupper($dbms_name)] . '</option>';
	}
	return $dbms_options;
}

/**
* Get tables of a database
*/
function get_tables($db)
{
	switch ($db->sql_layer)
	{
		case 'mysql':
		case 'mysql4':
		case 'mysqli':
			$sql = 'SHOW TABLES';
		break;

		case 'sqlite':
			$sql = 'SELECT name
				FROM sqlite_master
				WHERE type = "table"';
		break;

		case 'mssql':
		case 'mssql_odbc':
			$sql = "SELECT name
				FROM sysobjects
				WHERE type='U'";
		break;

		case 'postgres':
			$sql = 'SELECT relname
				FROM pg_stat_user_tables';
		break;

		case 'firebird':
			$sql = 'SELECT rdb$relation_name
				FROM rdb$relations
				WHERE rdb$view_source is null
					AND rdb$system_flag = 0';
		break;

		case 'oracle':
			$sql = 'SELECT table_name
				FROM USER_TABLES';
		break;
	}

	$result = $db->sql_query($sql);

	$tables = array();

	while ($row = $db->sql_fetchrow($result))
	{
		$tables[] = current($row);
	}

	$db->sql_freeresult($result);

	return $tables;
}

/**
* Used to test whether we are able to connect to the database the user has specified
* and identify any problems (eg there are already tables with the names we want to use
* @param	array	$dbms should be of the format of an element of the array returned by {@link get_available_dbms get_available_dbms()}
*					necessary extensions should be loaded already
*/
function connect_check_db($error_connect, &$error, $dbms_details, $table_prefix, $dbhost, $dbuser, $dbpasswd, $dbname, $dbport, $prefix_may_exist = false, $load_dbal = true, $unicode_check = true)
{
	global $phpbb_root_path, $phpEx, $config, $lang;

	$dbms = $dbms_details['DRIVER'];

	if ($load_dbal)
	{
		// Include the DB layer
		include($phpbb_root_path . 'includes/db/' . $dbms . '.' . $phpEx);
	}

	// Instantiate it and set return on error true
	$sql_db = 'dbal_' . $dbms;
	$db = new $sql_db();
	$db->sql_return_on_error(true);

	// Check that we actually have a database name before going any further.....
	if ($dbms_details['DRIVER'] != 'sqlite' && $dbms_details['DRIVER'] != 'oracle' && $dbname === '')
	{
		$error[] = $lang['INST_ERR_DB_NO_NAME'];
		return false;
	}

	// Make sure we don't have a daft user who thinks having the SQLite database in the forum directory is a good idea
	if ($dbms_details['DRIVER'] == 'sqlite' && stripos(phpbb_realpath($dbhost), phpbb_realpath('../')) === 0)
	{
		$error[] = $lang['INST_ERR_DB_FORUM_PATH'];
		return false;
	}

	// Check the prefix length to ensure that index names are not too long and does not contain invalid characters
	switch ($dbms_details['DRIVER'])
	{
		case 'mysql':
		case 'mysqli':
			if (strpos($table_prefix, '-') !== false || strpos($table_prefix, '.') !== false)
			{
				$error[] = $lang['INST_ERR_PREFIX_INVALID'];
				return false;
			}

		// no break;

		case 'postgres':
			$prefix_length = 36;
		break;

		case 'mssql':
		case 'mssql_odbc':
			$prefix_length = 90;
		break;

		case 'sqlite':
			$prefix_length = 200;
		break;

		case 'firebird':
		case 'oracle':
			$prefix_length = 6;
		break;
	}

	if (strlen($table_prefix) > $prefix_length)
	{
		$error[] = sprintf($lang['INST_ERR_PREFIX_TOO_LONG'], $prefix_length);
		return false;
	}

	// Try and connect ...
	if (is_array($db->sql_connect($dbhost, $dbuser, $dbpasswd, $dbname, $dbport, false, true)))
	{
		$db_error = $db->sql_error();
		$error[] = $lang['INST_ERR_DB_CONNECT'] . '<br />' . (($db_error['message']) ? $db_error['message'] : $lang['INST_ERR_DB_NO_ERROR']);
	}
	else
	{
		// Likely matches for an existing phpBB installation
		if (!$prefix_may_exist)
		{
			$temp_prefix = strtolower($table_prefix);
			$table_ary = array($temp_prefix . 'attachments', $temp_prefix . 'config', $temp_prefix . 'sessions', $temp_prefix . 'topics', $temp_prefix . 'users');

			$tables = get_tables($db);
			$tables = array_map('strtolower', $tables);
			$table_intersect = array_intersect($tables, $table_ary);

			if (sizeof($table_intersect))
			{
				$error[] = $lang['INST_ERR_PREFIX'];
			}
		}

		// Make sure that the user has selected a sensible DBAL for the DBMS actually installed
		switch ($dbms_details['DRIVER'])
		{
			case 'mysqli':
				if (version_compare(mysqli_get_server_info($db->db_connect_id), '4.1.3', '<'))
				{
					$error[] = $lang['INST_ERR_DB_NO_MYSQLI'];
				}
			break;

			case 'sqlite':
				if (version_compare(sqlite_libversion(), '2.8.2', '<'))
				{
					$error[] = $lang['INST_ERR_DB_NO_SQLITE'];
				}
			break;

			case 'firebird':
				// check the version of FB, use some hackery if we can't get access to the server info
				if ($db->service_handle !== false && function_exists('ibase_server_info'))
				{
					$val = @ibase_server_info($db->service_handle, IBASE_SVC_SERVER_VERSION);
					preg_match('#V([\d.]+)#', $val, $match);
					if ($match[1] < 2)
					{
						$error[] = $lang['INST_ERR_DB_NO_FIREBIRD'];
					}
					$db_info = @ibase_db_info($db->service_handle, $dbname, IBASE_STS_HDR_PAGES);

					preg_match('/^\\s*Page size\\s*(\\d+)/m', $db_info, $regs);
					$page_size = intval($regs[1]);
					if ($page_size < 8192)
					{
						$error[] = $lang['INST_ERR_DB_NO_FIREBIRD_PS'];
					}
				}
				else
				{
					$sql = "SELECT *
						FROM RDB$FUNCTIONS
						WHERE RDB$SYSTEM_FLAG IS NULL
							AND RDB$FUNCTION_NAME = 'CHAR_LENGTH'";
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					// if its a UDF, its too old
					if ($row)
					{
						$error[] = $lang['INST_ERR_DB_NO_FIREBIRD'];
					}
					else
					{
						$sql = "SELECT FIRST 0 char_length('')
							FROM RDB\$DATABASE";
						$result = $db->sql_query($sql);
						if (!$result) // This can only fail if char_length is not defined
						{
							$error[] = $lang['INST_ERR_DB_NO_FIREBIRD'];
						}
						$db->sql_freeresult($result);
					}

					// Setup the stuff for our random table
					$char_array = array_merge(range('A', 'Z'), range('0', '9'));
					$char_len = mt_rand(7, 9);
					$char_array_len = sizeof($char_array) - 1;

					$final = '';

					for ($i = 0; $i < $char_len; $i++)
					{
						$final .= $char_array[mt_rand(0, $char_array_len)];
					}

					// Create some random table
					$sql = 'CREATE TABLE ' . $final . " (
						FIELD1 VARCHAR(255) CHARACTER SET UTF8 DEFAULT '' NOT NULL COLLATE UNICODE,
						FIELD2 INTEGER DEFAULT 0 NOT NULL);";
					$db->sql_query($sql);

					// Create an index that should fail if the page size is less than 8192
					$sql = 'CREATE INDEX ' . $final . ' ON ' . $final . '(FIELD1, FIELD2);';
					$db->sql_query($sql);

					if (ibase_errmsg() !== false)
					{
						$error[] = $lang['INST_ERR_DB_NO_FIREBIRD_PS'];
					}
					else
					{
						// Kill the old table
						$db->sql_query('DROP TABLE ' . $final . ';');
					}
					unset($final);
				}
			break;
			
			case 'oracle':
				if ($unicode_check)
				{
					$sql = "SELECT *
						FROM NLS_DATABASE_PARAMETERS
						WHERE PARAMETER = 'NLS_RDBMS_VERSION'
							OR PARAMETER = 'NLS_CHARACTERSET'";
					$result = $db->sql_query($sql);

					while ($row = $db->sql_fetchrow($result))
					{
						$stats[$row['parameter']] = $row['value'];
					}
					$db->sql_freeresult($result);

					if (version_compare($stats['NLS_RDBMS_VERSION'], '9.2', '<') && $stats['NLS_CHARACTERSET'] !== 'UTF8')
					{
						$error[] = $lang['INST_ERR_DB_NO_ORACLE'];
					}
				}
			break;
			
			case 'postgres':
				if ($unicode_check)
				{
					$sql = "SHOW server_encoding;";
					$result = $db->sql_query($sql);
					$row = $db->sql_fetchrow($result);
					$db->sql_freeresult($result);

					if ($row['server_encoding'] !== 'UNICODE' && $row['server_encoding'] !== 'UTF8')
					{
						$error[] = $lang['INST_ERR_DB_NO_POSTGRES'];
					}
				}
			break;
		}

	}

	if ($error_connect && (!isset($error) || !sizeof($error)))
	{
		return true;
	}
	return false;
}

/**
* remove_remarks will strip the sql comment lines out of an uploaded sql file
*/
function remove_remarks(&$sql)
{
	$sql = preg_replace('/\n{2,}/', "\n", preg_replace('/^#.*$/m', "\n", $sql));
}

/**
* split_sql_file will split an uploaded sql file into single sql statements.
* Note: expects trim() to have already been run on $sql.
*/
function split_sql_file($sql, $delimiter)
{
	$sql = str_replace("\r" , '', $sql);
	$data = preg_split('/' . preg_quote($delimiter, '/') . '$/m', $sql);

	$data = array_map('trim', $data);

	// The empty case
	$end_data = end($data);

	if (empty($end_data))
	{
		unset($data[key($data)]);
	}

	return $data;
}

/**
* For replacing {L_*} strings with preg_replace_callback
*/
function adjust_language_keys_callback($matches)
{
	if (!empty($matches[1]))
	{
		global $lang, $db;

		return (!empty($lang[$matches[1]])) ? $db->sql_escape($lang[$matches[1]]) : $db->sql_escape($matches[1]);
	}
}

?>