<?php
/**
*
* @package acp
* @version $Id: acp_search.php 8479 2008-03-29 00:22:48Z naderman $
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
class acp_search
{
	var $u_action;
	var $state;
	var $search;
	var $max_post_id;
	var $batch_size = 100;

	function main($id, $mode)
	{
		global $user;

		$user->add_lang('acp/search');

		// For some this may be of help...
		@ini_set('memory_limit', '128M');

		switch ($mode)
		{
			case 'settings':
				$this->settings($id, $mode);
			break;

			case 'index':
				$this->index($id, $mode);
			break;
		}
	}

	function settings($id, $mode)
	{
		global $db, $user, $auth, $template, $cache;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		$submit = (isset($_POST['submit'])) ? true : false;

		$search_types = $this->get_search_types();

		$settings = array(
			'search_interval'			=> 'float',
			'search_anonymous_interval'	=> 'float',
			'load_search'				=> 'bool',
			'limit_search_load'			=> 'float',
			'min_search_author_chars'	=> 'integer',
			'search_store_results'		=> 'integer',
		);

		$search = null;
		$error = false;
		$search_options = '';
		foreach ($search_types as $type)
		{
			if ($this->init_search($type, $search, $error))
			{
				continue;
			}

			$name = ucfirst(strtolower(str_replace('_', ' ', $type)));
			$selected = ($config['search_type'] == $type) ? ' selected="selected"' : '';
			$search_options .= '<option value="' . $type . '"' . $selected . '>' . $name . '</option>';

			if (method_exists($search, 'acp'))
			{
				$vars = $search->acp();

				if (!$submit)
				{
					$template->assign_block_vars('backend', array(
						'NAME'		=> $name,
						'SETTINGS'	=> $vars['tpl'])
					);
				}
				else if (is_array($vars['config']))
				{
					$settings = array_merge($settings, $vars['config']);
				}
			}
		}
		unset($search);
		unset($error);

		$cfg_array = (isset($_REQUEST['config'])) ? request_var('config', array('' => ''), true) : array();
		$updated = request_var('updated', false);

		foreach ($settings as $config_name => $var_type)
		{
			if (!isset($cfg_array[$config_name]))
			{
				continue;
			}

			// e.g. integer:4:12 (min 4, max 12)
			$var_type = explode(':', $var_type);

			$config_value = $cfg_array[$config_name];
			settype($config_value, $var_type[0]);

			if (isset($var_type[1]))
			{
				$config_value = max($var_type[1], $config_value);
			}

			if (isset($var_type[2]))
			{
				$config_value = min($var_type[2], $config_value);
			}

			// only change config if anything was actually changed
			if ($submit && ($config[$config_name] != $config_value))
			{
				set_config($config_name, $config_value);
				$updated = true;
			}
		}

		if ($submit)
		{
			$extra_message = '';
			if ($updated)
			{
				add_log('admin', 'LOG_CONFIG_SEARCH');
			}

			if (isset($cfg_array['search_type']) && in_array($cfg_array['search_type'], $search_types, true) && ($cfg_array['search_type'] != $config['search_type']))
			{
				$search = null;
				$error = false;

				if (!$this->init_search($cfg_array['search_type'], $search, $error))
				{
					if (confirm_box(true))
					{
						if (!method_exists($search, 'init') || !($error = $search->init()))
						{
							set_config('search_type', $cfg_array['search_type']);

							if (!$updated)
							{
								add_log('admin', 'LOG_CONFIG_SEARCH');
							}
							$extra_message = '<br />' . $user->lang['SWITCHED_SEARCH_BACKEND'] . '<br /><a href="' . append_sid("{$phpbb_admin_path}index.$phpEx", 'i=search&amp;mode=index') . '">&raquo; ' . $user->lang['GO_TO_SEARCH_INDEX'] . '</a>';
						}
						else
						{
							trigger_error($error . adm_back_link($this->u_action), E_USER_WARNING);
						}
					}
					else
					{
						confirm_box(false, $user->lang['CONFIRM_SEARCH_BACKEND'], build_hidden_fields(array(
							'i'			=> $id,
							'mode'		=> $mode,
							'submit'	=> true,
							'updated'	=> $updated,
							'config'	=> array('search_type' => $cfg_array['search_type']),
						)));
					}
				}
				else
				{
					trigger_error($error . adm_back_link($this->u_action), E_USER_WARNING);
				}
			}

			$search = null;
			$error = false;
			if (!$this->init_search($config['search_type'], $search, $error))
			{
				if ($updated)
				{
					if (method_exists($search, 'config_updated'))
					{
						if ($search->config_updated())
						{
							trigger_error($error . adm_back_link($this->u_action), E_USER_WARNING);
						}
					}
				}
			}
			else
			{
				trigger_error($error . adm_back_link($this->u_action), E_USER_WARNING);
			}

			trigger_error($user->lang['CONFIG_UPDATED'] . $extra_message . adm_back_link($this->u_action));
		}
		unset($cfg_array);

		$this->tpl_name = 'acp_search';
		$this->page_title = 'ACP_SEARCH_SETTINGS';

		$template->assign_vars(array(
			'LIMIT_SEARCH_LOAD'		=> (float) $config['limit_search_load'],
			'MIN_SEARCH_AUTHOR_CHARS'	=> (int) $config['min_search_author_chars'],
			'SEARCH_INTERVAL'		=> (float) $config['search_interval'],
			'SEARCH_GUEST_INTERVAL'	=> (float) $config['search_anonymous_interval'],
			'SEARCH_STORE_RESULTS'	=> (int) $config['search_store_results'],

			'S_SEARCH_TYPES'		=> $search_options,
			'S_YES_SEARCH'			=> (bool) $config['load_search'],
			'S_SETTINGS'			=> true,

			'U_ACTION'				=> $this->u_action)
		);
	}

	function index($id, $mode)
	{
		global $db, $user, $auth, $template, $cache;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		if (isset($_REQUEST['action']) && is_array($_REQUEST['action']))
		{
			$action = request_var('action', array('' => false));
			$action = key($action);
		}
		else
		{
			$action = request_var('action', '');
		}
		$this->state = explode(',', $config['search_indexing_state']);

		if (isset($_POST['cancel']))
		{
			$action = '';
			$this->state = array();
			$this->save_state();
		}

		if ($action)
		{
			switch ($action)
			{
				case 'progress_bar':
					$type = request_var('type', '');
					$this->display_progress_bar($type);
				break;

				case 'delete':
					$this->state[1] = 'delete';
				break;

				case 'create':
					$this->state[1] = 'create';
				break;

				default:
					trigger_error('NO_ACTION', E_USER_ERROR);
				break;
			}

			if (empty($this->state[0]))
			{
				$this->state[0] = request_var('search_type', '');
			}

			$this->search = null;
			$error = false;
			if ($this->init_search($this->state[0], $this->search, $error))
			{
				trigger_error($error . adm_back_link($this->u_action), E_USER_WARNING);
			}
			$name = ucfirst(strtolower(str_replace('_', ' ', $this->state[0])));

			$action = &$this->state[1];

			$this->max_post_id = $this->get_max_post_id();

			$post_counter = (isset($this->state[2])) ? $this->state[2] : 0;
			$this->state[2] = &$post_counter;
			$this->save_state();

			switch ($action)
			{
				case 'delete':
					if (method_exists($this->search, 'delete_index'))
					{
						// pass a reference to myself so the $search object can make use of save_state() and attributes
						if ($error = $this->search->delete_index($this, append_sid("{$phpbb_admin_path}index.$phpEx", "i=$id&mode=$mode&action=delete", false)))
						{
							$this->state = array('');
							$this->save_state();
							trigger_error($error . adm_back_link($this->u_action) . $this->close_popup_js(), E_USER_WARNING);
						}
					}
					else
					{
						$starttime = explode(' ', microtime());
						$starttime = $starttime[1] + $starttime[0];
						$row_count = 0;
						while (still_on_time() && $post_counter <= $this->max_post_id)
						{
							$sql = 'SELECT post_id, poster_id, forum_id
								FROM ' . POSTS_TABLE . '
								WHERE post_id >= ' . (int) ($post_counter + 1) . '
									AND post_id <= ' . (int) ($post_counter + $this->batch_size);
							$result = $db->sql_query($sql);

							$ids = $posters = $forum_ids = array();
							while ($row = $db->sql_fetchrow($result))
							{
								$ids[] = $row['post_id'];
								$posters[] = $row['poster_id'];
								$forum_ids[] = $row['forum_id'];
							}
							$db->sql_freeresult($result);
							$row_count += sizeof($ids);

							if (sizeof($ids))
							{
								$this->search->index_remove($ids, $posters, $forum_ids);
							}

							$post_counter += $this->batch_size;
						}
						// save the current state
						$this->save_state();

						if ($post_counter <= $this->max_post_id)
						{
							$mtime = explode(' ', microtime());
							$totaltime = $mtime[0] + $mtime[1] - $starttime;
							$rows_per_second = $row_count / $totaltime;
							meta_refresh(1, append_sid($this->u_action . '&amp;action=delete&amp;skip_rows=' . $post_counter));
							trigger_error(sprintf($user->lang['SEARCH_INDEX_DELETE_REDIRECT'], $post_counter, $row_count, $rows_per_second));
						}
					}

					$this->search->tidy();

					$this->state = array('');
					$this->save_state();

					add_log('admin', 'LOG_SEARCH_INDEX_REMOVED', $name);
					trigger_error($user->lang['SEARCH_INDEX_REMOVED'] . adm_back_link($this->u_action) . $this->close_popup_js());
				break;

				case 'create':
					if (method_exists($this->search, 'create_index'))
					{
						// pass a reference to acp_search so the $search object can make use of save_state() and attributes
						if ($error = $this->search->create_index($this, append_sid("{$phpbb_admin_path}index.$phpEx", "i=$id&mode=$mode&action=create", false)))
						{
							$this->state = array('');
							$this->save_state();
							trigger_error($error . adm_back_link($this->u_action) . $this->close_popup_js(), E_USER_WARNING);
						}
					}
					else
					{
						$sql = 'SELECT forum_id, enable_indexing
							FROM ' . FORUMS_TABLE;
						$result = $db->sql_query($sql, 3600);

						while ($row = $db->sql_fetchrow($result))
						{
							$forums[$row['forum_id']] = (bool) $row['enable_indexing'];
						}
						$db->sql_freeresult($result);

						$starttime = explode(' ', microtime());
						$starttime = $starttime[1] + $starttime[0];
						$row_count = 0;
						while (still_on_time() && $post_counter <= $this->max_post_id)
						{
							$sql = 'SELECT post_id, post_subject, post_text, poster_id, forum_id
								FROM ' . POSTS_TABLE . '
								WHERE post_id >= ' . (int) ($post_counter + 1) . '
									AND post_id <= ' . (int) ($post_counter + $this->batch_size);
							$result = $db->sql_query($sql);

							while ($row = $db->sql_fetchrow($result))
							{
								// Indexing enabled for this forum or global announcement?
								// Global announcements get indexed by default.
								if (!$row['forum_id'] || (isset($forums[$row['forum_id']]) && $forums[$row['forum_id']]))
								{
									$this->search->index('post', $row['post_id'], $row['post_text'], $row['post_subject'], $row['poster_id'], $row['forum_id']);
								}
								$row_count++;
							}
							$db->sql_freeresult($result);

							$post_counter += $this->batch_size;
						}
						// save the current state
						$this->save_state();

						// pretend the number of posts was as big as the number of ids we indexed so far
						// just an estimation as it includes deleted posts
						$num_posts = $config['num_posts'];
						$config['num_posts'] = min($config['num_posts'], $post_counter);
						$this->search->tidy();
						$config['num_posts'] = $num_posts;

						if ($post_counter <= $this->max_post_id)
						{
							$mtime = explode(' ', microtime());
							$totaltime = $mtime[0] + $mtime[1] - $starttime;
							$rows_per_second = $row_count / $totaltime;
							meta_refresh(1, append_sid($this->u_action . '&amp;action=create&amp;skip_rows=' . $post_counter));
							trigger_error(sprintf($user->lang['SEARCH_INDEX_CREATE_REDIRECT'], $post_counter, $row_count, $rows_per_second));
						}
					}

					$this->search->tidy();

					$this->state = array('');
					$this->save_state();

					add_log('admin', 'LOG_SEARCH_INDEX_CREATED', $name);
					trigger_error($user->lang['SEARCH_INDEX_CREATED'] . adm_back_link($this->u_action) . $this->close_popup_js());
				break;
			}
		}

		$search_types = $this->get_search_types();

		$search = null;
		$error = false;
		$search_options = '';
		foreach ($search_types as $type)
		{
			if ($this->init_search($type, $search, $error) || !method_exists($search, 'index_created'))
			{
				continue;
			}

			$name = ucfirst(strtolower(str_replace('_', ' ', $type)));

			$data = array();
			if (method_exists($search, 'index_stats'))
			{
				$data = $search->index_stats();
			}

			$statistics = array();
			foreach ($data as $statistic => $value)
			{
				$n = sizeof($statistics);
				if ($n && sizeof($statistics[$n - 1]) < 3)
				{
					$statistics[$n - 1] += array('statistic_2' => $statistic, 'value_2' => $value);
				}
				else
				{
					$statistics[] = array('statistic_1' => $statistic, 'value_1' => $value);
				}
			}

			$template->assign_block_vars('backend', array(
				'L_NAME'			=> $name,
				'NAME'				=> $type,

				'S_ACTIVE'			=> ($type == $config['search_type']) ? true : false,
				'S_HIDDEN_FIELDS'	=> build_hidden_fields(array('search_type' => $type)),
				'S_INDEXED'			=> (bool) $search->index_created(),
				'S_STATS'			=> (bool) sizeof($statistics))
			);

			foreach ($statistics as $statistic)
			{
				$template->assign_block_vars('backend.data', array(
					'STATISTIC_1'	=> $statistic['statistic_1'],
					'VALUE_1'		=> $statistic['value_1'],
					'STATISTIC_2'	=> (isset($statistic['statistic_2'])) ? $statistic['statistic_2'] : '',
					'VALUE_2'		=> (isset($statistic['value_2'])) ? $statistic['value_2'] : '')
				);
			}
		}
		unset($search);
		unset($error);
		unset($statistics);
		unset($data);

		$this->tpl_name = 'acp_search';
		$this->page_title = 'ACP_SEARCH_INDEX';

		$template->assign_vars(array(
			'S_INDEX'				=> true,
			'U_ACTION'				=> $this->u_action,
			'U_PROGRESS_BAR'		=> append_sid("{$phpbb_admin_path}index.$phpEx", "i=$id&amp;mode=$mode&amp;action=progress_bar"),
			'UA_PROGRESS_BAR'		=> addslashes(append_sid("{$phpbb_admin_path}index.$phpEx", "i=$id&amp;mode=$mode&amp;action=progress_bar")),
		));

		if (isset($this->state[1]))
		{
			$template->assign_vars(array(
				'S_CONTINUE_INDEXING'	=> $this->state[1],
				'U_CONTINUE_INDEXING'	=> $this->u_action . '&amp;action=' . $this->state[1],
				'L_CONTINUE'			=> ($this->state[1] == 'create') ? $user->lang['CONTINUE_INDEXING'] : $user->lang['CONTINUE_DELETING_INDEX'],
				'L_CONTINUE_EXPLAIN'	=> ($this->state[1] == 'create') ? $user->lang['CONTINUE_INDEXING_EXPLAIN'] : $user->lang['CONTINUE_DELETING_INDEX_EXPLAIN'])
			);
		}
	}

	function display_progress_bar($type)
	{
		global $template, $user;

		$l_type = ($type == 'create') ? 'INDEXING_IN_PROGRESS' : 'DELETING_INDEX_IN_PROGRESS';

		adm_page_header($user->lang[$l_type]);

		$template->set_filenames(array(
			'body'	=> 'progress_bar.html')
		);

		$template->assign_vars(array(
			'L_PROGRESS'			=> $user->lang[$l_type],
			'L_PROGRESS_EXPLAIN'	=> $user->lang[$l_type . '_EXPLAIN'])
		);

		adm_page_footer();
	}

	function close_popup_js()
	{
		return "<script type=\"text/javascript\">\n" .
			"// <![CDATA[\n" .
			"	close_waitscreen = 1;\n" .
			"// ]]>\n" .
			"</script>\n";
	}

	function get_search_types()
	{
		global $phpbb_root_path, $phpEx;

		$search_types = array();

		$dp = @opendir($phpbb_root_path . 'includes/search');

		if ($dp)
		{
			while (($file = readdir($dp)) !== false)
			{
				if ((preg_match('#\.' . $phpEx . '$#', $file)) && ($file != "search.$phpEx"))
				{
					$search_types[] = preg_replace('#^(.*?)\.' . $phpEx . '$#', '\1', $file);
				}
			}
			closedir($dp);

			sort($search_types);
		}

		return $search_types;
	}

	function get_max_post_id()
	{
		global $db;

		$sql = 'SELECT MAX(post_id) as max_post_id
			FROM '. POSTS_TABLE;
		$result = $db->sql_query($sql);
		$max_post_id = (int) $db->sql_fetchfield('max_post_id');
		$db->sql_freeresult($result);

		return $max_post_id;
	}

	function save_state($state = false)
	{
		if ($state)
		{
			$this->state = $state;
		}

		ksort($this->state);

		set_config('search_indexing_state', implode(',', $this->state));
	}

	/**
	* Initialises a search backend object
	*
	* @return false if no error occurred else an error message
	*/
	function init_search($type, &$search, &$error)
	{
		global $phpbb_root_path, $phpEx, $user;

		if (!preg_match('#^\w+$#', $type) || !file_exists("{$phpbb_root_path}includes/search/$type.$phpEx"))
		{
			$error = $user->lang['NO_SUCH_SEARCH_MODULE'];
			return $error;
		}

		include_once("{$phpbb_root_path}includes/search/$type.$phpEx");

		if (!class_exists($type))
		{
			$error = $user->lang['NO_SUCH_SEARCH_MODULE'];
			return $error;
		}

		$error = false;
		$search = new $type($error);

		return $error;
	}
}

?>