<?php
/**
*
* @package acp
* @version $Id: acp_database.php 8479 2008-03-29 00:22:48Z naderman $
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
* @package acp
*/
class acp_database
{
	var $u_action;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template, $table_prefix;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;
		
		$user->add_lang('acp/database');

		$this->tpl_name = 'acp_database';
		$this->page_title = 'ACP_DATABASE';

		$action	= request_var('action', '');
		$submit = (isset($_POST['submit'])) ? true : false;

		$template->assign_vars(array(
			'MODE'	=> $mode
		));

		switch ($mode)
		{
			case 'backup':

				$this->page_title = 'ACP_BACKUP';

				switch ($action)
				{
					case 'download':
						$type	= request_var('type', '');
						$table	= request_var('table', array(''));
						$format	= request_var('method', '');
						$where	= request_var('where', '');

						if (!sizeof($table))
						{
							trigger_error($user->lang['TABLE_SELECT_ERROR'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						$store = $download = $structure = $schema_data = false;

						if ($where == 'store_and_download' || $where == 'store')
						{
							$store = true;
						}

						if ($where == 'store_and_download' || $where == 'download')
						{
							$download = true;
						}

						if ($type == 'full' || $type == 'structure')
						{
							$structure = true;
						}

						if ($type == 'full' || $type == 'data')
						{
							$schema_data = true;
						}

						@set_time_limit(1200);

						$time = time();

						$filename = 'backup_' . $time . '_' . unique_id();
						switch ($db->sql_layer)
						{
							case 'mysqli':
							case 'mysql4':
							case 'mysql':
								$extractor = new mysql_extractor($download, $store, $format, $filename, $time);
							break;

							case 'sqlite':
								$extractor = new sqlite_extractor($download, $store, $format, $filename, $time);
							break;

							case 'postgres':
								$extractor = new postgres_extractor($download, $store, $format, $filename, $time);
							break;

							case 'oracle':
								$extractor = new oracle_extractor($download, $store, $format, $filename, $time);
							break;

							case 'mssql':
							case 'mssql_odbc':
								$extractor = new mssql_extractor($download, $store, $format, $filename, $time);
							break;

							case 'firebird':
								$extractor = new firebird_extractor($download, $store, $format, $filename, $time);
							break;
						}

						$extractor->write_start($table_prefix);

						foreach ($table as $table_name)
						{
							// Get the table structure
							if ($structure)
							{
								$extractor->write_table($table_name);
							}
							else
							{
								// We might wanna empty out all that junk :D
								switch ($db->sql_layer)
								{
									case 'sqlite':
									case 'firebird':
										$extractor->flush('DELETE FROM ' . $table_name . ";\n");
									break;

									case 'mssql':
									case 'mssql_odbc':
										$extractor->flush('TRUNCATE TABLE ' . $table_name . "GO\n");
									break;

									case 'oracle':
										$extractor->flush('TRUNCATE TABLE ' . $table_name . "\\\n");
									break;

									default:
										$extractor->flush('TRUNCATE TABLE ' . $table_name . ";\n");
									break;
								}
							}

							// Data
							if ($schema_data)
							{
								$extractor->write_data($table_name);
							}
						}

						$extractor->write_end();

						if ($download == true)
						{
							exit;
						}

						add_log('admin', 'LOG_DB_BACKUP');
						trigger_error($user->lang['BACKUP_SUCCESS'] . adm_back_link($this->u_action));
					break;

					default:
						include($phpbb_root_path . 'includes/functions_install.' . $phpEx);
						$tables = get_tables($db);
						foreach ($tables as $table_name)
						{
							if (strlen($table_prefix) === 0 || stripos($table_name, $table_prefix) === 0)
							{
								$template->assign_block_vars('tables', array(
									'TABLE'	=> $table_name
								));
							}
						}
						unset($tables);

						$template->assign_vars(array(
							'U_ACTION'	=> $this->u_action . '&amp;action=download'
						));
						
						$available_methods = array('gzip' => 'zlib', 'bzip2' => 'bz2');

						foreach ($available_methods as $type => $module)
						{
							if (!@extension_loaded($module))
							{
								continue;
							}

							$template->assign_block_vars('methods', array(
								'TYPE'	=> $type
							));
						}

						$template->assign_block_vars('methods', array(
							'TYPE'	=> 'text'
						));
					break;
				}
			break;

			case 'restore':

				$this->page_title = 'ACP_RESTORE';

				switch ($action)
				{
					case 'submit':
						$delete = request_var('delete', '');
						$file = request_var('file', '');

						if (!preg_match('#^backup_\d{10,}_[a-z\d]{16}\.(sql(?:\.(?:gz|bz2))?)$#', $file, $matches))
						{
							trigger_error($user->lang['BACKUP_INVALID'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						$file_name = $phpbb_root_path . 'store/' . $matches[0];

						if (!file_exists($file_name) || !is_readable($file_name))
						{
							trigger_error($user->lang['BACKUP_INVALID'] . adm_back_link($this->u_action), E_USER_WARNING);
						}

						if ($delete)
						{
							if (confirm_box(true))
							{
								unlink($file_name);
								add_log('admin', 'LOG_DB_DELETE');
								trigger_error($user->lang['BACKUP_DELETE'] . adm_back_link($this->u_action));
							}
							else
							{
								confirm_box(false, $user->lang['DELETE_SELECTED_BACKUP'], build_hidden_fields(array('delete' => $delete, 'file' => $file)));
							}
						}
						else
						{
							$download = request_var('download', '');

							if ($download)
							{
								$name = $matches[0];

								switch ($matches[1])
								{
									case 'sql':
										$mimetype = 'text/x-sql';
									break;
									case 'sql.bz2':
										$mimetype = 'application/x-bzip2';
									break;
									case 'sql.gz':
										$mimetype = 'application/x-gzip';
									break;
								}

								header('Pragma: no-cache');
								header("Content-Type: $mimetype; name=\"$name\"");
								header("Content-disposition: attachment; filename=$name");

								@set_time_limit(0);

								$fp = @fopen($file_name, 'rb');

								if ($fp !== false)
								{
									while (!feof($fp))
									{
										echo fread($fp, 8192);
									}
									fclose($fp);
								}

								flush();
								exit;
							}

							switch ($matches[1])
							{
								case 'sql':
									$fp = fopen($file_name, 'rb');
									$read = 'fread';
									$seek = 'fseek';
									$eof = 'feof';
									$close = 'fclose';
									$fgetd = 'fgetd';
								break;

								case 'sql.bz2':
									$fp = bzopen($file_name, 'r');
									$read = 'bzread';
									$seek = '';
									$eof = 'feof';
									$close = 'bzclose';
									$fgetd = 'fgetd_seekless';
								break;

								case 'sql.gz':
									$fp = gzopen($file_name, 'rb');
									$read = 'gzread';
									$seek = 'gzseek';
									$eof = 'gzeof';
									$close = 'gzclose';
									$fgetd = 'fgetd';
								break;
							}

							switch ($db->sql_layer)
							{
								case 'mysql':
								case 'mysql4':
								case 'mysqli':
								case 'sqlite':
									while (($sql = $fgetd($fp, ";\n", $read, $seek, $eof)) !== false)
									{
										$db->sql_query($sql);
									}
								break;

								case 'firebird':
									$delim = ";\n";
									while (($sql = $fgetd($fp, $delim, $read, $seek, $eof)) !== false)
									{
										$query = trim($sql);
										if (substr($query, 0, 8) === 'SET TERM')
										{
											$delim = $query[9] . "\n";
											continue;
										}
										$db->sql_query($query);
									}
								break;

								case 'postgres':
									$delim = ";\n";
									while (($sql = $fgetd($fp, $delim, $read, $seek, $eof)) !== false)
									{
										$query = trim($sql);
										$db->sql_query($query);
										if (substr($query, 0, 4) == 'COPY')
										{
											while (($sub = $fgetd($fp, "\n", $read, $seek, $eof)) !== '\.')
											{
												if ($sub === false)
												{
													trigger_error($user->lang['RESTORE_FAILURE'] . adm_back_link($this->u_action), E_USER_WARNING);
												}
												pg_put_line($db->db_connect_id, $sub . "\n");
											}
											pg_put_line($db->db_connect_id, "\\.\n");
											pg_end_copy($db->db_connect_id);
										}
									}
								break;

								case 'oracle':
									while (($sql = $fgetd($fp, "/\n", $read, $seek, $eof)) !== false)
									{
										$db->sql_query($sql);
									}
								break;

								case 'mssql':
								case 'mssql_odbc':
									while (($sql = $fgetd($fp, "GO\n", $read, $seek, $eof)) !== false)
									{
										$db->sql_query($sql);
									}
								break;
							}

							$close($fp);

							add_log('admin', 'LOG_DB_RESTORE');
							trigger_error($user->lang['RESTORE_SUCCESS'] . adm_back_link($this->u_action));
							break;
						}

					default:
						$methods = array('sql');
						$available_methods = array('sql.gz' => 'zlib', 'sql.bz2' => 'bz2');

						foreach ($available_methods as $type => $module)
						{
							if (!@extension_loaded($module))
							{
								continue;
							}
							$methods[] = $type;
						}

						$dir = $phpbb_root_path . 'store/';
						$dh = @opendir($dir);

						if ($dh)
						{
							while (($file = readdir($dh)) !== false)
							{
								if (preg_match('#^backup_(\d{10,})_[a-z\d]{16}\.(sql(?:\.(?:gz|bz2))?)$#', $file, $matches))
								{
									$supported = in_array($matches[2], $methods);

									if ($supported == 'true')
									{
										$template->assign_block_vars('files', array(
											'FILE'		=> $file,
											'NAME'		=> gmdate("d-m-Y H:i:s", $matches[1]),
											'SUPPORTED'	=> $supported
										));
									}
								}
							}
							closedir($dh);
						}

						$template->assign_vars(array(
							'U_ACTION'	=> $this->u_action . '&amp;action=submit'
						));
					break;
				}
			break;
		}
	}
}

/**
* @package acp
*/
class base_extractor
{
	var $fh;
	var $fp;
	var $write;
	var $close;
	var $store;
	var $download;
	var $time;
	var $format;
	var $run_comp = false;

	function base_extractor($download = false, $store = false, $format, $filename, $time)
	{
		$this->download = $download;
		$this->store = $store;
		$this->time = $time;
		$this->format = $format;

		switch ($format)
		{
			case 'text':
				$ext = '.sql';
				$open = 'fopen';
				$this->write = 'fwrite';
				$this->close = 'fclose';
				$mimetype = 'text/x-sql';
			break;
			case 'bzip2':
				$ext = '.sql.bz2';
				$open = 'bzopen';
				$this->write = 'bzwrite';
				$this->close = 'bzclose';
				$mimetype = 'application/x-bzip2';
			break;
			case 'gzip':
				$ext = '.sql.gz';
				$open = 'gzopen';
				$this->write = 'gzwrite';
				$this->close = 'gzclose';
				$mimetype = 'application/x-gzip';
			break;
		}

		if ($download == true)
		{
			$name = $filename . $ext;
			header('Pragma: no-cache');
			header("Content-Type: $mimetype; name=\"$name\"");
			header("Content-disposition: attachment; filename=$name");
	
			switch ($format)
			{
				case 'bzip2':
					ob_start();
				break;

				case 'gzip':
					if ((isset($_SERVER['HTTP_ACCEPT_ENCODING']) && strpos($_SERVER['HTTP_ACCEPT_ENCODING'], 'gzip') !== false) && strpos(strtolower($_SERVER['HTTP_USER_AGENT']), 'msie') === false)
					{
						ob_start('ob_gzhandler');
					}
					else
					{
						$this->run_comp = true;
					}
				break;
			}
		}
		
		if ($store == true)
		{
			global $phpbb_root_path;
			$file = $phpbb_root_path . 'store/' . $filename . $ext;
	
			$this->fp = $open($file, 'w');
	
			if (!$this->fp)
			{
				trigger_error('Unable to write temporary file to storage folder', E_USER_ERROR);
			}
		}
	}

	function write_end()
	{
		static $close;
		if ($this->store)
		{
			if ($close === null)
			{
				$close = $this->close;
			}
			$close($this->fp);
		}

		// bzip2 must be written all the way at the end
		if ($this->download && $this->format === 'bzip2')
		{
			$c = ob_get_clean();
			echo bzcompress($c);
		}
	}

	function flush($data)
	{
		static $write;
		if ($this->store === true)
		{
			if ($write === null)
			{
				$write = $this->write;
			}
			$write($this->fp, $data);
		}

		if ($this->download === true)
		{
			if ($this->format === 'bzip2' || $this->format === 'text' || ($this->format === 'gzip' && !$this->run_comp))
			{
				echo $data;
			}

			// we can write the gzip data as soon as we get it
			if ($this->format === 'gzip')
			{
				if ($this->run_comp)
				{
					echo gzencode($data);
				}
				else
				{
					ob_flush();
					flush();
				}
			}
		}
	}
}

/**
* @package acp
*/
class mysql_extractor extends base_extractor
{
	function write_start($table_prefix)
	{
		$sql_data = "#\n";
		$sql_data .= "# phpBB Backup Script\n";
		$sql_data .= "# Dump of tables for $table_prefix\n";
		$sql_data .= "# DATE : " . gmdate("d-m-Y H:i:s", $this->time) . " GMT\n";
		$sql_data .= "#\n";
		$this->flush($sql_data);
	}

	function write_table($table_name)
	{
		global $db;
		static $new_extract;

		if ($new_extract === null)
		{
			if ($db->sql_layer === 'mysqli' || version_compare($db->mysql_version, '3.23.20', '>='))
			{
				$new_extract = true;
			}
			else
			{
				$new_extract = false;
			}
		}

		if ($new_extract)
		{
			$this->new_write_table($table_name);
		}
		else
		{
			$this->old_write_table($table_name);
		}
	}

	function write_data($table_name)
	{
		global $db;
		if ($db->sql_layer === 'mysqli')
		{
			$this->write_data_mysqli($table_name);
		}
		else
		{
			$this->write_data_mysql($table_name);
		}
	}

	function write_data_mysqli($table_name)
	{
		global $db;
		$sql = "SELECT *
			FROM $table_name";
		$result = mysqli_query($db->db_connect_id, $sql, MYSQLI_USE_RESULT);
		if ($result != false)
		{
			$fields_cnt = mysqli_num_fields($result);
		
			// Get field information
			$field = mysqli_fetch_fields($result);
			$field_set = array();
		
			for ($j = 0; $j < $fields_cnt; $j++)
			{
				$field_set[] = $field[$j]->name;
			}

			$search			= array("\\", "'", "\x00", "\x0a", "\x0d", "\x1a", '"');
			$replace		= array("\\\\", "\\'", '\0', '\n', '\r', '\Z', '\\"');
			$fields			= implode(', ', $field_set);
			$sql_data		= 'INSERT INTO ' . $table_name . ' (' . $fields . ') VALUES ';
			$first_set		= true;
			$query_len		= 0;
			$max_len		= get_usable_memory();
		
			while ($row = mysqli_fetch_row($result))
			{
				$values	= array();
				if ($first_set)
				{
					$query = $sql_data . '(';
				}
				else
				{
					$query  .= ',(';
				}

				for ($j = 0; $j < $fields_cnt; $j++)
				{
					if (!isset($row[$j]) || is_null($row[$j]))
					{
						$values[$j] = 'NULL';
					}
					else if (($field[$j]->flags & 32768) && !($field[$j]->flags & 1024))
					{
						$values[$j] = $row[$j];
					}
					else
					{
						$values[$j] = "'" . str_replace($search, $replace, $row[$j]) . "'";
					}
				}
				$query .= implode(', ', $values) . ')';

				$query_len += strlen($query);
				if ($query_len > $max_len)
				{
					$this->flush($query . ";\n\n");
					$query = '';
					$query_len = 0;
					$first_set = true;
				}
				else
				{
					$first_set = false;
				}
			}
			mysqli_free_result($result);

			// check to make sure we have nothing left to flush
			if (!$first_set && $query)
			{
				$this->flush($query . ";\n\n");
			}
		}
	}

	function write_data_mysql($table_name)
	{
		global $db;
		$sql = "SELECT *
			FROM $table_name";
		$result = mysql_unbuffered_query($sql, $db->db_connect_id);

		if ($result != false)
		{
			$fields_cnt = mysql_num_fields($result);

			// Get field information
			$field = array();
			for ($i = 0; $i < $fields_cnt; $i++)
			{
				$field[] = mysql_fetch_field($result, $i);
			}
			$field_set = array();
			
			for ($j = 0; $j < $fields_cnt; $j++)
			{
				$field_set[] = $field[$j]->name;
			}

			$search			= array("\\", "'", "\x00", "\x0a", "\x0d", "\x1a", '"');
			$replace		= array("\\\\", "\\'", '\0', '\n', '\r', '\Z', '\\"');
			$fields			= implode(', ', $field_set);
			$sql_data		= 'INSERT INTO ' . $table_name . ' (' . $fields . ') VALUES ';
			$first_set		= true;
			$query_len		= 0;
			$max_len		= get_usable_memory();

			while ($row = mysql_fetch_row($result))
			{
				$values = array();
				if ($first_set)
				{
					$query = $sql_data . '(';
				}
				else
				{
					$query  .= ',(';
				}

				for ($j = 0; $j < $fields_cnt; $j++)
				{
					if (!isset($row[$j]) || is_null($row[$j]))
					{
						$values[$j] = 'NULL';
					}
					else if ($field[$j]->numeric && ($field[$j]->type !== 'timestamp'))
					{
						$values[$j] = $row[$j];
					}
					else
					{
						$values[$j] = "'" . str_replace($search, $replace, $row[$j]) . "'";
					}
				}
				$query .= implode(', ', $values) . ')';

				$query_len += strlen($query);
				if ($query_len > $max_len)
				{
					$this->flush($query . ";\n\n");
					$query = '';
					$query_len = 0;
					$first_set = true;
				}
				else
				{
					$first_set = false;
				}
			}
			mysql_free_result($result);

			// check to make sure we have nothing left to flush
			if (!$first_set && $query)
			{
				$this->flush($query . ";\n\n");
			}
		}
	}

	function new_write_table($table_name)
	{
		global $db;

		$sql = 'SHOW CREATE TABLE ' . $table_name;
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);

		$sql_data = '# Table: ' . $table_name . "\n";
		$sql_data .= "DROP TABLE IF EXISTS $table_name;\n";
		$this->flush($sql_data . $row['Create Table'] . ";\n\n");

		$db->sql_freeresult($result);
	}

	function old_write_table($table_name)
	{
		global $db;

		$sql_data = '# Table: ' . $table_name . "\n";
		$sql_data .= "DROP TABLE IF EXISTS $table_name;\n";
		$sql_data .= "CREATE TABLE $table_name(\n";
		$rows = array();

		$sql = "SHOW FIELDS
			FROM $table_name";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$line = '   ' . $row['Field'] . ' ' . $row['Type'];

			if (!is_null($row['Default']))
			{
				$line .= " DEFAULT '{$row['Default']}'";
			}

			if ($row['Null'] != 'YES')
			{
				$line .= ' NOT NULL';
			}

			if ($row['Extra'] != '')
			{
				$line .= ' ' . $row['Extra'];
			}

			$rows[] = $line;
		}
		$db->sql_freeresult($result);

		$sql = "SHOW KEYS
			FROM $table_name";

		$result = $db->sql_query($sql);

		$index = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$kname = $row['Key_name'];

			if ($kname != 'PRIMARY')
			{
				if ($row['Non_unique'] == 0)
				{
					$kname = "UNIQUE|$kname";
				}
			}

			if ($row['Sub_part'])
			{
				$row['Column_name'] .= '(' . $row['Sub_part'] . ')';
			}
			$index[$kname][] = $row['Column_name'];
		}
		$db->sql_freeresult($result);

		foreach ($index as $key => $columns)
		{
			$line = '   ';

			if ($key == 'PRIMARY')
			{
				$line .= 'PRIMARY KEY (' . implode(', ', $columns) . ')';
			}
			else if (strpos($key, 'UNIQUE') === 0)
			{
				$line .= 'UNIQUE ' . substr($key, 7) . ' (' . implode(', ', $columns) . ')';
			}
			else if (strpos($key, 'FULLTEXT') === 0)
			{
				$line .= 'FULLTEXT ' . substr($key, 9) . ' (' . implode(', ', $columns) . ')';
			}
			else
			{
				$line .= "KEY $key (" . implode(', ', $columns) . ')';
			}

			$rows[] = $line;
		}

		$sql_data .= implode(",\n", $rows);
		$sql_data .= "\n);\n\n";

		$this->flush($sql_data);
	}
}

/**
* @package acp
*/
class sqlite_extractor extends base_extractor
{
	function write_start($prefix)
	{
		$sql_data = "--\n";
		$sql_data .= "-- phpBB Backup Script\n";
		$sql_data .= "-- Dump of tables for $prefix\n";
		$sql_data .= "-- DATE : " . gmdate("d-m-Y H:i:s", $this->time) . " GMT\n";
		$sql_data .= "--\n";
		$sql_data .= "BEGIN TRANSACTION;\n";
		$this->flush($sql_data);
	}

	function write_table($table_name)
	{
		global $db;
		$sql_data = '-- Table: ' . $table_name . "\n";
		$sql_data .= "DROP TABLE $table_name;\n";

		$sql = "SELECT sql
			FROM sqlite_master
			WHERE type = 'table'
				AND name = '" . $db->sql_escape($table_name) . "'
			ORDER BY type DESC, name;";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		// Create Table
		$sql_data .= $row['sql'] . ";\n";

		$result = $db->sql_query("PRAGMA index_list('" . $db->sql_escape($table_name) . "');");

		$ar = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$ar[] = $row;
		}
		$db->sql_freeresult($result);
		
		foreach ($ar as $value)
		{
			if (strpos($value['name'], 'autoindex') !== false)
			{
				continue;
			}

			$result = $db->sql_query("PRAGMA index_info('" . $db->sql_escape($value['name']) . "');");

			$fields = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$fields[] = $row['name'];
			}
			$db->sql_freeresult($result);

			$sql_data .= 'CREATE ' . ($value['unique'] ? 'UNIQUE ' : '') . 'INDEX ' . $value['name'] . ' on ' . $table_name . ' (' . implode(', ', $fields) . ");\n";
		}

		$this->flush($sql_data . "\n");
	}

	function write_data($table_name)
	{
		global $db;
		static $proper;

		if (is_null($proper))
		{
			$proper = version_compare(PHP_VERSION, '5.1.3', '>=');
		}

		if ($proper)
		{
			$col_types = sqlite_fetch_column_types($db->db_connect_id, $table_name);
		}
		else
		{
			$sql = "SELECT sql
				FROM sqlite_master
				WHERE type = 'table'
					AND name = '" . $table_name . "'";
			$table_data = sqlite_single_query($db->db_connect_id, $sql);
			$table_data = preg_replace('#CREATE\s+TABLE\s+"?' . $table_name . '"?#i', '', $table_data);
			$table_data = trim($table_data);

			preg_match('#\((.*)\)#s', $table_data, $matches);

			$table_cols = explode(',', trim($matches[1]));
			foreach ($table_cols as $declaration)
			{
				$entities = preg_split('#\s+#', trim($declaration));
				$column_name = preg_replace('/"?([^"]+)"?/', '\1', $entities[0]);

				// Hit a primary key, those are not what we need :D
				if (empty($entities[1]) || (strtolower($entities[0]) === 'primary' && strtolower($entities[1]) === 'key'))
				{
					continue;
				}
				$col_types[$column_name] = $entities[1];
			}
		}

		$sql = "SELECT *
			FROM $table_name";
		$result = sqlite_unbuffered_query($db->db_connect_id, $sql);
		$rows = sqlite_fetch_all($result, SQLITE_ASSOC);
		$sql_insert = 'INSERT INTO ' . $table_name . ' (' . implode(', ', array_keys($col_types)) . ') VALUES (';
		foreach ($rows as $row)
		{
			foreach ($row as $column_name => $column_data)
			{
				if (is_null($column_data))
				{
					$row[$column_name] = 'NULL';
				}
				else if ($column_data == '')
				{
					$row[$column_name] = "''";
				}
				else if (strpos($col_types[$column_name], 'text') !== false || strpos($col_types[$column_name], 'char') !== false || strpos($col_types[$column_name], 'blob') !== false)
				{
					$row[$column_name] = sanitize_data_generic(str_replace("'", "''", $column_data));
				}
			}
			$this->flush($sql_insert . implode(', ', $row) . ");\n");
		}
	}

	function write_end()
	{
		$this->flush("COMMIT;\n");
		parent::write_end();
	}
}

/**
* @package acp
*/
class postgres_extractor extends base_extractor
{
	function write_start($prefix)
	{
		$sql_data = "--\n";
		$sql_data .= "-- phpBB Backup Script\n";
		$sql_data .= "-- Dump of tables for $prefix\n";
		$sql_data .= "-- DATE : " . gmdate("d-m-Y H:i:s", $this->time) . " GMT\n";
		$sql_data .= "--\n";
		$sql_data .= "BEGIN TRANSACTION;\n";
		$this->flush($sql_data);
	}

	function write_table($table_name)
	{
		global $db;
		static $domains_created = array();

		$sql = "SELECT a.domain_name, a.data_type, a.character_maximum_length, a.domain_default
			FROM INFORMATION_SCHEMA.domains a, INFORMATION_SCHEMA.column_domain_usage b
			WHERE a.domain_name = b.domain_name
				AND b.table_name = '{$table_name}'";
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			if (empty($domains_created[$row['domain_name']]))
			{
				$domains_created[$row['domain_name']] = true;
				//$sql_data = "DROP DOMAIN {$row['domain_name']};\n";
				$sql_data = "CREATE DOMAIN {$row['domain_name']} as {$row['data_type']}";
				if (!empty($row['character_maximum_length']))
				{
					$sql_data .= '(' . $row['character_maximum_length'] . ')';
				}
				$sql_data .= ' NOT NULL';
				if (!empty($row['domain_default']))
				{
					$sql_data .= ' DEFAULT ' . $row['domain_default'];
				}
				$this->flush($sql_data . ";\n");
			}
		}

		$sql_data = '-- Table: ' . $table_name . "\n";
		//$sql_data .= "DROP TABLE $table_name;\n";
		// PGSQL does not "tightly" bind sequences and tables, we must guess...
		$sql = "SELECT relname
			FROM pg_class
			WHERE relkind = 'S'
				AND relname = '{$table_name}_seq'";
		$result = $db->sql_query($sql);
		// We don't even care about storing the results. We already know the answer if we get rows back.
		if ($db->sql_fetchrow($result))
		{
			$sql_data .= "DROP SEQUENCE {$table_name}_seq;\n";
			$sql_data .= "CREATE SEQUENCE {$table_name}_seq;\n";
		}
		$db->sql_freeresult($result);
	
		$field_query = "SELECT a.attnum, a.attname as field, t.typname as type, a.attlen as length, a.atttypmod as lengthvar, a.attnotnull as notnull
			FROM pg_class c, pg_attribute a, pg_type t
			WHERE c.relname = '" . $db->sql_escape($table_name) . "'
				AND a.attnum > 0
				AND a.attrelid = c.oid
				AND a.atttypid = t.oid
			ORDER BY a.attnum";
		$result = $db->sql_query($field_query);

		$sql_data .= "CREATE TABLE $table_name(\n";
		$lines = array();
		while ($row = $db->sql_fetchrow($result))
		{
			// Get the data from the table
			$sql_get_default = "SELECT pg_get_expr(d.adbin, d.adrelid) as rowdefault
				FROM pg_attrdef d, pg_class c
				WHERE (c.relname = '" . $db->sql_escape($table_name) . "')
					AND (c.oid = d.adrelid)
					AND d.adnum = " . $row['attnum'];
			$def_res = $db->sql_query($sql_get_default);

			if (!$def_res)
			{
				unset($row['rowdefault']);
			}
			else
			{
				$row['rowdefault'] = $db->sql_fetchfield('rowdefault', false, $def_res);
			}
			$db->sql_freeresult($def_res);

			if ($row['type'] == 'bpchar')
			{
				// Internally stored as bpchar, but isn't accepted in a CREATE TABLE statement.
				$row['type'] = 'char';
			}

			$line = '  ' . $row['field'] . ' ' . $row['type'];

			if (strpos($row['type'], 'char') !== false)
			{
				if ($row['lengthvar'] > 0)
				{
					$line .= '(' . ($row['lengthvar'] - 4) . ')';
				}
			}

			if (strpos($row['type'], 'numeric') !== false)
			{
				$line .= '(';
				$line .= sprintf("%s,%s", (($row['lengthvar'] >> 16) & 0xffff), (($row['lengthvar'] - 4) & 0xffff));
				$line .= ')';
			}

			if (!empty($row['rowdefault']))
			{
				$line .= ' DEFAULT ' . $row['rowdefault'];
			}

			if ($row['notnull'] == 't')
			{
				$line .= ' NOT NULL';
			}
			
			$lines[] = $line;
		}
		$db->sql_freeresult($result);


		// Get the listing of primary keys.
		$sql_pri_keys = "SELECT ic.relname as index_name, bc.relname as tab_name, ta.attname as column_name, i.indisunique as unique_key, i.indisprimary as primary_key
			FROM pg_class bc, pg_class ic, pg_index i, pg_attribute ta, pg_attribute ia
			WHERE (bc.oid = i.indrelid)
				AND (ic.oid = i.indexrelid)
				AND (ia.attrelid = i.indexrelid)
				AND	(ta.attrelid = bc.oid)
				AND (bc.relname = '" . $db->sql_escape($table_name) . "')
				AND (ta.attrelid = i.indrelid)
				AND (ta.attnum = i.indkey[ia.attnum-1])
			ORDER BY index_name, tab_name, column_name";

		$result = $db->sql_query($sql_pri_keys);

		$index_create = $index_rows = $primary_key = array();

		// We do this in two steps. It makes placing the comma easier
		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['primary_key'] == 't')
			{
				$primary_key[] = $row['column_name'];
				$primary_key_name = $row['index_name'];
			}
			else
			{
				// We have to store this all this info because it is possible to have a multi-column key...
				// we can loop through it again and build the statement
				$index_rows[$row['index_name']]['table'] = $table_name;
				$index_rows[$row['index_name']]['unique'] = ($row['unique_key'] == 't') ? true : false;
				$index_rows[$row['index_name']]['column_names'][] = $row['column_name'];
			}
		}
		$db->sql_freeresult($result);

		if (!empty($index_rows))
		{
			foreach ($index_rows as $idx_name => $props)
			{
				$index_create[] = 'CREATE ' . ($props['unique'] ? 'UNIQUE ' : '') . "INDEX $idx_name ON $table_name (" . implode(', ', $props['column_names']) . ");";
			}
		}

		if (!empty($primary_key))
		{
			$lines[] = "  CONSTRAINT $primary_key_name PRIMARY KEY (" . implode(', ', $primary_key) . ")";
		}

		// Generate constraint clauses for CHECK constraints
		$sql_checks = "SELECT conname as index_name, consrc
			FROM pg_constraint, pg_class bc
			WHERE conrelid = bc.oid
				AND bc.relname = '" . $db->sql_escape($table_name) . "'
				AND NOT EXISTS (
					SELECT *
						FROM pg_constraint as c, pg_inherits as i
						WHERE i.inhrelid = pg_constraint.conrelid
							AND c.conname = pg_constraint.conname
							AND c.consrc = pg_constraint.consrc
							AND c.conrelid = i.inhparent
				)";
		$result = $db->sql_query($sql_checks);

		// Add the constraints to the sql file.
		while ($row = $db->sql_fetchrow($result))
		{
			if (!is_null($row['consrc']))
			{
				$lines[] = '  CONSTRAINT ' . $row['index_name'] . ' CHECK ' . $row['consrc'];
			}
		}
		$db->sql_freeresult($result);

		$sql_data .= implode(", \n", $lines);
		$sql_data .= "\n);\n";

		if (!empty($index_create))
		{
			$sql_data .= implode("\n", $index_create) . "\n\n";
		}
		$this->flush($sql_data);
	}

	function write_data($table_name)
	{
		global $db;
		// Grab all of the data from current table.
		$sql = "SELECT *
			FROM $table_name";
		$result = $db->sql_query($sql);

		$i_num_fields = pg_num_fields($result);
		$seq = '';

		for ($i = 0; $i < $i_num_fields; $i++)
		{
			$ary_type[] = pg_field_type($result, $i);
			$ary_name[] = pg_field_name($result, $i);


			$sql = "SELECT pg_get_expr(d.adbin, d.adrelid) as rowdefault
				FROM pg_attrdef d, pg_class c
				WHERE (c.relname = '{$table_name}')
					AND (c.oid = d.adrelid)
					AND d.adnum = " . strval($i + 1);
			$result2 = $db->sql_query($sql);
			if ($row = $db->sql_fetchrow($result2))
			{
				// Determine if we must reset the sequences
				if (strpos($row['rowdefault'], "nextval('") === 0)
				{
					$seq .= "SELECT SETVAL('{$table_name}_seq',(select case when max({$ary_name[$i]})>0 then max({$ary_name[$i]})+1 else 1 end FROM {$table_name}));\n";
				}
			}
		}

		$this->flush("COPY $table_name (" . implode(', ', $ary_name) . ') FROM stdin;' . "\n");
		while ($row = $db->sql_fetchrow($result))
		{
			$schema_vals = array();

			// Build the SQL statement to recreate the data.
			for ($i = 0; $i < $i_num_fields; $i++)
			{
				$str_val = $row[$ary_name[$i]];

				if (preg_match('#char|text|bool|bytea#i', $ary_type[$i]))
				{
					$str_val = str_replace(array("\n", "\t", "\r", "\b", "\f", "\v"), array('\n', '\t', '\r', '\b', '\f', '\v'), addslashes($str_val));
					$str_empty = '';
				}
				else
				{
					$str_empty = '\N';
				}

				if (empty($str_val) && $str_val !== '0')
				{
					$str_val = $str_empty;
				}

				$schema_vals[] = $str_val;
			}

			// Take the ordered fields and their associated data and build it
			// into a valid sql statement to recreate that field in the data.
			$this->flush(implode("\t", $schema_vals) . "\n");
		}
		$db->sql_freeresult($result);
		$this->flush("\\.\n");

		// Write out the sequence statements
		$this->flush($seq);
	}

	function write_end()
	{
		$this->flush("COMMIT;\n");
		parent::write_end();
	}
}

/**
* @package acp
*/
class mssql_extractor extends base_extractor
{
	function write_end()
	{
		$this->flush("COMMIT\nGO\n");
		parent::write_end();
	}

	function write_start($prefix)
	{
		$sql_data = "--\n";
		$sql_data .= "-- phpBB Backup Script\n";
		$sql_data .= "-- Dump of tables for $prefix\n";
		$sql_data .= "-- DATE : " . gmdate("d-m-Y H:i:s", $this->time) . " GMT\n";
		$sql_data .= "--\n";
		$sql_data .= "BEGIN TRANSACTION\n";
		$sql_data .= "GO\n";
		$this->flush($sql_data);
	}

	function write_table($table_name)
	{
		global $db;
		$sql_data = '-- Table: ' . $table_name . "\n";
		$sql_data .= "IF OBJECT_ID(N'$table_name', N'U') IS NOT NULL\n";
		$sql_data .= "DROP TABLE $table_name;\n";
		$sql_data .= "GO\n";
		$sql_data .= "\nCREATE TABLE [$table_name] (\n";
		$rows = array();
	
		$text_flag = false;
	
		$sql = "SELECT COLUMN_NAME, COLUMN_DEFAULT, IS_NULLABLE, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH, COLUMNPROPERTY(object_id(TABLE_NAME), COLUMN_NAME, 'IsIdentity') as IS_IDENTITY
			FROM INFORMATION_SCHEMA.COLUMNS
			WHERE TABLE_NAME = '$table_name'";
		$result = $db->sql_query($sql);
	
		while ($row = $db->sql_fetchrow($result))
		{
			$line = "\t[{$row['COLUMN_NAME']}] [{$row['DATA_TYPE']}]";
	
			if ($row['DATA_TYPE'] == 'text')
			{
				$text_flag = true;
			}
	
			if ($row['IS_IDENTITY'])
			{
				$line .= ' IDENTITY (1 , 1)';
			}
	
			if ($row['CHARACTER_MAXIMUM_LENGTH'] && $row['DATA_TYPE'] !== 'text')
			{
				$line .= ' (' . $row['CHARACTER_MAXIMUM_LENGTH'] . ')';
			}
	
			if ($row['IS_NULLABLE'] == 'YES')
			{
				$line .= ' NULL';
			}
			else
			{
				$line .= ' NOT NULL';
			}
	
			if ($row['COLUMN_DEFAULT'])
			{
				$line .= ' DEFAULT ' . $row['COLUMN_DEFAULT'];
			}
	
			$rows[] = $line;
		}
		$db->sql_freeresult($result);
	
		$sql_data .= implode(",\n", $rows);
		$sql_data .= "\n) ON [PRIMARY]";
	
		if ($text_flag)
		{
			$sql_data .= " TEXTIMAGE_ON [PRIMARY]";
		}
	
		$sql_data .= "\nGO\n\n";
		$rows = array();
	
		$sql = "SELECT CONSTRAINT_NAME, COLUMN_NAME
			FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE
			WHERE TABLE_NAME = '$table_name'";
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			if (!sizeof($rows))
			{
				$sql_data .= "ALTER TABLE [$table_name] WITH NOCHECK ADD\n";
				$sql_data .= "\tCONSTRAINT [{$row['CONSTRAINT_NAME']}] PRIMARY KEY  CLUSTERED \n\t(\n";
			}
			$rows[] = "\t\t[{$row['COLUMN_NAME']}]";
		}
		if (sizeof($rows))
		{
			$sql_data .= implode(",\n", $rows);
			$sql_data .= "\n\t)  ON [PRIMARY] \nGO\n";
		}
		$db->sql_freeresult($result);
	
		$index = array();
		$sql = "EXEC sp_statistics '$table_name'";
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['TYPE'] == 3)
			{
				$index[$row['INDEX_NAME']][] = '[' . $row['COLUMN_NAME'] . ']';
			}
		}
		$db->sql_freeresult($result);
	
		foreach ($index as $index_name => $column_name)
		{
			$index[$index_name] = implode(', ', $column_name);
		}
	
		foreach ($index as $index_name => $columns)
		{
			$sql_data .= "\nCREATE  INDEX [$index_name] ON [$table_name]($columns) ON [PRIMARY]\nGO\n";
		}
		$this->flush($sql_data);
	}

	function write_data($table_name)
	{
		global $db;

		if ($db->sql_layer === 'mssql')
		{
			$this->write_data_mssql($table_name);
		}
		else
		{
			$this->write_data_odbc($table_name);
		}
	}

	function write_data_mssql($table_name)
	{
		global $db;
		$ary_type = $ary_name = array();
		$ident_set = false;
		$sql_data = '';
		
		// Grab all of the data from current table.
		$sql = "SELECT *
			FROM $table_name";
		$result = $db->sql_query($sql);

		$retrieved_data = mssql_num_rows($result);

		$i_num_fields = mssql_num_fields($result);

		for ($i = 0; $i < $i_num_fields; $i++)
		{
			$ary_type[$i] = mssql_field_type($result, $i);
			$ary_name[$i] = mssql_field_name($result, $i);
		}

		if ($retrieved_data)
		{
			$sql = "SELECT 1 as has_identity
				FROM INFORMATION_SCHEMA.COLUMNS
				WHERE COLUMNPROPERTY(object_id('$table_name'), COLUMN_NAME, 'IsIdentity') = 1";
			$result2 = $db->sql_query($sql);
			$row2 = $db->sql_fetchrow($result2);
			if (!empty($row2['has_identity']))
			{
				$sql_data .= "\nSET IDENTITY_INSERT $table_name ON\nGO\n";
				$ident_set = true;
			}
			$db->sql_freeresult($result2);
		}

		while ($row = $db->sql_fetchrow($result))
		{
			$schema_vals = $schema_fields = array();

			// Build the SQL statement to recreate the data.
			for ($i = 0; $i < $i_num_fields; $i++)
			{
				$str_val = $row[$ary_name[$i]];

				if (preg_match('#char|text|bool|varbinary#i', $ary_type[$i]))
				{
					$str_quote = '';
					$str_empty = "''";
					$str_val = sanitize_data_mssql(str_replace("'", "''", $str_val));
				}
				else if (preg_match('#date|timestamp#i', $ary_type[$i]))
				{
					if (empty($str_val))
					{
						$str_quote = '';
					}
					else
					{
						$str_quote = "'";
					}
				}
				else
				{
					$str_quote = '';
					$str_empty = 'NULL';
				}

				if (empty($str_val) && $str_val !== '0' && !(is_int($str_val) || is_float($str_val)))
				{
					$str_val = $str_empty;
				}

				$schema_vals[$i] = $str_quote . $str_val . $str_quote;
				$schema_fields[$i] = $ary_name[$i];
			}

			// Take the ordered fields and their associated data and build it
			// into a valid sql statement to recreate that field in the data.
			$sql_data .= "INSERT INTO $table_name (" . implode(', ', $schema_fields) . ') VALUES (' . implode(', ', $schema_vals) . ");\nGO\n";

			$this->flush($sql_data);
			$sql_data = '';
		}
		$db->sql_freeresult($result);

		if ($retrieved_data && $ident_set)
		{
			$sql_data .= "\nSET IDENTITY_INSERT $table_name OFF\nGO\n";
		}
		$this->flush($sql_data);
	}

	function write_data_odbc($table_name)
	{
		global $db;
		$ary_type = $ary_name = array();
		$ident_set = false;
		$sql_data = '';
		
		// Grab all of the data from current table.
		$sql = "SELECT *
			FROM $table_name";
		$result = $db->sql_query($sql);

		$retrieved_data = odbc_num_rows($result);

		if ($retrieved_data)
		{
			$sql = "SELECT 1 as has_identity
				FROM INFORMATION_SCHEMA.COLUMNS
				WHERE COLUMNPROPERTY(object_id('$table_name'), COLUMN_NAME, 'IsIdentity') = 1";
			$result2 = $db->sql_query($sql);
			$row2 = $db->sql_fetchrow($result2);
			if (!empty($row2['has_identity']))
			{
				$sql_data .= "\nSET IDENTITY_INSERT $table_name ON\nGO\n";
				$ident_set = true;
			}
			$db->sql_freeresult($result2);
		}

		$i_num_fields = odbc_num_fields($result);

		for ($i = 0; $i < $i_num_fields; $i++)
		{
			$ary_type[$i] = odbc_field_type($result, $i + 1);
			$ary_name[$i] = odbc_field_name($result, $i + 1);
		}

		while ($row = $db->sql_fetchrow($result))
		{
			$schema_vals = $schema_fields = array();

			// Build the SQL statement to recreate the data.
			for ($i = 0; $i < $i_num_fields; $i++)
			{
				$str_val = $row[$ary_name[$i]];

				if (preg_match('#char|text|bool|varbinary#i', $ary_type[$i]))
				{
					$str_quote = '';
					$str_empty = "''";
					$str_val = sanitize_data_mssql(str_replace("'", "''", $str_val));
				}
				else if (preg_match('#date|timestamp#i', $ary_type[$i]))
				{
					if (empty($str_val))
					{
						$str_quote = '';
					}
					else
					{
						$str_quote = "'";
					}
				}
				else
				{
					$str_quote = '';
					$str_empty = 'NULL';
				}

				if (empty($str_val) && $str_val !== '0' && !(is_int($str_val) || is_float($str_val)))
				{
					$str_val = $str_empty;
				}

				$schema_vals[$i] = $str_quote . $str_val . $str_quote;
				$schema_fields[$i] = $ary_name[$i];
			}

			// Take the ordered fields and their associated data and build it
			// into a valid sql statement to recreate that field in the data.
			$sql_data .= "INSERT INTO $table_name (" . implode(', ', $schema_fields) . ') VALUES (' . implode(', ', $schema_vals) . ");\nGO\n";

			$this->flush($sql_data);

			$sql_data = '';

		}
		$db->sql_freeresult($result);

		if ($retrieved_data && $ident_set)
		{
			$sql_data .= "\nSET IDENTITY_INSERT $table_name OFF\nGO\n";
		}
		$this->flush($sql_data);
	}

}

/**
* @package acp
*/
class oracle_extractor extends base_extractor
{
	function write_table($table_name)
	{
		global $db;
		$sql_data = '-- Table: ' . $table_name . "\n";
		$sql_data .= "DROP TABLE $table_name;\n";
		$sql_data .= '\\' . "\n";
		$sql_data .= "\nCREATE TABLE $table_name (\n";

		$sql = "SELECT COLUMN_NAME, DATA_TYPE, DATA_PRECISION, DATA_LENGTH, NULLABLE, DATA_DEFAULT
			FROM ALL_TAB_COLS
			WHERE table_name = '{$table_name}'";
		$result = $db->sql_query($sql);

		$rows = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$line = '  "' . $row['column_name'] . '" ' . $row['data_type'];

			if ($row['data_type'] !== 'CLOB')
			{
				if ($row['data_type'] !== 'VARCHAR2')
				{
					$line .= '(' . $row['data_precision'] . ')';
				}
				else
				{
					$line .= '(' . $row['data_length'] . ')';
				}
			}

			if (!empty($row['data_default']))
			{
				$line .= ' DEFAULT ' . $row['data_default'];
			}

			if ($row['nullable'] == 'N')
			{
				$line .= ' NOT NULL';
			}
			$rows[] = $line;
		}
		$db->sql_freeresult($result);

		$sql = "SELECT A.CONSTRAINT_NAME, A.COLUMN_NAME
			FROM USER_CONS_COLUMNS A, USER_CONSTRAINTS B
			WHERE A.CONSTRAINT_NAME = B.CONSTRAINT_NAME
				AND B.CONSTRAINT_TYPE = 'P'
				AND A.TABLE_NAME = '{$table_name}'";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$rows[] = "  CONSTRAINT {$row['constraint_name']} PRIMARY KEY ({$row['column_name']})";
		}
		$db->sql_freeresult($result);

		$sql = "SELECT A.CONSTRAINT_NAME, A.COLUMN_NAME
			FROM USER_CONS_COLUMNS A, USER_CONSTRAINTS B
			WHERE A.CONSTRAINT_NAME = B.CONSTRAINT_NAME
				AND B.CONSTRAINT_TYPE = 'U'
				AND A.TABLE_NAME = '{$table_name}'";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$rows[] = "  CONSTRAINT {$row['constraint_name']} UNIQUE ({$row['column_name']})";
		}
		$db->sql_freeresult($result);

		$sql_data .= implode(",\n", $rows);
		$sql_data .= "\n)\n\\";

		$sql = "SELECT A.REFERENCED_NAME
			FROM USER_DEPENDENCIES A, USER_TRIGGERS B
			WHERE A.REFERENCED_TYPE = 'SEQUENCE'
				AND A.NAME = B.TRIGGER_NAME
				AND B. TABLE_NAME = '{$table_name}'";
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			$sql_data .= "\nCREATE SEQUENCE {$row['referenced_name']}\\\n";
		}
		$db->sql_freeresult($result);

		$sql = "SELECT DESCRIPTION, WHEN_CLAUSE, TRIGGER_BODY
			FROM USER_TRIGGERS
			WHERE TABLE_NAME = '{$table_name}'";
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			$sql_data .= "\nCREATE OR REPLACE TRIGGER {$row['description']}WHEN ({$row['when_clause']})\n{$row['trigger_body']}\\";
		}
		$db->sql_freeresult($result);

		$sql = "SELECT A.INDEX_NAME, B.COLUMN_NAME
			FROM USER_INDEXES A, USER_IND_COLUMNS B
			WHERE A.UNIQUENESS = 'NONUNIQUE'
				AND A.INDEX_NAME = B.INDEX_NAME
				AND B.TABLE_NAME = '{$table_name}'";
		$result = $db->sql_query($sql);

		$index = array();

		while ($row = $db->sql_fetchrow($result))
		{
			$index[$row['index_name']][] = $row['column_name'];
		}

		foreach ($index as $index_name => $column_names)
		{
			$sql_data .= "\nCREATE INDEX $index_name ON $table_name(" . implode(', ', $column_names) . ")\n\\";
		}
		$db->sql_freeresult($result);
		$this->flush($sql_data);
	}

	function write_data($table_name)
	{
		global $db;
		$ary_type = $ary_name = array();
		
		// Grab all of the data from current table.
		$sql = "SELECT *
			FROM $table_name";
		$result = $db->sql_query($sql);

		$i_num_fields = ocinumcols($result);

		for ($i = 0; $i < $i_num_fields; $i++)
		{
			$ary_type[$i] = ocicolumntype($result, $i + 1);
			$ary_name[$i] = ocicolumnname($result, $i + 1);
		}

		$sql_data = '';

		while ($row = $db->sql_fetchrow($result))
		{
			$schema_vals = $schema_fields = array();

			// Build the SQL statement to recreate the data.
			for ($i = 0; $i < $i_num_fields; $i++)
			{
				$str_val = $row[$ary_name[$i]];

				if (preg_match('#char|text|bool|raw#i', $ary_type[$i]))
				{
					$str_quote = '';
					$str_empty = "''";
					$str_val = sanitize_data_oracle($str_val);
				}
				else if (preg_match('#date|timestamp#i', $ary_type[$i]))
				{
					if (empty($str_val))
					{
						$str_quote = '';
					}
					else
					{
						$str_quote = "'";
					}
				}
				else
				{
					$str_quote = '';
					$str_empty = 'NULL';
				}

				if (empty($str_val) && $str_val !== '0')
				{
					$str_val = $str_empty;
				}

				$schema_vals[$i] = $str_quote . $str_val . $str_quote;
				$schema_fields[$i] = '"' . $ary_name[$i] . "'";
			}

			// Take the ordered fields and their associated data and build it
			// into a valid sql statement to recreate that field in the data.
			$sql_data = "INSERT INTO $table_name (" . implode(', ', $schema_fields) . ') VALUES (' . implode(', ', $schema_vals) . ");\n";

			$this->flush($sql_data);
		}
		$db->sql_freeresult($result);
	}

	function write_start($prefix)
	{
		$sql_data = "--\n";
		$sql_data .= "-- phpBB Backup Script\n";
		$sql_data .= "-- Dump of tables for $prefix\n";
		$sql_data .= "-- DATE : " . gmdate("d-m-Y H:i:s", $this->time) . " GMT\n";
		$sql_data .= "--\n";
		$this->flush($sql_data);
	}
}

/**
* @package acp
*/
class firebird_extractor extends base_extractor
{
	function write_start($prefix)
	{
		$sql_data = "--\n";
		$sql_data .= "-- phpBB Backup Script\n";
		$sql_data .= "-- Dump of tables for $prefix\n";
		$sql_data .= "-- DATE : " . gmdate("d-m-Y H:i:s", $this->time) . " GMT\n";
		$sql_data .= "--\n";
		$this->flush($sql_data);
	}

	function write_data($table_name)
	{
		global $db;
		$ary_type = $ary_name = array();
		
		// Grab all of the data from current table.
		$sql = "SELECT *
			FROM $table_name";
		$result = $db->sql_query($sql);

		$i_num_fields = ibase_num_fields($result);

		for ($i = 0; $i < $i_num_fields; $i++)
		{
			$info = ibase_field_info($result, $i);
			$ary_type[$i] = $info['type'];
			$ary_name[$i] = $info['name'];
		}

		while ($row = $db->sql_fetchrow($result))
		{
			$schema_vals = $schema_fields = array();

			// Build the SQL statement to recreate the data.
			for ($i = 0; $i < $i_num_fields; $i++)
			{
				$str_val = $row[strtolower($ary_name[$i])];

				if (preg_match('#char|text|bool|varbinary|blob#i', $ary_type[$i]))
				{
					$str_quote = '';
					$str_empty = "''";
					$str_val = sanitize_data_generic(str_replace("'", "''", $str_val));
				}
				else if (preg_match('#date|timestamp#i', $ary_type[$i]))
				{
					if (empty($str_val))
					{
						$str_quote = '';
					}
					else
					{
						$str_quote = "'";
					}
				}
				else
				{
					$str_quote = '';
					$str_empty = 'NULL';
				}

				if (empty($str_val) && $str_val !== '0')
				{
					$str_val = $str_empty;
				}

				$schema_vals[$i] = $str_quote . $str_val . $str_quote;
				$schema_fields[$i] = '"' . $ary_name[$i] . '"';
			}

			// Take the ordered fields and their associated data and build it
			// into a valid sql statement to recreate that field in the data.
			$sql_data = "INSERT INTO $table_name (" . implode(', ', $schema_fields) . ') VALUES (' . implode(', ', $schema_vals) . ");\n";

			$this->flush($sql_data);
		}
		$db->sql_freeresult($result);
	}

	function write_table($table_name)
	{
		global $db;

		$sql_data = '-- Table: ' . $table_name . "\n";
		$sql_data .= "DROP TABLE $table_name;\n";

		$data_types = array(7 => 'SMALLINT', 8 => 'INTEGER', 10 => 'FLOAT', 12 => 'DATE', 13 => 'TIME', 14 => 'CHARACTER', 27 => 'DOUBLE PRECISION', 35 => 'TIMESTAMP', 37 => 'VARCHAR', 40 => 'CSTRING', 261 => 'BLOB', 701 => 'DECIMAL', 702 => 'NUMERIC');

		$sql_data .= "\nCREATE TABLE $table_name (\n";

		$sql = 'SELECT DISTINCT R.RDB$FIELD_NAME as FNAME, R.RDB$NULL_FLAG as NFLAG, R.RDB$DEFAULT_SOURCE as DSOURCE, F.RDB$FIELD_TYPE as FTYPE, F.RDB$FIELD_SUB_TYPE as STYPE, F.RDB$FIELD_LENGTH as FLEN
			FROM RDB$RELATION_FIELDS R
			JOIN RDB$FIELDS F ON R.RDB$FIELD_SOURCE=F.RDB$FIELD_NAME
			LEFT JOIN RDB$FIELD_DIMENSIONS D ON R.RDB$FIELD_SOURCE = D.RDB$FIELD_NAME
			WHERE F.RDB$SYSTEM_FLAG = 0
				AND R.RDB$RELATION_NAME = \''. $table_name . '\'
			ORDER BY R.RDB$FIELD_POSITION';
		$result = $db->sql_query($sql);

		$rows = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$line = "\t" . '"' . $row['fname'] . '" ' . $data_types[$row['ftype']];

			if ($row['ftype'] == 261 && $row['stype'] == 1)
			{
				$line .= ' SUB_TYPE TEXT';
			}

			if ($row['ftype'] == 37 || $row['ftype'] == 14)
			{
				$line .= ' (' . $row['flen'] . ')';
			}

			if (!empty($row['dsource']))
			{
				$line .= ' ' . $row['dsource'];
			}

			if (!empty($row['nflag']))
			{
				$line .= ' NOT NULL';
			}
			$rows[] = $line;
		}
		$db->sql_freeresult($result);

		$sql_data .= implode(",\n", $rows);
		$sql_data .= "\n);\n";
		$keys = array();

		$sql = 'SELECT I.RDB$FIELD_NAME as NAME
			FROM RDB$RELATION_CONSTRAINTS RC, RDB$INDEX_SEGMENTS I, RDB$INDICES IDX
			WHERE (I.RDB$INDEX_NAME = RC.RDB$INDEX_NAME)
				AND (IDX.RDB$INDEX_NAME = RC.RDB$INDEX_NAME)
				AND (RC.RDB$RELATION_NAME = \''. $table_name . '\')
			ORDER BY I.RDB$FIELD_POSITION';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$keys[] = $row['name'];
		}

		if (sizeof($keys))
		{
			$sql_data .= "\nALTER TABLE $table_name ADD PRIMARY KEY (" . implode(', ', $keys) . ');';
		}

		$db->sql_freeresult($result);

		$sql = 'SELECT I.RDB$INDEX_NAME as INAME, I.RDB$UNIQUE_FLAG as UFLAG, S.RDB$FIELD_NAME as FNAME
			FROM RDB$INDICES I JOIN RDB$INDEX_SEGMENTS S ON S.RDB$INDEX_NAME=I.RDB$INDEX_NAME
			WHERE (I.RDB$SYSTEM_FLAG IS NULL  OR  I.RDB$SYSTEM_FLAG=0)
				AND I.RDB$FOREIGN_KEY IS NULL
				AND I.RDB$RELATION_NAME = \''. $table_name . '\'
				AND I.RDB$INDEX_NAME NOT STARTING WITH \'RDB$\'
			ORDER BY S.RDB$FIELD_POSITION';
		$result = $db->sql_query($sql);

		$index = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$index[$row['iname']]['unique'] = !empty($row['uflag']);
			$index[$row['iname']]['values'][] = $row['fname'];
		}

		foreach ($index as $index_name => $data)
		{
			$sql_data .= "\nCREATE ";
			if ($data['unique'])
			{
				$sql_data .= 'UNIQUE ';
			}
			$sql_data .= "INDEX $index_name ON $table_name(" . implode(', ', $data['values']) . ");";
		}
		$sql_data .= "\n";

		$db->sql_freeresult($result);

		$sql = 'SELECT D1.RDB$DEPENDENT_NAME as DNAME, D1.RDB$FIELD_NAME as FNAME, D1.RDB$DEPENDENT_TYPE, R1.RDB$RELATION_NAME
			FROM RDB$DEPENDENCIES D1
			LEFT JOIN RDB$RELATIONS R1 ON ((D1.RDB$DEPENDENT_NAME = R1.RDB$RELATION_NAME) AND (NOT (R1.RDB$VIEW_BLR IS NULL)))
			WHERE (D1.RDB$DEPENDED_ON_TYPE = 0)
				AND (D1.RDB$DEPENDENT_TYPE <> 3)
				AND (D1.RDB$DEPENDED_ON_NAME = \'' . $table_name . '\')
			UNION SELECT DISTINCT F2.RDB$RELATION_NAME, D2.RDB$FIELD_NAME, D2.RDB$DEPENDENT_TYPE, R2.RDB$RELATION_NAME FROM RDB$DEPENDENCIES D2, RDB$RELATION_FIELDS F2
			LEFT JOIN RDB$RELATIONS R2 ON ((F2.RDB$RELATION_NAME = R2.RDB$RELATION_NAME) AND (NOT (R2.RDB$VIEW_BLR IS NULL)))
			WHERE (D2.RDB$DEPENDENT_TYPE = 3)
				AND (D2.RDB$DEPENDENT_NAME = F2.RDB$FIELD_SOURCE)
				AND (D2.RDB$DEPENDED_ON_NAME = \'' . $table_name . '\')
			ORDER BY 1, 2';
		$result = $db->sql_query($sql);
		while ($row = $db->sql_fetchrow($result))
		{
			$sql = 'SELECT T1.RDB$DEPENDED_ON_NAME as GEN, T1.RDB$FIELD_NAME, T1.RDB$DEPENDED_ON_TYPE
				FROM RDB$DEPENDENCIES T1
				WHERE (T1.RDB$DEPENDENT_NAME = \'' . $row['dname'] . '\')
					AND (T1.RDB$DEPENDENT_TYPE = 2 AND T1.RDB$DEPENDED_ON_TYPE = 14)
				UNION ALL SELECT DISTINCT D.RDB$DEPENDED_ON_NAME, D.RDB$FIELD_NAME, D.RDB$DEPENDED_ON_TYPE
				FROM RDB$DEPENDENCIES D, RDB$RELATION_FIELDS F
				WHERE (D.RDB$DEPENDENT_TYPE = 3)
					AND (D.RDB$DEPENDENT_NAME = F.RDB$FIELD_SOURCE)
					AND (F.RDB$RELATION_NAME = \'' . $row['dname'] . '\')
				ORDER BY 1,2';
			$result2 = $db->sql_query($sql);
			$row2 = $db->sql_fetchrow($result2);
			$db->sql_freeresult($result2);
			$gen_name = $row2['gen'];

			$sql_data .= "\nDROP GENERATOR " . $gen_name . ";";
			$sql_data .= "\nSET TERM ^ ;";
			$sql_data .= "\nCREATE GENERATOR " . $gen_name . "^";
			$sql_data .= "\nSET GENERATOR  " . $gen_name . " TO 0^\n";
			$sql_data .= "\nCREATE TRIGGER {$row['dname']} FOR $table_name";
			$sql_data .= "\nBEFORE INSERT\nAS\nBEGIN";
			$sql_data .= "\n  NEW.{$row['fname']} = GEN_ID(" . $gen_name . ", 1);";
			$sql_data .= "\nEND^\n";
			$sql_data .= "\nSET TERM ; ^\n";
		}

		$this->flush($sql_data);

		$db->sql_freeresult($result);
	}
}

// get how much space we allow for a chunk of data, very similar to phpMyAdmin's way of doing things ;-) (hey, we only do this for MySQL anyway :P)
function get_usable_memory()
{
	$val = trim(@ini_get('memory_limit'));

	if (preg_match('/(\\d+)([mkg]?)/i', $val, $regs))
	{
		$memory_limit = (int) $regs[1];
		switch ($regs[2])
		{

			case 'k':
			case 'K':
				$memory_limit *= 1024;
			break;

			case 'm':
			case 'M':
				$memory_limit *= 1048576;
			break;

			case 'g':
			case 'G':
				$memory_limit *= 1073741824;
			break;
		}

		// how much memory PHP requires at the start of export (it is really a little less)
		if ($memory_limit > 6100000)
		{
			$memory_limit -= 6100000;
		}

		// allow us to consume half of the total memory available
		$memory_limit /= 2;
	}
	else
	{
		// set the buffer to 1M if we have no clue how much memory PHP will give us :P
		$memory_limit = 1048576;
	}

	return $memory_limit;
}

function sanitize_data_mssql($text)
{
	$data = preg_split('/[\n\t\r\b\f]/', $text);
	preg_match_all('/[\n\t\r\b\f]/', $text, $matches);

	$val = array();

	foreach ($data as $value)
	{
		if (strlen($value))
		{
			$val[] = "'" . $value . "'";
		}
		if (sizeof($matches[0]))
		{
			$val[] = 'char(' . ord(array_shift($matches[0])) . ')';
		}
	}

	return implode('+', $val);
}

function sanitize_data_oracle($text)
{
	$data = preg_split('/[\0\n\t\r\b\f\'"\\\]/', $text);
	preg_match_all('/[\0\n\t\r\b\f\'"\\\]/', $text, $matches);

	$val = array();

	foreach ($data as $value)
	{
		if (strlen($value))
		{
			$val[] = "'" . $value . "'";
		}
		if (sizeof($matches[0]))
		{
			$val[] = 'chr(' . ord(array_shift($matches[0])) . ')';
		}
	}

	return implode('||', $val);
}

function sanitize_data_generic($text)
{
	$data = preg_split('/[\n\t\r\b\f]/', $text);
	preg_match_all('/[\n\t\r\b\f]/', $text, $matches);

	$val = array();

	foreach ($data as $value)
	{
		if (strlen($value))
		{
			$val[] = "'" . $value . "'";
		}
		if (sizeof($matches[0]))
		{
			$val[] = "'" . array_shift($matches[0]) . "'";
		}
	}

	return implode('||', $val);
}

// modified from PHP.net
function fgetd(&$fp, $delim, $read, $seek, $eof, $buffer = 8192)
{
	$record = '';
	$delim_len = strlen($delim);
	
	while (!$eof($fp))
	{
		$pos = strpos($record, $delim);
		if ($pos === false)
		{
			$record .= $read($fp, $buffer);
			if ($eof($fp) && ($pos = strpos($record, $delim)) !== false)
			{
				$seek($fp, $pos + $delim_len - strlen($record), SEEK_CUR);
				return substr($record, 0, $pos);
			}
		}
		else
		{
			$seek($fp, $pos + $delim_len - strlen($record), SEEK_CUR);
			return substr($record, 0, $pos);
		}
	}

	return false;
}

function fgetd_seekless(&$fp, $delim, $read, $seek, $eof, $buffer = 8192)
{
	static $array = array();
	static $record = '';

	if (!sizeof($array))
	{
		while (!$eof($fp))
		{
			if (strpos($record, $delim) !== false)
			{
				$array = explode($delim, $record);
				$record = array_pop($array);
				break;
			}
			else
			{
				$record .= $read($fp, $buffer);
			}
		}
		if ($eof($fp) && strpos($record, $delim) !== false)
		{
			$array = explode($delim, $record);
			$record = array_pop($array);
		}
	}

	if (sizeof($array))
	{
		return array_shift($array);
	}

	return false;
}

?>