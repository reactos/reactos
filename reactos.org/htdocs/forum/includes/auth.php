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
* Permission/Auth class
* @package phpBB3
*/
class auth
{
	var $acl = array();
	var $cache = array();
	var $acl_options = array();
	var $acl_forum_ids = false;

	/**
	* Init permissions
	*/
	function acl(&$userdata)
	{
		global $db, $cache;

		$this->acl = $this->cache = $this->acl_options = array();
		$this->acl_forum_ids = false;

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
			$this->acl_cache($userdata);
		}
		else if (!trim($userdata['user_permissions']))
		{
			$this->acl_cache($userdata);
		}

		$user_permissions = explode("\n", $userdata['user_permissions']);

		foreach ($user_permissions as $f => $seq)
		{
			if ($seq)
			{
				$i = 0;

				if (!isset($this->acl[$f]))
				{
					$this->acl[$f] = '';
				}

				while ($subseq = substr($seq, $i, 6))
				{
					// We put the original bitstring into the acl array
					$this->acl[$f] .= str_pad(base_convert($subseq, 36, 2), 31, 0, STR_PAD_LEFT);
					$i += 6;
				}
			}
		}

		return;
	}

	/**
	* Look up an option
	* if the option is prefixed with !, then the result becomes negated
	*
	* If a forum id is specified the local option will be combined with a global option if one exist.
	* If a forum id is not specified, only the global option will be checked.
	*/
	function acl_get($opt, $f = 0)
	{
		$negate = false;

		if (strpos($opt, '!') === 0)
		{
			$negate = true;
			$opt = substr($opt, 1);
		}

		if (!isset($this->cache[$f][$opt]))
		{
			// We combine the global/local option with an OR because some options are global and local.
			// If the user has the global permission the local one is true too and vice versa
			$this->cache[$f][$opt] = false;

			// Is this option a global permission setting?
			if (isset($this->acl_options['global'][$opt]))
			{
				if (isset($this->acl[0]))
				{
					$this->cache[$f][$opt] = $this->acl[0][$this->acl_options['global'][$opt]];
				}
			}

			// Is this option a local permission setting?
			// But if we check for a global option only, we won't combine the options...
			if ($f != 0 && isset($this->acl_options['local'][$opt]))
			{
				if (isset($this->acl[$f]) && isset($this->acl[$f][$this->acl_options['local'][$opt]]))
				{
					$this->cache[$f][$opt] |= $this->acl[$f][$this->acl_options['local'][$opt]];
				}
			}
		}

		// Founder always has all global options set to true...
		return ($negate) ? !$this->cache[$f][$opt] : $this->cache[$f][$opt];
	}

	/**
	* Get forums with the specified permission setting
	* if the option is prefixed with !, then the result becomes nagated
	*
	* @param bool $clean set to true if only values needs to be returned which are set/unset
	*/
	function acl_getf($opt, $clean = false)
	{
		$acl_f = array();
		$negate = false;

		if (strpos($opt, '!') === 0)
		{
			$negate = true;
			$opt = substr($opt, 1);
		}

		// If we retrieve a list of forums not having permissions in, we need to get every forum_id
		if ($negate)
		{
			if ($this->acl_forum_ids === false)
			{
				global $db;

				$sql = 'SELECT forum_id
					FROM ' . FORUMS_TABLE;
				
				if (sizeof($this->acl))
				{
					$sql .= ' WHERE ' . $db->sql_in_set('forum_id', array_keys($this->acl), true);
				}
				$result = $db->sql_query($sql);

				$this->acl_forum_ids = array();
				while ($row = $db->sql_fetchrow($result))
				{
					$this->acl_forum_ids[] = $row['forum_id'];
				}
				$db->sql_freeresult($result);
			}
		}
		
		if (isset($this->acl_options['local'][$opt]))
		{
			foreach ($this->acl as $f => $bitstring)
			{
				// Skip global settings
				if (!$f)
				{
					continue;
				}

				$allowed = (!isset($this->cache[$f][$opt])) ? $this->acl_get($opt, $f) : $this->cache[$f][$opt];

				if (!$clean)
				{
					$acl_f[$f][$opt] = ($negate) ? !$allowed : $allowed;
				}
				else
				{
					if (($negate && !$allowed) || (!$negate && $allowed))
					{
						$acl_f[$f][$opt] = 1;
					}
				}
			}
		}

		// If we get forum_ids not having this permission, we need to fill the remaining parts
		if ($negate && sizeof($this->acl_forum_ids))
		{
			foreach ($this->acl_forum_ids as $f)
			{
				$acl_f[$f][$opt] = 1;
			}
		}

		return $acl_f;
	}

	/**
	* Get local permission state for any forum.
	*
	* Returns true if user has the permission in one or more forums, false if in no forum.
	* If global option is checked it returns the global state (same as acl_get($opt))
	* Local option has precedence...
	*/
	function acl_getf_global($opt)
	{
		if (is_array($opt))
		{
			// evaluates to true as soon as acl_getf_global is true for one option
			foreach ($opt as $check_option)
			{
				if ($this->acl_getf_global($check_option))
				{
					return true;
				}
			}

			return false;
		}

		if (isset($this->acl_options['local'][$opt]))
		{
			foreach ($this->acl as $f => $bitstring)
			{
				// Skip global settings
				if (!$f)
				{
					continue;
				}

				// as soon as the user has any permission we're done so return true
				if ((!isset($this->cache[$f][$opt])) ? $this->acl_get($opt, $f) : $this->cache[$f][$opt])
				{
					return true;
				}
			}
		}
		else if (isset($this->acl_options['global'][$opt]))
		{
			return $this->acl_get($opt);
		}

		return false;
	}

	/**
	* Get permission settings (more than one)
	*/
	function acl_gets()
	{
		$args = func_get_args();
		$f = array_pop($args);

		if (!is_numeric($f))
		{
			$args[] = $f;
			$f = 0;
		}

		// alternate syntax: acl_gets(array('m_', 'a_'), $forum_id)
		if (is_array($args[0]))
		{
			$args = $args[0];
		}

		$acl = 0;
		foreach ($args as $opt)
		{
			$acl |= $this->acl_get($opt, $f);
		}

		return $acl;
	}

	/**
	* Get permission listing based on user_id/options/forum_ids
	*/
	function acl_get_list($user_id = false, $opts = false, $forum_id = false)
	{
		if ($user_id !== false && !is_array($user_id) && $opts === false && $forum_id === false)
		{
			$hold_ary = array($user_id => $this->acl_raw_data_single_user($user_id));
		}
		else
		{
			$hold_ary = $this->acl_raw_data($user_id, $opts, $forum_id);
		}

		$auth_ary = array();
		foreach ($hold_ary as $user_id => $forum_ary)
		{
			foreach ($forum_ary as $forum_id => $auth_option_ary)
			{
				foreach ($auth_option_ary as $auth_option => $auth_setting)
				{
					if ($auth_setting)
					{
						$auth_ary[$forum_id][$auth_option][] = $user_id;
					}
				}
			}
		}

		return $auth_ary;
	}

	/**
	* Cache data to user_permissions row
	*/
	function acl_cache(&$userdata)
	{
		global $db;

		// Empty user_permissions
		$userdata['user_permissions'] = '';

		$hold_ary = $this->acl_raw_data_single_user($userdata['user_id']);

		// Key 0 in $hold_ary are global options, all others are forum_ids

		// If this user is founder we're going to force fill the admin options ...
		if ($userdata['user_type'] == USER_FOUNDER)
		{
			foreach ($this->acl_options['global'] as $opt => $id)
			{
				if (strpos($opt, 'a_') === 0)
				{
					$hold_ary[0][$this->acl_options['id'][$opt]] = ACL_YES;
				}
			}
		}

		$hold_str = $this->build_bitstring($hold_ary);

		if ($hold_str)
		{
			$userdata['user_permissions'] = $hold_str;

			$sql = 'UPDATE ' . USERS_TABLE . "
				SET user_permissions = '" . $db->sql_escape($userdata['user_permissions']) . "',
					user_perm_from = 0
				WHERE user_id = " . $userdata['user_id'];
			$db->sql_query($sql);
		}

		return;
	}

	/**
	* Build bitstring from permission set
	*/
	function build_bitstring(&$hold_ary)
	{
		$hold_str = '';

		if (sizeof($hold_ary))
		{
			ksort($hold_ary);

			$last_f = 0;

			foreach ($hold_ary as $f => $auth_ary)
			{
				$ary_key = (!$f) ? 'global' : 'local';

				$bitstring = array();
				foreach ($this->acl_options[$ary_key] as $opt => $id)
				{
					if (isset($auth_ary[$this->acl_options['id'][$opt]]))
					{
						$bitstring[$id] = $auth_ary[$this->acl_options['id'][$opt]];

						$option_key = substr($opt, 0, strpos($opt, '_') + 1);

						// If one option is allowed, the global permission for this option has to be allowed too
						// example: if the user has the a_ permission this means he has one or more a_* permissions
						if ($auth_ary[$this->acl_options['id'][$opt]] == ACL_YES && (!isset($bitstring[$this->acl_options[$ary_key][$option_key]]) || $bitstring[$this->acl_options[$ary_key][$option_key]] == ACL_NEVER))
						{
							$bitstring[$this->acl_options[$ary_key][$option_key]] = ACL_YES;
						}
					}
					else
					{
						$bitstring[$id] = ACL_NEVER;
					}
				}

				// Now this bitstring defines the permission setting for the current forum $f (or global setting)
				$bitstring = implode('', $bitstring);

				// The line number indicates the id, therefore we have to add empty lines for those ids not present
				$hold_str .= str_repeat("\n", $f - $last_f);
			
				// Convert bitstring for storage - we do not use binary/bytes because PHP's string functions are not fully binary safe
				for ($i = 0, $bit_length = strlen($bitstring); $i < $bit_length; $i += 31)
				{
					$hold_str .= str_pad(base_convert(str_pad(substr($bitstring, $i, 31), 31, 0, STR_PAD_RIGHT), 2, 36), 6, 0, STR_PAD_LEFT);
				}

				$last_f = $f;
			}
			unset($bitstring);

			$hold_str = rtrim($hold_str);
		}

		return $hold_str;
	}

	/**
	* Clear one or all users cached permission settings
	*/
	function acl_clear_prefetch($user_id = false)
	{
		global $db, $cache;

		// Rebuild options cache
		$cache->destroy('_role_cache');

		$sql = 'SELECT *
			FROM ' . ACL_ROLES_DATA_TABLE . '
			ORDER BY role_id ASC';
		$result = $db->sql_query($sql);

		$this->role_cache = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$this->role_cache[$row['role_id']][$row['auth_option_id']] = (int) $row['auth_setting'];
		}
		$db->sql_freeresult($result);

		foreach ($this->role_cache as $role_id => $role_options)
		{
			$this->role_cache[$role_id] = serialize($role_options);
		}

		$cache->put('_role_cache', $this->role_cache);

		// Now empty user permissions
		$where_sql = '';

		if ($user_id !== false)
		{
			$user_id = (!is_array($user_id)) ? $user_id = array((int) $user_id) : array_map('intval', $user_id);
			$where_sql = ' WHERE ' . $db->sql_in_set('user_id', $user_id);
		}

		$sql = 'UPDATE ' . USERS_TABLE . "
			SET user_permissions = '',
				user_perm_from = 0
			$where_sql";
		$db->sql_query($sql);

		return;
	}

	/**
	* Get assigned roles
	*/
	function acl_role_data($user_type, $role_type, $ug_id = false, $forum_id = false)
	{
		global $db;

		$roles = array();

		$sql_id = ($user_type == 'user') ? 'user_id' : 'group_id';

		$sql_ug = ($ug_id !== false) ? ((!is_array($ug_id)) ? "AND a.$sql_id = $ug_id" : 'AND ' . $db->sql_in_set("a.$sql_id", $ug_id)) : '';
		$sql_forum = ($forum_id !== false) ? ((!is_array($forum_id)) ? "AND a.forum_id = $forum_id" : 'AND ' . $db->sql_in_set('a.forum_id', $forum_id)) : '';

		// Grab assigned roles...
		$sql = 'SELECT a.auth_role_id, a.' . $sql_id . ', a.forum_id
			FROM ' . (($user_type == 'user') ? ACL_USERS_TABLE : ACL_GROUPS_TABLE) . ' a, ' . ACL_ROLES_TABLE . " r
			WHERE a.auth_role_id = r.role_id
				AND r.role_type = '" . $db->sql_escape($role_type) . "'
				$sql_ug
				$sql_forum
			ORDER BY r.role_order ASC";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$roles[$row[$sql_id]][$row['forum_id']] = $row['auth_role_id'];
		}
		$db->sql_freeresult($result);

		return $roles;
	}

	/**
	* Get raw acl data based on user/option/forum
	*/
	function acl_raw_data($user_id = false, $opts = false, $forum_id = false)
	{
		global $db;

		$sql_user = ($user_id !== false) ? ((!is_array($user_id)) ? 'user_id = ' . (int) $user_id : $db->sql_in_set('user_id', array_map('intval', $user_id))) : '';
		$sql_forum = ($forum_id !== false) ? ((!is_array($forum_id)) ? 'AND a.forum_id = ' . (int) $forum_id : 'AND ' . $db->sql_in_set('a.forum_id', array_map('intval', $forum_id))) : '';

		$sql_opts = $sql_opts_select = $sql_opts_from = '';
		$hold_ary = array();

		if ($opts !== false)
		{
			$sql_opts_select = ', ao.auth_option';
			$sql_opts_from = ', ' . ACL_OPTIONS_TABLE . ' ao';
			$this->build_auth_option_statement('ao.auth_option', $opts, $sql_opts);
		}

		$sql_ary = array();

		// Grab non-role settings - user-specific
		$sql_ary[] = 'SELECT a.user_id, a.forum_id, a.auth_setting, a.auth_option_id' . $sql_opts_select . '
			FROM ' . ACL_USERS_TABLE . ' a' . $sql_opts_from . '
			WHERE a.auth_role_id = 0 ' .
				(($sql_opts_from) ? 'AND a.auth_option_id = ao.auth_option_id ' : '') .
				(($sql_user) ? 'AND a.' . $sql_user : '') . "
				$sql_forum
				$sql_opts";

		// Now the role settings - user-specific
		$sql_ary[] = 'SELECT a.user_id, a.forum_id, r.auth_option_id, r.auth_setting, r.auth_option_id' . $sql_opts_select . '
			FROM ' . ACL_USERS_TABLE . ' a, ' . ACL_ROLES_DATA_TABLE . ' r' . $sql_opts_from . '
			WHERE a.auth_role_id = r.role_id ' . 
				(($sql_opts_from) ? 'AND r.auth_option_id = ao.auth_option_id ' : '') .
				(($sql_user) ? 'AND a.' . $sql_user : '') . "
				$sql_forum
				$sql_opts";

		foreach ($sql_ary as $sql)
		{
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$option = ($sql_opts_select) ? $row['auth_option'] : $this->acl_options['option'][$row['auth_option_id']];
				$hold_ary[$row['user_id']][$row['forum_id']][$option] = $row['auth_setting'];
			}
			$db->sql_freeresult($result);
		}

		$sql_ary = array();

		// Now grab group settings - non-role specific...
		$sql_ary[] = 'SELECT ug.user_id, a.forum_id, a.auth_setting, a.auth_option_id' . $sql_opts_select . '
			FROM ' . ACL_GROUPS_TABLE . ' a, ' . USER_GROUP_TABLE . ' ug' . $sql_opts_from . '
			WHERE a.auth_role_id = 0 ' .
				(($sql_opts_from) ? 'AND a.auth_option_id = ao.auth_option_id ' : '') . '
				AND a.group_id = ug.group_id
				AND ug.user_pending = 0
				' . (($sql_user) ? 'AND ug.' . $sql_user : '') . "
				$sql_forum
				$sql_opts";

		// Now grab group settings - role specific...
		$sql_ary[] = 'SELECT ug.user_id, a.forum_id, r.auth_setting, r.auth_option_id' . $sql_opts_select . '
			FROM ' . ACL_GROUPS_TABLE . ' a, ' . USER_GROUP_TABLE . ' ug, ' . ACL_ROLES_DATA_TABLE . ' r' . $sql_opts_from . '
			WHERE a.auth_role_id = r.role_id ' .
				(($sql_opts_from) ? 'AND r.auth_option_id = ao.auth_option_id ' : '') . '
				AND a.group_id = ug.group_id
				AND ug.user_pending = 0
				' . (($sql_user) ? 'AND ug.' . $sql_user : '') . "
				$sql_forum
				$sql_opts";

		foreach ($sql_ary as $sql)
		{
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$option = ($sql_opts_select) ? $row['auth_option'] : $this->acl_options['option'][$row['auth_option_id']];

				if (!isset($hold_ary[$row['user_id']][$row['forum_id']][$option]) || (isset($hold_ary[$row['user_id']][$row['forum_id']][$option]) && $hold_ary[$row['user_id']][$row['forum_id']][$option] != ACL_NEVER))
				{
					$hold_ary[$row['user_id']][$row['forum_id']][$option] = $row['auth_setting'];

					// If we detect ACL_NEVER, we will unset the flag option (within building the bitstring it is correctly set again)
					if ($row['auth_setting'] == ACL_NEVER)
					{
						$flag = substr($option, 0, strpos($option, '_') + 1);
	
						if (isset($hold_ary[$row['user_id']][$row['forum_id']][$flag]) && $hold_ary[$row['user_id']][$row['forum_id']][$flag] == ACL_YES)
						{
							unset($hold_ary[$row['user_id']][$row['forum_id']][$flag]);

/*							if (in_array(ACL_YES, $hold_ary[$row['user_id']][$row['forum_id']]))
							{
								$hold_ary[$row['user_id']][$row['forum_id']][$flag] = ACL_YES;
							}
*/
						}
					}
				}
			}
			$db->sql_freeresult($result);
		}

		return $hold_ary;
	}

	/**
	* Get raw user based permission settings
	*/
	function acl_user_raw_data($user_id = false, $opts = false, $forum_id = false)
	{
		global $db;

		$sql_user = ($user_id !== false) ? ((!is_array($user_id)) ? 'user_id = ' . (int) $user_id : $db->sql_in_set('user_id', array_map('intval', $user_id))) : '';
		$sql_forum = ($forum_id !== false) ? ((!is_array($forum_id)) ? 'AND a.forum_id = ' . (int) $forum_id : 'AND ' . $db->sql_in_set('a.forum_id', array_map('intval', $forum_id))) : '';

		$sql_opts = '';
		$hold_ary = $sql_ary = array();

		if ($opts !== false)
		{
			$this->build_auth_option_statement('ao.auth_option', $opts, $sql_opts);
		}

		// Grab user settings - non-role specific...
		$sql_ary[] = 'SELECT a.user_id, a.forum_id, a.auth_setting, a.auth_option_id, ao.auth_option
			FROM ' . ACL_USERS_TABLE . ' a, ' . ACL_OPTIONS_TABLE . ' ao
			WHERE a.auth_role_id = 0
				AND a.auth_option_id = ao.auth_option_id ' .
				(($sql_user) ? 'AND a.' . $sql_user : '') . "
				$sql_forum
				$sql_opts
			ORDER BY a.forum_id, ao.auth_option";

		// Now the role settings - user-specific
		$sql_ary[] = 'SELECT a.user_id, a.forum_id, r.auth_option_id, r.auth_setting, r.auth_option_id, ao.auth_option
			FROM ' . ACL_USERS_TABLE . ' a, ' . ACL_ROLES_DATA_TABLE . ' r, ' . ACL_OPTIONS_TABLE . ' ao
			WHERE a.auth_role_id = r.role_id
				AND r.auth_option_id = ao.auth_option_id ' .
				(($sql_user) ? 'AND a.' . $sql_user : '') . "
				$sql_forum
				$sql_opts
			ORDER BY a.forum_id, ao.auth_option";

		foreach ($sql_ary as $sql)
		{
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$hold_ary[$row['user_id']][$row['forum_id']][$row['auth_option']] = $row['auth_setting'];
			}
			$db->sql_freeresult($result);
		}

		return $hold_ary;
	}

	/**
	* Get raw group based permission settings
	*/
	function acl_group_raw_data($group_id = false, $opts = false, $forum_id = false)
	{
		global $db;

		$sql_group = ($group_id !== false) ? ((!is_array($group_id)) ? 'group_id = ' . (int) $group_id : $db->sql_in_set('group_id', array_map('intval', $group_id))) : '';
		$sql_forum = ($forum_id !== false) ? ((!is_array($forum_id)) ? 'AND a.forum_id = ' . (int) $forum_id : 'AND ' . $db->sql_in_set('a.forum_id', array_map('intval', $forum_id))) : '';

		$sql_opts = '';
		$hold_ary = $sql_ary = array();

		if ($opts !== false)
		{
			$this->build_auth_option_statement('ao.auth_option', $opts, $sql_opts);
		}

		// Grab group settings - non-role specific...
		$sql_ary[] = 'SELECT a.group_id, a.forum_id, a.auth_setting, a.auth_option_id, ao.auth_option
			FROM ' . ACL_GROUPS_TABLE . ' a, ' . ACL_OPTIONS_TABLE . ' ao
			WHERE a.auth_role_id = 0
				AND a.auth_option_id = ao.auth_option_id ' .
				(($sql_group) ? 'AND a.' . $sql_group : '') . "
				$sql_forum
				$sql_opts
			ORDER BY a.forum_id, ao.auth_option";

		// Now grab group settings - role specific...
		$sql_ary[] = 'SELECT a.group_id, a.forum_id, r.auth_setting, r.auth_option_id, ao.auth_option
			FROM ' . ACL_GROUPS_TABLE . ' a, ' . ACL_ROLES_DATA_TABLE . ' r, ' . ACL_OPTIONS_TABLE . ' ao
			WHERE a.auth_role_id = r.role_id
				AND r.auth_option_id = ao.auth_option_id ' .
				(($sql_group) ? 'AND a.' . $sql_group : '') . "
				$sql_forum
				$sql_opts
			ORDER BY a.forum_id, ao.auth_option";

		foreach ($sql_ary as $sql)
		{
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$hold_ary[$row['group_id']][$row['forum_id']][$row['auth_option']] = $row['auth_setting'];
			}
			$db->sql_freeresult($result);
		}

		return $hold_ary;
	}

	/**
	* Get raw acl data based on user for caching user_permissions
	* This function returns the same data as acl_raw_data(), but without the user id as the first key within the array.
	*/
	function acl_raw_data_single_user($user_id)
	{
		global $db, $cache;

		// Check if the role-cache is there
		if (($this->role_cache = $cache->get('_role_cache')) === false)
		{
			$this->role_cache = array();

			// We pre-fetch roles
			$sql = 'SELECT *
				FROM ' . ACL_ROLES_DATA_TABLE . '
				ORDER BY role_id ASC';
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$this->role_cache[$row['role_id']][$row['auth_option_id']] = (int) $row['auth_setting'];
			}
			$db->sql_freeresult($result);

			foreach ($this->role_cache as $role_id => $role_options)
			{
				$this->role_cache[$role_id] = serialize($role_options);
			}

			$cache->put('_role_cache', $this->role_cache);
		}

		$hold_ary = array();

		// Grab user-specific permission settings
		$sql = 'SELECT forum_id, auth_option_id, auth_role_id, auth_setting
			FROM ' . ACL_USERS_TABLE . '
			WHERE user_id = ' . $user_id;
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			// If a role is assigned, assign all options included within this role. Else, only set this one option.
			if ($row['auth_role_id'])
			{
				$hold_ary[$row['forum_id']] = (empty($hold_ary[$row['forum_id']])) ? unserialize($this->role_cache[$row['auth_role_id']]) : $hold_ary[$row['forum_id']] + unserialize($this->role_cache[$row['auth_role_id']]);
			}
			else
			{
				$hold_ary[$row['forum_id']][$row['auth_option_id']] = $row['auth_setting'];
			}
		}
		$db->sql_freeresult($result);

		// Now grab group-specific permission settings
		$sql = 'SELECT a.forum_id, a.auth_option_id, a.auth_role_id, a.auth_setting
			FROM ' . ACL_GROUPS_TABLE . ' a, ' . USER_GROUP_TABLE . ' ug
			WHERE a.group_id = ug.group_id
				AND ug.user_pending = 0
				AND ug.user_id = ' . $user_id;
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			if (!$row['auth_role_id'])
			{
				$this->_set_group_hold_ary($hold_ary[$row['forum_id']], $row['auth_option_id'], $row['auth_setting']);
			}
			else
			{
				foreach (unserialize($this->role_cache[$row['auth_role_id']]) as $option_id => $setting)
				{
					$this->_set_group_hold_ary($hold_ary[$row['forum_id']], $option_id, $setting);
				}
			}
		}
		$db->sql_freeresult($result);

		return $hold_ary;
	}

	/**
	* Private function snippet for setting a specific piece of the hold_ary
	*/
	function _set_group_hold_ary(&$hold_ary, $option_id, $setting)
	{
		if (!isset($hold_ary[$option_id]) || (isset($hold_ary[$option_id]) && $hold_ary[$option_id] != ACL_NEVER))
		{
			$hold_ary[$option_id] = $setting;

			// If we detect ACL_NEVER, we will unset the flag option (within building the bitstring it is correctly set again)
			if ($setting == ACL_NEVER)
			{
				$flag = substr($this->acl_options['option'][$option_id], 0, strpos($this->acl_options['option'][$option_id], '_') + 1);
				$flag = (int) $this->acl_options['id'][$flag];
		
				if (isset($hold_ary[$flag]) && $hold_ary[$flag] == ACL_YES)
				{
					unset($hold_ary[$flag]);

/*					This is uncommented, because i suspect this being slightly wrong due to mixed permission classes being possible
					if (in_array(ACL_YES, $hold_ary))
					{
						$hold_ary[$flag] = ACL_YES;
					}*/
				}
			}
		}
	}

	/**
	* Authentication plug-ins is largely down to Sergey Kanareykin, our thanks to him.
	*/
	function login($username, $password, $autologin = false, $viewonline = 1, $admin = 0)
	{
		global $config, $db, $user, $phpbb_root_path, $phpEx;

		$method = trim(basename($config['auth_method']));
		include_once($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx);

		$method = 'login_' . $method;
		if (function_exists($method))
		{
			$login = $method($username, $password);

			// If the auth module wants us to create an empty profile do so and then treat the status as LOGIN_SUCCESS
			if ($login['status'] == LOGIN_SUCCESS_CREATE_PROFILE)
			{
				// we are going to use the user_add function so include functions_user.php if it wasn't defined yet
				if (!function_exists('user_add'))
				{
					include($phpbb_root_path . 'includes/functions_user.' . $phpEx);
				}

				user_add($login['user_row'], (isset($login['cp_data'])) ? $login['cp_data'] : false);

				$sql = 'SELECT user_id, username, user_password, user_passchg, user_email, user_type
					FROM ' . USERS_TABLE . "
					WHERE username_clean = '" . $db->sql_escape(utf8_clean_string($username)) . "'";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if (!$row)
				{
					return array(
						'status'		=> LOGIN_ERROR_EXTERNAL_AUTH,
						'error_msg'		=> 'AUTH_NO_PROFILE_CREATED',
						'user_row'		=> array('user_id' => ANONYMOUS),
					);
				}

				$login = array(
					'status'	=> LOGIN_SUCCESS,
					'error_msg'	=> false,
					'user_row'	=> $row,
				);
			}

			// If login succeeded, we will log the user in... else we pass the login array through...
			if ($login['status'] == LOGIN_SUCCESS)
			{
				$old_session_id = $user->session_id;

				if ($admin)
				{
					global $SID, $_SID;

					$cookie_expire = time() - 31536000;
					$user->set_cookie('u', '', $cookie_expire);
					$user->set_cookie('sid', '', $cookie_expire);
					unset($cookie_expire);

					$SID = '?sid=';
					$user->session_id = $_SID = '';
				}

				$result = $user->session_create($login['user_row']['user_id'], $admin, $autologin, $viewonline);

				// Successful session creation
				if ($result === true)
				{
					// If admin re-authentication we remove the old session entry because a new one has been created...
					if ($admin)
					{
						// the login array is used because the user ids do not differ for re-authentication
						$sql = 'DELETE FROM ' . SESSIONS_TABLE . "
							WHERE session_id = '" . $db->sql_escape($old_session_id) . "'
							AND session_user_id = {$login['user_row']['user_id']}";
						$db->sql_query($sql);
					}

					return array(
						'status'		=> LOGIN_SUCCESS,
						'error_msg'		=> false,
						'user_row'		=> $login['user_row'],
					);
				}

				return array(
					'status'		=> LOGIN_BREAK,
					'error_msg'		=> $result,
					'user_row'		=> $login['user_row'],
				);
			}

			return $login;
		}

		trigger_error('Authentication method not found', E_USER_ERROR);
	}

	/**
	* Fill auth_option statement for later querying based on the supplied options
	*/
	function build_auth_option_statement($key, $auth_options, &$sql_opts)
	{
		global $db;

		if (!is_array($auth_options))
		{
			if (strpos($auth_options, '%') !== false)
			{
				$sql_opts = "AND $key " . $db->sql_like_expression(str_replace('%', $db->any_char, $auth_options));
			}
			else
			{
				$sql_opts = "AND $key = '" . $db->sql_escape($auth_options) . "'";
			}
		}
		else
		{
			$is_like_expression = false;

			foreach ($auth_options as $option)
			{
				if (strpos($option, '%') !== false)
				{
					$is_like_expression = true;
				}
			}

			if (!$is_like_expression)
			{
				$sql_opts = 'AND ' . $db->sql_in_set($key, $auth_options);
			}
			else
			{
				$sql = array();

				foreach ($auth_options as $option)
				{
					if (strpos($option, '%') !== false)
					{
						$sql[] = $key . ' ' . $db->sql_like_expression(str_replace('%', $db->any_char, $option));
					}
					else
					{
						$sql[] = $key . " = '" . $db->sql_escape($option) . "'";
					}
				}

				$sql_opts = 'AND (' . implode(' OR ', $sql) . ')';
			}
		}
	}
}

?>