<?php
/**
*
* @package phpBB3
* @version $Id: functions_module.php 8479 2008-03-29 00:22:48Z naderman $
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
* Class handling all types of 'plugins' (a future term)
* @package phpBB3
*/
class p_master
{
	var $p_id;
	var $p_class;
	var $p_name;
	var $p_mode;
	var $p_parent;

	var $active_module = false;
	var $active_module_row_id = false;
	var $acl_forum_id = false;
	var $module_ary = array();

	/**
	* List modules
	*
	* This creates a list, stored in $this->module_ary of all available
	* modules for the given class (ucp, mcp and acp). Additionally
	* $this->module_y_ary is created with indentation information for
	* displaying the module list appropriately. Only modules for which
	* the user has access rights are included in these lists.
	*/
	function list_modules($p_class)
	{
		global $auth, $db, $user, $cache;
		global $config, $phpbb_root_path, $phpEx;

		// Sanitise for future path use, it's escaped as appropriate for queries
		$this->p_class = str_replace(array('.', '/', '\\'), '', basename($p_class));

		// Get cached modules
		if (($this->module_cache = $cache->get('_modules_' . $this->p_class)) === false)
		{
			// Get modules
			$sql = 'SELECT *
				FROM ' . MODULES_TABLE . "
				WHERE module_class = '" . $db->sql_escape($this->p_class) . "'
				ORDER BY left_id ASC";
			$result = $db->sql_query($sql);

			$rows = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$rows[$row['module_id']] = $row;
			}
			$db->sql_freeresult($result);

			$this->module_cache = array();
			foreach ($rows as $module_id => $row)
			{
				$this->module_cache['modules'][] = $row;
				$this->module_cache['parents'][$row['module_id']] = $this->get_parents($row['parent_id'], $row['left_id'], $row['right_id'], $rows);
			}
			unset($rows);

			$cache->put('_modules_' . $this->p_class, $this->module_cache);
		}

		if (empty($this->module_cache))
		{
			$this->module_cache = array('modules' => array(), 'parents' => array());
		}

		// We "could" build a true tree with this function - maybe mod authors want to use this...
		// Functions for traversing and manipulating the tree are not available though
		// We might re-structure the module system to use true trees in 3.2.x...
		// $tree = $this->build_tree($this->module_cache['modules'], $this->module_cache['parents']);

		// Clean up module cache array to only let survive modules the user can access
		$right_id = false;
		foreach ($this->module_cache['modules'] as $key => $row)
		{
			// Not allowed to view module?
			if (!$this->module_auth($row['module_auth']))
			{
				unset($this->module_cache['modules'][$key]);
				continue;
			}

			// Category with no members, ignore
			if (!$row['module_basename'] && ($row['left_id'] + 1 == $row['right_id']))
			{
				unset($this->module_cache['modules'][$key]);
				continue;
			}

			// Skip branch
			if ($right_id !== false)
			{
				if ($row['left_id'] < $right_id)
				{
					unset($this->module_cache['modules'][$key]);
					continue;
				}

				$right_id = false;
			}

			// Not enabled?
			if (!$row['module_enabled'])
			{
				// If category is disabled then disable every child too
				unset($this->module_cache['modules'][$key]);
				$right_id = $row['right_id'];
				continue;
			}
		}

		// Re-index (this is needed, else we are not able to array_slice later)
		$this->module_cache['modules'] = array_merge($this->module_cache['modules']);

		// Include MOD _info files for populating language entries within the menus
		$this->add_mod_info($this->p_class);

		// Now build the module array, but exclude completely empty categories...
		$right_id = false;
		$names = array();

		foreach ($this->module_cache['modules'] as $key => $row)
		{
			// Skip branch
			if ($right_id !== false)
			{
				if ($row['left_id'] < $right_id)
				{
					continue;
				}

				$right_id = false;
			}

			// Category with no members on their way down (we have to check every level)
			if (!$row['module_basename'])
			{
				$empty_category = true;

				// We go through the branch and look for an activated module
				foreach (array_slice($this->module_cache['modules'], $key + 1) as $temp_row)
				{
					if ($temp_row['left_id'] > $row['left_id'] && $temp_row['left_id'] < $row['right_id'])
					{
						// Module there
						if ($temp_row['module_basename'] && $temp_row['module_enabled'])
						{
							$empty_category = false;
							break;
						}
						continue;
					}
					break;
				}

				// Skip the branch
				if ($empty_category)
				{
					$right_id = $row['right_id'];
					continue;
				}
			}

			$depth = sizeof($this->module_cache['parents'][$row['module_id']]);

			// We need to prefix the functions to not create a naming conflict

			// Function for building 'url_extra'
			$url_func = '_module_' . $row['module_basename'] . '_url';

			// Function for building the language name
			$lang_func = '_module_' . $row['module_basename'] . '_lang';

			// Custom function for calling parameters on module init (for example assigning template variables)
			$custom_func = '_module_' . $row['module_basename'];

			$names[$row['module_basename'] . '_' . $row['module_mode']][] = true;

			$module_row = array(
				'depth'		=> $depth,

				'id'		=> (int) $row['module_id'],
				'parent'	=> (int) $row['parent_id'],
				'cat'		=> ($row['right_id'] > $row['left_id'] + 1) ? true : false,

				'is_duplicate'	=> ($row['module_basename'] && sizeof($names[$row['module_basename'] . '_' . $row['module_mode']]) > 1) ? true : false,

				'name'		=> (string) $row['module_basename'],
				'mode'		=> (string) $row['module_mode'],
				'display'	=> (int) $row['module_display'],

				'url_extra'	=> (function_exists($url_func)) ? $url_func($row['module_mode'], $row) : '',

				'lang'		=> ($row['module_basename'] && function_exists($lang_func)) ? $lang_func($row['module_mode'], $row['module_langname']) : ((!empty($user->lang[$row['module_langname']])) ? $user->lang[$row['module_langname']] : $row['module_langname']),
				'langname'	=> $row['module_langname'],

				'left'		=> $row['left_id'],
				'right'		=> $row['right_id'],
			);

			if (function_exists($custom_func))
			{
				$custom_func($row['module_mode'], $module_row);
			}

			$this->module_ary[] = $module_row;
		}

		unset($this->module_cache['modules'], $names);
	}

	/**
	* Check if a certain main module is accessible/loaded
	* By giving the module mode you are able to additionally check for only one mode within the main module
	*
	* @param string $module_basename The module base name, for example logs, reports, main (for the mcp).
	* @param mixed $module_mode The module mode to check. If provided the mode will be checked in addition for presence.
	*
	* @return bool Returns true if module is loaded and accessible, else returns false
	*/
	function loaded($module_basename, $module_mode = false)
	{
		if (empty($this->loaded_cache))
		{
			$this->loaded_cache = array();

			foreach ($this->module_ary as $row)
			{
				if (!$row['name'])
				{
					continue;
				}

				if (!isset($this->loaded_cache[$row['name']]))
				{
					$this->loaded_cache[$row['name']] = array();
				}

				if (!$row['mode'])
				{
					continue;
				}

				$this->loaded_cache[$row['name']][$row['mode']] = true;
			}
		}

		if ($module_mode === false)
		{
			return (isset($this->loaded_cache[$module_basename])) ? true : false;
		}

		return (!empty($this->loaded_cache[$module_basename][$module_mode])) ? true : false;
	}

	/**
	* Check module authorisation
	*/
	function module_auth($module_auth, $forum_id = false)
	{
		global $auth, $config;

		$module_auth = trim($module_auth);

		// Generally allowed to access module if module_auth is empty
		if (!$module_auth)
		{
			return true;
		}

		// With the code below we make sure only those elements get eval'd we really want to be checked
		preg_match_all('/(?:
			"[^"\\\\]*(?:\\\\.[^"\\\\]*)*"         |
			\'[^\'\\\\]*(?:\\\\.[^\'\\\\]*)*\'     |
			[(),]                                  |
			[^\s(),]+)/x', $module_auth, $match);

		$tokens = $match[0];
		for ($i = 0, $size = sizeof($tokens); $i < $size; $i++)
		{
			$token = &$tokens[$i];

			switch ($token)
			{
				case ')':
				case '(':
				case '&&':
				case '||':
				case ',':
				break;

				default:
					if (!preg_match('#(?:acl_([a-z0-9_]+)(,\$id)?)|(?:\$id)|(?:aclf_([a-z0-9_]+))|(?:cfg_([a-z0-9_]+))|(?:request_([a-zA-Z0-9_]+))#', $token))
					{
						$token = '';
					}
				break;
			}
		}

		$module_auth = implode(' ', $tokens);

		// Make sure $id seperation is working fine
		$module_auth = str_replace(' , ', ',', $module_auth);

		$forum_id = ($forum_id === false) ? $this->acl_forum_id : $forum_id;

		$is_auth = false;
		eval('$is_auth = (int) (' . preg_replace(array('#acl_([a-z0-9_]+)(,\$id)?#', '#\$id#', '#aclf_([a-z0-9_]+)#', '#cfg_([a-z0-9_]+)#', '#request_([a-zA-Z0-9_]+)#'), array('(int) $auth->acl_get(\'\\1\'\\2)', '(int) $forum_id', '(int) $auth->acl_getf_global(\'\\1\')', '(int) $config[\'\\1\']', '!empty($_REQUEST[\'\\1\'])'), $module_auth) . ');');

		return $is_auth;
	}

	/**
	* Set active module
	*/
	function set_active($id = false, $mode = false)
	{
		$icat = false;
		$this->active_module = false;

		if (request_var('icat', ''))
		{
			$icat = $id;
			$id = request_var('icat', '');
		}

		$category = false;
		foreach ($this->module_ary as $row_id => $item_ary)
		{
			// If this is a module and it's selected, active
			// If this is a category and the module is the first within it, active
			// If this is a module and no mode selected, select first mode
			// If no category or module selected, go active for first module in first category
			if (
				(($item_ary['name'] === $id || $item_ary['id'] === (int) $id) && (($item_ary['mode'] == $mode && !$item_ary['cat']) || ($icat && $item_ary['cat']))) ||
				($item_ary['parent'] === $category && !$item_ary['cat'] && !$icat && $item_ary['display']) ||
				(($item_ary['name'] === $id || $item_ary['id'] === (int) $id) && !$mode && !$item_ary['cat']) ||
				(!$id && !$mode && !$item_ary['cat'] && $item_ary['display'])
				)
			{
				if ($item_ary['cat'])
				{
					$id = $icat;
					$icat = false;

					continue;
				}

				$this->p_id		= $item_ary['id'];
				$this->p_parent	= $item_ary['parent'];
				$this->p_name	= $item_ary['name'];
				$this->p_mode 	= $item_ary['mode'];
				$this->p_left	= $item_ary['left'];
				$this->p_right	= $item_ary['right'];

				$this->module_cache['parents'] = $this->module_cache['parents'][$this->p_id];
				$this->active_module = $item_ary['id'];
				$this->active_module_row_id = $row_id;

				break;
			}
			else if (($item_ary['cat'] && $item_ary['id'] === (int) $id) || ($item_ary['parent'] === $category && $item_ary['cat']))
			{
				$category = $item_ary['id'];
			}
		}
	}

	/**
	* Loads currently active module
	*
	* This method loads a given module, passing it the relevant id and mode.
	*/
	function load_active($mode = false, $module_url = false, $execute_module = true)
	{
		global $phpbb_root_path, $phpbb_admin_path, $phpEx, $user;

		$module_path = $phpbb_root_path . 'includes/' . $this->p_class;
		$icat = request_var('icat', '');

		if ($this->active_module === false)
		{
			trigger_error('Module not accessible', E_USER_ERROR);
		}

		if (!class_exists("{$this->p_class}_$this->p_name"))
		{
			if (!file_exists("$module_path/{$this->p_class}_$this->p_name.$phpEx"))
			{
				trigger_error("Cannot find module $module_path/{$this->p_class}_$this->p_name.$phpEx", E_USER_ERROR);
			}

			include("$module_path/{$this->p_class}_$this->p_name.$phpEx");

			if (!class_exists("{$this->p_class}_$this->p_name"))
			{
				trigger_error("Module file $module_path/{$this->p_class}_$this->p_name.$phpEx does not contain correct class [{$this->p_class}_$this->p_name]", E_USER_ERROR);
			}

			if (!empty($mode))
			{
				$this->p_mode = $mode;
			}

			// Create a new instance of the desired module ... if it has a
			// constructor it will of course be executed
			$instance = "{$this->p_class}_$this->p_name";

			$this->module = new $instance($this);

			// We pre-define the action parameter we are using all over the place
			if (defined('IN_ADMIN'))
			{
				// Is first module automatically enabled a duplicate and the category not passed yet?
				if (!$icat && $this->module_ary[$this->active_module_row_id]['is_duplicate'])
				{
					$icat = $this->module_ary[$this->active_module_row_id]['parent'];
				}

				// Not being able to overwrite ;)
				$this->module->u_action = append_sid("{$phpbb_admin_path}index.$phpEx", "i={$this->p_name}") . (($icat) ? '&amp;icat=' . $icat : '') . "&amp;mode={$this->p_mode}";
			}
			else
			{
				// If user specified the module url we will use it...
				if ($module_url !== false)
				{
					$this->module->u_action = $module_url;
				}
				else
				{
					$this->module->u_action = $phpbb_root_path . (($user->page['page_dir']) ? $user->page['page_dir'] . '/' : '') . $user->page['page_name'];
				}

				$this->module->u_action = append_sid($this->module->u_action, "i={$this->p_name}") . (($icat) ? '&amp;icat=' . $icat : '') . "&amp;mode={$this->p_mode}";
			}

			// Add url_extra parameter to u_action url
			if (!empty($this->module_ary) && $this->active_module !== false && $this->module_ary[$this->active_module_row_id]['url_extra'])
			{
				$this->module->u_action .= $this->module_ary[$this->active_module_row_id]['url_extra'];
			}

			// Assign the module path for re-usage
			$this->module->module_path = $module_path . '/';

			// Execute the main method for the new instance, we send the module id and mode as parameters
			// Users are able to call the main method after this function to be able to assign additional parameters manually
			if ($execute_module)
			{
				$this->module->main($this->p_name, $this->p_mode);
			}

			return;
		}
	}

	/**
	* Appending url parameter to the currently active module.
	*
	* This function is called for adding specific url parameters while executing the current module.
	* It is doing the same as the _module_{name}_url() function, apart from being able to be called after
	* having dynamically parsed specific parameters. This allows more freedom in choosing additional parameters.
	* One example can be seen in /includes/mcp/mcp_notes.php - $this->p_master->adjust_url() call.
	*
	* @param string $url_extra Extra url parameters, e.g.: &amp;u=$user_id
	*
	*/
	function adjust_url($url_extra)
	{
		if (empty($this->module_ary[$this->active_module_row_id]))
		{
			return;
		}

		$row = &$this->module_ary[$this->active_module_row_id];

		// We check for the same url_extra in $row['url_extra'] to overcome doubled additions...
		if (strpos($row['url_extra'], $url_extra) === false)
		{
			$row['url_extra'] .= $url_extra;
		}
	}

	/**
	* Check if a module is active
	*/
	function is_active($id, $mode = false)
	{
		// If we find a name by this id and being enabled we have our active one...
		foreach ($this->module_ary as $row_id => $item_ary)
		{
			if (($item_ary['name'] === $id || $item_ary['id'] === (int) $id) && $item_ary['display'])
			{
				if ($mode === false || $mode === $item_ary['mode'])
				{
					return true;
				}
			}
		}

		return false;
	}

	/**
	* Get parents
	*/
	function get_parents($parent_id, $left_id, $right_id, &$all_parents)
	{
		global $db;

		$parents = array();

		if ($parent_id > 0)
		{
			foreach ($all_parents as $module_id => $row)
			{
				if ($row['left_id'] < $left_id && $row['right_id'] > $right_id)
				{
					$parents[$module_id] = $row['parent_id'];
				}

				if ($row['left_id'] > $left_id)
				{
					break;
				}
			}
		}

		return $parents;
	}

	/**
	* Get tree branch
	*/
	function get_branch($left_id, $right_id, $remaining)
	{
		$branch = array();

		foreach ($remaining as $key => $row)
		{
			if ($row['left_id'] > $left_id && $row['left_id'] < $right_id)
			{
				$branch[] = $row;
				continue;
			}
			break;
		}

		return $branch;
	}

	/**
	* Build true binary tree from given array
	* Not in use
	*/
	function build_tree(&$modules, &$parents)
	{
		$tree = array();

		foreach ($modules as $row)
		{
			$branch = &$tree;

			if ($row['parent_id'])
			{
				// Go through the tree to find our branch
				$parent_tree = $parents[$row['module_id']];

				foreach ($parent_tree as $id => $value)
				{
					if (!isset($branch[$id]) && isset($branch['child']))
					{
						$branch = &$branch['child'];
					}
					$branch = &$branch[$id];
				}
				$branch = &$branch['child'];
			}

			$branch[$row['module_id']] = $row;
			if (!isset($branch[$row['module_id']]['child']))
			{
				$branch[$row['module_id']]['child'] = array();
			}
		}

		return $tree;
	}

	/**
	* Build navigation structure
	*/
	function assign_tpl_vars($module_url)
	{
		global $template;

		$current_id = $right_id = false;

		// Make sure the module_url has a question mark set, effectively determining the delimiter to use
		$delim = (strpos($module_url, '?') === false) ? '?' : '&amp;';

		$current_padding = $current_depth = 0;
		$linear_offset 	= 'l_block1';
		$tabular_offset = 't_block2';

		// Generate the list of modules, we'll do this in two ways ...
		// 1) In a linear fashion
		// 2) In a combined tabbed + linear fashion ... tabs for the categories
		//    and a linear list for subcategories/items
		foreach ($this->module_ary as $row_id => $item_ary)
		{
			// Skip hidden modules
			if (!$item_ary['display'])
			{
				continue;
			}

			// Skip branch
			if ($right_id !== false)
			{
				if ($item_ary['left'] < $right_id)
				{
					continue;
				}

				$right_id = false;
			}

			// Category with no members on their way down (we have to check every level)
			if (!$item_ary['name'])
			{
				$empty_category = true;

				// We go through the branch and look for an activated module
				foreach (array_slice($this->module_ary, $row_id + 1) as $temp_row)
				{
					if ($temp_row['left'] > $item_ary['left'] && $temp_row['left'] < $item_ary['right'])
					{
						// Module there and displayed?
						if ($temp_row['name'] && $temp_row['display'])
						{
							$empty_category = false;
							break;
						}
						continue;
					}
					break;
				}

				// Skip the branch
				if ($empty_category)
				{
					$right_id = $item_ary['right'];
					continue;
				}
			}

			// Select first id we can get
			if (!$current_id && (isset($this->module_cache['parents'][$item_ary['id']]) || $item_ary['id'] == $this->p_id))
			{
				$current_id = $item_ary['id'];
			}

			$depth = $item_ary['depth'];

			if ($depth > $current_depth)
			{
				$linear_offset = $linear_offset . '.l_block' . ($depth + 1);
				$tabular_offset = ($depth + 1 > 2) ? $tabular_offset . '.t_block' . ($depth + 1) : $tabular_offset;
			}
			else if ($depth < $current_depth)
			{
				for ($i = $current_depth - $depth; $i > 0; $i--)
				{
					$linear_offset = substr($linear_offset, 0, strrpos($linear_offset, '.'));
					$tabular_offset = ($i + $depth > 1) ? substr($tabular_offset, 0, strrpos($tabular_offset, '.')) : $tabular_offset;
				}
			}

			$u_title = $module_url . $delim . 'i=' . (($item_ary['cat']) ? $item_ary['id'] : $item_ary['name'] . (($item_ary['is_duplicate']) ? '&amp;icat=' . $current_id : '') . '&amp;mode=' . $item_ary['mode']);

			// Was not allowed in categories before - /*!$item_ary['cat'] && */
			$u_title .= (isset($item_ary['url_extra'])) ? $item_ary['url_extra'] : '';

			// Only output a categories items if it's currently selected
			if (!$depth || ($depth && (in_array($item_ary['parent'], array_values($this->module_cache['parents'])) || $item_ary['parent'] == $this->p_parent)))
			{
				$use_tabular_offset = (!$depth) ? 't_block1' : $tabular_offset;

				$tpl_ary = array(
					'L_TITLE'		=> $item_ary['lang'],
					'S_SELECTED'	=> (isset($this->module_cache['parents'][$item_ary['id']]) || $item_ary['id'] == $this->p_id) ? true : false,
					'U_TITLE'		=> $u_title
				);

				$template->assign_block_vars($use_tabular_offset, array_merge($tpl_ary, array_change_key_case($item_ary, CASE_UPPER)));
			}

			$tpl_ary = array(
				'L_TITLE'		=> $item_ary['lang'],
				'S_SELECTED'	=> (isset($this->module_cache['parents'][$item_ary['id']]) || $item_ary['id'] == $this->p_id) ? true : false,
				'U_TITLE'		=> $u_title
			);

			$template->assign_block_vars($linear_offset, array_merge($tpl_ary, array_change_key_case($item_ary, CASE_UPPER)));

			$current_depth = $depth;
		}
	}

	/**
	* Returns desired template name
	*/
	function get_tpl_name()
	{
		return $this->module->tpl_name . '.html';
	}

	/**
	* Returns the desired page title
	*/
	function get_page_title()
	{
		global $user;

		if (!isset($this->module->page_title))
		{
			return '';
		}

		return (isset($user->lang[$this->module->page_title])) ? $user->lang[$this->module->page_title] : $this->module->page_title;
	}

	/**
	* Load module as the current active one without the need for registering it
	*/
	function load($class, $name, $mode = false)
	{
		$this->p_class = $class;
		$this->p_name = $name;

		// Set active module to true instead of using the id
		$this->active_module = true;

		$this->load_active($mode);
	}

	/**
	* Display module
	*/
	function display($page_title, $display_online_list = true)
	{
		global $template, $user;

		// Generate the page
		if (defined('IN_ADMIN') && isset($user->data['session_admin']) && $user->data['session_admin'])
		{
			adm_page_header($page_title);
		}
		else
		{
			page_header($page_title, $display_online_list);
		}

		$template->set_filenames(array(
			'body' => $this->get_tpl_name())
		);

		if (defined('IN_ADMIN') && isset($user->data['session_admin']) && $user->data['session_admin'])
		{
			adm_page_footer();
		}
		else
		{
			page_footer();
		}
	}

	/**
	* Toggle whether this module will be displayed or not
	*/
	function set_display($id, $mode = false, $display = true)
	{
		foreach ($this->module_ary as $row_id => $item_ary)
		{
			if (($item_ary['name'] === $id || $item_ary['id'] === (int) $id) && (!$mode || $item_ary['mode'] === $mode))
			{
				$this->module_ary[$row_id]['display'] = (int) $display;
			}
		}
	}

	/**
	* Add custom MOD info language file
	*/
	function add_mod_info($module_class)
	{
		global $user, $phpEx;

		if (file_exists($user->lang_path . 'mods'))
		{
			$add_files = array();

			$dir = @opendir($user->lang_path . 'mods');

			if ($dir)
			{
				while (($entry = readdir($dir)) !== false)
				{
					if (strpos($entry, 'info_' . strtolower($module_class) . '_') === 0 && substr(strrchr($entry, '.'), 1) == $phpEx)
					{
						$add_files[] = 'mods/' . substr(basename($entry), 0, -(strlen($phpEx) + 1));
					}
				}
				closedir($dir);
			}

			if (sizeof($add_files))
			{
				$user->add_lang($add_files);
			}
		}
	}
}

?>