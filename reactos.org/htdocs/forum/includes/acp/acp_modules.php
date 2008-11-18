<?php
/**
*
* @package acp
* @version $Id: acp_modules.php 8479 2008-03-29 00:22:48Z naderman $
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
* - Able to check for new module versions (modes changed/adjusted/added/removed)
* Icons for:
* - module enabled and displayed (common)
* - module enabled and not displayed
* - module deactivated
* - category (enabled)
* - category disabled
*/

/**
* @package acp
*/
class acp_modules
{
	var $module_class = '';
	var $parent_id;
	var $u_action;

	function main($id, $mode)
	{
		global $db, $user, $auth, $template, $module;
		global $config, $phpbb_admin_path, $phpbb_root_path, $phpEx;

		// Set a global define for modules we might include (the author is able to prevent execution of code by checking this constant)
		define('MODULE_INCLUDE', true);

		$user->add_lang('acp/modules');
		$this->tpl_name = 'acp_modules';

		// module class
		$this->module_class = $mode;

		if ($this->module_class == 'ucp')
		{
			$user->add_lang('ucp');
		}
		else if ($this->module_class == 'mcp')
		{
			$user->add_lang('mcp');
		}

		if ($module->p_class != $this->module_class)
		{
			$module->add_mod_info($this->module_class);
		}

		$this->page_title = strtoupper($this->module_class);

		$this->parent_id = request_var('parent_id', 0);
		$module_id = request_var('m', 0);
		$action = request_var('action', '');
		$errors = array();

		switch ($action)
		{
			case 'delete':
				if (!$module_id)
				{
					trigger_error($user->lang['NO_MODULE_ID'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				if (confirm_box(true))
				{
					// Make sure we are not directly within a module
					if ($module_id == $this->parent_id)
					{
						$sql = 'SELECT parent_id
							FROM ' . MODULES_TABLE . '
							WHERE module_id = ' . $module_id;
						$result = $db->sql_query($sql);
						$this->parent_id = (int) $db->sql_fetchfield('parent_id');
						$db->sql_freeresult($result);
					}

					$errors = $this->delete_module($module_id);

					if (!sizeof($errors))
					{
						$this->remove_cache_file();
						trigger_error($user->lang['MODULE_DELETED'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id));
					}
				}
				else
				{
					confirm_box(false, 'DELETE_MODULE', build_hidden_fields(array(
						'i'			=> $id,
						'mode'		=> $mode,
						'parent_id'	=> $this->parent_id,
						'module_id'	=> $module_id,
						'action'	=> $action,
					)));
				}

			break;
			
			case 'enable':
			case 'disable':
				if (!$module_id)
				{
					trigger_error($user->lang['NO_MODULE_ID'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . MODULES_TABLE . "
					WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
						AND module_id = $module_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['NO_MODULE'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$sql = 'UPDATE ' . MODULES_TABLE . '
					SET module_enabled = ' . (($action == 'enable') ? 1 : 0) . "
					WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
						AND module_id = $module_id";
				$db->sql_query($sql);

				add_log('admin', 'LOG_MODULE_' . strtoupper($action), $this->lang_name($row['module_langname']));
				$this->remove_cache_file();

			break;

			case 'move_up':
			case 'move_down':
				if (!$module_id)
				{
					trigger_error($user->lang['NO_MODULE_ID'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$sql = 'SELECT *
					FROM ' . MODULES_TABLE . "
					WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
						AND module_id = $module_id";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					trigger_error($user->lang['NO_MODULE'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				$move_module_name = $this->move_module_by($row, $action, 1);

				if ($move_module_name !== false)
				{
					add_log('admin', 'LOG_MODULE_' . strtoupper($action), $this->lang_name($row['module_langname']), $move_module_name);
					$this->remove_cache_file();
				}
		
			break;

			case 'quickadd':
				$quick_install = request_var('quick_install', '');

				if (confirm_box(true))
				{
					if (!$quick_install || strpos($quick_install, '::') === false)
					{
						break;
					}

					list($module_basename, $module_mode) = explode('::', $quick_install);

					// Check if module name and mode exist...
					$fileinfo = $this->get_module_infos($module_basename);
					$fileinfo = $fileinfo[$module_basename];

					if (isset($fileinfo['modes'][$module_mode]))
					{
						$module_data = array(
							'module_basename'	=> $module_basename,
							'module_enabled'	=> 0,
							'module_display'	=> (isset($fileinfo['modes'][$module_mode]['display'])) ? $fileinfo['modes'][$module_mode]['display'] : 1,
							'parent_id'			=> $this->parent_id,
							'module_class'		=> $this->module_class,
							'module_langname'	=> $fileinfo['modes'][$module_mode]['title'],
							'module_mode'		=> $module_mode,
							'module_auth'		=> $fileinfo['modes'][$module_mode]['auth'],
						);

						$errors = $this->update_module_data($module_data);

						if (!sizeof($errors))
						{
							$this->remove_cache_file();
	
							trigger_error($user->lang['MODULE_ADDED'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id));
						}
					}
				}
				else
				{
					confirm_box(false, 'ADD_MODULE', build_hidden_fields(array(
						'i'			=> $id,
						'mode'		=> $mode,
						'parent_id'	=> $this->parent_id,
						'action'	=> 'quickadd',
						'quick_install'	=> $quick_install,
					)));
				}

			break;

			case 'edit':

				if (!$module_id)
				{
					trigger_error($user->lang['NO_MODULE_ID'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}
				
				$module_row = $this->get_module_row($module_id);

			// no break

			case 'add':

				if ($action == 'add')
				{
					$module_row = array(
						'module_basename'	=> '',
						'module_enabled'	=> 0,
						'module_display'	=> 1,
						'parent_id'			=> 0,
						'module_langname'	=> utf8_normalize_nfc(request_var('module_langname', '', true)),
						'module_mode'		=> '',
						'module_auth'		=> '',
					);
				}
				
				$module_data = array();

				$module_data['module_basename'] = request_var('module_basename', (string) $module_row['module_basename']);
				$module_data['module_enabled'] = request_var('module_enabled', (int) $module_row['module_enabled']);
				$module_data['module_display'] = request_var('module_display', (int) $module_row['module_display']);
				$module_data['parent_id'] = request_var('module_parent_id', (int) $module_row['parent_id']);
				$module_data['module_class'] = $this->module_class;
				$module_data['module_langname'] = utf8_normalize_nfc(request_var('module_langname', (string) $module_row['module_langname'], true));
				$module_data['module_mode'] = request_var('module_mode', (string) $module_row['module_mode']);

				$submit = (isset($_POST['submit'])) ? true : false;

				if ($submit)
				{
					if (!$module_data['module_langname'])
					{
						trigger_error($user->lang['NO_MODULE_LANGNAME'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
					}

					$module_type = request_var('module_type', 'category');

					if ($module_type == 'category')
					{
						$module_data['module_basename'] = $module_data['module_mode'] = $module_data['module_auth'] = '';
						$module_data['module_display'] = 1;
					}

					if ($action == 'edit')
					{
						$module_data['module_id'] = $module_id;
					}

					// Adjust auth row
					if ($module_data['module_basename'] && $module_data['module_mode'])
					{
						$fileinfo = $this->get_module_infos($module_data['module_basename']);
						$module_data['module_auth'] = $fileinfo[$module_data['module_basename']]['modes'][$module_data['module_mode']]['auth'];
					}

					$errors = $this->update_module_data($module_data);

					if (!sizeof($errors))
					{
						$this->remove_cache_file();
	
						trigger_error((($action == 'add') ? $user->lang['MODULE_ADDED'] : $user->lang['MODULE_EDITED']) . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id));
					}
				}

				// Category/not category?
				$is_cat = (!$module_data['module_basename']) ? true : false;

				// Get module information
				$module_infos = $this->get_module_infos();

				// Build name options
				$s_name_options = $s_mode_options = '';
				foreach ($module_infos as $option => $values)
				{
					if (!$module_data['module_basename'])
					{
						$module_data['module_basename'] = $option;
					}

					// Name options
					$s_name_options .= '<option value="' . $option . '"' . (($option == $module_data['module_basename']) ? ' selected="selected"' : '') . '>' . $this->lang_name($values['title']) . ' [' . $this->module_class . '_' . $option . ']</option>';

					$template->assign_block_vars('m_names', array('NAME' => $option, 'A_NAME' => addslashes($option)));

					// Build module modes
					foreach ($values['modes'] as $m_mode => $m_values)
					{
						if ($option == $module_data['module_basename'])
						{
							$s_mode_options .= '<option value="' . $m_mode . '"' . (($m_mode == $module_data['module_mode']) ? ' selected="selected"' : '') . '>' . $this->lang_name($m_values['title']) . '</option>';
						}
						
						$template->assign_block_vars('m_names.modes', array(
							'OPTION'		=> $m_mode,
							'VALUE'			=> $this->lang_name($m_values['title']),
							'A_OPTION'		=> addslashes($m_mode),
							'A_VALUE'		=> addslashes($this->lang_name($m_values['title'])))
						);
					}
				}
				
				$s_cat_option = '<option value="0"' . (($module_data['parent_id'] == 0) ? ' selected="selected"' : '') . '>' . $user->lang['NO_PARENT'] . '</option>';

				$template->assign_vars(array_merge(array(
					'S_EDIT_MODULE'		=> true,
					'S_IS_CAT'			=> $is_cat,
					'S_CAT_OPTIONS'		=> $s_cat_option . $this->make_module_select($module_data['parent_id'], ($action == 'edit') ? $module_row['module_id'] : false, false, false, false, true),
					'S_MODULE_NAMES'	=> $s_name_options,
					'S_MODULE_MODES'	=> $s_mode_options,
					'U_BACK'			=> $this->u_action . '&amp;parent_id=' . $this->parent_id,
					'U_EDIT_ACTION'		=> $this->u_action . '&amp;parent_id=' . $this->parent_id,

					'L_TITLE'			=> $user->lang[strtoupper($action) . '_MODULE'],
					
					'MODULENAME'		=> $this->lang_name($module_data['module_langname']),
					'ACTION'			=> $action,
					'MODULE_ID'			=> $module_id,

				),
					array_change_key_case($module_data, CASE_UPPER))
				);

				if (sizeof($errors))
				{
					$template->assign_vars(array(
						'S_ERROR'	=> true,
						'ERROR_MSG'	=> implode('<br />', $errors))
					);
				}

				return;

			break;
		}

		// Default management page
		if (sizeof($errors))
		{
			$template->assign_vars(array(
				'S_ERROR'	=> true,
				'ERROR_MSG'	=> implode('<br />', $errors))
			);
		}

		if (!$this->parent_id)
		{
			$navigation = strtoupper($this->module_class);
		}
		else
		{
			$navigation = '<a href="' . $this->u_action . '">' . strtoupper($this->module_class) . '</a>';

			$modules_nav = $this->get_module_branch($this->parent_id, 'parents', 'descending');

			foreach ($modules_nav as $row)
			{
				$langname = $this->lang_name($row['module_langname']);

				if ($row['module_id'] == $this->parent_id)
				{
					$navigation .= ' -&gt; ' . $langname;
				}
				else
				{
					$navigation .= ' -&gt; <a href="' . $this->u_action . '&amp;parent_id=' . $row['module_id'] . '">' . $langname . '</a>';
				}
			}
		}

		// Jumpbox
		$module_box = $this->make_module_select($this->parent_id, false, false, false, false);

		$sql = 'SELECT *
			FROM ' . MODULES_TABLE . "
			WHERE parent_id = {$this->parent_id}
				AND module_class = '" . $db->sql_escape($this->module_class) . "'
			ORDER BY left_id";
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			do
			{
				$langname = $this->lang_name($row['module_langname']);

				if (!$row['module_enabled'])
				{
					$module_image = '<img src="images/icon_folder_lock.gif" alt="' . $user->lang['DEACTIVATED_MODULE'] .'" />';
				}
				else
				{
					$module_image = (!$row['module_basename'] || $row['left_id'] + 1 != $row['right_id']) ? '<img src="images/icon_subfolder.gif" alt="' . $user->lang['CATEGORY'] . '" />' : '<img src="images/icon_folder.gif" alt="' . $user->lang['MODULE'] . '" />';
				}

				$url = $this->u_action . '&amp;parent_id=' . $this->parent_id . '&amp;m=' . $row['module_id'];

				$template->assign_block_vars('modules', array(
					'MODULE_IMAGE'		=> $module_image,
					'MODULE_TITLE'		=> $langname,
					'MODULE_ENABLED'	=> ($row['module_enabled']) ? true : false,
					'MODULE_DISPLAYED'	=> ($row['module_display']) ? true : false,

					'S_ACP_CAT_SYSTEM'			=> ($this->module_class == 'acp' && $row['module_langname'] == 'ACP_CAT_SYSTEM') ? true : false,
					'S_ACP_MODULE_MANAGEMENT'	=> ($this->module_class == 'acp' && ($row['module_basename'] == 'modules' || $row['module_langname'] == 'ACP_MODULE_MANAGEMENT')) ? true : false,

					'U_MODULE'			=> $this->u_action . '&amp;parent_id=' . $row['module_id'],
					'U_MOVE_UP'			=> $url . '&amp;action=move_up',
					'U_MOVE_DOWN'		=> $url . '&amp;action=move_down',
					'U_EDIT'			=> $url . '&amp;action=edit',
					'U_DELETE'			=> $url . '&amp;action=delete',
					'U_ENABLE'			=> $url . '&amp;action=enable',
					'U_DISABLE'			=> $url . '&amp;action=disable')
				);
			}
			while ($row = $db->sql_fetchrow($result));
		}
		else if ($this->parent_id)
		{
			$row = $this->get_module_row($this->parent_id);

			$url = $this->u_action . '&amp;parent_id=' . $this->parent_id . '&amp;m=' . $row['module_id'];

			$template->assign_vars(array(
				'S_NO_MODULES'		=> true,
				'MODULE_TITLE'		=> $langname,
				'MODULE_ENABLED'	=> ($row['module_enabled']) ? true : false,
				'MODULE_DISPLAYED'	=> ($row['module_display']) ? true : false,

				'U_EDIT'			=> $url . '&amp;action=edit',
				'U_DELETE'			=> $url . '&amp;action=delete',
				'U_ENABLE'			=> $url . '&amp;action=enable',
				'U_DISABLE'			=> $url . '&amp;action=disable')
			);
		}
		$db->sql_freeresult($result);

		// Quick adding module
		$module_infos = $this->get_module_infos();

		// Build quick options
		$s_install_options = '';
		foreach ($module_infos as $option => $values)
		{
			// Name options
			$s_install_options .= '<optgroup label="' . $this->lang_name($values['title']) . ' [' . $this->module_class . '_' . $option . ']">';

			// Build module modes
			foreach ($values['modes'] as $m_mode => $m_values)
			{
				$s_install_options .= '<option value="' . $option . '::' . $m_mode . '">&nbsp; &nbsp;' . $this->lang_name($m_values['title']) . '</option>';
			}

			$s_install_options .= '</optgroup>';
		}

		$template->assign_vars(array(
			'U_SEL_ACTION'		=> $this->u_action,
			'U_ACTION'			=> $this->u_action . '&amp;parent_id=' . $this->parent_id,
			'NAVIGATION'		=> $navigation,
			'MODULE_BOX'		=> $module_box,
			'PARENT_ID'			=> $this->parent_id,
			'S_INSTALL_OPTIONS'	=> $s_install_options,
			)
		);
	}

	/**
	* Get row for specified module
	*/
	function get_module_row($module_id)
	{
		global $db, $user;

		$sql = 'SELECT *
			FROM ' . MODULES_TABLE . "
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND module_id = $module_id";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);
		
		if (!$row)
		{
			trigger_error($user->lang['NO_MODULE'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
		}

		return $row;
	}
	
	/**
	* Get available module information from module files
	*/
	function get_module_infos($module = '', $module_class = false)
	{
		global $phpbb_root_path, $phpEx;
		
		$module_class = ($module_class === false) ? $this->module_class : $module_class;

		$directory = $phpbb_root_path . 'includes/' . $module_class . '/info/';
		$fileinfo = array();

		if (!$module)
		{
			$dh = @opendir($directory);

			if (!$dh)
			{
				return $fileinfo;
			}

			while (($file = readdir($dh)) !== false)
			{
				// Is module?
				if (preg_match('/^' . $module_class . '_.+\.' . $phpEx . '$/', $file))
				{
					$class = str_replace(".$phpEx", '', $file) . '_info';

					if (!class_exists($class))
					{
						include($directory . $file);
					}

					// Get module title tag
					if (class_exists($class))
					{
						$c_class = new $class();
						$module_info = $c_class->module();
						$fileinfo[str_replace($module_class . '_', '', $module_info['filename'])] = $module_info;
					}
				}
			}
			closedir($dh);

			ksort($fileinfo);
		}
		else
		{
			$filename = $module_class . '_' . basename($module);
			$class = $module_class . '_' . basename($module) . '_info';

			if (!class_exists($class))
			{
				include($directory . $filename . '.' . $phpEx);
			}

			// Get module title tag
			if (class_exists($class))
			{
				$c_class = new $class();
				$module_info = $c_class->module();
				$fileinfo[str_replace($module_class . '_', '', $module_info['filename'])] = $module_info;
			}
		}
		
		return $fileinfo;
	}

	/**
	* Simple version of jumpbox, just lists modules
	*/
	function make_module_select($select_id = false, $ignore_id = false, $ignore_acl = false, $ignore_nonpost = false, $ignore_emptycat = true, $ignore_noncat = false)
	{
		global $db, $user, $auth, $config;

		$sql = 'SELECT module_id, module_enabled, module_basename, parent_id, module_langname, left_id, right_id, module_auth
			FROM ' . MODULES_TABLE . "
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
			ORDER BY left_id ASC";
		$result = $db->sql_query($sql);

		$right = $iteration = 0;
		$padding_store = array('0' => '');
		$module_list = $padding = '';

		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['left_id'] < $right)
			{
				$padding .= '&nbsp; &nbsp;';
				$padding_store[$row['parent_id']] = $padding;
			}
			else if ($row['left_id'] > $right + 1)
			{
				$padding = (isset($padding_store[$row['parent_id']])) ? $padding_store[$row['parent_id']] : '';
			}

			$right = $row['right_id'];

			if (!$ignore_acl && $row['module_auth'])
			{
				// We use zero as the forum id to check - global setting.
				if (!p_master::module_auth($row['module_auth'], 0))
				{
					continue;
				}
			}

			// ignore this module?
			if ((is_array($ignore_id) && in_array($row['module_id'], $ignore_id)) || $row['module_id'] == $ignore_id)
			{
				continue;
			}

			// empty category
			if (!$row['module_basename'] && ($row['left_id'] + 1 == $row['right_id']) && $ignore_emptycat)
			{
				continue;
			}

			// ignore non-category?
			if ($row['module_basename'] && $ignore_noncat)
			{
				continue;
			}

			$selected = (is_array($select_id)) ? ((in_array($row['module_id'], $select_id)) ? ' selected="selected"' : '') : (($row['module_id'] == $select_id) ? ' selected="selected"' : '');

			$langname = $this->lang_name($row['module_langname']);
			$module_list .= '<option value="' . $row['module_id'] . '"' . $selected . ((!$row['module_enabled']) ? ' class="disabled"' : '') . '>' . $padding . $langname . '</option>';

			$iteration++;
		}
		unset($padding_store);

		return $module_list;
	}

	/**
	* Get module branch
	*/
	function get_module_branch($module_id, $type = 'all', $order = 'descending', $include_module = true)
	{
		global $db;

		switch ($type)
		{
			case 'parents':
				$condition = 'm1.left_id BETWEEN m2.left_id AND m2.right_id';
			break;

			case 'children':
				$condition = 'm2.left_id BETWEEN m1.left_id AND m1.right_id';
			break;

			default:
				$condition = 'm2.left_id BETWEEN m1.left_id AND m1.right_id OR m1.left_id BETWEEN m2.left_id AND m2.right_id';
			break;
		}

		$rows = array();

		$sql = 'SELECT m2.*
			FROM ' . MODULES_TABLE . ' m1
			LEFT JOIN ' . MODULES_TABLE . " m2 ON ($condition)
			WHERE m1.module_class = '" . $db->sql_escape($this->module_class) . "'
				AND m2.module_class = '" . $db->sql_escape($this->module_class) . "'
				AND m1.module_id = $module_id
			ORDER BY m2.left_id " . (($order == 'descending') ? 'ASC' : 'DESC');
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			if (!$include_module && $row['module_id'] == $module_id)
			{
				continue;
			}

			$rows[] = $row;
		}
		$db->sql_freeresult($result);

		return $rows;
	}

	/**
	* Remove modules cache file
	*/
	function remove_cache_file()
	{
		global $cache;

		// Sanitise for future path use, it's escaped as appropriate for queries
		$p_class = str_replace(array('.', '/', '\\'), '', basename($this->module_class));
		
		$cache->destroy('_modules_' . $p_class);

		// Additionally remove sql cache
		$cache->destroy('sql', MODULES_TABLE);
	}

	/**
	* Return correct language name
	*/
	function lang_name($module_langname)
	{
		global $user;

		return (!empty($user->lang[$module_langname])) ? $user->lang[$module_langname] : $module_langname;
	}

	/**
	* Update/Add module
	*
	* @param bool $run_inline if set to true errors will be returned and no logs being written
	*/
	function update_module_data(&$module_data, $run_inline = false)
	{
		global $db, $user;

		if (!isset($module_data['module_id']))
		{
			// no module_id means we're creating a new category/module
			if ($module_data['parent_id'])
			{
				$sql = 'SELECT left_id, right_id
					FROM ' . MODULES_TABLE . "
					WHERE module_class = '" . $db->sql_escape($module_data['module_class']) . "'
						AND module_id = " . (int) $module_data['parent_id'];
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					if ($run_inline)
					{
						return 'PARENT_NO_EXIST';
					}

					trigger_error($user->lang['PARENT_NO_EXIST'] . adm_back_link($this->u_action . '&amp;parent_id=' . $this->parent_id), E_USER_WARNING);
				}

				// Workaround
				$row['left_id'] = (int) $row['left_id'];
				$row['right_id'] = (int) $row['right_id'];

				$sql = 'UPDATE ' . MODULES_TABLE . "
					SET left_id = left_id + 2, right_id = right_id + 2
					WHERE module_class = '" . $db->sql_escape($module_data['module_class']) . "'
						AND left_id > {$row['right_id']}";
				$db->sql_query($sql);

				$sql = 'UPDATE ' . MODULES_TABLE . "
					SET right_id = right_id + 2
					WHERE module_class = '" . $db->sql_escape($module_data['module_class']) . "'
						AND {$row['left_id']} BETWEEN left_id AND right_id";
				$db->sql_query($sql);

				$module_data['left_id'] = (int) $row['right_id'];
				$module_data['right_id'] = (int) $row['right_id'] + 1;
			}
			else
			{
				$sql = 'SELECT MAX(right_id) AS right_id
					FROM ' . MODULES_TABLE . "
					WHERE module_class = '" . $db->sql_escape($module_data['module_class']) . "'";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$module_data['left_id'] = (int) $row['right_id'] + 1;
				$module_data['right_id'] = (int) $row['right_id'] + 2;
			}

			$sql = 'INSERT INTO ' . MODULES_TABLE . ' ' . $db->sql_build_array('INSERT', $module_data);
			$db->sql_query($sql);

			$module_data['module_id'] = $db->sql_nextid();

			if (!$run_inline)
			{
				add_log('admin', 'LOG_MODULE_ADD', $this->lang_name($module_data['module_langname']));
			}
		}
		else
		{
			$row = $this->get_module_row($module_data['module_id']);

			if ($module_data['module_basename'] && !$row['module_basename'])
			{
				// we're turning a category into a module
				$branch = $this->get_module_branch($module_data['module_id'], 'children', 'descending', false);

				if (sizeof($branch))
				{
					return array($user->lang['NO_CATEGORY_TO_MODULE']);
				}
			}

			if ($row['parent_id'] != $module_data['parent_id'])
			{
				$this->move_module($module_data['module_id'], $module_data['parent_id']);
			}

			$update_ary = $module_data;
			unset($update_ary['module_id']);

			$sql = 'UPDATE ' . MODULES_TABLE . '
				SET ' . $db->sql_build_array('UPDATE', $update_ary) . "
				WHERE module_class = '" . $db->sql_escape($module_data['module_class']) . "'
					AND module_id = " . (int) $module_data['module_id'];
			$db->sql_query($sql);

			if (!$run_inline)
			{
				add_log('admin', 'LOG_MODULE_EDIT', $this->lang_name($module_data['module_langname']));
			}
		}

		return array();
	}

	/**
	* Move module around the tree
	*/
	function move_module($from_module_id, $to_parent_id)
	{
		global $db;

		$moved_modules = $this->get_module_branch($from_module_id, 'children', 'descending');
		$from_data = $moved_modules[0];
		$diff = sizeof($moved_modules) * 2;

		$moved_ids = array();
		for ($i = 0; $i < sizeof($moved_modules); ++$i)
		{
			$moved_ids[] = $moved_modules[$i]['module_id'];
		}

		// Resync parents
		$sql = 'UPDATE ' . MODULES_TABLE . "
			SET right_id = right_id - $diff
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND left_id < " . (int) $from_data['right_id'] . '
				AND right_id > ' . (int) $from_data['right_id'];
		$db->sql_query($sql);

		// Resync righthand side of tree
		$sql = 'UPDATE ' . MODULES_TABLE . "
			SET left_id = left_id - $diff, right_id = right_id - $diff
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND left_id > " . (int) $from_data['right_id'];
		$db->sql_query($sql);

		if ($to_parent_id > 0)
		{
			$to_data = $this->get_module_row($to_parent_id);

			// Resync new parents
			$sql = 'UPDATE ' . MODULES_TABLE . "
				SET right_id = right_id + $diff
				WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
					AND " . (int) $to_data['right_id'] . ' BETWEEN left_id AND right_id
					AND ' . $db->sql_in_set('module_id', $moved_ids, true);
			$db->sql_query($sql);

			// Resync the righthand side of the tree
			$sql = 'UPDATE ' . MODULES_TABLE . "
				SET left_id = left_id + $diff, right_id = right_id + $diff
				WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
					AND left_id > " . (int) $to_data['right_id'] . '
					AND ' . $db->sql_in_set('module_id', $moved_ids, true);
			$db->sql_query($sql);

			// Resync moved branch
			$to_data['right_id'] += $diff;
			if ($to_data['right_id'] > $from_data['right_id'])
			{
				$diff = '+ ' . ($to_data['right_id'] - $from_data['right_id'] - 1);
			}
			else
			{
				$diff = '- ' . abs($to_data['right_id'] - $from_data['right_id'] - 1);
			}
		}
		else
		{
			$sql = 'SELECT MAX(right_id) AS right_id
				FROM ' . MODULES_TABLE . "
				WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
					AND " . $db->sql_in_set('module_id', $moved_ids, true);
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$diff = '+ ' . (int) ($row['right_id'] - $from_data['left_id'] + 1);
		}

		$sql = 'UPDATE ' . MODULES_TABLE . "
			SET left_id = left_id $diff, right_id = right_id $diff
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND " . $db->sql_in_set('module_id', $moved_ids);
		$db->sql_query($sql);
	}

	/**
	* Remove module from tree
	*/
	function delete_module($module_id)
	{
		global $db, $user;

		$row = $this->get_module_row($module_id);

		$branch = $this->get_module_branch($module_id, 'children', 'descending', false);

		if (sizeof($branch))
		{
			return array($user->lang['CANNOT_REMOVE_MODULE']);
		}

		// If not move
		$diff = 2;
		$sql = 'DELETE FROM ' . MODULES_TABLE . "
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND module_id = $module_id";
		$db->sql_query($sql);

		$row['right_id'] = (int) $row['right_id'];
		$row['left_id'] = (int) $row['left_id'];

		// Resync tree
		$sql = 'UPDATE ' . MODULES_TABLE . "
			SET right_id = right_id - $diff
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND left_id < {$row['right_id']} AND right_id > {$row['right_id']}";
		$db->sql_query($sql);

		$sql = 'UPDATE ' . MODULES_TABLE . "
			SET left_id = left_id - $diff, right_id = right_id - $diff
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND left_id > {$row['right_id']}";
		$db->sql_query($sql);

		add_log('admin', 'LOG_MODULE_REMOVED', $this->lang_name($row['module_langname']));

		return array();

	}

	/**
	* Move module position by $steps up/down
	*/
	function move_module_by($module_row, $action = 'move_up', $steps = 1)
	{
		global $db;

		/**
		* Fetch all the siblings between the module's current spot
		* and where we want to move it to. If there are less than $steps
		* siblings between the current spot and the target then the
		* module will move as far as possible
		*/
		$sql = 'SELECT module_id, left_id, right_id, module_langname
			FROM ' . MODULES_TABLE . "
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND parent_id = " . (int) $module_row['parent_id'] . '
				AND ' . (($action == 'move_up') ? 'right_id < ' . (int) $module_row['right_id'] . ' ORDER BY right_id DESC' : 'left_id > ' . (int) $module_row['left_id'] . ' ORDER BY left_id ASC');
		$result = $db->sql_query_limit($sql, $steps);

		$target = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$target = $row;
		}
		$db->sql_freeresult($result);

		if (!sizeof($target))
		{
			// The module is already on top or bottom
			return false;
		}

		/**
		* $left_id and $right_id define the scope of the nodes that are affected by the move.
		* $diff_up and $diff_down are the values to substract or add to each node's left_id
		* and right_id in order to move them up or down.
		* $move_up_left and $move_up_right define the scope of the nodes that are moving
		* up. Other nodes in the scope of ($left_id, $right_id) are considered to move down.
		*/
		if ($action == 'move_up')
		{
			$left_id = (int) $target['left_id'];
			$right_id = (int) $module_row['right_id'];

			$diff_up = (int) ($module_row['left_id'] - $target['left_id']);
			$diff_down = (int) ($module_row['right_id'] + 1 - $module_row['left_id']);

			$move_up_left = (int) $module_row['left_id'];
			$move_up_right = (int) $module_row['right_id'];
		}
		else
		{
			$left_id = (int) $module_row['left_id'];
			$right_id = (int) $target['right_id'];

			$diff_up = (int) ($module_row['right_id'] + 1 - $module_row['left_id']);
			$diff_down = (int) ($target['right_id'] - $module_row['right_id']);

			$move_up_left = (int) ($module_row['right_id'] + 1);
			$move_up_right = (int) $target['right_id'];
		}

		// Now do the dirty job
		$sql = 'UPDATE ' . MODULES_TABLE . "
			SET left_id = left_id + CASE
				WHEN left_id BETWEEN {$move_up_left} AND {$move_up_right} THEN -{$diff_up}
				ELSE {$diff_down}
			END,
			right_id = right_id + CASE
				WHEN right_id BETWEEN {$move_up_left} AND {$move_up_right} THEN -{$diff_up}
				ELSE {$diff_down}
			END
			WHERE module_class = '" . $db->sql_escape($this->module_class) . "'
				AND left_id BETWEEN {$left_id} AND {$right_id}
				AND right_id BETWEEN {$left_id} AND {$right_id}";
		$db->sql_query($sql);

		$this->remove_cache_file();

		return $this->lang_name($target['module_langname']);
	}
}

?>