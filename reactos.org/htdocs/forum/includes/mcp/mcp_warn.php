<?php
/**
*
* @package mcp
* @version $Id: mcp_warn.php 8479 2008-03-29 00:22:48Z naderman $
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
* mcp_warn
* Handling warning the users
* @package mcp
*/
class mcp_warn
{
	var $p_master;
	var $u_action;

	function mcp_warn(&$p_master)
	{
		$this->p_master = &$p_master;
	}

	function main($id, $mode)
	{
		global $auth, $db, $user, $template;
		global $config, $phpbb_root_path, $phpEx;

		$action = request_var('action', array('' => ''));

		if (is_array($action))
		{
			list($action, ) = each($action);
		}

		$this->page_title = 'MCP_WARN';

		add_form_key('mcp_warn');

		switch ($mode)
		{
			case 'front':
				$this->mcp_warn_front_view();
				$this->tpl_name = 'mcp_warn_front';
			break;

			case 'list':
				$this->mcp_warn_list_view($action);
				$this->tpl_name = 'mcp_warn_list';
			break;

			case 'warn_post':
				$this->mcp_warn_post_view($action);
				$this->tpl_name = 'mcp_warn_post';
			break;

			case 'warn_user':
				$this->mcp_warn_user_view($action);
				$this->tpl_name = 'mcp_warn_user';
			break;
		}
	}

	/**
	* Generates the summary on the main page of the warning module
	*/
	function mcp_warn_front_view()
	{
		global $phpEx, $phpbb_root_path, $config;
		global $template, $db, $user, $auth;

		$template->assign_vars(array(
			'U_FIND_USERNAME'	=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=searchuser&amp;form=mcp&amp;field=username&amp;select_single=true'),
			'U_POST_ACTION'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=warn&amp;mode=warn_user'),
		));

		// Obtain a list of the 5 naughtiest users....
		// These are the 5 users with the highest warning count
		$highest = array();
		$count = 0;

		view_warned_users($highest, $count, 5);

		foreach ($highest as $row)
		{
			$template->assign_block_vars('highest', array(
				'U_NOTES'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=notes&amp;mode=user_notes&amp;u=' . $row['user_id']),

				'USERNAME_FULL'		=> get_username_string('full', $row['user_id'], $row['username'], $row['user_colour']),
				'USERNAME'			=> $row['username'],
				'USERNAME_COLOUR'	=> ($row['user_colour']) ? '#' . $row['user_colour'] : '',
				'U_USER'			=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=viewprofile&amp;u=' . $row['user_id']),

				'WARNING_TIME'	=> $user->format_date($row['user_last_warning']),
				'WARNINGS'		=> $row['user_warnings'],
			));
		}

		// And now the 5 most recent users to get in trouble
		$sql = 'SELECT u.user_id, u.username, u.username_clean, u.user_colour, u.user_warnings, w.warning_time
			FROM ' . USERS_TABLE . ' u, ' . WARNINGS_TABLE . ' w
			WHERE u.user_id = w.user_id
			ORDER BY w.warning_time DESC';
		$result = $db->sql_query_limit($sql, 5);

		while ($row = $db->sql_fetchrow($result))
		{
			$template->assign_block_vars('latest', array(
				'U_NOTES'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=notes&amp;mode=user_notes&amp;u=' . $row['user_id']),

				'USERNAME_FULL'		=> get_username_string('full', $row['user_id'], $row['username'], $row['user_colour']),
				'USERNAME'			=> $row['username'],
				'USERNAME_COLOUR'	=> ($row['user_colour']) ? '#' . $row['user_colour'] : '',
				'U_USER'			=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=viewprofile&amp;u=' . $row['user_id']),

				'WARNING_TIME'	=> $user->format_date($row['warning_time']),
				'WARNINGS'		=> $row['user_warnings'],
			));
		}
		$db->sql_freeresult($result);
	}

	/**
	* Lists all users with warnings
	*/
	function mcp_warn_list_view($action)
	{
		global $phpEx, $phpbb_root_path, $config;
		global $template, $db, $user, $auth;

		$user->add_lang('memberlist');

		$start	= request_var('start', 0);
		$st		= request_var('st', 0);
		$sk		= request_var('sk', 'b');
		$sd		= request_var('sd', 'd');

		$limit_days = array(0 => $user->lang['ALL_ENTRIES'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
		$sort_by_text = array('a' => $user->lang['SORT_USERNAME'], 'b' => $user->lang['SORT_DATE'], 'c' => $user->lang['SORT_WARNINGS']);
		$sort_by_sql = array('a' => 'username_clean', 'b' => 'user_last_warning', 'c' => 'user_warnings');

		$s_limit_days = $s_sort_key = $s_sort_dir = $u_sort_param = '';
		gen_sort_selects($limit_days, $sort_by_text, $st, $sk, $sd, $s_limit_days, $s_sort_key, $s_sort_dir, $u_sort_param);

		// Define where and sort sql for use in displaying logs
		$sql_where = ($st) ? (time() - ($st * 86400)) : 0;
		$sql_sort = $sort_by_sql[$sk] . ' ' . (($sd == 'd') ? 'DESC' : 'ASC');

		$users = array();
		$user_count = 0;

		view_warned_users($users, $user_count, $config['topics_per_page'], $start, $sql_where, $sql_sort);

		foreach ($users as $row)
		{
			$template->assign_block_vars('user', array(
				'U_NOTES'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=notes&amp;mode=user_notes&amp;u=' . $row['user_id']),

				'USERNAME_FULL'		=> get_username_string('full', $row['user_id'], $row['username'], $row['user_colour']),
				'USERNAME'			=> $row['username'],
				'USERNAME_COLOUR'	=> ($row['user_colour']) ? '#' . $row['user_colour'] : '',
				'U_USER'			=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=viewprofile&amp;u=' . $row['user_id']),
			
				'WARNING_TIME'	=> $user->format_date($row['user_last_warning']),
				'WARNINGS'		=> $row['user_warnings'],
			));
		}

		$template->assign_vars(array(
			'U_POST_ACTION'			=> $this->u_action,
			'S_CLEAR_ALLOWED'		=> ($auth->acl_get('a_clearlogs')) ? true : false,
			'S_SELECT_SORT_DIR'		=> $s_sort_dir,
			'S_SELECT_SORT_KEY'		=> $s_sort_key,
			'S_SELECT_SORT_DAYS'	=> $s_limit_days,

			'PAGE_NUMBER'		=> on_page($user_count, $config['topics_per_page'], $start),
			'PAGINATION'		=> generate_pagination(append_sid("{$phpbb_root_path}mcp.$phpEx", "i=warn&amp;mode=list&amp;st=$st&amp;sk=$sk&amp;sd=$sd"), $user_count, $config['topics_per_page'], $start),
			'TOTAL_USERS'		=> ($user_count == 1) ? $user->lang['LIST_USER'] : sprintf($user->lang['LIST_USERS'], $user_count),
		));
	}

	/**
	* Handles warning the user when the warning is for a specific post
	*/
	function mcp_warn_post_view($action)
	{
		global $phpEx, $phpbb_root_path, $config;
		global $template, $db, $user, $auth;

		$post_id = request_var('p', 0);
		$forum_id = request_var('f', 0);
		$notify = (isset($_REQUEST['notify_user'])) ? true : false;
		$warning = utf8_normalize_nfc(request_var('warning', '', true));

		$sql = 'SELECT u.*, p.*
			FROM ' . POSTS_TABLE . ' p, ' . USERS_TABLE . " u
			WHERE post_id = $post_id
				AND u.user_id = p.poster_id";
		$result = $db->sql_query($sql);
		$user_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$user_row)
		{
			trigger_error('NO_POST');
		}

		// There is no point issuing a warning to ignored users (ie anonymous and bots)
		if ($user_row['user_type'] == USER_IGNORE)
		{
			trigger_error('CANNOT_WARN_ANONYMOUS');
		}

		// Prevent someone from warning themselves
		if ($user_row['user_id'] == $user->data['user_id'])
		{
			trigger_error('CANNOT_WARN_SELF');
		}

		// Check if there is already a warning for this post to prevent multiple
		// warnings for the same offence
		$sql = 'SELECT post_id
			FROM ' . WARNINGS_TABLE . "
			WHERE post_id = $post_id";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if ($row)
		{
			trigger_error('ALREADY_WARNED');
		}

		$user_id = $user_row['user_id'];

		if (strpos($this->u_action, "&amp;f=$forum_id&amp;p=$post_id") === false)
		{
			$this->p_master->adjust_url("&amp;f=$forum_id&amp;p=$post_id");
			$this->u_action .= "&amp;f=$forum_id&amp;p=$post_id";
		}

		if ($warning && $action == 'add_warning')
		{
			if (check_form_key('mcp_warn'))
			{
				add_warning($user_row, $warning, $notify, $post_id);
				$msg = $user->lang['USER_WARNING_ADDED'];
			}
			else
			{
				$msg = $user->lang['FORM_INVALID'];
			}
			$redirect = append_sid("{$phpbb_root_path}mcp.$phpEx", "i=notes&amp;mode=user_notes&amp;u=$user_id");
			meta_refresh(2, $redirect);
			trigger_error($user->lang['USER_WARNING_ADDED'] . '<br /><br />' . sprintf($user->lang['RETURN_PAGE'], '<a href="' . $redirect . '">', '</a>'));
		}

		// OK, they didn't submit a warning so lets build the page for them to do so
		
		// We want to make the message available here as a reminder
		// Parse the message and subject
		$message = censor_text($user_row['post_text']);

		// Second parse bbcode here
		if ($user_row['bbcode_bitfield'])
		{
			include_once($phpbb_root_path . 'includes/bbcode.' . $phpEx);

			$bbcode = new bbcode($user_row['bbcode_bitfield']);
			$bbcode->bbcode_second_pass($message, $user_row['bbcode_uid'], $user_row['bbcode_bitfield']);
		}

		$message = bbcode_nl2br($message);
		$message = smiley_text($message);

		// Generate the appropriate user information for the user we are looking at
		if (!function_exists('get_user_avatar'))
		{
			include($phpbb_root_path . 'includes/functions_display.' . $phpEx);
		}

		$rank_title = $rank_img = '';
		$avatar_img = get_user_avatar($user_row['user_avatar'], $user_row['user_avatar_type'], $user_row['user_avatar_width'], $user_row['user_avatar_height']);

		$template->assign_vars(array(
			'U_POST_ACTION'		=> $this->u_action,

			'POST'				=> $message,
			'USERNAME'			=> $user_row['username'],
			'USER_COLOR'		=> (!empty($user_row['user_colour'])) ? $user_row['user_colour'] : '',
			'RANK_TITLE'		=> $rank_title,
			'JOINED'			=> $user->format_date($user_row['user_regdate']),
			'POSTS'				=> ($user_row['user_posts']) ? $user_row['user_posts'] : 0,
			'WARNINGS'			=> ($user_row['user_warnings']) ? $user_row['user_warnings'] : 0,

			'AVATAR_IMG'		=> $avatar_img,
			'RANK_IMG'			=> $rank_img,

			'L_WARNING_POST_DEFAULT'	=> sprintf($user->lang['WARNING_POST_DEFAULT'], generate_board_url() . "/viewtopic.$phpEx?f=$forum_id&amp;p=$post_id#p$post_id"),
		));
	}

	/**
	* Handles warning the user
	*/
	function mcp_warn_user_view($action)
	{
		global $phpEx, $phpbb_root_path, $config, $module;
		global $template, $db, $user, $auth;

		$user_id = request_var('u', 0);
		$username = request_var('username', '', true);
		$notify = (isset($_REQUEST['notify_user'])) ? true : false;
		$warning = utf8_normalize_nfc(request_var('warning', '', true));

		$sql_where = ($user_id) ? "user_id = $user_id" : "username_clean = '" . $db->sql_escape(utf8_clean_string($username)) . "'";

		$sql = 'SELECT *
			FROM ' . USERS_TABLE . '
			WHERE ' . $sql_where;
		$result = $db->sql_query($sql);
		$user_row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$user_row)
		{
			trigger_error('NO_USER');
		}

		// Prevent someone from warning themselves
		if ($user_row['user_id'] == $user->data['user_id'])
		{
			trigger_error('CANNOT_WARN_SELF');
		}

		$user_id = $user_row['user_id'];

		if (strpos($this->u_action, "&amp;u=$user_id") === false)
		{
			$this->p_master->adjust_url('&amp;u=' . $user_id);
			$this->u_action .= "&amp;u=$user_id";
		}

		if ($warning && $action == 'add_warning')
		{
			if (check_form_key('mcp_warn'))
			{
				add_warning($user_row, $warning, $notify);
				$msg = $user->lang['USER_WARNING_ADDED'];
			}
			else
			{
				$msg = $user->lang['FORM_INVALID'];
			}
			$redirect = append_sid("{$phpbb_root_path}mcp.$phpEx", "i=notes&amp;mode=user_notes&amp;u=$user_id");
			meta_refresh(2, $redirect);
			trigger_error($msg . '<br /><br />' . sprintf($user->lang['RETURN_PAGE'], '<a href="' . $redirect . '">', '</a>'));
		}

		// Generate the appropriate user information for the user we are looking at
		if (!function_exists('get_user_avatar'))
		{
			include($phpbb_root_path . 'includes/functions_display.' . $phpEx);
		}

		$rank_title = $rank_img = '';
		$avatar_img = get_user_avatar($user_row['user_avatar'], $user_row['user_avatar_type'], $user_row['user_avatar_width'], $user_row['user_avatar_height']);

		// OK, they didn't submit a warning so lets build the page for them to do so
		$template->assign_vars(array(
			'U_POST_ACTION'		=> $this->u_action,

			'USERNAME'			=> $user_row['username'],
			'USER_COLOR'		=> (!empty($user_row['user_colour'])) ? $user_row['user_colour'] : '',
			'RANK_TITLE'		=> $rank_title,
			'JOINED'			=> $user->format_date($user_row['user_regdate']),
			'POSTS'				=> ($user_row['user_posts']) ? $user_row['user_posts'] : 0,
			'WARNINGS'			=> ($user_row['user_warnings']) ? $user_row['user_warnings'] : 0,

			'AVATAR_IMG'		=> $avatar_img,
			'RANK_IMG'			=> $rank_img,
		));

		return $user_id;
	}
}

/**
* Insert the warning into the database
*/
function add_warning($user_row, $warning, $send_pm = true, $post_id = 0)
{
	global $phpEx, $phpbb_root_path, $config;
	global $template, $db, $user, $auth;

	if ($send_pm)
	{
		include_once($phpbb_root_path . 'includes/functions_privmsgs.' . $phpEx);
		include_once($phpbb_root_path . 'includes/message_parser.' . $phpEx);

		$user_row['user_lang'] = (file_exists($phpbb_root_path . 'language/' . $user_row['user_lang'] . "/mcp.$phpEx")) ? $user_row['user_lang'] : $config['default_lang'];
		include($phpbb_root_path . 'language/' . basename($user_row['user_lang']) . "/mcp.$phpEx");

		$message_parser = new parse_message();

		$message_parser->message = sprintf($lang['WARNING_PM_BODY'], $warning);
		$message_parser->parse(true, true, true, false, false, true, true);

		$pm_data = array(
			'from_user_id'			=> $user->data['user_id'],
			'from_user_ip'			=> $user->ip,
			'from_username'			=> $user->data['username'],
			'enable_sig'			=> false,
			'enable_bbcode'			=> true,
			'enable_smilies'		=> true,
			'enable_urls'			=> false,
			'icon_id'				=> 0,
			'bbcode_bitfield'		=> $message_parser->bbcode_bitfield,
			'bbcode_uid'			=> $message_parser->bbcode_uid,
			'message'				=> $message_parser->message,
			'address_list'			=> array('u' => array($user_row['user_id'] => 'to')),
		);

		submit_pm('post', $lang['WARNING_PM_SUBJECT'], $pm_data, false);
	}

	add_log('admin', 'LOG_USER_WARNING', $user_row['username']);
	$log_id = add_log('user', $user_row['user_id'], 'LOG_USER_WARNING_BODY', $warning);

	$sql_ary = array(
		'user_id'		=> $user_row['user_id'],
		'post_id'		=> $post_id,
		'log_id'		=> $log_id,
		'warning_time'	=> time(),
	);

	$db->sql_query('INSERT INTO ' . WARNINGS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));

	$sql = 'UPDATE ' . USERS_TABLE . '
		SET user_warnings = user_warnings + 1,
			user_last_warning = ' . time() . '
		WHERE user_id = ' . $user_row['user_id'];
	$db->sql_query($sql);

	// We add this to the mod log too for moderators to see that a specific user got warned.
	$sql = 'SELECT forum_id, topic_id
		FROM ' . POSTS_TABLE . '
		WHERE post_id = ' . $post_id;
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	add_log('mod', $row['forum_id'], $row['topic_id'], 'LOG_USER_WARNING', $user_row['username']);
}

?>