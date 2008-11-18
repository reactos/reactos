<?php
/**
*
* @package install
* @version $Id: install_convert.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2006 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
*/

if (!defined('IN_INSTALL'))
{
	// Someone has tried to access the file direct. This is not a good idea, so exit
	exit;
}

if (!empty($setmodules))
{
	$module[] = array(
		'module_type'		=> 'install',
		'module_title'		=> 'CONVERT',
		'module_filename'	=> substr(basename(__FILE__), 0, -strlen($phpEx)-1),
		'module_order'		=> 20,
		'module_subs'		=> '',
		'module_stages'		=> array('INTRO', 'SETTINGS', 'IN_PROGRESS', 'FINAL'),
		'module_reqs'		=> ''
	);
}

/**
* Class holding all convertor-specific details.
* @package install
*/
class convert
{
	var $options = array();

	var $convertor_tag = '';
	var $src_dbms = '';
	var $src_dbhost = '';
	var $src_dbport = '';
	var $src_dbuser = '';
	var $src_dbpasswd = '';
	var $src_dbname = '';
	var $src_table_prefix = '';

	var $convertor_data = array();
	var $tables = array();
	var $config_schema = array();
	var $convertor = array();
	var $src_truncate_statement = 'DELETE FROM ';
	var $truncate_statement = 'DELETE FROM ';

	var $fulltext_search;

	// Batch size, can be adjusted by the conversion file
	// For big boards a value of 6000 seems to be optimal
	var $batch_size = 2000;
	// Number of rows to be inserted at once (extended insert) if supported
	// For installations having enough memory a value of 60 may be good.
	var $num_wait_rows = 20;

	// Mysqls internal recoding engine messing up with our (better) functions? We at least support more encodings than mysql so should use it in favor.
	var $mysql_convert = false;

	var $p_master;

	function convert(&$p_master)
	{
		$this->p_master = &$p_master;
	}
}

/**
* Convert class for conversions
* @package install
*/
class install_convert extends module
{
	/**
	* Variables used while converting, they are accessible from the global variable $convert
	*/
	function install_convert(&$p_master)
	{
		$this->p_master = &$p_master;
	}

	function main($mode, $sub)
	{
		global $lang, $template, $phpbb_root_path, $phpEx, $cache, $config, $language, $table_prefix;
		global $convert;

		$this->tpl_name = 'install_convert';
		$this->mode = $mode;

		$convert = new convert($this->p_master);

		switch ($sub)
		{
			case 'intro':
				// Try opening config file
				// @todo If phpBB is not installed, we need to do a cut-down installation here
				// For now, we redirect to the installation script instead
				if (@file_exists($phpbb_root_path . 'config.' . $phpEx))
				{
					include($phpbb_root_path . 'config.' . $phpEx);
				}

				if (!defined('PHPBB_INSTALLED'))
				{
					$template->assign_vars(array(
						'S_NOT_INSTALLED'		=> true,
						'TITLE'					=> $lang['BOARD_NOT_INSTALLED'],
						'BODY'					=> sprintf($lang['BOARD_NOT_INSTALLED_EXPLAIN'], append_sid($phpbb_root_path . 'install/index.' . $phpEx, 'mode=install&amp;language=' . $language)),
					));

					return;
				}

				require($phpbb_root_path . 'config.' . $phpEx);
				require($phpbb_root_path . 'includes/constants.' . $phpEx);
				require($phpbb_root_path . 'includes/db/' . $dbms . '.' . $phpEx);
				require($phpbb_root_path . 'includes/functions_convert.' . $phpEx);

				$db = new $sql_db();
				$db->sql_connect($dbhost, $dbuser, $dbpasswd, $dbname, $dbport, false, true);
				unset($dbpasswd);

				// We need to fill the config to let internal functions correctly work
				$sql = 'SELECT *
					FROM ' . CONFIG_TABLE;
				$result = $db->sql_query($sql);

				$config = array();
				while ($row = $db->sql_fetchrow($result))
				{
					$config[$row['config_name']] = $row['config_value'];
				}
				$db->sql_freeresult($result);

				// Detect if there is already a conversion in progress at this point and offer to resume
				// It's quite possible that the user will get disconnected during a large conversion so they need to be able to resume it
				$new_conversion = request_var('new_conv', 0);

				if ($new_conversion)
				{
					$config['convert_progress'] = '';
					$config['convert_db_server'] = '';
					$config['convert_db_user'] = '';
					$db->sql_query('DELETE FROM ' . CONFIG_TABLE . "
						WHERE config_name = 'convert_progress'
							OR config_name = 'convert_db_server'
							OR config_name = 'convert_db_user'"
					);
				}

				// Let's see if there is a conversion in the works...
				$options = array();
				if (!empty($config['convert_progress']) && !empty($config['convert_db_server']) && !empty($config['convert_db_user']) && !empty($config['convert_options']))
				{
					$options = unserialize($config['convert_progress']);
					$options = array_merge($options, unserialize($config['convert_db_server']), unserialize($config['convert_db_user']), unserialize($config['convert_options']));
				}

				// This information should have already been checked once, but do it again for safety
				if (!empty($options) && !empty($options['tag']) &&
					isset($options['dbms']) &&
					isset($options['dbhost']) &&
					isset($options['dbport']) &&
					isset($options['dbuser']) &&
					isset($options['dbpasswd']) &&
					isset($options['dbname']) &&
					isset($options['table_prefix']))
				{
					$this->page_title = $lang['CONTINUE_CONVERT'];

					$template->assign_vars(array(
						'TITLE'			=> $lang['CONTINUE_CONVERT'],
						'BODY'			=> $lang['CONTINUE_CONVERT_BODY'],
						'L_NEW'			=> $lang['CONVERT_NEW_CONVERSION'],
						'L_CONTINUE'	=> $lang['CONTINUE_OLD_CONVERSION'],
						'S_CONTINUE'	=> true,

						'U_NEW_ACTION'		=> $this->p_master->module_url . "?mode={$this->mode}&amp;sub=intro&amp;new_conv=1&amp;language=$language",
						'U_CONTINUE_ACTION'	=> $this->p_master->module_url . "?mode={$this->mode}&amp;sub=in_progress&amp;tag={$options['tag']}{$options['step']}&amp;language=$language",
					));

					return;
				}

				$this->list_convertors($sub);

			break;

			case 'settings':
				$this->get_convert_settings($sub);
			break;

			case 'in_progress':
				$this->convert_data($sub);
			break;

			case 'final':
				$this->page_title = $lang['CONVERT_COMPLETE'];

				$template->assign_vars(array(
					'TITLE'		=> $lang['CONVERT_COMPLETE'],
					'BODY'		=> $lang['CONVERT_COMPLETE_EXPLAIN'],
				));

				// If we reached this step (conversion completed) we want to purge the cache and log the user out.
				// This is for making sure the session get not screwed due to the 3.0.x users table being completely new.
				$cache->purge();

				require($phpbb_root_path . 'config.' . $phpEx);
				require($phpbb_root_path . 'includes/constants.' . $phpEx);
				require($phpbb_root_path . 'includes/db/' . $dbms . '.' . $phpEx);
				require($phpbb_root_path . 'includes/functions_convert.' . $phpEx);

				$db = new $sql_db();
				$db->sql_connect($dbhost, $dbuser, $dbpasswd, $dbname, $dbport, false, true);
				unset($dbpasswd);

				$sql = 'SELECT config_value
					FROM ' . CONFIG_TABLE . '
					WHERE config_name = \'search_type\'';
				$result = $db->sql_query($sql);

				if ($db->sql_fetchfield('config_value') != 'fulltext_mysql')
				{
					$template->assign_vars(array(
						'S_ERROR_BOX'	=> true,
						'ERROR_TITLE'	=> $lang['SEARCH_INDEX_UNCONVERTED'],
						'ERROR_MSG'		=> $lang['SEARCH_INDEX_UNCONVERTED_EXPLAIN'],
					));
				}

				switch ($db->sql_layer)
				{
					case 'sqlite':
					case 'firebird':
						$db->sql_query('DELETE FROM ' . SESSIONS_KEYS_TABLE);
						$db->sql_query('DELETE FROM ' . SESSIONS_TABLE);
					break;

					default:
						$db->sql_query('TRUNCATE TABLE ' . SESSIONS_KEYS_TABLE);
						$db->sql_query('TRUNCATE TABLE ' . SESSIONS_TABLE);
					break;
				}

			break;
		}
	}

	/**
	* Generate a list of all available conversion modules
	*/
	function list_convertors($sub)
	{
		global $lang, $language, $template, $phpbb_root_path, $phpEx;

		$this->page_title = $lang['SUB_INTRO'];

		$template->assign_vars(array(
			'TITLE'		=> $lang['CONVERT_INTRO'],
			'BODY'		=> $lang['CONVERT_INTRO_BODY'],

			'L_AUTHOR'					=> $lang['AUTHOR'],
			'L_AVAILABLE_CONVERTORS'	=> $lang['AVAILABLE_CONVERTORS'],
			'L_CONVERT'					=> $lang['CONVERT'],
			'L_NO_CONVERTORS'			=> $lang['NO_CONVERTORS'],
			'L_OPTIONS'					=> $lang['CONVERT_OPTIONS'],
			'L_SOFTWARE'				=> $lang['SOFTWARE'],
			'L_VERSION'					=> $lang['VERSION'],

			'S_LIST'	=> true,
		));

		$convertors = $sort = array();
		$get_info = true;

		$handle = @opendir('./convertors/');

		if (!$handle)
		{
			$this->error('Unable to access the convertors directory', __LINE__, __FILE__);
		}

		while ($entry = readdir($handle))
		{
			if (preg_match('/^convert_([a-z0-9_]+).' . $phpEx . '$/i', $entry, $m))
			{
				include('./convertors/' . $entry);
				if (isset($convertor_data))
				{
					$sort[strtolower($convertor_data['forum_name'])] = sizeof($convertors);
					
					$convertors[] = array(
						'tag'			=>	$m[1],
						'forum_name'	=>	$convertor_data['forum_name'],
						'version'		=>	$convertor_data['version'],
						'dbms'			=>	$convertor_data['dbms'],
						'dbhost'		=>	$convertor_data['dbhost'],
						'dbport'		=>	$convertor_data['dbport'],
						'dbuser'		=>	$convertor_data['dbuser'],
						'dbpasswd'		=>	$convertor_data['dbpasswd'],
						'dbname'		=>	$convertor_data['dbname'],
						'table_prefix'	=>	$convertor_data['table_prefix'],
						'author'		=>	$convertor_data['author']
					);
				}
				unset($convertor_data);
			}
		}
		closedir($handle);

		@ksort($sort);

		foreach ($sort as $void => $index)
		{
			$template->assign_block_vars('convertors', array(
				'AUTHOR'	=> $convertors[$index]['author'],
				'SOFTWARE'	=> $convertors[$index]['forum_name'],
				'VERSION'	=> $convertors[$index]['version'],

				'U_CONVERT'	=> $this->p_master->module_url . "?mode={$this->mode}&amp;language=$language&amp;sub=settings&amp;tag=" . $convertors[$index]['tag'],
			));
		}
	}

	/**
	*/
	function get_convert_settings($sub)
	{
		global $lang, $language, $template, $db, $phpbb_root_path, $phpEx, $config, $cache;

		require($phpbb_root_path . 'config.' . $phpEx);
		require($phpbb_root_path . 'includes/constants.' . $phpEx);
		require($phpbb_root_path . 'includes/db/' . $dbms . '.' . $phpEx);
		require($phpbb_root_path . 'includes/functions_convert.' . $phpEx);

		$db = new $sql_db();
		$db->sql_connect($dbhost, $dbuser, $dbpasswd, $dbname, $dbport, false, true);
		unset($dbpasswd);

		$this->page_title = $lang['STAGE_SETTINGS'];

		// We need to fill the config to let internal functions correctly work
		$sql = 'SELECT *
			FROM ' . CONFIG_TABLE;
		$result = $db->sql_query($sql);

		$config = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$config[$row['config_name']] = $row['config_value'];
		}
		$db->sql_freeresult($result);

		$convertor_tag = request_var('tag', '');

		if (empty($convertor_tag))
		{
			$this->p_master->error($lang['NO_CONVERT_SPECIFIED'], __LINE__, __FILE__);
		}
		$get_info = true;

		// check security implications of direct inclusion
		$convertor_tag = basename($convertor_tag);
		if (!file_exists('./convertors/convert_' . $convertor_tag . '.' . $phpEx))
		{
			$this->p_master->error($lang['CONVERT_NOT_EXIST'], __LINE__, __FILE__);
		}

		include('./convertors/convert_' . $convertor_tag . '.' . $phpEx);

		// The test_file is a file that should be present in the location of the old board.
		if (!isset($test_file))
		{
			$this->p_master->error($lang['DEV_NO_TEST_FILE'], __LINE__, __FILE__);
		}

		$submit = (isset($_POST['submit'])) ? true : false;

		$src_dbms			= request_var('src_dbms', $convertor_data['dbms']);
		$src_dbhost			= request_var('src_dbhost', $convertor_data['dbhost']);
		$src_dbport			= request_var('src_dbport', $convertor_data['dbport']);
		$src_dbuser			= request_var('src_dbuser', $convertor_data['dbuser']);
		$src_dbpasswd		= request_var('src_dbpasswd', $convertor_data['dbpasswd']);
		$src_dbname			= request_var('src_dbname', $convertor_data['dbname']);
		$src_table_prefix	= request_var('src_table_prefix', $convertor_data['table_prefix']);
		$forum_path			= request_var('forum_path', $convertor_data['forum_path']);
		$refresh			= request_var('refresh', 1);

		// Default URL of the old board
		// @todo Are we going to use this for attempting to convert URL references in posts, or should we remove it?
		//		-> We should convert old urls to the new relative urls format
		// $src_url = request_var('src_url', 'Not in use at the moment');

		// strip trailing slash from old forum path
		$forum_path = (strlen($forum_path) && $forum_path[strlen($forum_path) - 1] == '/') ? substr($forum_path, 0, -1) : $forum_path;

		$error = array();
		if ($submit)
		{
			if (!@file_exists('./../' . $forum_path . '/' . $test_file))
			{
				$error[] = sprintf($lang['COULD_NOT_FIND_PATH'], $forum_path);
			}

			$connect_test = false;
			$available_dbms = get_available_dbms(false, true, true);

			if (!isset($available_dbms[$src_dbms]) || !$available_dbms[$src_dbms]['AVAILABLE'])
			{
				$error['db'][] = $lang['INST_ERR_NO_DB'];
				$connect_test = false;
			}
			else
			{
				$connect_test = connect_check_db(true, $error, $available_dbms[$src_dbms], $src_table_prefix, $src_dbhost, $src_dbuser, htmlspecialchars_decode($src_dbpasswd), $src_dbname, $src_dbport, true, ($src_dbms == $dbms) ? false : true, false);
			}

			// The forum prefix of the old and the new forum can only be the same if two different databases are used.
			if ($src_table_prefix == $table_prefix && $src_dbms == $dbms && $src_dbhost == $dbhost && $src_dbport == $dbport && $src_dbname == $dbname)
			{
				$error[] = sprintf($lang['TABLE_PREFIX_SAME'], $src_table_prefix);
			}

			// Check table prefix
			if (!sizeof($error))
			{
				// initiate database connection to old db if old and new db differ
				global $src_db, $same_db;
				$src_db = $same_db = false;

				if ($src_dbms != $dbms || $src_dbhost != $dbhost || $src_dbport != $dbport || $src_dbname != $dbname || $src_dbuser != $dbuser)
				{
					$sql_db = 'dbal_' . $src_dbms;
					$src_db = new $sql_db();
					$src_db->sql_connect($src_dbhost, $src_dbuser, htmlspecialchars_decode($src_dbpasswd), $src_dbname, $src_dbport, false, true);
					$same_db = false;
				}
				else
				{
					$src_db = $db;
					$same_db = true;
				}

				$src_db->sql_return_on_error(true);
				$db->sql_return_on_error(true);

				// Try to select one row from the first table to see if the prefix is OK
				$result = $src_db->sql_query_limit('SELECT * FROM ' . $src_table_prefix . $tables[0], 1);

				if (!$result)
				{
					$prefixes = array();

					$tables_existing = get_tables($src_db);
					$tables_existing = array_map('strtolower', $tables_existing);
					foreach ($tables_existing as $table_name)
					{
						compare_table($tables, $table_name, $prefixes);
					}
					unset($tables_existing);

					foreach ($prefixes as $prefix => $count)
					{
						if ($count >= sizeof($tables))
						{
							$possible_prefix = $prefix;
							break;
						}
					}

					$msg = '';
					if (!empty($convertor_data['table_prefix']))
					{
						$msg .= sprintf($lang['DEFAULT_PREFIX_IS'], $convertor_data['forum_name'], $convertor_data['table_prefix']);
					}

					if (!empty($possible_prefix))
					{
						$msg .= '<br />';
						$msg .= ($possible_prefix == '*') ? $lang['BLANK_PREFIX_FOUND'] : sprintf($lang['PREFIX_FOUND'], $possible_prefix);
						$src_table_prefix = ($possible_prefix == '*') ? '' : $possible_prefix;
					}

					$error[] = $msg;
				}
				$src_db->sql_freeresult($result);
				$src_db->sql_return_on_error(false);
			}

			if (!sizeof($error))
			{
				// Save convertor Status
				set_config('convert_progress', serialize(array(
					'step'			=> '',
					'table_prefix'	=> $src_table_prefix,
					'tag'			=> $convertor_tag,
				)), true);
				set_config('convert_db_server', serialize(array(
					'dbms'			=> $src_dbms,
					'dbhost'		=> $src_dbhost,
					'dbport'		=> $src_dbport,
					'dbname'		=> $src_dbname,
				)), true);
				set_config('convert_db_user', serialize(array(
					'dbuser'		=> $src_dbuser,
					'dbpasswd'		=> $src_dbpasswd,
				)), true);

				// Save options
				set_config('convert_options', serialize(array('forum_path' => './../' . $forum_path, 'refresh' => $refresh)), true);

				$template->assign_block_vars('checks', array(
					'TITLE'		=> $lang['VERIFY_OPTIONS'],
					'RESULT'	=> $lang['CONVERT_SETTINGS_VERIFIED'],
				));

				$template->assign_vars(array(
					'L_SUBMIT'	=> $lang['BEGIN_CONVERT'],
//					'S_HIDDEN'	=> $s_hidden_fields,
					'U_ACTION'	=> $this->p_master->module_url . "?mode={$this->mode}&amp;sub=in_progress&amp;tag=$convertor_tag&amp;language=$language",
				));

				return;
			}
			else
			{
				$template->assign_block_vars('checks', array(
					'TITLE'		=> $lang['VERIFY_OPTIONS'],
					'RESULT'	=> '<b style="color:red">' . implode('<br />', $error) . '</b>',
				));
			}
		} // end submit

		foreach ($this->convert_options as $config_key => $vars)
		{
			if (!is_array($vars) && strpos($config_key, 'legend') === false)
			{
				continue;
			}

			if (strpos($config_key, 'legend') !== false)
			{
				$template->assign_block_vars('options', array(
					'S_LEGEND'		=> true,
					'LEGEND'		=> $lang[$vars])
				);

				continue;
			}

			$options = isset($vars['options']) ? $vars['options'] : '';

			$template->assign_block_vars('options', array(
				'KEY'			=> $config_key,
				'TITLE'			=> $lang[$vars['lang']],
				'S_EXPLAIN'		=> $vars['explain'],
				'S_LEGEND'		=> false,
				'TITLE_EXPLAIN'	=> ($vars['explain']) ? $lang[$vars['lang'] . '_EXPLAIN'] : '',
				'CONTENT'		=> $this->p_master->input_field($config_key, $vars['type'], $$config_key, $options),
				)
			);
		}

		$template->assign_vars(array(
			'TITLE'		=> $lang['STAGE_SETTINGS'],
			'BODY'		=> $lang['CONV_OPTIONS_BODY'],
			'L_SUBMIT'	=> $lang['BEGIN_CONVERT'],
			'U_ACTION'	=> $this->p_master->module_url . "?mode={$this->mode}&amp;sub=settings&amp;tag=$convertor_tag&amp;language=$language",
		));
	}

	/**
	* The function which does the actual work (or dispatches it to the relevant places)
	*/
	function convert_data($sub)
	{
		global $template, $user, $phpbb_root_path, $phpEx, $db, $lang, $config, $cache;
		global $convert, $convert_row, $message_parser, $skip_rows;

		require($phpbb_root_path . 'config.' . $phpEx);
		require($phpbb_root_path . 'includes/constants.' . $phpEx);
		require($phpbb_root_path . 'includes/db/' . $dbms . '.' . $phpEx);
		require($phpbb_root_path . 'includes/functions_convert.' . $phpEx);

		$db = new $sql_db();
		$db->sql_connect($dbhost, $dbuser, $dbpasswd, $dbname, $dbport, false, true);
		unset($dbpasswd);

		$sql = 'SELECT *
			FROM ' . CONFIG_TABLE;
		$result = $db->sql_query($sql);

		$config = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$config[$row['config_name']] = $row['config_value'];
		}
		$db->sql_freeresult($result);

		// Override a couple of config variables for the duration
		$config['max_quote_depth'] = 0;

		// @todo Need to confirm that max post length in source is <= max post length in destination or there may be interesting formatting issues
		$config['max_post_chars'] = 0;

		// Set up a user as well. We _should_ have enough of a database here at this point to do this
		// and it helps for any core code we call
		$user->session_begin();
		$user->page = $user->extract_current_page($phpbb_root_path);

		// This is a little bit of a fudge, but it allows the language entries to be available to the
		// core code without us loading them again
		$user->lang = &$lang;

		$this->page_title = $user->lang['STAGE_IN_PROGRESS'];

		$convert->options = array();
		if (isset($config['convert_progress']))
		{
			$convert->options = unserialize($config['convert_progress']);
			$convert->options = array_merge($convert->options, unserialize($config['convert_db_server']), unserialize($config['convert_db_user']), unserialize($config['convert_options']));
		}

		// This information should have already been checked once, but do it again for safety
		if (empty($convert->options) || empty($convert->options['tag']) ||
			!isset($convert->options['dbms']) ||
			!isset($convert->options['dbhost']) ||
			!isset($convert->options['dbport']) ||
			!isset($convert->options['dbuser']) ||
			!isset($convert->options['dbpasswd']) ||
			!isset($convert->options['dbname']) ||
			!isset($convert->options['table_prefix']))
		{
			$this->p_master->error($user->lang['NO_CONVERT_SPECIFIED'], __LINE__, __FILE__);
		}

		// Make some short variables accessible, for easier referencing
		$convert->convertor_tag = basename($convert->options['tag']);
		$convert->src_dbms = $convert->options['dbms'];
		$convert->src_dbhost = $convert->options['dbhost'];
		$convert->src_dbport = $convert->options['dbport'];
		$convert->src_dbuser = $convert->options['dbuser'];
		$convert->src_dbpasswd = $convert->options['dbpasswd'];
		$convert->src_dbname = $convert->options['dbname'];
		$convert->src_table_prefix = $convert->options['table_prefix'];

		// initiate database connection to old db if old and new db differ
		global $src_db, $same_db;
		$src_db = $same_db = null;
		if ($convert->src_dbms != $dbms || $convert->src_dbhost != $dbhost || $convert->src_dbport != $dbport || $convert->src_dbname != $dbname || $convert->src_dbuser != $dbuser)
		{
			if ($convert->src_dbms != $dbms)
			{
				require($phpbb_root_path . 'includes/db/' . $convert->src_dbms . '.' . $phpEx);
			}
			$sql_db = 'dbal_' . $convert->src_dbms;
			$src_db = new $sql_db();
			$src_db->sql_connect($convert->src_dbhost, $convert->src_dbuser, htmlspecialchars_decode($convert->src_dbpasswd), $convert->src_dbname, $convert->src_dbport, false, true);
			$same_db = false;
		}
		else
		{
			$src_db = $db;
			$same_db = true;
		}

		$convert->mysql_convert = false;
		switch ($src_db->sql_layer)
		{
			case 'sqlite':
			case 'firebird':
				$convert->src_truncate_statement = 'DELETE FROM ';
			break;

			// Thanks MySQL, for silently converting...
			case 'mysql':
			case 'mysql4':
				if (version_compare($src_db->mysql_version, '4.1.3', '>='))
				{
					$convert->mysql_convert = true;
				}
				$convert->src_truncate_statement = 'TRUNCATE TABLE ';
			break;

			case 'mysqli':
				$convert->mysql_convert = true;
				$convert->src_truncate_statement = 'TRUNCATE TABLE ';
			break;

			default:
				$convert->src_truncate_statement = 'TRUNCATE TABLE ';
			break;
		}

		if ($convert->mysql_convert && !$same_db)
		{
			$src_db->sql_query("SET NAMES 'binary'");
		}

		switch ($db->sql_layer)
		{
			case 'sqlite':
			case 'firebird':
				$convert->truncate_statement = 'DELETE FROM ';
			break;

			default:
				$convert->truncate_statement = 'TRUNCATE TABLE ';
			break;
		}

		$get_info = false;

		// check security implications of direct inclusion
		if (!file_exists('./convertors/convert_' . $convert->convertor_tag . '.' . $phpEx))
		{
			$this->p_master->error($user->lang['CONVERT_NOT_EXIST'], __LINE__, __FILE__);
		}

		if (file_exists('./convertors/functions_' . $convert->convertor_tag . '.' . $phpEx))
		{
			include('./convertors/functions_' . $convert->convertor_tag . '.' . $phpEx);
		}

		$get_info = true;
		include('./convertors/convert_' . $convert->convertor_tag . '.' . $phpEx);

		// Map some variables...
		$convert->convertor_data = $convertor_data;
		$convert->tables = $tables;
		$convert->config_schema = $config_schema;

		// Now include the real data
		$get_info = false;
		include('./convertors/convert_' . $convert->convertor_tag . '.' . $phpEx);

		$convert->convertor_data = $convertor_data;
		$convert->tables = $tables;
		$convert->config_schema = $config_schema;
		$convert->convertor = $convertor;

		// The test_file is a file that should be present in the location of the old board.
		if (!file_exists($convert->options['forum_path'] . '/' . $test_file))
		{
			$this->p_master->error(sprintf($user->lang['COULD_NOT_FIND_PATH'], $convert->options['forum_path']), __LINE__, __FILE__);
		}

		$search_type = basename(trim($config['search_type']));

		// For conversions we are a bit less strict and set to a search backend we know exist...
		if (!file_exists($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx))
		{
			$search_type = 'fulltext_native';
			set_config('search_type', $search_type);
		}

		if (!file_exists($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx))
		{
			trigger_error('NO_SUCH_SEARCH_MODULE');
		}

		require($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx);

		$error = false;
		$convert->fulltext_search = new $search_type($error);

		if ($error)
		{
			trigger_error($error);
		}

		include($phpbb_root_path . 'includes/message_parser.' . $phpEx);
		$message_parser = new parse_message();

		$jump = request_var('jump', 0);
		$final_jump = request_var('final_jump', 0);
		$sync_batch = request_var('sync_batch', -1);
		$last_statement = request_var('last', 0);

		// We are running sync...
		if ($sync_batch >= 0)
		{
			$this->sync_forums($sync_batch);
			return;
		}

		if ($jump)
		{
			$this->jump($jump, $last_statement);
			return;
		}

		if ($final_jump)
		{
			$this->final_jump($final_jump);
			return;
		}

		$current_table = request_var('current_table', 0);
		$old_current_table = min(-1, $current_table - 1);
		$skip_rows = request_var('skip_rows', 0);

		if (!$current_table && !$skip_rows)
		{
			if (empty($_REQUEST['confirm']))
			{
				// If avatars / ranks / smilies folders are specified make sure they are writable
				$bad_folders = array();

				$local_paths = array(
					'avatar_path'			=> path($config['avatar_path']),
					'avatar_gallery_path'	=> path($config['avatar_gallery_path']),
					'icons_path'			=> path($config['icons_path']),
					'ranks_path'			=> path($config['ranks_path']),
					'smilies_path'			=> path($config['smilies_path'])
				);

				foreach ($local_paths as $folder => $local_path)
				{
					if (isset($convert->convertor[$folder]))
					{
						if (empty($convert->convertor['test_file']))
						{
							// test_file is mandantory at the moment so this should never be reached, but just in case...
							$this->p_master->error($user->lang['DEV_NO_TEST_FILE'], __LINE__, __FILE__);
						}

						if (!$local_path || !@is_writable($phpbb_root_path . $local_path))
						{
							if (!$local_path)
							{
								$bad_folders[] = sprintf($user->lang['CONFIG_PHPBB_EMPTY'], $folder);
							}
							else
							{
								$bad_folders[] = $local_path;
							}
						}
					}
				}

				if (sizeof($bad_folders))
				{
					$msg = (sizeof($bad_folders) == 1) ? $user->lang['MAKE_FOLDER_WRITABLE'] : $user->lang['MAKE_FOLDERS_WRITABLE'];
					sort($bad_folders);
					$this->p_master->error(sprintf($msg, implode('<br />', $bad_folders)), __LINE__, __FILE__, true);

					$template->assign_vars(array(
						'L_SUBMIT'	=> $user->lang['INSTALL_TEST'],
						'U_ACTION'	=> $this->p_master->module_url . "?mode={$this->mode}&amp;sub=in_progress&amp;tag={$convert->convertor_tag}&amp;language=$language",
					));
					return;
				}

				// Grab all the tables used in convertor
				$missing_tables = $tables_list = $aliases = array();

				foreach ($convert->convertor['schema'] as $schema)
				{
					// Skip those not used (because of addons/plugins not detected)
					if (!$schema['target'])
					{
						continue;
					}

					foreach ($schema as $key => $val)
					{
						// we're dealing with an array like:
						// array('forum_status',			'forums.forum_status',				'is_item_locked')
						if (is_int($key) && !empty($val[1]))
						{
							$temp_data = $val[1];
							if (!is_array($temp_data))
							{
								$temp_data = array($temp_data);
							}

							foreach ($temp_data as $val)
							{
								if (preg_match('/([a-z0-9_]+)\.([a-z0-9_]+)\)* ?A?S? ?([a-z0-9_]*?)\.?([a-z0-9_]*)$/i', $val, $m))
								{
									$table = $convert->src_table_prefix . $m[1];
									$tables_list[$table] = $table;
		
									if (!empty($m[3]))
									{
										$aliases[] = $convert->src_table_prefix . $m[3];
									}
								}
							}
						}
						// 'left_join'		=> 'topics LEFT JOIN vote_desc ON topics.topic_id = vote_desc.topic_id AND topics.topic_vote = 1'
						else if ($key == 'left_join')
						{
							// Convert the value if it wasn't an array already.
							if (!is_array($val))
							{
								$val = array($val);
							}

							for ($j = 0; $j < sizeof($val); ++$j)
							{
								if (preg_match('/LEFT JOIN ([a-z0-9_]+) AS ([a-z0-9_]+)/i', $val[$j], $m))
								{
									$table = $convert->src_table_prefix . $m[1];
									$tables_list[$table] = $table;

									if (!empty($m[2]))
									{
										$aliases[] = $convert->src_table_prefix . $m[2];
									}
								}
							}
						}
					}
				}

				// Remove aliased tables from $tables_list
				foreach ($aliases as $alias)
				{
					unset($tables_list[$alias]);
				}

				// Check if the tables that we need exist
				$src_db->sql_return_on_error(true);
				foreach ($tables_list as $table => $null)
				{
					$sql = 'SELECT 1 FROM ' . $table;
					$_result = $src_db->sql_query_limit($sql, 1);

					if (!$_result)
					{
						$missing_tables[] = $table;
					}
					$src_db->sql_freeresult($_result);
				}
				$src_db->sql_return_on_error(false);

				// Throw an error if some tables are missing
				// We used to do some guessing here, but since we have a suggestion of possible values earlier, I don't see it adding anything here to do it again
				
				if (sizeof($missing_tables) == sizeof($tables_list))
				{
					$this->p_master->error($user->lang['NO_TABLES_FOUND'] . ' ' . $user->lang['CHECK_TABLE_PREFIX'], __LINE__, __FILE__);
				}
				else if (sizeof($missing_tables))
				{
					$this->p_master->error(sprintf($user->lang['TABLES_MISSING'], implode(', ', $missing_tables)) . '<br /><br />' . $user->lang['CHECK_TABLE_PREFIX'], __LINE__, __FILE__);
				}

				$url = $this->save_convert_progress('&amp;confirm=1');
				$msg = $user->lang['PRE_CONVERT_COMPLETE'];

				if ($convert->convertor_data['author_notes'])
				{
					$msg .= '</p><p>' . sprintf($user->lang['AUTHOR_NOTES'], $convert->convertor_data['author_notes']);
				}

				$template->assign_vars(array(
					'L_SUBMIT'		=> $user->lang['CONTINUE_CONVERT'],
					'L_MESSAGE'		=> $msg,
					'U_ACTION'		=> $url,
				));

				return;
			} // if (empty($_REQUEST['confirm']))

			$template->assign_block_vars('checks', array(
				'S_LEGEND'		=> true,
				'LEGEND'		=> $user->lang['STARTING_CONVERT'],
			));

			// Convert the config table and load the settings of the old board
			if (!empty($convert->config_schema))
			{
				restore_config($convert->config_schema);

				// Override a couple of config variables for the duration
				$config['max_quote_depth'] = 0;

				// @todo Need to confirm that max post length in source is <= max post length in destination or there may be interesting formatting issues
				$config['max_post_chars'] = 0;
			}

			$template->assign_block_vars('checks', array(
				'TITLE'		=> $user->lang['CONFIG_CONVERT'],
				'RESULT'	=> $user->lang['DONE'],
			));

			// Now process queries and execute functions that have to be executed prior to the conversion
			if (!empty($convert->convertor['execute_first']))
			{
				eval($convert->convertor['execute_first']);
			}

			if (!empty($convert->convertor['query_first']))
			{
				if (!is_array($convert->convertor['query_first']))
				{
					$convert->convertor['query_first'] = array('target', array($convert->convertor['query_first']));
				}
				else if (!is_array($convert->convertor['query_first'][0]))
				{
					$convert->convertor['query_first'] = array(array($convert->convertor['query_first'][0], $convert->convertor['query_first'][1]));
				}

				foreach ($convert->convertor['query_first'] as $query_first)
				{
					if ($query_first[0] == 'src')
					{
						if ($convert->mysql_convert && $same_db)
						{
							$src_db->sql_query("SET NAMES 'binary'");
						}

						$src_db->sql_query($query_first[1]);

						if ($convert->mysql_convert && $same_db)
						{
							$src_db->sql_query("SET NAMES 'utf8'");
						}
					}
					else
					{
						$db->sql_query($query_first[1]);
					}
				}
			}

			$template->assign_block_vars('checks', array(
				'TITLE'		=> $user->lang['PREPROCESS_STEP'],
				'RESULT'	=> $user->lang['DONE'],
			));
		} // if (!$current_table && !$skip_rows)

		$template->assign_block_vars('checks', array(
			'S_LEGEND'		=> true,
			'LEGEND'		=> $user->lang['FILLING_TABLES'],
		));

		// This loop takes one target table and processes it
		while ($current_table < sizeof($convert->convertor['schema']))
		{
			$schema = $convert->convertor['schema'][$current_table];

			// The target table isn't set, this can be because a module (for example the attachement mod) is taking care of this.
			if (empty($schema['target']))
			{
				$current_table++;
				continue;
			}

			$template->assign_block_vars('checks', array(
				'TITLE'	=> sprintf($user->lang['FILLING_TABLE'], $schema['target']),
			));

			// This is only the case when we first start working on the tables.
			if (!$skip_rows)
			{
				// process execute_first and query_first for this table...
				if (!empty($schema['execute_first']))
				{
					eval($schema['execute_first']);
				}

				if (!empty($schema['query_first']))
				{
					if (!is_array($schema['query_first']))
					{
						$schema['query_first'] = array('target', array($schema['query_first']));
					}
					else if (!is_array($schema['query_first'][0]))
					{
						$schema['query_first'] = array(array($schema['query_first'][0], $schema['query_first'][1]));
					}

					foreach ($schema['query_first'] as $query_first)
					{
						if ($query_first[0] == 'src')
						{
							if ($convert->mysql_convert && $same_db)
							{
								$src_db->sql_query("SET NAMES 'binary'");
							}
							$src_db->sql_query($query_first[1]);
							if ($convert->mysql_convert && $same_db)
							{
								$src_db->sql_query("SET NAMES 'utf8'");
							}
						}
						else
						{
							$db->sql_query($query_first[1]);
						}
					}
				}

				if (!empty($schema['autoincrement']))
				{
					switch ($db->sql_layer)
					{
						case 'postgres':
							$db->sql_query("SELECT SETVAL('" . $schema['target'] . "_seq',(select case when max(" . $schema['autoincrement'] . ")>0 then max(" . $schema['autoincrement'] . ")+1 else 1 end from " . $schema['target'] . '));');
						break;

						case 'oracle':
							$result = $db->sql_query('SELECT MAX(' . $schema['autoincrement'] . ') as max_id FROM ' . $schema['target']);
							$row = $db->sql_fetchrow($result);
							$db->sql_freeresult($result);

							$largest_id = (int) $row['max_id'];

							if ($largest_id)
							{
								$db->sql_query('DROP SEQUENCE ' . $schema['target'] . '_seq');
								$db->sql_query('CREATE SEQUENCE ' . $schema['target'] . '_seq START WITH ' . ($largest_id + 1));
							}
						break;
					}
				}
			}

			// Process execute_always for this table
			// This is for code which needs to be executed on every pass of this table if
			// it gets split because of time restrictions
			if (!empty($schema['execute_always']))
			{
				eval($schema['execute_always']);
			}

			//
			// Set up some variables
			//
			// $waiting_rows	holds rows for multirows insertion (MySQL only)
			// $src_tables		holds unique tables with aliases to select from
			// $src_fields		will quickly refer source fields (or aliases) corresponding to the current index
			// $select_fields	holds the names of the fields to retrieve
			//

			$sql_data = array(
				'source_fields'		=> array(),
				'target_fields'		=> array(),
				'source_tables'		=> array(),
				'select_fields'		=> array(),
			);

			// This statement is building the keys for later insertion.
			$insert_query = $this->build_insert_query($schema, $sql_data, $current_table);

			// If no source table is affected, we skip the table
			if (empty($sql_data['source_tables']))
			{
				$skip_rows = 0;
				$current_table++;
				continue;
			}

			$distinct = (!empty($schema['distinct'])) ? 'DISTINCT ' : '';

			$sql = 'SELECT ' . $distinct . implode(', ', $sql_data['select_fields']) . " \nFROM " . implode(', ', $sql_data['source_tables']);

			// Where
			$sql .= (!empty($schema['where'])) ? "\nWHERE (" . $schema['where'] . ')' : '';

			// Group By
			if (!empty($schema['group_by']))
			{
				$schema['group_by'] = array($schema['group_by']);
				foreach ($sql_data['select_fields'] as $select)
				{
					$alias = strpos(strtolower($select), ' as ');
					$select = ($alias) ? substr($select, 0, $alias) : $select;
					if (!in_array($select, $schema['group_by']))
					{
						$schema['group_by'][] = $select;
					}
				}
			}
			$sql .= (!empty($schema['group_by'])) ? "\nGROUP BY " . implode(', ', $schema['group_by']) : '';

			// Having
			$sql .= (!empty($schema['having'])) ? "\nHAVING " . $schema['having'] : '';

			// Order By
			if (empty($schema['order_by']) && !empty($schema['primary']))
			{
				$schema['order_by'] = $schema['primary'];
			}
			$sql .= (!empty($schema['order_by'])) ? "\nORDER BY " . $schema['order_by'] : '';

			// Counting basically holds the amount of rows processed.
			$counting = -1;
			$batch_time = 0;

			while ($counting === -1 || ($counting >= $convert->batch_size && still_on_time()))
			{
				$old_current_table = $current_table;

				$rows = '';
				$waiting_rows = array();

				if (!empty($batch_time))
				{
					$mtime = explode(' ', microtime());
					$mtime = $mtime[0] + $mtime[1];
					$rows = ceil($counting/($mtime - $batch_time)) . " rows/s ($counting rows) | ";
				}

				$template->assign_block_vars('checks', array(
					'TITLE'		=> "skip_rows = $skip_rows",
					'RESULT'	=> $rows . ((defined('DEBUG_EXTRA') && function_exists('memory_get_usage')) ? ceil(memory_get_usage()/1024) . ' ' . $user->lang['KIB'] : ''),
				));

				$mtime = explode(' ', microtime());
				$batch_time = $mtime[0] + $mtime[1];

				if ($convert->mysql_convert && $same_db)
				{
					$src_db->sql_query("SET NAMES 'binary'");
				}

				// Take skip rows into account and only fetch batch_size amount of rows
				$___result = $src_db->sql_query_limit($sql, $convert->batch_size, $skip_rows);

				if ($convert->mysql_convert && $same_db)
				{
					$src_db->sql_query("SET NAMES 'utf8'");
				}

				// This loop processes each row
				$counting = 0;

				$convert->row = $convert_row = array();

				if (!empty($schema['autoincrement']))
				{
					switch ($db->sql_layer)
					{
						case 'mssql':
						case 'mssql_odbc':
							$db->sql_query('SET IDENTITY_INSERT ' . $schema['target'] . ' ON');
						break;
					}
				}

				// Now handle the rows until time is over or no more rows to process...
				while ($counting === 0 || still_on_time())
				{
					$convert_row = $src_db->sql_fetchrow($___result);

					if (!$convert_row)
					{
						// move to the next batch or table
						break;
					}

					// With this we are able to always save the last state
					$convert->row = $convert_row;

					// Increment the counting variable, it stores the number of rows we have processed
					$counting++;

					$insert_values = array();

					$sql_flag = $this->process_row($schema, $sql_data, $insert_values);

					if ($sql_flag === true)
					{
						switch ($db->sql_layer)
						{
							// If MySQL, we'll wait to have num_wait_rows rows to submit at once
							case 'mysql':
							case 'mysql4':
							case 'mysqli':
								$waiting_rows[] = '(' . implode(', ', $insert_values) . ')';

								if (sizeof($waiting_rows) >= $convert->num_wait_rows)
								{
									$errored = false;

									$db->sql_return_on_error(true);

									if (!$db->sql_query($insert_query . implode(', ', $waiting_rows)))
									{
										$errored = true;
									}
									$db->sql_return_on_error(false);

									if ($errored)
									{
										$db->sql_return_on_error(true);
	
										// Because it errored out we will try to insert the rows one by one... most of the time this
										// is caused by duplicate entries - but we also do not want to miss one...
										foreach ($waiting_rows as $waiting_sql)
										{
											if (!$db->sql_query($insert_query . $waiting_sql))
											{
												$this->p_master->db_error($user->lang['DB_ERR_INSERT'], htmlspecialchars($insert_query . $waiting_sql) . '<br /><br />' . htmlspecialchars(print_r($db->_sql_error(), true)), __LINE__, __FILE__, true);
											}
										}

										$db->sql_return_on_error(false);
									}

									$waiting_rows = array();
								}

							break;

							default:
								$insert_sql = $insert_query . '(' . implode(', ', $insert_values) . ')';

								$db->sql_return_on_error(true);

								if (!$db->sql_query($insert_sql))
								{
									$this->p_master->db_error($user->lang['DB_ERR_INSERT'], htmlspecialchars($insert_sql) . '<br /><br />' . htmlspecialchars(print_r($db->_sql_error(), true)), __LINE__, __FILE__, true);
								}
								$db->sql_return_on_error(false);

								$waiting_rows = array();

							break;
						}
					}

					$skip_rows++;
				}
				$src_db->sql_freeresult($___result);

				// We might still have some rows waiting
				if (sizeof($waiting_rows))
				{
					$errored = false;
					$db->sql_return_on_error(true);

					if (!$db->sql_query($insert_query . implode(', ', $waiting_rows)))
					{
						$errored = true;
					}
					$db->sql_return_on_error(false);

					if ($errored)
					{
						$db->sql_return_on_error(true);

						// Because it errored out we will try to insert the rows one by one... most of the time this
						// is caused by duplicate entries - but we also do not want to miss one...
						foreach ($waiting_rows as $waiting_sql)
						{
							$db->sql_query($insert_query . $waiting_sql);
							$this->p_master->db_error($user->lang['DB_ERR_INSERT'], htmlspecialchars($insert_query . $waiting_sql) . '<br /><br />' . htmlspecialchars(print_r($db->_sql_error(), true)), __LINE__, __FILE__, true);
						}

						$db->sql_return_on_error(false);
					}

					$waiting_rows = array();
				}

				if (!empty($schema['autoincrement']))
				{
					switch ($db->sql_layer)
					{
						case 'mssql':
						case 'mssql_odbc':
							$db->sql_query('SET IDENTITY_INSERT ' . $schema['target'] . ' OFF');
						break;

						case 'postgres':
							$db->sql_query("SELECT SETVAL('" . $schema['target'] . "_seq',(select case when max(" . $schema['autoincrement'] . ")>0 then max(" . $schema['autoincrement'] . ")+1 else 1 end from " . $schema['target'] . '));');
						break;

						case 'oracle':
							$result = $db->sql_query('SELECT MAX(' . $schema['autoincrement'] . ') as max_id FROM ' . $schema['target']);
							$row = $db->sql_fetchrow($result);
							$db->sql_freeresult($result);

							$largest_id = (int) $row['max_id'];

							if ($largest_id)
							{
								$db->sql_query('DROP SEQUENCE ' . $schema['target'] . '_seq');
								$db->sql_query('CREATE SEQUENCE ' . $schema['target'] . '_seq START WITH ' . ($largest_id + 1));
							}
						break;
					}
				}
			}

			// When we reach this point, either the current table has been processed or we're running out of time.
			if (still_on_time() && $counting < $convert->batch_size/* && !defined('DEBUG_EXTRA')*/)
			{
				$skip_rows = 0;
				$current_table++;
			}
			else
			{/*
				if (still_on_time() && $counting < $convert->batch_size)
				{
					$skip_rows = 0;
					$current_table++;
				}*/

				// Looks like we ran out of time.
				$url = $this->save_convert_progress('&amp;current_table=' . $current_table . '&amp;skip_rows=' . $skip_rows);

				$current_table++;
//				$percentage = ($skip_rows == 0) ? 0 : floor(100 / ($total_rows / $skip_rows));

				$msg = sprintf($user->lang['STEP_PERCENT_COMPLETED'], $current_table, sizeof($convert->convertor['schema']));

				$template->assign_vars(array(
					'L_MESSAGE'		=> $msg,
					'L_SUBMIT'		=> $user->lang['CONTINUE_CONVERT'],
					'U_ACTION'		=> $url,
				));

				$this->meta_refresh($url);
				return;
			}
		}

		// Process execute_last then we'll be done
		$url = $this->save_convert_progress('&amp;jump=1');

		$template->assign_vars(array(
			'L_SUBMIT'		=> $user->lang['FINAL_STEP'],
			'U_ACTION'		=> $url,
		));

		$this->meta_refresh($url);
		return;
	}

	/**
	* Sync function being executed at the middle, some functions need to be executed after a successful sync.
	*/
	function sync_forums($sync_batch)
	{
		global $template, $user, $db, $phpbb_root_path, $phpEx, $config, $cache;
		global $convert;

		$template->assign_block_vars('checks', array(
			'S_LEGEND'	=> true,
			'LEGEND'	=> $user->lang['SYNC_TOPICS'],
		));

		$batch_size = $convert->batch_size;

		$sql = 'SELECT MIN(topic_id) as min_value, MAX(topic_id) AS max_value
			FROM ' . TOPICS_TABLE;
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		// Set values of minimum/maximum primary value for this table.
		$primary_min = $row['min_value'];
		$primary_max = $row['max_value'];

		if ($sync_batch == 0)
		{
			$sync_batch = (int) $primary_min;
		}

		if ($sync_batch == 0)
		{
			$sync_batch = 1;
		}

		// Fetch a batch of rows, process and insert them.
		while ($sync_batch <= $primary_max && still_on_time())
		{
			$end = ($sync_batch + $batch_size - 1);

			// Sync all topics in batch mode...
			sync('topic_approved', 'range', 'topic_id BETWEEN ' . $sync_batch . ' AND ' . $end, true, false);
			sync('topic', 'range', 'topic_id BETWEEN ' . $sync_batch . ' AND ' . $end, true, true);

			$template->assign_block_vars('checks', array(
				'TITLE'		=> sprintf($user->lang['SYNC_TOPIC_ID'], $sync_batch, ($sync_batch + $batch_size)) . ((defined('DEBUG_EXTRA') && function_exists('memory_get_usage')) ? ' [' . ceil(memory_get_usage()/1024) . ' ' . $user->lang['KIB'] . ']' : ''),
				'RESULT'	=> $user->lang['DONE'],
			));

			$sync_batch += $batch_size;
		}

		if ($sync_batch >= $primary_max)
		{
			$url = $this->save_convert_progress('&amp;final_jump=1');

			$template->assign_vars(array(
				'L_SUBMIT'		=> $user->lang['CONTINUE_CONVERT'],
				'U_ACTION'		=> $url,
			));

			$this->meta_refresh($url);
			return;
		}
		else
		{
			$sync_batch--;
		}

		$url = $this->save_convert_progress('&amp;sync_batch=' . $sync_batch);

		$template->assign_vars(array(
			'L_SUBMIT'		=> $user->lang['CONTINUE_CONVERT'],
			'U_ACTION'		=> $url,
		));

		$this->meta_refresh($url);
		return;
	}

	/**
	* Save the convertor status
	*/
	function save_convert_progress($step)
	{
		global $convert, $language;

		// Save convertor Status
		set_config('convert_progress', serialize(array(
			'step'			=> $step,
			'table_prefix'	=> $convert->src_table_prefix,
			'tag'			=> $convert->convertor_tag,
		)), true);

		set_config('convert_db_server', serialize(array(
			'dbms'			=> $convert->src_dbms,
			'dbhost'		=> $convert->src_dbhost,
			'dbport'		=> $convert->src_dbport,
			'dbname'		=> $convert->src_dbname,
		)), true);

		set_config('convert_db_user', serialize(array(
			'dbuser'		=> $convert->src_dbuser,
			'dbpasswd'		=> $convert->src_dbpasswd,
		)), true);

		return $this->p_master->module_url . "?mode={$this->mode}&amp;sub=in_progress&amp;tag={$convert->convertor_tag}$step&amp;language=$language";
	}

	/**
	* Finish conversion, the last function to be called.
	*/
	function finish_conversion()
	{
		global $db, $phpbb_root_path, $convert, $config, $language, $user, $template;

		$db->sql_query('DELETE FROM ' . CONFIG_TABLE . "
			WHERE config_name = 'convert_progress'
				OR config_name = 'convert_options'
				OR config_name = 'convert_db_server'
				OR config_name = 'convert_db_user'");
		$db->sql_query('DELETE FROM ' . SESSIONS_TABLE);

		@unlink($phpbb_root_path . 'cache/data_global.php');
		cache_moderators();

		// And finally, add a note to the log
		add_log('admin', 'LOG_INSTALL_CONVERTED', $convert->convertor_data['forum_name'], $config['version']);

		$url = $this->p_master->module_url . "?mode={$this->mode}&amp;sub=final&amp;language=$language";

		$template->assign_vars(array(
			'L_SUBMIT'		=> $user->lang['FINAL_STEP'],
			'U_ACTION'		=> $url,
		));

		$this->meta_refresh($url);
		return;
	}

	/**
	* This function marks the steps after syncing
	*/
	function final_jump($final_jump)
	{
		global $template, $user, $src_db, $same_db, $db, $phpbb_root_path, $phpEx, $config, $cache;
		global $convert;

		$template->assign_block_vars('checks', array(
			'S_LEGEND'	=> true,
			'LEGEND'	=> $user->lang['PROCESS_LAST'],
		));

		if ($final_jump == 1)
		{
			$db->sql_return_on_error(true);

			update_topics_posted();

			$template->assign_block_vars('checks', array(
				'TITLE'		=> $user->lang['UPDATE_TOPICS_POSTED'],
				'RESULT'	=> $user->lang['DONE'],
			));
			
			if ($db->sql_error_triggered)
			{
				$template->assign_vars(array(
					'S_ERROR_BOX'	=> true,
					'ERROR_TITLE'	=> $user->lang['UPDATE_TOPICS_POSTED'],
					'ERROR_MSG'		=> $user->lang['UPDATE_TOPICS_POSTED_ERR'],
				));
			}
			$db->sql_return_on_error(false);
			
			$this->finish_conversion();
			return;
		}
	}

	/**
	* This function marks the steps before syncing (jump=1)
	*/
	function jump($jump, $last_statement)
	{
		global $template, $user, $src_db, $same_db, $db, $phpbb_root_path, $phpEx, $config, $cache;
		global $convert;

		$template->assign_block_vars('checks', array(
			'S_LEGEND'	=> true,
			'LEGEND'	=> $user->lang['PROCESS_LAST'],
		));

		if ($jump == 1)
		{
			// Execute 'last' statements/queries
			if (!empty($convert->convertor['execute_last']))
			{
				if (!is_array($convert->convertor['execute_last']))
				{
					eval($convert->convertor['execute_last']);
				}
				else
				{
					while ($last_statement < sizeof($convert->convertor['execute_last']))
					{
						eval($convert->convertor['execute_last'][$last_statement]);

						$template->assign_block_vars('checks', array(
							'TITLE'		=> $convert->convertor['execute_last'][$last_statement],
							'RESULT'	=> $user->lang['DONE'],
						));

						$last_statement++;
						$url = $this->save_convert_progress('&amp;jump=1&amp;last=' . $last_statement);

						$percentage = ($last_statement == 0) ? 0 : floor(100 / (sizeof($convert->convertor['execute_last']) / $last_statement));
						$msg = sprintf($user->lang['STEP_PERCENT_COMPLETED'], $last_statement, sizeof($convert->convertor['execute_last']), $percentage);

						$template->assign_vars(array(
							'L_SUBMIT'		=> $user->lang['CONTINUE_LAST'],
							'L_MESSAGE'		=> $msg,
							'U_ACTION'		=> $url,
						));

						$this->meta_refresh($url);
						return;
					}
				}
			}

			if (!empty($convert->convertor['query_last']))
			{
				if (!is_array($convert->convertor['query_last']))
				{
					$convert->convertor['query_last'] = array('target', array($convert->convertor['query_last']));
				}
				else if (!is_array($convert->convertor['query_last'][0]))
				{
					$convert->convertor['query_last'] = array(array($convert->convertor['query_last'][0], $convert->convertor['query_last'][1]));
				}

				foreach ($convert->convertor['query_last'] as $query_last)
				{
					if ($query_last[0] == 'src')
					{
						if ($convert->mysql_convert && $same_db)
						{
							$src_db->sql_query("SET NAMES 'binary'");
						}

						$src_db->sql_query($query_last[1]);

						if ($convert->mysql_convert && $same_db)
						{
							$src_db->sql_query("SET NAMES 'utf8'");
						}
					}
					else
					{
						$db->sql_query($query_last[1]);
					}
				}
			}

			// Sanity check
			$db->sql_return_on_error(false);
			$src_db->sql_return_on_error(false);

			fix_empty_primary_groups();

			if (!isset($config['board_startdate']))
			{
				$sql = 'SELECT MIN(user_regdate) AS board_startdate
					FROM ' . USERS_TABLE;
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (($row['board_startdate'] < $config['board_startdate'] && $row['board_startdate'] > 0) || !isset($config['board_startdate']))
				{
					set_config('board_startdate', $row['board_startdate']);
					$db->sql_query('UPDATE ' . USERS_TABLE . ' SET user_regdate = ' . $row['board_startdate'] . ' WHERE user_id = ' . ANONYMOUS);
				}
			}

			update_dynamic_config();

			$template->assign_block_vars('checks', array(
				'TITLE'		=> $user->lang['CLEAN_VERIFY'],
				'RESULT'	=> $user->lang['DONE'],
			));

			$url = $this->save_convert_progress('&amp;jump=2');

			$template->assign_vars(array(
				'L_SUBMIT'		=> $user->lang['CONTINUE_CONVERT'],
				'U_ACTION'		=> $url,
			));

			$this->meta_refresh($url);
			return;
		}

		if ($jump == 2)
		{
			$db->sql_query('UPDATE ' . USERS_TABLE . " SET user_permissions = ''");

			// TODO: sync() is likely going to bomb out on forums with a considerable amount of topics.
			// TODO: the sync function is able to handle FROM-TO values, we should use them here (batch processing)
			sync('forum', '', '', false, true);
			$cache->destroy('sql', FORUMS_TABLE);

			$template->assign_block_vars('checks', array(
				'TITLE'		=> $user->lang['SYNC_FORUMS'],
				'RESULT'	=> $user->lang['DONE'],
			));

			// Continue with synchronizing the forums...
			$url = $this->save_convert_progress('&amp;sync_batch=0');

			$template->assign_vars(array(
				'L_SUBMIT'		=> $user->lang['CONTINUE_CONVERT'],
				'U_ACTION'		=> $url,
			));

			$this->meta_refresh($url);
			return;
		}
	}

	function build_insert_query(&$schema, &$sql_data, $current_table)
	{
		global $db, $user;
		global $convert;

		// Can we use IGNORE with this DBMS?
		$sql_ignore = (strpos($db->sql_layer, 'mysql') === 0 && !defined('DEBUG_EXTRA')) ? 'IGNORE ' : '';
		$insert_query = 'INSERT ' . $sql_ignore . 'INTO ' . $schema['target'] . ' (';

		$aliases = array();

		$sql_data = array(
			'source_fields'		=> array(),
			'target_fields'		=> array(),
			'source_tables'		=> array(),
			'select_fields'		=> array(),
		);

		foreach ($schema as $key => $val)
		{
			// Example: array('group_name',				'extension_groups.group_name',		'htmlspecialchars'),
			if (is_int($key))
			{
				if (!empty($val[0]))
				{
					// Target fields
					$sql_data['target_fields'][$val[0]] = $key;
					$insert_query .= $val[0] . ', ';
				}

				if (!is_array($val[1]))
				{
					$val[1] = array($val[1]);
				}

				foreach ($val[1] as $valkey => $value_1)
				{
					// This should cover about any case:
					//
					// table.field					=> SELECT table.field				FROM table
					// table.field AS alias			=> SELECT table.field	AS alias	FROM table
					// table.field AS table2.alias	=> SELECT table2.field	AS alias	FROM table table2
					// table.field AS table2.field	=> SELECT table2.field				FROM table table2
					//
					if (preg_match('/^([a-z0-9_]+)\.([a-z0-9_]+)( +AS +(([a-z0-9_]+?)\.)?([a-z0-9_]+))?$/i', $value_1, $m))
					{
						// There is 'AS ...' in the field names
						if (!empty($m[3]))
						{
							$value_1 = ($m[2] == $m[6]) ? $m[1] . '.' . $m[2] : $m[1] . '.' . $m[2] . ' AS ' . $m[6];

							// Table alias: store it then replace the source table with it
							if (!empty($m[5]) && $m[5] != $m[1])
							{
								$aliases[$m[5]] = $m[1];
								$value_1 = str_replace($m[1] . '.' . $m[2], $m[5] . '.' . $m[2], $value_1);
							}
						}
						else
						{
							// No table alias
							$sql_data['source_tables'][$m[1]] = (empty($convert->src_table_prefix)) ? $m[1] : $convert->src_table_prefix . $m[1] . ' ' . $m[1];
						}

						$sql_data['select_fields'][$value_1] = $value_1;
						$sql_data['source_fields'][$key][$valkey] = (!empty($m[6])) ? $m[6] : $m[2];
					}
				}
			}
			else if ($key == 'where' || $key == 'group_by' || $key == 'order_by' || $key == 'having')
			{
				if (@preg_match_all('/([a-z0-9_]+)\.([a-z0-9_]+)/i', $val, $m))
				{
					foreach ($m[1] as $value)
					{
						$sql_data['source_tables'][$value] = (empty($convert->src_table_prefix)) ? $value : $convert->src_table_prefix . $value . ' ' . $value;
					}
				}
			}
		}

		// Add the aliases to the list of tables
		foreach ($aliases as $alias => $table)
		{
			$sql_data['source_tables'][$alias] = $convert->src_table_prefix . $table . ' ' . $alias;
		}

		// 'left_join'		=> 'forums LEFT JOIN forum_prune ON forums.forum_id = forum_prune.forum_id',
		if (!empty($schema['left_join']))
		{
			if (!is_array($schema['left_join']))
			{
				$schema['left_join'] = array($schema['left_join']);
			}

			foreach ($schema['left_join'] as $left_join)
			{
				// This won't handle concatened LEFT JOINs
				if (!preg_match('/([a-z0-9_]+) LEFT JOIN ([a-z0-9_]+) A?S? ?([a-z0-9_]*?) ?(ON|USING)(.*)/i', $left_join, $m))
				{
					$this->p_master->error(sprintf($user->lang['NOT_UNDERSTAND'], 'LEFT JOIN', $left_join, $current_table, $schema['target']), __LINE__, __FILE__);
				}

				if (!empty($aliases[$m[2]]))
				{
					if (!empty($m[3]))
					{
						$this->p_master->error(sprintf($user->lang['NAMING_CONFLICT'], $m[2], $m[3], $schema['left_join']), __LINE__, __FILE__);
					}

					$m[2] = $aliases[$m[2]];
					$m[3] = $m[2];
				}

				$right_table = $convert->src_table_prefix . $m[2];
				if (!empty($m[3]))
				{
					unset($sql_data['source_tables'][$m[3]]);
				}
				else if ($m[2] != $m[1])
				{
					unset($sql_data['source_tables'][$m[2]]);
				}

				if (strpos($sql_data['source_tables'][$m[1]], "\nLEFT JOIN") !== false)
				{
					$sql_data['source_tables'][$m[1]] = '(' . $sql_data['source_tables'][$m[1]] . ")\nLEFT JOIN $right_table";
				}
				else
				{
					$sql_data['source_tables'][$m[1]] .= "\nLEFT JOIN $right_table";
				}

				if (!empty($m[3]))
				{
					unset($sql_data['source_tables'][$m[3]]);
					$sql_data['source_tables'][$m[1]] .= ' AS ' . $m[3];
				}
				else if (!empty($convert->src_table_prefix))
				{
					$sql_data['source_tables'][$m[1]] .= ' AS ' . $m[2];
				}
				$sql_data['source_tables'][$m[1]] .= ' ' . $m[4] . $m[5];
			}
		}

		// Remove ", " from the end of the insert query
		$insert_query = substr($insert_query, 0, -2) . ') VALUES ';

		return $insert_query;
	}

	/**
	* Function for processing the currently handled row
	*/
	function process_row(&$schema, &$sql_data, &$insert_values)
	{
		global $template, $user, $phpbb_root_path, $phpEx, $db, $lang, $config, $cache;
		global $convert, $convert_row;

		$sql_flag = false;

		foreach ($schema as $key => $fields)
		{
			// We are only interested in the lines with:
			// array('comment', 'attachments_desc.comment', 'htmlspecialchars'),
			if (is_int($key))
			{
				if (!is_array($fields[1]))
				{
					$fields[1] = array($fields[1]);
				}

				$firstkey_set = false;
				$firstkey = 0;
	
				foreach ($fields[1] as $inner_key => $inner_value)
				{
					if (!$firstkey_set)
					{
						$firstkey = $inner_key;
						$firstkey_set = true;
					}

					$src_field = isset($sql_data['source_fields'][$key][$inner_key]) ? $sql_data['source_fields'][$key][$inner_key] : '';

					if (!empty($src_field))
					{
						$fields[1][$inner_key] = $convert->row[$src_field];
					}
				}

				if (!empty($fields[0]))
				{
					// We have a target field, if we haven't set $sql_flag yet it will be set to TRUE.
					// If a function has already set it to FALSE it won't change it.
					if ($sql_flag === false)
					{
						$sql_flag = true;
					}
		
					// No function assigned?
					if (empty($fields[2]))
					{
						$value = $fields[1][$firstkey];
					}
					else if (is_array($fields[2]))
					{
						// Execute complex function/eval/typecast
						$value = $fields[1];

						foreach ($fields[2] as $type => $execution)
						{
							if (strpos($type, 'typecast') === 0)
							{
								if (!is_array($value))
								{
									$value = array($value);
								}
								$value = $value[0];
								settype($value, $execution);
							}
							else if (strpos($type, 'function') === 0)
							{
								if (!is_array($value))
								{
									$value = array($value);
								}

								$value = call_user_func_array($execution, $value);
							}
							else if (strpos($type, 'execute') === 0)
							{
								if (!is_array($value))
								{
									$value = array($value);
								}

								$execution = str_replace('{RESULT}', '$value', $execution);
								$execution = str_replace('{VALUE}', '$value', $execution);
								eval($execution);
							}
						}
					}
					else
					{
						$value = call_user_func_array($fields[2], $fields[1]);
					}

					if (is_null($value))
					{
						$value = '';
					}

					$insert_values[] = $db->_sql_validate_value($value);
				}
				else if (!empty($fields[2]))
				{
					if (is_array($fields[2]))
					{
						// Execute complex function/eval/typecast
						$value = '';

						foreach ($fields[2] as $type => $execution)
						{
							if (strpos($type, 'typecast') === 0)
							{
								$value = settype($value, $execution);
							}
							else if (strpos($type, 'function') === 0)
							{
								if (!is_array($value))
								{
									$value = array($value);
								}

								$value = call_user_func_array($execution, $value);
							}
							else if (strpos($type, 'execute') === 0)
							{
								if (!is_array($value))
								{
									$value = array($value);
								}

								$execution = str_replace('{RESULT}', '$value', $execution);
								$execution = str_replace('{VALUE}', '$value', $execution);
								eval($execution);
							}
						}
					}
					else
					{
						call_user_func_array($fields[2], $fields[1]);
					}
				}
			}
		}

		return $sql_flag;
	}

	/**
	* Own meta refresh function to be able to change the global time used
	*/
	function meta_refresh($url)
	{
		global $convert, $template;

		if ($convert->options['refresh'])
		{
			// Because we should not rely on correct settings, we simply use the relative path here directly.
			$template->assign_vars(array(
				'S_REFRESH'	=> true,
				'META'		=> '<meta http-equiv="refresh" content="5;url=' . $url . '" />')
			);
		}
	}

	/**
	* The information below will be used to build the input fields presented to the user
	*/
	var $convert_options = array(
		'legend1'			=> 'SPECIFY_OPTIONS',
		'src_dbms'			=> array('lang' => 'DBMS',			'type' => 'select', 'options' => 'dbms_select(\'{VALUE}\', true)', 'explain' => false),
		'src_dbhost'		=> array('lang' => 'DB_HOST',		'type' => 'text:25:100', 'explain' => true),
		'src_dbport'		=> array('lang' => 'DB_PORT',		'type' => 'text:25:100', 'explain' => true),
		'src_dbname'		=> array('lang' => 'DB_NAME',		'type' => 'text:25:100', 'explain' => false),
		'src_dbuser'		=> array('lang' => 'DB_USERNAME',	'type' => 'text:25:100', 'explain' => false),
		'src_dbpasswd'		=> array('lang' => 'DB_PASSWORD',	'type' => 'password:25:100', 'explain' => false),
		'src_table_prefix'	=> array('lang' => 'TABLE_PREFIX',	'type' => 'text:25:100', 'explain' => false),
		//'src_url'			=> array('lang' => 'FORUM_ADDRESS',	'type' => 'text:50:100', 'explain' => true),
		'forum_path'		=> array('lang' => 'FORUM_PATH',	'type' => 'text:25:100', 'explain' => true),
		'refresh'			=> array('lang' => 'REFRESH_PAGE',	'type' => 'radio:yes_no', 'explain' => true),
	);
}

?>