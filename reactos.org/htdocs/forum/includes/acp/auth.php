<?php
/**
*
* @package phpBB3
* @version $Id: auth.php 8479 2008-03-29 00:22:48Z naderman $
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
* ACP Permission/Auth class
* @package phpBB3
*/
class auth_admin extends auth
{
	/**
	* Init auth settings
	*/
	function auth_admin()
	{
		global $db, $cache;

		if (($this->acl_options = $cache->get('_acl_options')) === false)
		{
			$sql = 'SELECT auth_option_id, auth_option, is_global, is_local
				FROM ' . ACL_OPTIONS_TABLE . '
				ORDER BY auth_option_id';
			$result = $db->sql_query($sql);

			$global = $local = 0;
			$this->acl_options = array();
			while ($row = $db->sql_fetchrow($result))
			{
				if ($row['is_global'])
				{
					$this->acl_options['global'][$row['auth_option']] = $global++;
				}

				if ($row['is_local'])
				{
					$this->acl_options['local'][$row['auth_option']] = $local++;
				}

				$this->acl_options['id'][$row['auth_option']] = (int) $row['auth_option_id'];
				$this->acl_options['option'][(int) $row['auth_option_id']] = $row['auth_option'];
			}
			$db->sql_freeresult($result);

			$cache->put('_acl_options', $this->acl_options);
		}
	}
	
	/**
	* Get permission mask
	* This function only supports getting permissions of one type (for example a_)
	*
	* @param set|view $mode defines the permissions we get, view gets effective permissions (checking user AND group permissions), set only gets the user or group permission set alone
	* @param mixed $user_id user ids to search for (a user_id or a group_id has to be specified at least)
	* @param mixed $group_id group ids to search for, return group related settings (a user_id or a group_id has to be specified at least)
	* @param mixed $forum_id forum_ids to search for. Defining a forum id also means getting local settings
	* @param string $auth_option the auth_option defines the permission setting to look for (a_ for example)
	* @param local|global $scope the scope defines the permission scope. If local, a forum_id is additionally required
	* @param ACL_NEVER|ACL_NO|ACL_YES $acl_fill defines the mode those permissions not set are getting filled with
	*/
	function get_mask($mode, $user_id = false, $group_id = false, $forum_id = false, $auth_option = false, $scope = false, $acl_fill = ACL_NEVER)
	{
		global $db, $user;

		$hold_ary = array();
		$view_user_mask = ($mode == 'view' && $group_id === false) ? true : false;

		if ($auth_option === false || $scope === false)
		{
			return array();
		}

		$acl_user_function = ($mode == 'set') ? 'acl_user_raw_data' : 'acl_raw_data';

		if (!$view_user_mask)
		{
			if ($forum_id !== false)
			{
				$hold_ary = ($group_id !== false) ? $this->acl_group_raw_data($group_id, $auth_option . '%', $forum_id) : $this->$acl_user_function($user_id, $auth_option . '%', $forum_id);
			}
			else
			{
				$hold_ary = ($group_id !== false) ? $this->acl_group_raw_data($group_id, $auth_option . '%', ($scope == 'global') ? 0 : false) : $this->$acl_user_function($user_id, $auth_option . '%', ($scope == 'global') ? 0 : false);
			}
		}

		// Make sure hold_ary is filled with every setting (prevents missing forums/users/groups)
		$ug_id = ($group_id !== false) ? ((!is_array($group_id)) ? array($group_id) : $group_id) : ((!is_array($user_id)) ? array($user_id) : $user_id);
		$forum_ids = ($forum_id !== false) ? ((!is_array($forum_id)) ? array($forum_id) : $forum_id) : (($scope == 'global') ? array(0) : array());

		// Only those options we need
		$compare_options = array_diff(preg_replace('/^((?!' . $auth_option . ').+)|(' . $auth_option . ')$/', '', array_keys($this->acl_options[$scope])), array(''));

		// If forum_ids is false and the scope is local we actually want to have all forums within the array
		if ($scope == 'local' && !sizeof($forum_ids))
		{
			$sql = 'SELECT forum_id
				FROM ' . FORUMS_TABLE;
			$result = $db->sql_query($sql, 120);

			while ($row = $db->sql_fetchrow($result))
			{
				$forum_ids[] = (int) $row['forum_id'];
			}
			$db->sql_freeresult($result);
		}

		if ($view_user_mask)
		{
			$auth2 = null;

			$sql = 'SELECT user_id, user_permissions, user_type
				FROM ' . USERS_TABLE . '
				WHERE ' . $db->sql_in_set('user_id', $ug_id);
			$result = $db->sql_query($sql);

			while ($userdata = $db->sql_fetchrow($result))
			{
				if ($user->data['user_id'] != $userdata['user_id'])
				{
					$auth2 = new auth();
					$auth2->acl($userdata);
				}
				else
				{
					global $auth;
					$auth2 = &$auth;
				}

				
				$hold_ary[$userdata['user_id']] = array();
				foreach ($forum_ids as $f_id)
				{
					$hold_ary[$userdata['user_id']][$f_id] = array();
					foreach ($compare_options as $option)
					{
						$hold_ary[$userdata['user_id']][$f_id][$option] = $auth2->acl_get($option, $f_id);
					}
				}
			}
			$db->sql_freeresult($result);

			unset($userdata);
			unset($auth2);
		}

		foreach ($ug_id as $_id)
		{
			if (!isset($hold_ary[$_id]))
			{
				$hold_ary[$_id] = array();
			}

			foreach ($forum_ids as $f_id)
			{
				if (!isset($hold_ary[$_id][$f_id]))
				{
					$hold_ary[$_id][$f_id] = array();
				}
			}
		}

		// Now, we need to fill the gaps with $acl_fill. ;)

		// Now switch back to keys
		if (sizeof($compare_options))
		{
			$compare_options = array_combine($compare_options, array_fill(1, sizeof($compare_options), $acl_fill));
		}

		// Defining the user-function here to save some memory
		$return_acl_fill = create_function('$value', 'return ' . $acl_fill . ';');

		// Actually fill the gaps
		if (sizeof($hold_ary))
		{
			foreach ($hold_ary as $ug_id => $row)
			{
				foreach ($row as $id => $options)
				{
					// Do not include the global auth_option
					unset($options[$auth_option]);

					// Not a "fine" solution, but at all it's a 1-dimensional
					// array_diff_key function filling the resulting array values with zeros
					// The differences get merged into $hold_ary (all permissions having $acl_fill set)
					$hold_ary[$ug_id][$id] = array_merge($options,

						array_map($return_acl_fill,
							array_flip(
								array_diff(
									array_keys($compare_options), array_keys($options)
								)
							)
						)
					);
				}
			}
		}
		else
		{
			$hold_ary[($group_id !== false) ? $group_id : $user_id][(int) $forum_id] = $compare_options;
		}

		return $hold_ary;
	}

	/**
	* Get permission mask for roles
	* This function only supports getting masks for one role
	*/
	function get_role_mask($role_id)
	{
		global $db;

		$hold_ary = array();

		// Get users having this role set...
		$sql = 'SELECT user_id, forum_id
			FROM ' . ACL_USERS_TABLE . '
			WHERE auth_role_id = ' . $role_id . '
			ORDER BY forum_id';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$hold_ary[$row['forum_id']]['users'][] = $row['user_id'];
		}
		$db->sql_freeresult($result);

		// Now grab groups...
		$sql = 'SELECT group_id, forum_id
			FROM ' . ACL_GROUPS_TABLE . '
			WHERE auth_role_id = ' . $role_id . '
			ORDER BY forum_id';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$hold_ary[$row['forum_id']]['groups'][] = $row['group_id'];
		}
		$db->sql_freeresult($result);

		return $hold_ary;
	}

	/**
	* Display permission mask (assign to template)
	*/
	function display_mask($mode, $permission_type, &$hold_ary, $user_mode = 'user', $local = false, $group_display = true)
	{
		global $template, $user, $db, $phpbb_root_path, $phpEx;

		// Define names for template loops, might be able to be set
		$tpl_pmask = 'p_mask';
		$tpl_fmask = 'f_mask';
		$tpl_category = 'category';
		$tpl_mask = 'mask';

		$l_acl_type = (isset($user->lang['ACL_TYPE_' . (($local) ? 'LOCAL' : 'GLOBAL') . '_' . strtoupper($permission_type)])) ? $user->lang['ACL_TYPE_' . (($local) ? 'LOCAL' : 'GLOBAL') . '_' . strtoupper($permission_type)] : 'ACL_TYPE_' . (($local) ? 'LOCAL' : 'GLOBAL') . '_' . strtoupper($permission_type);

		// Allow trace for viewing permissions and in user mode
		$show_trace = ($mode == 'view' && $user_mode == 'user') ? true : false;

		// Get names
		if ($user_mode == 'user')
		{
			$sql = 'SELECT user_id as ug_id, username as ug_name
				FROM ' . USERS_TABLE . '
				WHERE ' . $db->sql_in_set('user_id', array_keys($hold_ary)) . '
				ORDER BY username_clean ASC';
		}
		else
		{
			$sql = 'SELECT group_id as ug_id, group_name as ug_name, group_type
				FROM ' . GROUPS_TABLE . '
				WHERE ' . $db->sql_in_set('group_id', array_keys($hold_ary)) . '
				ORDER BY group_type DESC, group_name ASC';
		}
		$result = $db->sql_query($sql);

		$ug_names_ary = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$ug_names_ary[$row['ug_id']] = ($user_mode == 'user') ? $row['ug_name'] : (($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['ug_name']] : $row['ug_name']);
		}
		$db->sql_freeresult($result);

		// Get used forums
		$forum_ids = array();
		foreach ($hold_ary as $ug_id => $row)
		{
			$forum_ids = array_merge($forum_ids, array_keys($row));
		}
		$forum_ids = array_unique($forum_ids);

		$forum_names_ary = array();
		if ($local)
		{
			$forum_names_ary = make_forum_select(false, false, true, false, false, false, true);

			// Remove the disabled ones, since we do not create an option field here...
			foreach ($forum_names_ary as $key => $value)
			{
				if (!$value['disabled'])
				{
					continue;
				}
				unset($forum_names_ary[$key]);
			}
		}
		else
		{
			$forum_names_ary[0] = $l_acl_type;
		}

		// Get available roles
		$sql = 'SELECT *
			FROM ' . ACL_ROLES_TABLE . "
			WHERE role_type = '" . $db->sql_escape($permission_type) . "'
			ORDER BY role_order ASC";
		$result = $db->sql_query($sql);

		$roles = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$roles[$row['role_id']] = $row;
		}
		$db->sql_freeresult($result);

		$cur_roles = $this->acl_role_data($user_mode, $permission_type, array_keys($hold_ary));

		// Build js roles array (role data assignments)
		$s_role_js_array = '';
		
		if (sizeof($roles))
		{
			$s_role_js_array = array();

			// Make sure every role (even if empty) has its array defined
			foreach ($roles as $_role_id => $null)
			{
				$s_role_js_array[$_role_id] = "\n" . 'role_options[' . $_role_id . '] = new Array();' . "\n";
			}

			$sql = 'SELECT r.role_id, o.auth_option, r.auth_setting
				FROM ' . ACL_ROLES_DATA_TABLE . ' r, ' . ACL_OPTIONS_TABLE . ' o
				WHERE o.auth_option_id = r.auth_option_id
					AND ' . $db->sql_in_set('r.role_id', array_keys($roles));
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$flag = substr($row['auth_option'], 0, strpos($row['auth_option'], '_') + 1);
				if ($flag == $row['auth_option'])
				{
					continue;
				}

				$s_role_js_array[$row['role_id']] .= 'role_options[' . $row['role_id'] . '][\'' . addslashes($row['auth_option']) . '\'] = ' . $row['auth_setting'] . '; ';
			}
			$db->sql_freeresult($result);

			$s_role_js_array = implode('', $s_role_js_array);
		}

		$template->assign_var('S_ROLE_JS_ARRAY', $s_role_js_array);
		unset($s_role_js_array);

		// Now obtain memberships
		$user_groups_default = $user_groups_custom = array();
		if ($user_mode == 'user' && $group_display)
		{
			$sql = 'SELECT group_id, group_name, group_type
				FROM ' . GROUPS_TABLE . '
				ORDER BY group_type DESC, group_name ASC';
			$result = $db->sql_query($sql);

			$groups = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$groups[$row['group_id']] = $row;
			}
			$db->sql_freeresult($result);

			$memberships = group_memberships(false, array_keys($hold_ary), false);

			// User is not a member of any group? Bad admin, bad bad admin...
			if ($memberships)
			{
				foreach ($memberships as $row)
				{
					if ($groups[$row['group_id']]['group_type'] == GROUP_SPECIAL)
					{
						$user_groups_default[$row['user_id']][] = $user->lang['G_' . $groups[$row['group_id']]['group_name']];
					}
					else
					{
						$user_groups_custom[$row['user_id']][] = $groups[$row['group_id']]['group_name'];
					}
				}
			}
			unset($memberships, $groups);
		}

		// If we only have one forum id to display or being in local mode and more than one user/group to display,
		// we switch the complete interface to group by user/usergroup instead of grouping by forum
		// To achieve this, we need to switch the array a bit
		if (sizeof($forum_ids) == 1 || ($local && sizeof($ug_names_ary) > 1))
		{
			$hold_ary_temp = $hold_ary;
			$hold_ary = array();
			foreach ($hold_ary_temp as $ug_id => $row)
			{
				foreach ($forum_names_ary as $forum_id => $forum_row)
				{
					if (isset($row[$forum_id]))
					{
						$hold_ary[$forum_id][$ug_id] = $row[$forum_id];
					}
				}
			}
			unset($hold_ary_temp);

			foreach ($hold_ary as $forum_id => $forum_array)
			{
				$content_array = $categories = array();
				$this->build_permission_array($hold_ary[$forum_id], $content_array, $categories, array_keys($ug_names_ary));

				$template->assign_block_vars($tpl_pmask, array(
					'NAME'			=> ($forum_id == 0) ? $forum_names_ary[0] : $forum_names_ary[$forum_id]['forum_name'],
					'PADDING'		=> ($forum_id == 0) ? '' : $forum_names_ary[$forum_id]['padding'],

					'CATEGORIES'	=> implode('</th><th>', $categories),

					'L_ACL_TYPE'	=> $l_acl_type,

					'S_LOCAL'		=> ($local) ? true : false,
					'S_GLOBAL'		=> (!$local) ? true : false,
					'S_NUM_CATS'	=> sizeof($categories),
					'S_VIEW'		=> ($mode == 'view') ? true : false,
					'S_NUM_OBJECTS'	=> sizeof($content_array),
					'S_USER_MODE'	=> ($user_mode == 'user') ? true : false,
					'S_GROUP_MODE'	=> ($user_mode == 'group') ? true : false)
				);

				@reset($content_array);
				while (list($ug_id, $ug_array) = each($content_array))
				{
					// Build role dropdown options
					$current_role_id = (isset($cur_roles[$ug_id][$forum_id])) ? $cur_roles[$ug_id][$forum_id] : 0;

					$s_role_options = '';

					@reset($roles);
					while (list($role_id, $role_row) = each($roles))
					{
						$role_description = (!empty($user->lang[$role_row['role_description']])) ? $user->lang[$role_row['role_description']] : nl2br($role_row['role_description']);
						$role_name = (!empty($user->lang[$role_row['role_name']])) ? $user->lang[$role_row['role_name']] : $role_row['role_name'];

						$title = ($role_description) ? ' title="' . $role_description . '"' : '';
						$s_role_options .= '<option value="' . $role_id . '"' . (($role_id == $current_role_id) ? ' selected="selected"' : '') . $title . '>' . $role_name . '</option>';
					}

					if ($s_role_options)
					{
						$s_role_options = '<option value="0"' . ((!$current_role_id) ? ' selected="selected"' : '') . ' title="' . htmlspecialchars($user->lang['NO_ROLE_ASSIGNED_EXPLAIN']) . '">' . $user->lang['NO_ROLE_ASSIGNED'] . '</option>' . $s_role_options;
					}

					if (!$current_role_id && $mode != 'view')
					{
						$s_custom_permissions = false;

						foreach ($ug_array as $key => $value)
						{
							if ($value['S_NEVER'] || $value['S_YES'])
							{
								$s_custom_permissions = true;
								break;
							}
						}
					}
					else
					{
						$s_custom_permissions = false;
					}

					$template->assign_block_vars($tpl_pmask . '.' . $tpl_fmask, array(
						'NAME'				=> $ug_names_ary[$ug_id],
						'S_ROLE_OPTIONS'	=> $s_role_options,
						'UG_ID'				=> $ug_id,
						'S_CUSTOM'			=> $s_custom_permissions,
						'FORUM_ID'			=> $forum_id)
					);

					$this->assign_cat_array($ug_array, $tpl_pmask . '.' . $tpl_fmask . '.' . $tpl_category, $tpl_mask, $ug_id, $forum_id, $show_trace, ($mode == 'view'));

					unset($content_array[$ug_id]);
				}

				unset($hold_ary[$forum_id]);
			}
		}
		else
		{
			foreach ($ug_names_ary as $ug_id => $ug_name)
			{
				if (!isset($hold_ary[$ug_id]))
				{
					continue;
				}

				$content_array = $categories = array();
				$this->build_permission_array($hold_ary[$ug_id], $content_array, $categories, array_keys($forum_names_ary));

				$template->assign_block_vars($tpl_pmask, array(
					'NAME'			=> $ug_name,
					'CATEGORIES'	=> implode('</th><th>', $categories),

					'USER_GROUPS_DEFAULT'	=> ($user_mode == 'user' && isset($user_groups_default[$ug_id]) && sizeof($user_groups_default[$ug_id])) ? implode(', ', $user_groups_default[$ug_id]) : '',
					'USER_GROUPS_CUSTOM'	=> ($user_mode == 'user' && isset($user_groups_custom[$ug_id]) && sizeof($user_groups_custom[$ug_id])) ? implode(', ', $user_groups_custom[$ug_id]) : '',
					'L_ACL_TYPE'			=> $l_acl_type,

					'S_LOCAL'		=> ($local) ? true : false,
					'S_GLOBAL'		=> (!$local) ? true : false,
					'S_NUM_CATS'	=> sizeof($categories),
					'S_VIEW'		=> ($mode == 'view') ? true : false,
					'S_NUM_OBJECTS'	=> sizeof($content_array),
					'S_USER_MODE'	=> ($user_mode == 'user') ? true : false,
					'S_GROUP_MODE'	=> ($user_mode == 'group') ? true : false)
				);

				@reset($content_array);
				while (list($forum_id, $forum_array) = each($content_array))
				{
					// Build role dropdown options
					$current_role_id = (isset($cur_roles[$ug_id][$forum_id])) ? $cur_roles[$ug_id][$forum_id] : 0;

					$s_role_options = '';

					@reset($roles);
					while (list($role_id, $role_row) = each($roles))
					{
						$role_description = (!empty($user->lang[$role_row['role_description']])) ? $user->lang[$role_row['role_description']] : nl2br($role_row['role_description']);
						$role_name = (!empty($user->lang[$role_row['role_name']])) ? $user->lang[$role_row['role_name']] : $role_row['role_name'];

						$title = ($role_description) ? ' title="' . $role_description . '"' : '';
						$s_role_options .= '<option value="' . $role_id . '"' . (($role_id == $current_role_id) ? ' selected="selected"' : '') . $title . '>' . $role_name . '</option>';
					}

					if ($s_role_options)
					{
						$s_role_options = '<option value="0"' . ((!$current_role_id) ? ' selected="selected"' : '') . ' title="' . htmlspecialchars($user->lang['NO_ROLE_ASSIGNED_EXPLAIN']) . '">' . $user->lang['NO_ROLE_ASSIGNED'] . '</option>' . $s_role_options;
					}

					if (!$current_role_id && $mode != 'view')
					{
						$s_custom_permissions = false;

						foreach ($forum_array as $key => $value)
						{
							if ($value['S_NEVER'] || $value['S_YES'])
							{
								$s_custom_permissions = true;
								break;
							}
						}
					}
					else
					{
						$s_custom_permissions = false;
					}

					$template->assign_block_vars($tpl_pmask . '.' . $tpl_fmask, array(
						'NAME'				=> ($forum_id == 0) ? $forum_names_ary[0] : $forum_names_ary[$forum_id]['forum_name'],
						'PADDING'			=> ($forum_id == 0) ? '' : $forum_names_ary[$forum_id]['padding'],
						'S_ROLE_OPTIONS'	=> $s_role_options,
						'S_CUSTOM'			=> $s_custom_permissions,
						'UG_ID'				=> $ug_id,
						'FORUM_ID'			=> $forum_id)
					);

					$this->assign_cat_array($forum_array, $tpl_pmask . '.' . $tpl_fmask . '.' . $tpl_category, $tpl_mask, $ug_id, $forum_id, $show_trace, ($mode == 'view'));
				}

				unset($hold_ary[$ug_id], $ug_names_ary[$ug_id]);
			}
		}
	}

	/**
	* Display permission mask for roles
	*/
	function display_role_mask(&$hold_ary)
	{
		global $db, $template, $user, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		if (!sizeof($hold_ary))
		{
			return;
		}

		// Get forum names
		$sql = 'SELECT forum_id, forum_name
			FROM ' . FORUMS_TABLE . '
			WHERE ' . $db->sql_in_set('forum_id', array_keys($hold_ary)) . '
			ORDER BY left_id';
		$result = $db->sql_query($sql);

		// If the role is used globally, then reflect that
		$forum_names = (isset($hold_ary[0])) ? array(0 => '') : array();
		while ($row = $db->sql_fetchrow($result))
		{
			$forum_names[$row['forum_id']] = $row['forum_name'];
		}
		$db->sql_freeresult($result);

		foreach ($forum_names as $forum_id => $forum_name)
		{
			$auth_ary = $hold_ary[$forum_id];

			$template->assign_block_vars('role_mask', array(
				'NAME'				=> ($forum_id == 0) ? $user->lang['GLOBAL_MASK'] : $forum_name,
				'FORUM_ID'			=> $forum_id)
			);

			if (isset($auth_ary['users']) && sizeof($auth_ary['users']))
			{
				$sql = 'SELECT user_id, username
					FROM ' . USERS_TABLE . '
					WHERE ' . $db->sql_in_set('user_id', $auth_ary['users']) . '
					ORDER BY username_clean ASC';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$template->assign_block_vars('role_mask.users', array(
						'USER_ID'		=> $row['user_id'],
						'USERNAME'		=> $row['username'],
						'U_PROFILE'		=> append_sid("{$phpbb_root_path}memberlist.$phpEx", "mode=viewprofile&amp;u={$row['user_id']}"))
					);
				}
				$db->sql_freeresult($result);
			}

			if (isset($auth_ary['groups']) && sizeof($auth_ary['groups']))
			{
				$sql = 'SELECT group_id, group_name, group_type
					FROM ' . GROUPS_TABLE . '
					WHERE ' . $db->sql_in_set('group_id', $auth_ary['groups']) . '
					ORDER BY group_type ASC, group_name';
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$template->assign_block_vars('role_mask.groups', array(
						'GROUP_ID'		=> $row['group_id'],
						'GROUP_NAME'	=> ($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['group_name']] : $row['group_name'],
						'U_PROFILE'		=> append_sid("{$phpbb_root_path}memberlist.$phpEx", "mode=group&amp;g={$row['group_id']}"))
					);
				}
				$db->sql_freeresult($result);
			}
		}
	}

	/**
	* NOTE: this function is not in use atm
	* Add a new option to the list ... $options is a hash of form ->
	* $options = array(
	*	'local'		=> array('option1', 'option2', ...),
	*	'global'	=> array('optionA', 'optionB', ...)
	* );
	*/
	function acl_add_option($options)
	{
		global $db, $cache;

		if (!is_array($options))
		{
			return false;
		}

		$cur_options = array();

		$sql = 'SELECT auth_option, is_global, is_local
			FROM ' . ACL_OPTIONS_TABLE . '
			ORDER BY auth_option_id';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['is_global'])
			{
				$cur_options['global'][] = $row['auth_option'];
			}

			if ($row['is_local'])
			{
				$cur_options['local'][] = $row['auth_option'];
			}
		}
		$db->sql_freeresult($result);

		// Here we need to insert new options ... this requires discovering whether
		// an options is global, local or both and whether we need to add an permission
		// set flag (x_)
		$new_options = array('local' => array(), 'global' => array());

		foreach ($options as $type => $option_ary)
		{
			$option_ary = array_unique($option_ary);

			foreach ($option_ary as $option_value)
			{
				if (!in_array($option_value, $cur_options[$type]))
				{
					$new_options[$type][] = $option_value;
				}

				$flag = substr($option_value, 0, strpos($option_value, '_') + 1);

				if (!in_array($flag, $cur_options[$type]) && !in_array($flag, $new_options[$type]))
				{
					$new_options[$type][] = $flag;
				}
			}
		}
		unset($options);

		$options = array();
		$options['local'] = array_diff($new_options['local'], $new_options['global']);
		$options['global'] = array_diff($new_options['global'], $new_options['local']);
		$options['local_global'] = array_intersect($new_options['local'], $new_options['global']);

		$sql_ary = array();

		foreach ($options as $type => $option_ary)
		{
			foreach ($option_ary as $option)
			{
				$sql_ary[] = array(
					'auth_option'	=> (string) $option,
					'is_global'		=> ($type == 'global' || $type == 'local_global') ? 1 : 0,
					'is_local'		=> ($type == 'local' || $type == 'local_global') ? 1 : 0
				);
			}
		}

		$db->sql_multi_insert(ACL_OPTIONS_TABLE, $sql_ary);

		$cache->destroy('_acl_options');
		$this->acl_clear_prefetch();

		// Because we just changed the options and also purged the options cache, we instantly update/regenerate it for later calls to succeed.
		$this->acl_options = array();
		$this->auth_admin();

		return true;
	}

	/**
	* Set a user or group ACL record
	*/
	function acl_set($ug_type, $forum_id, $ug_id, $auth, $role_id = 0, $clear_prefetch = true)
	{
		global $db;

		// One or more forums
		if (!is_array($forum_id))
		{
			$forum_id = array($forum_id);
		}

		// One or more users
		if (!is_array($ug_id))
		{
			$ug_id = array($ug_id);
		}

		$ug_id_sql = $db->sql_in_set($ug_type . '_id', array_map('intval', $ug_id));
		$forum_sql = $db->sql_in_set('forum_id', array_map('intval', $forum_id));

		// Instead of updating, inserting, removing we just remove all current settings and re-set everything...
		$table = ($ug_type == 'user') ? ACL_USERS_TABLE : ACL_GROUPS_TABLE;
		$id_field = $ug_type . '_id';

		// Get any flags as required
		reset($auth);
		$flag = key($auth);
		$flag = substr($flag, 0, strpos($flag, '_') + 1);
		
		// This ID (the any-flag) is set if one or more permissions are true...
		$any_option_id = (int) $this->acl_options['id'][$flag];

		// Remove any-flag from auth ary
		if (isset($auth[$flag]))
		{
			unset($auth[$flag]);
		}

		// Remove current auth options...
		$auth_option_ids = array((int)$any_option_id);
		foreach ($auth as $auth_option => $auth_setting)
		{
			$auth_option_ids[] = (int) $this->acl_options['id'][$auth_option];
		}

		$sql = "DELETE FROM $table
			WHERE $forum_sql
				AND $ug_id_sql
				AND " . $db->sql_in_set('auth_option_id', $auth_option_ids);
		$db->sql_query($sql);

		// Remove those having a role assigned... the correct type of course...
		$sql = 'SELECT role_id
			FROM ' . ACL_ROLES_TABLE . "
			WHERE role_type = '" . $db->sql_escape($flag) . "'";
		$result = $db->sql_query($sql);

		$role_ids = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$role_ids[] = $row['role_id'];
		}
		$db->sql_freeresult($result);

		if (sizeof($role_ids))
		{
			$sql = "DELETE FROM $table
				WHERE $forum_sql
					AND $ug_id_sql
					AND auth_option_id = 0
					AND " . $db->sql_in_set('auth_role_id', $role_ids);
			$db->sql_query($sql);
		}

		// Ok, include the any-flag if one or more auth options are set to yes...
		foreach ($auth as $auth_option => $setting)
		{
			if ($setting == ACL_YES && (!isset($auth[$flag]) || $auth[$flag] == ACL_NEVER))
			{
				$auth[$flag] = ACL_YES;
			}
		}

		$sql_ary = array();
		foreach ($forum_id as $forum)
		{
			$forum = (int) $forum;

			if ($role_id)
			{
				foreach ($ug_id as $id)
				{
					$sql_ary[] = array(
						$id_field			=> (int) $id,
						'forum_id'			=> (int) $forum,
						'auth_option_id'	=> 0,
						'auth_setting'		=> 0,
						'auth_role_id'		=> (int) $role_id,
					);
				}
			}
			else
			{
				foreach ($auth as $auth_option => $setting)
				{
					$auth_option_id = (int) $this->acl_options['id'][$auth_option];

					if ($setting != ACL_NO)
					{
						foreach ($ug_id as $id)
						{
							$sql_ary[] = array(
								$id_field			=> (int) $id,
								'forum_id'			=> (int) $forum,
								'auth_option_id'	=> (int) $auth_option_id,
								'auth_setting'		=> (int) $setting
							);
						}
					}
				}
			}
		}

		$db->sql_multi_insert($table, $sql_ary);

		if ($clear_prefetch)
		{
			$this->acl_clear_prefetch();
		}
	}

	/**
	* Set a role-specific ACL record
	*/
	function acl_set_role($role_id, $auth)
	{
		global $db;

		// Get any-flag as required
		reset($auth);
		$flag = key($auth);
		$flag = substr($flag, 0, strpos($flag, '_') + 1);
		
		// Remove any-flag from auth ary
		if (isset($auth[$flag]))
		{
			unset($auth[$flag]);
		}

		// Re-set any flag...
		foreach ($auth as $auth_option => $setting)
		{
			if ($setting == ACL_YES && (!isset($auth[$flag]) || $auth[$flag] == ACL_NEVER))
			{
				$auth[$flag] = ACL_YES;
			}
		}

		$sql_ary = array();
		foreach ($auth as $auth_option => $setting)
		{
			$auth_option_id = (int) $this->acl_options['id'][$auth_option];

			if ($setting != ACL_NO)
			{
				$sql_ary[] = array(
					'role_id'			=> (int) $role_id,
					'auth_option_id'	=> (int) $auth_option_id,
					'auth_setting'		=> (int) $setting
				);
			}
		}

		// If no data is there, we set the any-flag to ACL_NEVER...
		if (!sizeof($sql_ary))
		{
			$sql_ary[] = array(
				'role_id'			=> (int) $role_id,
				'auth_option_id'	=> (int) $this->acl_options['id'][$flag],
				'auth_setting'		=> ACL_NEVER
			);
		}

		// Remove current auth options...
		$sql = 'DELETE FROM ' . ACL_ROLES_DATA_TABLE . '
			WHERE role_id = ' . $role_id;
		$db->sql_query($sql);

		// Now insert the new values
		$db->sql_multi_insert(ACL_ROLES_DATA_TABLE, $sql_ary);

		$this->acl_clear_prefetch();
	}

	/**
	* Remove local permission
	*/
	function acl_delete($mode, $ug_id = false, $forum_id = false, $permission_type = false)
	{
		global $db;

		if ($ug_id === false && $forum_id === false)
		{
			return;
		}

		$option_id_ary = array();
		$table = ($mode == 'user') ? ACL_USERS_TABLE : ACL_GROUPS_TABLE;
		$id_field = $mode . '_id';

		$where_sql = array();

		if ($forum_id !== false)
		{
			$where_sql[] = (!is_array($forum_id)) ? 'forum_id = ' . (int) $forum_id : $db->sql_in_set('forum_id', array_map('intval', $forum_id));
		}

		if ($ug_id !== false)
		{
			$where_sql[] = (!is_array($ug_id)) ? $id_field . ' = ' . (int) $ug_id : $db->sql_in_set($id_field, array_map('intval', $ug_id));
		}

		// There seem to be auth options involved, therefore we need to go through the list and make sure we capture roles correctly
		if ($permission_type !== false)
		{
			// Get permission type
			$sql = 'SELECT auth_option, auth_option_id
				FROM ' . ACL_OPTIONS_TABLE . "
				WHERE auth_option " . $db->sql_like_expression($permission_type . $db->any_char);
			$result = $db->sql_query($sql);

			$auth_id_ary = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$option_id_ary[] = $row['auth_option_id'];
				$auth_id_ary[$row['auth_option']] = ACL_NO;
			}
			$db->sql_freeresult($result);

			// First of all, lets grab the items having roles with the specified auth options assigned
			$sql = "SELECT auth_role_id, $id_field, forum_id
				FROM $table, " . ACL_ROLES_TABLE . " r
				WHERE auth_role_id <> 0
					AND auth_role_id = r.role_id
					AND r.role_type = '{$permission_type}'
					AND " . implode(' AND ', $where_sql) . '
				ORDER BY auth_role_id';
			$result = $db->sql_query($sql);

			$cur_role_auth = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$cur_role_auth[$row['auth_role_id']][$row['forum_id']][] = $row[$id_field];
			}
			$db->sql_freeresult($result);

			// Get role data for resetting data
			if (sizeof($cur_role_auth))
			{
				$sql = 'SELECT ao.auth_option, rd.role_id, rd.auth_setting
					FROM ' . ACL_OPTIONS_TABLE . ' ao, ' . ACL_ROLES_DATA_TABLE . ' rd
					WHERE ao.auth_option_id = rd.auth_option_id
						AND ' . $db->sql_in_set('rd.role_id', array_keys($cur_role_auth));
				$result = $db->sql_query($sql);

				$auth_settings = array();
				while ($row = $db->sql_fetchrow($result))
				{
					// We need to fill all auth_options, else setting it will fail...
					if (!isset($auth_settings[$row['role_id']]))
					{
						$auth_settings[$row['role_id']] = $auth_id_ary;
					}
					$auth_settings[$row['role_id']][$row['auth_option']] = $row['auth_setting'];
				}
				$db->sql_freeresult($result);

				// Set the options
				foreach ($cur_role_auth as $role_id => $auth_row)
				{
					foreach ($auth_row as $f_id => $ug_row)
					{
						$this->acl_set($mode, $f_id, $ug_row, $auth_settings[$role_id], 0, false);
					}
				}
			}
		}

		// Now, normally remove permissions...
		if ($permission_type !== false)
		{
			$where_sql[] = $db->sql_in_set('auth_option_id', array_map('intval', $option_id_ary));
		}
		
		$sql = "DELETE FROM $table
			WHERE " . implode(' AND ', $where_sql);
		$db->sql_query($sql);

		$this->acl_clear_prefetch();
	}

	/**
	* Assign category to template
	* used by display_mask()
	*/
	function assign_cat_array(&$category_array, $tpl_cat, $tpl_mask, $ug_id, $forum_id, $show_trace = false, $s_view)
	{
		global $template, $user, $phpbb_admin_path, $phpEx;

		@reset($category_array);
		while (list($cat, $cat_array) = each($category_array))
		{
			$template->assign_block_vars($tpl_cat, array(
				'S_YES'		=> ($cat_array['S_YES'] && !$cat_array['S_NEVER'] && !$cat_array['S_NO']) ? true : false,
				'S_NEVER'	=> ($cat_array['S_NEVER'] && !$cat_array['S_YES'] && !$cat_array['S_NO']) ? true : false,
				'S_NO'		=> ($cat_array['S_NO'] && !$cat_array['S_NEVER'] && !$cat_array['S_YES']) ? true : false,
							
				'CAT_NAME'	=> $user->lang['permission_cat'][$cat])
			);

			/*	Sort permissions by name (more naturaly and user friendly than sorting by a primary key)
			*	Commented out due to it's memory consumption and time needed
			*
			$key_array = array_intersect(array_keys($user->lang), array_map(create_function('$a', 'return "acl_" . $a;'), array_keys($cat_array['permissions'])));
			$values_array = $cat_array['permissions'];

			$cat_array['permissions'] = array();

			foreach ($key_array as $key)
			{
				$key = str_replace('acl_', '', $key);
				$cat_array['permissions'][$key] = $values_array[$key];
			}
			unset($key_array, $values_array);
*/
			@reset($cat_array['permissions']);
			while (list($permission, $allowed) = each($cat_array['permissions']))
			{
				if ($s_view)
				{
					$template->assign_block_vars($tpl_cat . '.' . $tpl_mask, array(
						'S_YES'		=> ($allowed == ACL_YES) ? true : false,
						'S_NEVER'	=> ($allowed == ACL_NEVER) ? true : false,

						'UG_ID'			=> $ug_id,
						'FORUM_ID'		=> $forum_id,
						'FIELD_NAME'	=> $permission,
						'S_FIELD_NAME'	=> 'setting[' . $ug_id . '][' . $forum_id . '][' . $permission . ']',

						'U_TRACE'		=> ($show_trace) ? append_sid("{$phpbb_admin_path}index.$phpEx", "i=permissions&amp;mode=trace&amp;u=$ug_id&amp;f=$forum_id&amp;auth=$permission") : '',
						'UA_TRACE'		=> ($show_trace) ? append_sid("{$phpbb_admin_path}index.$phpEx", "i=permissions&mode=trace&u=$ug_id&f=$forum_id&auth=$permission", false) : '',

						'PERMISSION'	=> $user->lang['acl_' . $permission]['lang'])
					);
				}
				else
				{
					$template->assign_block_vars($tpl_cat . '.' . $tpl_mask, array(
						'S_YES'		=> ($allowed == ACL_YES) ? true : false,
						'S_NEVER'	=> ($allowed == ACL_NEVER) ? true : false,
						'S_NO'		=> ($allowed == ACL_NO) ? true : false,

						'UG_ID'			=> $ug_id,
						'FORUM_ID'		=> $forum_id,
						'FIELD_NAME'	=> $permission,
						'S_FIELD_NAME'	=> 'setting[' . $ug_id . '][' . $forum_id . '][' . $permission . ']',

						'U_TRACE'		=> ($show_trace) ? append_sid("{$phpbb_admin_path}index.$phpEx", "i=permissions&amp;mode=trace&amp;u=$ug_id&amp;f=$forum_id&amp;auth=$permission") : '',
						'UA_TRACE'		=> ($show_trace) ? append_sid("{$phpbb_admin_path}index.$phpEx", "i=permissions&mode=trace&u=$ug_id&f=$forum_id&auth=$permission", false) : '',

						'PERMISSION'	=> $user->lang['acl_' . $permission]['lang'])
					);
				}
			}
		}
	}

	/**
	* Building content array from permission rows with explicit key ordering
	* used by display_mask()
	*/
	function build_permission_array(&$permission_row, &$content_array, &$categories, $key_sort_array)
	{
		global $user;

		foreach ($key_sort_array as $forum_id)
		{
			if (!isset($permission_row[$forum_id]))
			{
				continue;
			}

			$permissions = $permission_row[$forum_id];
			ksort($permissions);

			@reset($permissions);
			while (list($permission, $auth_setting) = each($permissions))
			{
				if (!isset($user->lang['acl_' . $permission]))
				{
					$user->lang['acl_' . $permission] = array(
						'cat'	=> 'misc',
						'lang'	=> '{ acl_' . $permission . ' }'
					);
				}
			
				$cat = $user->lang['acl_' . $permission]['cat'];
			
				// Build our categories array
				if (!isset($categories[$cat]))
				{
					$categories[$cat] = $user->lang['permission_cat'][$cat];
				}

				// Build our content array
				if (!isset($content_array[$forum_id]))
				{
					$content_array[$forum_id] = array();
				}

				if (!isset($content_array[$forum_id][$cat]))
				{
					$content_array[$forum_id][$cat] = array(
						'S_YES'			=> false,
						'S_NEVER'		=> false,
						'S_NO'			=> false,
						'permissions'	=> array(),
					);
				}

				$content_array[$forum_id][$cat]['S_YES'] |= ($auth_setting == ACL_YES) ? true : false;
				$content_array[$forum_id][$cat]['S_NEVER'] |= ($auth_setting == ACL_NEVER) ? true : false;
				$content_array[$forum_id][$cat]['S_NO'] |= ($auth_setting == ACL_NO) ? true : false;

				$content_array[$forum_id][$cat]['permissions'][$permission] = $auth_setting;
			}
		}
	}

	/**
	* Use permissions from another user. This transferes a permission set from one user to another.
	* The other user is always able to revert back to his permission set.
	* This function does not check for lower/higher permissions, it is possible for the user to gain
	* "more" permissions by this.
	* Admin permissions will not be copied.
	*/
	function ghost_permissions($from_user_id, $to_user_id)
	{
		global $db;

		if ($to_user_id == ANONYMOUS)
		{
			return false;
		}

		$hold_ary = $this->acl_raw_data_single_user($from_user_id);

		// Key 0 in $hold_ary are global options, all others are forum_ids

		// We disallow copying admin permissions
		foreach ($this->acl_options['global'] as $opt => $id)
		{
			if (strpos($opt, 'a_') === 0)
			{
				$hold_ary[0][$this->acl_options['id'][$opt]] = ACL_NEVER;
			}
		}

		// Force a_switchperm to be allowed
		$hold_ary[0][$this->acl_options['id']['a_switchperm']] = ACL_YES;

		$user_permissions = $this->build_bitstring($hold_ary);

		if (!$user_permissions)
		{
			return false;
		}

		$sql = 'UPDATE ' . USERS_TABLE . "
			SET user_permissions = '" . $db->sql_escape($user_permissions) . "',
				user_perm_from = $from_user_id
			WHERE user_id = " . $to_user_id;
		$db->sql_query($sql);

		return true;
	}
}

?>