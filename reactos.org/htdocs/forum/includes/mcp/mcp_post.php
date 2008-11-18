<?php
/**
*
* @package mcp
* @version $Id: mcp_post.php 8479 2008-03-29 00:22:48Z naderman $
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
* Handling actions in post details screen
*/
function mcp_post_details($id, $mode, $action)
{
	global $phpEx, $phpbb_root_path, $config;
	global $template, $db, $user, $auth, $cache;

	$user->add_lang('posting');

	$post_id = request_var('p', 0);
	$start	= request_var('start', 0);

	// Get post data
	$post_info = get_post_data(array($post_id), false, true);

	add_form_key('mcp_post_details');

	if (!sizeof($post_info))
	{
		trigger_error('POST_NOT_EXIST');
	}

	$post_info = $post_info[$post_id];
	$url = append_sid("{$phpbb_root_path}mcp.$phpEx?" . extra_url());

	switch ($action)
	{
		case 'whois':

			if ($auth->acl_get('m_info', $post_info['forum_id']))
			{
				$ip = request_var('ip', '');
				include($phpbb_root_path . 'includes/functions_user.' . $phpEx);

				$template->assign_vars(array(
					'RETURN_POST'	=> sprintf($user->lang['RETURN_POST'], '<a href="' . append_sid("{$phpbb_root_path}mcp.$phpEx", "i=$id&amp;mode=$mode&amp;p=$post_id") . '">', '</a>'),
					'U_RETURN_POST'	=> append_sid("{$phpbb_root_path}mcp.$phpEx", "i=$id&amp;mode=$mode&amp;p=$post_id"),
					'L_RETURN_POST'	=> sprintf($user->lang['RETURN_POST'], '', ''),
					'WHOIS'			=> user_ipwhois($ip),
				));
			}

			// We're done with the whois page so return
			return;

		break;

		case 'chgposter':
		case 'chgposter_ip':

			if ($action == 'chgposter')
			{
				$username = request_var('username', '', true);
				$sql_where = "username_clean = '" . $db->sql_escape(utf8_clean_string($username)) . "'";
			}
			else
			{
				$new_user_id = request_var('u', 0);
				$sql_where = 'user_id = ' . $new_user_id;
			}

			$sql = 'SELECT *
				FROM ' . USERS_TABLE . '
				WHERE ' . $sql_where;
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if (!$row)
			{
				trigger_error('NO_USER');
			}

			if ($auth->acl_get('m_chgposter', $post_info['forum_id']))
			{
				if (check_form_key('mcp_post_details'))
				{
					change_poster($post_info, $row);
				}
				else
				{
					trigger_error('FORM_INVALID');
				}
			}

		break;
	}

	// Set some vars
	$users_ary = $usernames_ary = array();
	$attachments = $extensions = array();
	$post_id = $post_info['post_id'];
	$topic_tracking_info = array();

	// Get topic tracking info
	if ($config['load_db_lastread'])
	{
		$tmp_topic_data = array($post_info['topic_id'] => $post_info);
		$topic_tracking_info = get_topic_tracking($post_info['forum_id'], $post_info['topic_id'], $tmp_topic_data, array($post_info['forum_id'] => $post_info['forum_mark_time']));
		unset($tmp_topic_data);
	}
	else
	{
		$topic_tracking_info = get_complete_topic_tracking($post_info['forum_id'], $post_info['topic_id']);
	}

	$post_unread = (isset($topic_tracking_info[$post_info['topic_id']]) && $post_info['post_time'] > $topic_tracking_info[$post_info['topic_id']]) ? true : false;

	// Process message, leave it uncensored
	$message = $post_info['post_text'];

	if ($post_info['bbcode_bitfield'])
	{
		include_once($phpbb_root_path . 'includes/bbcode.' . $phpEx);
		$bbcode = new bbcode($post_info['bbcode_bitfield']);
		$bbcode->bbcode_second_pass($message, $post_info['bbcode_uid'], $post_info['bbcode_bitfield']);
	}

	$message = bbcode_nl2br($message);
	$message = smiley_text($message);

	if ($post_info['post_attachment'] && $auth->acl_get('u_download') && $auth->acl_get('f_download', $post_info['forum_id']))
	{
		$extensions = $cache->obtain_attach_extensions($post_info['forum_id']);

		$sql = 'SELECT *
			FROM ' . ATTACHMENTS_TABLE . '
			WHERE post_msg_id = ' . $post_id . '
				AND in_message = 0
			ORDER BY filetime DESC, post_msg_id ASC';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$attachments[] = $row;
		}
		$db->sql_freeresult($result);

		if (sizeof($attachments))
		{
			$update_count = array();
			parse_attachments($post_info['forum_id'], $message, $attachments, $update_count);
		}

		// Display not already displayed Attachments for this post, we already parsed them. ;)
		if (!empty($attachments))
		{
			$template->assign_var('S_HAS_ATTACHMENTS', true);

			foreach ($attachments as $attachment)
			{
				$template->assign_block_vars('attachment', array(
					'DISPLAY_ATTACHMENT'	=> $attachment)
				);
			}
		}
	}

	$template->assign_vars(array(
		'U_MCP_ACTION'			=> "$url&amp;i=main&amp;quickmod=1", // Use this for mode paramaters
		'U_POST_ACTION'			=> "$url&amp;i=$id&amp;mode=post_details", // Use this for action parameters
		'U_APPROVE_ACTION'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", "i=queue&amp;p=$post_id&amp;f={$post_info['forum_id']}"),

		'S_CAN_VIEWIP'			=> $auth->acl_get('m_info', $post_info['forum_id']),
		'S_CAN_CHGPOSTER'		=> $auth->acl_get('m_chgposter', $post_info['forum_id']),
		'S_CAN_LOCK_POST'		=> $auth->acl_get('m_lock', $post_info['forum_id']),
		'S_CAN_DELETE_POST'		=> $auth->acl_get('m_delete', $post_info['forum_id']),

		'S_POST_REPORTED'		=> ($post_info['post_reported']) ? true : false,
		'S_POST_UNAPPROVED'		=> (!$post_info['post_approved']) ? true : false,
		'S_POST_LOCKED'			=> ($post_info['post_edit_locked']) ? true : false,
		'S_USER_NOTES'			=> true,
		'S_CLEAR_ALLOWED'		=> ($auth->acl_get('a_clearlogs')) ? true : false,

		'U_EDIT'				=> ($auth->acl_get('m_edit', $post_info['forum_id'])) ? append_sid("{$phpbb_root_path}posting.$phpEx", "mode=edit&amp;f={$post_info['forum_id']}&amp;p={$post_info['post_id']}") : '',
		'U_FIND_USERNAME'		=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=searchuser&amp;form=mcp_chgposter&amp;field=username&amp;select_single=true'),
		'U_MCP_APPROVE'			=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=queue&amp;mode=approve_details&amp;f=' . $post_info['forum_id'] . '&amp;p=' . $post_id),
		'U_MCP_REPORT'			=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=reports&amp;mode=report_details&amp;f=' . $post_info['forum_id'] . '&amp;p=' . $post_id),
		'U_MCP_USER_NOTES'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=notes&amp;mode=user_notes&amp;u=' . $post_info['user_id']),
		'U_MCP_WARN_USER'		=> ($auth->acl_get('m_warn')) ? append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=warn&amp;mode=warn_user&amp;u=' . $post_info['user_id']) : '',
		'U_VIEW_POST'			=> append_sid("{$phpbb_root_path}viewtopic.$phpEx", 'f=' . $post_info['forum_id'] . '&amp;p=' . $post_info['post_id'] . '#p' . $post_info['post_id']),
		'U_VIEW_TOPIC'			=> append_sid("{$phpbb_root_path}viewtopic.$phpEx", 'f=' . $post_info['forum_id'] . '&amp;t=' . $post_info['topic_id']),

		'MINI_POST_IMG'			=> ($post_unread) ? $user->img('icon_post_target_unread', 'NEW_POST') : $user->img('icon_post_target', 'POST'),

		'RETURN_TOPIC'			=> sprintf($user->lang['RETURN_TOPIC'], '<a href="' . append_sid("{$phpbb_root_path}viewtopic.$phpEx", "f={$post_info['forum_id']}&amp;p=$post_id") . "#p$post_id\">", '</a>'),
		'RETURN_FORUM'			=> sprintf($user->lang['RETURN_FORUM'], '<a href="' . append_sid("{$phpbb_root_path}viewforum.$phpEx", "f={$post_info['forum_id']}&amp;start={$start}") . '">', '</a>'),
		'REPORTED_IMG'			=> $user->img('icon_topic_reported', $user->lang['POST_REPORTED']),
		'UNAPPROVED_IMG'		=> $user->img('icon_topic_unapproved', $user->lang['POST_UNAPPROVED']),
		'EDIT_IMG'				=> $user->img('icon_post_edit', $user->lang['EDIT_POST']),
		'SEARCH_IMG'			=> $user->img('icon_user_search', $user->lang['SEARCH']),

		'POST_AUTHOR_FULL'		=> get_username_string('full', $post_info['user_id'], $post_info['username'], $post_info['user_colour'], $post_info['post_username']),
		'POST_AUTHOR_COLOUR'	=> get_username_string('colour', $post_info['user_id'], $post_info['username'], $post_info['user_colour'], $post_info['post_username']),
		'POST_AUTHOR'			=> get_username_string('username', $post_info['user_id'], $post_info['username'], $post_info['user_colour'], $post_info['post_username']),
		'U_POST_AUTHOR'			=> get_username_string('profile', $post_info['user_id'], $post_info['username'], $post_info['user_colour'], $post_info['post_username']),

		'POST_PREVIEW'			=> $message,
		'POST_SUBJECT'			=> $post_info['post_subject'],
		'POST_DATE'				=> $user->format_date($post_info['post_time']),
		'POST_IP'				=> $post_info['poster_ip'],
		'POST_IPADDR'			=> ($auth->acl_get('m_info', $post_info['forum_id']) && request_var('lookup', '')) ? @gethostbyaddr($post_info['poster_ip']) : '',
		'POST_ID'				=> $post_info['post_id'],

		'U_LOOKUP_IP'			=> ($auth->acl_get('m_info', $post_info['forum_id'])) ? "$url&amp;i=$id&amp;mode=$mode&amp;lookup={$post_info['poster_ip']}#ip" : '',
		'U_WHOIS'				=> ($auth->acl_get('m_info', $post_info['forum_id'])) ? append_sid("{$phpbb_root_path}mcp.$phpEx", "i=$id&amp;mode=$mode&amp;action=whois&amp;p=$post_id&amp;ip={$post_info['poster_ip']}") : '',
	));

	// Get User Notes
	$log_data = array();
	$log_count = 0;
	view_log('user', $log_data, $log_count, $config['posts_per_page'], 0, 0, 0, $post_info['user_id']);

	if ($log_count)
	{
		$template->assign_var('S_USER_NOTES', true);

		foreach ($log_data as $row)
		{
			$template->assign_block_vars('usernotes', array(
				'REPORT_BY'		=> $row['username_full'],
				'REPORT_AT'		=> $user->format_date($row['time']),
				'ACTION'		=> $row['action'],
				'ID'			=> $row['id'])
			);
		}
	}

	// Get Reports
	if ($auth->acl_get('m_', $post_info['forum_id']))
	{
		$sql = 'SELECT r.*, re.*, u.user_id, u.username
			FROM ' . REPORTS_TABLE . ' r, ' . USERS_TABLE . ' u, ' . REPORTS_REASONS_TABLE . " re
			WHERE r.post_id = $post_id
				AND r.reason_id = re.reason_id
				AND u.user_id = r.user_id
			ORDER BY r.report_time DESC";
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			$template->assign_var('S_SHOW_REPORTS', true);

			do
			{
				// If the reason is defined within the language file, we will use the localized version, else just use the database entry...
				if (isset($user->lang['report_reasons']['TITLE'][strtoupper($row['reason_title'])]) && isset($user->lang['report_reasons']['DESCRIPTION'][strtoupper($row['reason_title'])]))
				{
					$row['reson_description'] = $user->lang['report_reasons']['DESCRIPTION'][strtoupper($row['reason_title'])];
					$row['reason_title'] = $user->lang['report_reasons']['TITLE'][strtoupper($row['reason_title'])];
				}

				$template->assign_block_vars('reports', array(
					'REPORT_ID'		=> $row['report_id'],
					'REASON_TITLE'	=> $row['reason_title'],
					'REASON_DESC'	=> $row['reason_description'],
					'REPORTER'		=> ($row['user_id'] != ANONYMOUS) ? $row['username'] : $user->lang['GUEST'],
					'U_REPORTER'	=> ($row['user_id'] != ANONYMOUS) ? append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=viewprofile&amp;u=' . $row['user_id']) : '',
					'USER_NOTIFY'	=> ($row['user_notify']) ? true : false,
					'REPORT_TIME'	=> $user->format_date($row['report_time']),
					'REPORT_TEXT'	=> bbcode_nl2br(trim($row['report_text'])),
				));
			}
			while ($row = $db->sql_fetchrow($result));
		}
		$db->sql_freeresult($result);
	}

	// Get IP
	if ($auth->acl_get('m_info', $post_info['forum_id']))
	{
		$rdns_ip_num = request_var('rdns', '');

		if ($rdns_ip_num != 'all')
		{
			$template->assign_vars(array(
				'U_LOOKUP_ALL'	=> "$url&amp;i=main&amp;mode=post_details&amp;rdns=all")
			);
		}

		// Get other users who've posted under this IP
		$sql = 'SELECT poster_id, COUNT(poster_id) as postings
			FROM ' . POSTS_TABLE . "
			WHERE poster_ip = '" . $db->sql_escape($post_info['poster_ip']) . "'
			GROUP BY poster_id
			ORDER BY postings DESC";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			// Fill the user select list with users who have posted under this IP
			if ($row['poster_id'] != $post_info['poster_id'])
			{
				$users_ary[$row['poster_id']] = $row;
			}
		}
		$db->sql_freeresult($result);

		if (sizeof($users_ary))
		{
			// Get the usernames
			$sql = 'SELECT user_id, username
				FROM ' . USERS_TABLE . '
				WHERE ' . $db->sql_in_set('user_id', array_keys($users_ary));
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$users_ary[$row['user_id']]['username'] = $row['username'];
				$usernames_ary[utf8_clean_string($row['username'])] = $users_ary[$row['user_id']];
			}
			$db->sql_freeresult($result);

			foreach ($users_ary as $user_id => $user_row)
			{
				$template->assign_block_vars('userrow', array(
					'USERNAME'		=> ($user_id == ANONYMOUS) ? $user->lang['GUEST'] : $user_row['username'],
					'NUM_POSTS'		=> $user_row['postings'],
					'L_POST_S'		=> ($user_row['postings'] == 1) ? $user->lang['POST'] : $user->lang['POSTS'],

					'U_PROFILE'		=> ($user_id == ANONYMOUS) ? '' : append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=viewprofile&amp;u=' . $user_id),
					'U_SEARCHPOSTS' => append_sid("{$phpbb_root_path}search.$phpEx", 'author_id=' . $user_id . '&amp;sr=topics'))
				);
			}
		}

		// Get other IP's this user has posted under

		// A compound index on poster_id, poster_ip (posts table) would help speed up this query a lot,
		// but the extra size is only valuable if there are persons having more than a thousands posts.
		// This is better left to the really really big forums.

		$sql = 'SELECT poster_ip, COUNT(poster_ip) AS postings
			FROM ' . POSTS_TABLE . '
			WHERE poster_id = ' . $post_info['poster_id'] . "
			GROUP BY poster_ip
			ORDER BY postings DESC";
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$hostname = (($rdns_ip_num == $row['poster_ip'] || $rdns_ip_num == 'all') && $row['poster_ip']) ? @gethostbyaddr($row['poster_ip']) : '';

			$template->assign_block_vars('iprow', array(
				'IP'			=> $row['poster_ip'],
				'HOSTNAME'		=> $hostname,
				'NUM_POSTS'		=> $row['postings'],
				'L_POST_S'		=> ($row['postings'] == 1) ? $user->lang['POST'] : $user->lang['POSTS'],

				'U_LOOKUP_IP'	=> ($rdns_ip_num == $row['poster_ip'] || $rdns_ip_num == 'all') ? '' : "$url&amp;i=$id&amp;mode=post_details&amp;rdns={$row['poster_ip']}#ip",
				'U_WHOIS'		=> append_sid("{$phpbb_root_path}mcp.$phpEx", "i=$id&amp;mode=$mode&amp;action=whois&amp;p=$post_id&amp;ip={$row['poster_ip']}"))
			);
		}
		$db->sql_freeresult($result);

		$user_select = '';

		if (sizeof($usernames_ary))
		{
			ksort($usernames_ary);

			foreach ($usernames_ary as $row)
			{
				$user_select .= '<option value="' . $row['poster_id'] . '">' . $row['username'] . "</option>\n";
			}
		}

		$template->assign_var('S_USER_SELECT', $user_select);
	}

}

/**
* Change a post's poster
*/
function change_poster(&$post_info, $userdata)
{
	global $auth, $db, $config, $phpbb_root_path, $phpEx;

	if (empty($userdata) || $userdata['user_id'] == $post_info['user_id'])
	{
		return;
	}

	$post_id = $post_info['post_id'];

	$sql = 'UPDATE ' . POSTS_TABLE . "
		SET poster_id = {$userdata['user_id']}
		WHERE post_id = $post_id";
	$db->sql_query($sql);

	// Resync topic/forum if needed
	if ($post_info['topic_last_post_id'] == $post_id || $post_info['forum_last_post_id'] == $post_id || $post_info['topic_first_post_id'] == $post_id)
	{
		sync('topic', 'topic_id', $post_info['topic_id'], false, false);
		sync('forum', 'forum_id', $post_info['forum_id'], false, false);
	}

	// Adjust post counts
	if ($post_info['post_postcount'])
	{
		$sql = 'UPDATE ' . USERS_TABLE . '
			SET user_posts = user_posts - 1
			WHERE user_id = ' . $post_info['user_id'] .'
			AND user_posts > 0';
		$db->sql_query($sql);

		$sql = 'UPDATE ' . USERS_TABLE . '
			SET user_posts = user_posts + 1
			WHERE user_id = ' . $userdata['user_id'];
		$db->sql_query($sql);
	}

	// Add posted to information for this topic for the new user
	markread('post', $post_info['forum_id'], $post_info['topic_id'], time(), $userdata['user_id']);

	// Remove the dotted topic option if the old user has no more posts within this topic
	if ($config['load_db_track'] && $post_info['user_id'] != ANONYMOUS)
	{
		$sql = 'SELECT topic_id
			FROM ' . POSTS_TABLE . '
			WHERE topic_id = ' . $post_info['topic_id'] . '
				AND poster_id = ' . $post_info['user_id'];
		$result = $db->sql_query_limit($sql, 1);
		$topic_id = (int) $db->sql_fetchfield('topic_id');
		$db->sql_freeresult($result);

		if (!$topic_id)
		{
			$sql = 'DELETE FROM ' . TOPICS_POSTED_TABLE . '
				WHERE user_id = ' . $post_info['user_id'] . '
					AND topic_id = ' . $post_info['topic_id'];
			$db->sql_query($sql);
		}
	}

	// change the poster_id within the attachments table, else the data becomes out of sync and errors displayed because of wrong ownership
	if ($post_info['post_attachment'])
	{
		$sql = 'UPDATE ' . ATTACHMENTS_TABLE . '
			SET poster_id = ' . $userdata['user_id'] . '
			WHERE poster_id = ' . $post_info['user_id'] . '
				AND post_msg_id = ' . $post_info['post_id'] . '
				AND topic_id = ' . $post_info['topic_id'];
		$db->sql_query($sql);
	}

	// refresh search cache of this post
	$search_type = basename($config['search_type']);

	if (file_exists($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx))
	{
		require("{$phpbb_root_path}includes/search/$search_type.$phpEx");
	
		// We do some additional checks in the module to ensure it can actually be utilised
		$error = false;
		$search = new $search_type($error);
	
		if (!$error && method_exists($search, 'destroy_cache'))
		{
			$search->destroy_cache(array(), array($post_info['user_id'], $userdata['user_id']));
		}
	}

	$from_username = $post_info['username'];
	$to_username = $userdata['username'];

	// Renew post info
	$post_info = get_post_data(array($post_id), false, true);

	if (!sizeof($post_info))
	{
		trigger_error('POST_NOT_EXIST');
	}

	$post_info = $post_info[$post_id];

	// Now add log entry
	add_log('mod', $post_info['forum_id'], $post_info['topic_id'], 'LOG_MCP_CHANGE_POSTER', $post_info['topic_title'], $from_username, $to_username);
}

?>