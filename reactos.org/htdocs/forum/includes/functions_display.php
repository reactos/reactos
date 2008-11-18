<?php
/**
*
* @package phpBB3
* @version $Id: functions_display.php 8479 2008-03-29 00:22:48Z naderman $
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
* Display Forums
*/
function display_forums($root_data = '', $display_moderators = true, $return_moderators = false)
{
	global $db, $auth, $user, $template;
	global $phpbb_root_path, $phpEx, $config;

	$forum_rows = $subforums = $forum_ids = $forum_ids_moderator = $forum_moderators = $active_forum_ary = array();
	$parent_id = $visible_forums = 0;
	$sql_from = '';

	// Mark forums read?
	$mark_read = request_var('mark', '');

	if ($mark_read == 'all')
	{
		$mark_read = '';
	}

	if (!$root_data)
	{
		if ($mark_read == 'forums')
		{
			$mark_read = 'all';
		}

		$root_data = array('forum_id' => 0);
		$sql_where = '';
	}
	else
	{
		$sql_where = 'left_id > ' . $root_data['left_id'] . ' AND left_id < ' . $root_data['right_id'];
	}

	// Display list of active topics for this category?
	$show_active = (isset($root_data['forum_flags']) && ($root_data['forum_flags'] & FORUM_FLAG_ACTIVE_TOPICS)) ? true : false;

	$sql_array = array(
		'SELECT'	=> 'f.*',
		'FROM'		=> array(
			FORUMS_TABLE		=> 'f'
		),
		'LEFT_JOIN'	=> array(),
	);

	if ($config['load_db_lastread'] && $user->data['is_registered'])
	{
		$sql_array['LEFT_JOIN'][] = array('FROM' => array(FORUMS_TRACK_TABLE => 'ft'), 'ON' => 'ft.user_id = ' . $user->data['user_id'] . ' AND ft.forum_id = f.forum_id');
		$sql_array['SELECT'] .= ', ft.mark_time';
	}
	else if ($config['load_anon_lastread'] || $user->data['is_registered'])
	{
		$tracking_topics = (isset($_COOKIE[$config['cookie_name'] . '_track'])) ? ((STRIP) ? stripslashes($_COOKIE[$config['cookie_name'] . '_track']) : $_COOKIE[$config['cookie_name'] . '_track']) : '';
		$tracking_topics = ($tracking_topics) ? tracking_unserialize($tracking_topics) : array();

		if (!$user->data['is_registered'])
		{
			$user->data['user_lastmark'] = (isset($tracking_topics['l'])) ? (int) (base_convert($tracking_topics['l'], 36, 10) + $config['board_startdate']) : 0;
		}
	}

	if ($show_active)
	{
		$sql_array['LEFT_JOIN'][] = array(
			'FROM'	=> array(FORUMS_ACCESS_TABLE => 'fa'),
			'ON'	=> "fa.forum_id = f.forum_id AND fa.session_id = '" . $db->sql_escape($user->session_id) . "'"
		);

		$sql_array['SELECT'] .= ', fa.user_id';
	}

	$sql = $db->sql_build_query('SELECT', array(
		'SELECT'	=> $sql_array['SELECT'],
		'FROM'		=> $sql_array['FROM'],
		'LEFT_JOIN'	=> $sql_array['LEFT_JOIN'],

		'WHERE'		=> $sql_where,

		'ORDER_BY'	=> 'f.left_id',
	));

	$result = $db->sql_query($sql);

	$forum_tracking_info = array();
	$branch_root_id = $root_data['forum_id'];
	while ($row = $db->sql_fetchrow($result))
	{
		$forum_id = $row['forum_id'];

		// Mark forums read?
		if ($mark_read == 'forums' || $mark_read == 'all')
		{
			if ($auth->acl_get('f_list', $forum_id))
			{
				$forum_ids[] = $forum_id;
				continue;
			}
		}

		// Category with no members
		if ($row['forum_type'] == FORUM_CAT && ($row['left_id'] + 1 == $row['right_id']))
		{
			continue;
		}

		// Skip branch
		if (isset($right_id))
		{
			if ($row['left_id'] < $right_id)
			{
				continue;
			}
			unset($right_id);
		}

		if (!$auth->acl_get('f_list', $forum_id))
		{
			// if the user does not have permissions to list this forum, skip everything until next branch
			$right_id = $row['right_id'];
			continue;
		}

		$forum_ids[] = $forum_id;

		if ($config['load_db_lastread'] && $user->data['is_registered'])
		{
			$forum_tracking_info[$forum_id] = (!empty($row['mark_time'])) ? $row['mark_time'] : $user->data['user_lastmark'];
		}
		else if ($config['load_anon_lastread'] || $user->data['is_registered'])
		{
			if (!$user->data['is_registered'])
			{
				$user->data['user_lastmark'] = (isset($tracking_topics['l'])) ? (int) (base_convert($tracking_topics['l'], 36, 10) + $config['board_startdate']) : 0;
			}
			$forum_tracking_info[$forum_id] = (isset($tracking_topics['f'][$forum_id])) ? (int) (base_convert($tracking_topics['f'][$forum_id], 36, 10) + $config['board_startdate']) : $user->data['user_lastmark'];
		}

		$row['forum_topics'] = ($auth->acl_get('m_approve', $forum_id)) ? $row['forum_topics_real'] : $row['forum_topics'];

		// Display active topics from this forum?
		if ($show_active && $row['forum_type'] == FORUM_POST && $auth->acl_get('f_read', $forum_id) && ($row['forum_flags'] & FORUM_FLAG_ACTIVE_TOPICS))
		{
			if (!isset($active_forum_ary['forum_topics']))
			{
				$active_forum_ary['forum_topics'] = 0;
			}

			if (!isset($active_forum_ary['forum_posts']))
			{
				$active_forum_ary['forum_posts'] = 0;
			}

			$active_forum_ary['forum_id'][]		= $forum_id;
			$active_forum_ary['enable_icons'][]	= $row['enable_icons'];
			$active_forum_ary['forum_topics']	+= $row['forum_topics'];
			$active_forum_ary['forum_posts']	+= $row['forum_posts'];

			// If this is a passworded forum we do not show active topics from it if the user is not authorised to view it...
			if ($row['forum_password'] && $row['user_id'] != $user->data['user_id'])
			{
				$active_forum_ary['exclude_forum_id'][] = $forum_id;
			}
		}

		//
		if ($row['parent_id'] == $root_data['forum_id'] || $row['parent_id'] == $branch_root_id)
		{
			if ($row['forum_type'] != FORUM_CAT)
			{
				$forum_ids_moderator[] = (int) $forum_id;
			}

			// Direct child of current branch
			$parent_id = $forum_id;
			$forum_rows[$forum_id] = $row;

			if ($row['forum_type'] == FORUM_CAT && $row['parent_id'] == $root_data['forum_id'])
			{
				$branch_root_id = $forum_id;
			}
			$forum_rows[$parent_id]['forum_id_last_post'] = $row['forum_id'];
			$forum_rows[$parent_id]['orig_forum_last_post_time'] = $row['forum_last_post_time'];
		}
		else if ($row['forum_type'] != FORUM_CAT)
		{
			$subforums[$parent_id][$forum_id]['display'] = ($row['display_on_index']) ? true : false;
			$subforums[$parent_id][$forum_id]['name'] = $row['forum_name'];
			$subforums[$parent_id][$forum_id]['orig_forum_last_post_time'] = $row['forum_last_post_time'];

			$forum_rows[$parent_id]['forum_topics'] += $row['forum_topics'];

			// Do not list redirects in LINK Forums as Posts.
			if ($row['forum_type'] != FORUM_LINK)
			{
				$forum_rows[$parent_id]['forum_posts'] += $row['forum_posts'];
			}

			if ($row['forum_last_post_time'] > $forum_rows[$parent_id]['forum_last_post_time'])
			{
				$forum_rows[$parent_id]['forum_last_post_id'] = $row['forum_last_post_id'];
				$forum_rows[$parent_id]['forum_last_post_subject'] = $row['forum_last_post_subject'];
				$forum_rows[$parent_id]['forum_last_post_time'] = $row['forum_last_post_time'];
				$forum_rows[$parent_id]['forum_last_poster_id'] = $row['forum_last_poster_id'];
				$forum_rows[$parent_id]['forum_last_poster_name'] = $row['forum_last_poster_name'];
				$forum_rows[$parent_id]['forum_last_poster_colour'] = $row['forum_last_poster_colour'];
				$forum_rows[$parent_id]['forum_id_last_post'] = $forum_id;
			}
		}
	}
	$db->sql_freeresult($result);

	// Handle marking posts
	if ($mark_read == 'forums' || $mark_read == 'all')
	{
		$redirect = build_url('mark');

		if ($mark_read == 'all')
		{
			markread('all');

			$message = sprintf($user->lang['RETURN_INDEX'], '<a href="' . $redirect . '">', '</a>');
		}
		else
		{
			markread('topics', $forum_ids);

			$message = sprintf($user->lang['RETURN_FORUM'], '<a href="' . $redirect . '">', '</a>');
		}

		meta_refresh(3, $redirect);
		trigger_error($user->lang['FORUMS_MARKED'] . '<br /><br />' . $message);
	}

	// Grab moderators ... if necessary
	if ($display_moderators)
	{
		if ($return_moderators)
		{
			$forum_ids_moderator[] = $root_data['forum_id'];
		}
		get_moderators($forum_moderators, $forum_ids_moderator);
	}

	// Used to tell whatever we have to create a dummy category or not.
	$last_catless = true;
	foreach ($forum_rows as $row)
	{
		// Empty category
		if ($row['parent_id'] == $root_data['forum_id'] && $row['forum_type'] == FORUM_CAT)
		{
			$template->assign_block_vars('forumrow', array(
				'S_IS_CAT'				=> true,
				'FORUM_ID'				=> $row['forum_id'],
				'FORUM_NAME'			=> $row['forum_name'],
				'FORUM_DESC'			=> generate_text_for_display($row['forum_desc'], $row['forum_desc_uid'], $row['forum_desc_bitfield'], $row['forum_desc_options']),
				'FORUM_FOLDER_IMG'		=> '',
				'FORUM_FOLDER_IMG_SRC'	=> '',
				'FORUM_IMAGE'			=> ($row['forum_image']) ? '<img src="' . $phpbb_root_path . $row['forum_image'] . '" alt="' . $user->lang['FORUM_CAT'] . '" />' : '',
				'FORUM_IMAGE_SRC'		=> ($row['forum_image']) ? $phpbb_root_path . $row['forum_image'] : '',
				'U_VIEWFORUM'			=> append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $row['forum_id']))
			);

			continue;
		}

		$visible_forums++;
		$forum_id = $row['forum_id'];

		$forum_unread = (isset($forum_tracking_info[$forum_id]) && $row['orig_forum_last_post_time'] > $forum_tracking_info[$forum_id]) ? true : false;

		$folder_image = $folder_alt = $l_subforums = '';
		$subforums_list = array();

		// Generate list of subforums if we need to
		if (isset($subforums[$forum_id]))
		{
			foreach ($subforums[$forum_id] as $subforum_id => $subforum_row)
			{
				$subforum_unread = (isset($forum_tracking_info[$subforum_id]) && $subforum_row['orig_forum_last_post_time'] > $forum_tracking_info[$subforum_id]) ? true : false;

				if ($subforum_row['display'] && $subforum_row['name'])
				{
					$subforums_list[] = array(
						'link'		=> append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $subforum_id),
						'name'		=> $subforum_row['name'],
						'unread'	=> $subforum_unread,
					);
				}
				else
				{
					unset($subforums[$forum_id][$subforum_id]);
				}

				// If one subforum is unread the forum gets unread too...
				if ($subforum_unread)
				{
					$forum_unread = true;
				}
			}

			$l_subforums = (sizeof($subforums[$forum_id]) == 1) ? $user->lang['SUBFORUM'] . ': ' : $user->lang['SUBFORUMS'] . ': ';
			$folder_image = ($forum_unread) ? 'forum_unread_subforum' : 'forum_read_subforum';
		}
		else
		{
			switch ($row['forum_type'])
			{
				case FORUM_POST:
					$folder_image = ($forum_unread) ? 'forum_unread' : 'forum_read';
				break;

				case FORUM_LINK:
					$folder_image = 'forum_link';
				break;
			}
		}

		// Which folder should we display?
		if ($row['forum_status'] == ITEM_LOCKED)
		{
			$folder_image = ($forum_unread) ? 'forum_unread_locked' : 'forum_read_locked';
			$folder_alt = 'FORUM_LOCKED';
		}
		else
		{
			$folder_alt = ($forum_unread) ? 'NEW_POSTS' : 'NO_NEW_POSTS';
		}

		// Create last post link information, if appropriate
		if ($row['forum_last_post_id'])
		{
			$last_post_subject = $row['forum_last_post_subject'];
			$last_post_time = $user->format_date($row['forum_last_post_time']);
			$last_post_url = append_sid("{$phpbb_root_path}viewtopic.$phpEx", 'f=' . $row['forum_id_last_post'] . '&amp;p=' . $row['forum_last_post_id']) . '#p' . $row['forum_last_post_id'];
		}
		else
		{
			$last_post_subject = $last_post_time = $last_post_url = '';
		}

		// Output moderator listing ... if applicable
		$l_moderator = $moderators_list = '';
		if ($display_moderators && !empty($forum_moderators[$forum_id]))
		{
			$l_moderator = (sizeof($forum_moderators[$forum_id]) == 1) ? $user->lang['MODERATOR'] : $user->lang['MODERATORS'];
			$moderators_list = implode(', ', $forum_moderators[$forum_id]);
		}

		$l_post_click_count = ($row['forum_type'] == FORUM_LINK) ? 'CLICKS' : 'POSTS';
		$post_click_count = ($row['forum_type'] != FORUM_LINK || $row['forum_flags'] & FORUM_FLAG_LINK_TRACK) ? $row['forum_posts'] : '';

		$s_subforums_list = array();
		foreach ($subforums_list as $subforum)
		{
			$s_subforums_list[] = '<a href="' . $subforum['link'] . '" class="subforum ' . (($subforum['unread']) ? 'unread' : 'read') . '" title="' . (($subforum['unread']) ? $user->lang['NEW_POSTS'] : $user->lang['NO_NEW_POSTS']) . '">' . $subforum['name'] . '</a>';
		}
		$s_subforums_list = (string) implode(', ', $s_subforums_list);
		$catless = ($row['parent_id'] == $root_data['forum_id']) ? true : false;

		if ($row['forum_type'] != FORUM_LINK)
		{
			$u_viewforum = append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $row['forum_id']);
		}
		else
		{
			// If the forum is a link and we count redirects we need to visit it
			// If the forum is having a password or no read access we do not expose the link, but instead handle it in viewforum
			if (($row['forum_flags'] & FORUM_FLAG_LINK_TRACK) || $row['forum_password'] || !$auth->acl_get('f_read', $forum_id))
			{
				$u_viewforum = append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $row['forum_id']);
			}
			else
			{
				$u_viewforum = $row['forum_link'];
			}
		}

		$template->assign_block_vars('forumrow', array(
			'S_IS_CAT'			=> false,
			'S_NO_CAT'			=> $catless && !$last_catless,
			'S_IS_LINK'			=> ($row['forum_type'] == FORUM_LINK) ? true : false,
			'S_UNREAD_FORUM'	=> $forum_unread,
			'S_LOCKED_FORUM'	=> ($row['forum_status'] == ITEM_LOCKED) ? true : false,
			'S_LIST_SUBFORUMS'	=> ($row['display_subforum_list']) ? true : false,
			'S_SUBFORUMS'		=> (sizeof($subforums_list)) ? true : false,

			'FORUM_ID'				=> $row['forum_id'],
			'FORUM_NAME'			=> $row['forum_name'],
			'FORUM_DESC'			=> generate_text_for_display($row['forum_desc'], $row['forum_desc_uid'], $row['forum_desc_bitfield'], $row['forum_desc_options']),
			'TOPICS'				=> $row['forum_topics'],
			$l_post_click_count		=> $post_click_count,
			'FORUM_FOLDER_IMG'		=> $user->img($folder_image, $folder_alt),
			'FORUM_FOLDER_IMG_SRC'	=> $user->img($folder_image, $folder_alt, false, '', 'src'),
			'FORUM_FOLDER_IMG_ALT'	=> isset($user->lang[$folder_alt]) ? $user->lang[$folder_alt] : '',
			'FORUM_IMAGE'			=> ($row['forum_image']) ? '<img src="' . $phpbb_root_path . $row['forum_image'] . '" alt="' . $user->lang[$folder_alt] . '" />' : '',
			'FORUM_IMAGE_SRC'		=> ($row['forum_image']) ? $phpbb_root_path . $row['forum_image'] : '',
			'LAST_POST_SUBJECT'		=> censor_text($last_post_subject),
			'LAST_POST_TIME'		=> $last_post_time,
			'LAST_POSTER'			=> get_username_string('username', $row['forum_last_poster_id'], $row['forum_last_poster_name'], $row['forum_last_poster_colour']),
			'LAST_POSTER_COLOUR'	=> get_username_string('colour', $row['forum_last_poster_id'], $row['forum_last_poster_name'], $row['forum_last_poster_colour']),
			'LAST_POSTER_FULL'		=> get_username_string('full', $row['forum_last_poster_id'], $row['forum_last_poster_name'], $row['forum_last_poster_colour']),
			'MODERATORS'			=> $moderators_list,
			'SUBFORUMS'				=> $s_subforums_list,

			'L_SUBFORUM_STR'		=> $l_subforums,
			'L_FORUM_FOLDER_ALT'	=> $folder_alt,
			'L_MODERATOR_STR'		=> $l_moderator,

			'U_VIEWFORUM'		=> $u_viewforum,
			'U_LAST_POSTER'		=> get_username_string('profile', $row['forum_last_poster_id'], $row['forum_last_poster_name'], $row['forum_last_poster_colour']),
			'U_LAST_POST'		=> $last_post_url)
		);

		// Assign subforums loop for style authors
		foreach ($subforums_list as $subforum)
		{
			$template->assign_block_vars('forumrow.subforum', array(
				'U_SUBFORUM'	=> $subforum['link'],
				'SUBFORUM_NAME'	=> $subforum['name'],
				'S_UNREAD'		=> $subforum['unread'])
			);
		}

		$last_catless = $catless;
	}

	$template->assign_vars(array(
		'U_MARK_FORUMS'		=> ($user->data['is_registered'] || $config['load_anon_lastread']) ? append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $root_data['forum_id'] . '&amp;mark=forums') : '',
		'S_HAS_SUBFORUM'	=> ($visible_forums) ? true : false,
		'L_SUBFORUM'		=> ($visible_forums == 1) ? $user->lang['SUBFORUM'] : $user->lang['SUBFORUMS'],
		'LAST_POST_IMG'		=> $user->img('icon_topic_latest', 'VIEW_LATEST_POST'))
	);

	if ($return_moderators)
	{
		return array($active_forum_ary, $forum_moderators);
	}

	return array($active_forum_ary, array());
}

/**
* Create forum rules for given forum
*/
function generate_forum_rules(&$forum_data)
{
	if (!$forum_data['forum_rules'] && !$forum_data['forum_rules_link'])
	{
		return;
	}

	global $template, $phpbb_root_path, $phpEx;

	if ($forum_data['forum_rules'])
	{
		$forum_data['forum_rules'] = generate_text_for_display($forum_data['forum_rules'], $forum_data['forum_rules_uid'], $forum_data['forum_rules_bitfield'], $forum_data['forum_rules_options']);
	}

	$template->assign_vars(array(
		'S_FORUM_RULES'	=> true,
		'U_FORUM_RULES'	=> $forum_data['forum_rules_link'],
		'FORUM_RULES'	=> $forum_data['forum_rules'])
	);
}

/**
* Create forum navigation links for given forum, create parent
* list if currently null, assign basic forum info to template
*/
function generate_forum_nav(&$forum_data)
{
	global $db, $user, $template, $auth;
	global $phpEx, $phpbb_root_path;

	if (!$auth->acl_get('f_list', $forum_data['forum_id']))
	{
		return;
	}

	// Get forum parents
	$forum_parents = get_forum_parents($forum_data);

	// Build navigation links
	if (!empty($forum_parents))
	{
		foreach ($forum_parents as $parent_forum_id => $parent_data)
		{
			list($parent_name, $parent_type) = array_values($parent_data);

			// Skip this parent if the user does not have the permission to view it
			if (!$auth->acl_get('f_list', $parent_forum_id))
			{
				continue;
			}

			$template->assign_block_vars('navlinks', array(
				'S_IS_CAT'		=> ($parent_type == FORUM_CAT) ? true : false,
				'S_IS_LINK'		=> ($parent_type == FORUM_LINK) ? true : false,
				'S_IS_POST'		=> ($parent_type == FORUM_POST) ? true : false,
				'FORUM_NAME'	=> $parent_name,
				'FORUM_ID'		=> $parent_forum_id,
				'U_VIEW_FORUM'	=> append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $parent_forum_id))
			);
		}
	}

	$template->assign_block_vars('navlinks', array(
		'S_IS_CAT'		=> ($forum_data['forum_type'] == FORUM_CAT) ? true : false,
		'S_IS_LINK'		=> ($forum_data['forum_type'] == FORUM_LINK) ? true : false,
		'S_IS_POST'		=> ($forum_data['forum_type'] == FORUM_POST) ? true : false,
		'FORUM_NAME'	=> $forum_data['forum_name'],
		'FORUM_ID'		=> $forum_data['forum_id'],
		'U_VIEW_FORUM'	=> append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $forum_data['forum_id']))
	);

	$template->assign_vars(array(
		'FORUM_ID' 		=> $forum_data['forum_id'],
		'FORUM_NAME'	=> $forum_data['forum_name'],
		'FORUM_DESC'	=> generate_text_for_display($forum_data['forum_desc'], $forum_data['forum_desc_uid'], $forum_data['forum_desc_bitfield'], $forum_data['forum_desc_options']))
	);

	return;
}

/**
* Returns forum parents as an array. Get them from forum_data if available, or update the database otherwise
*/
function get_forum_parents(&$forum_data)
{
	global $db;

	$forum_parents = array();

	if ($forum_data['parent_id'] > 0)
	{
		if ($forum_data['forum_parents'] == '')
		{
			$sql = 'SELECT forum_id, forum_name, forum_type
				FROM ' . FORUMS_TABLE . '
				WHERE left_id < ' . $forum_data['left_id'] . '
					AND right_id > ' . $forum_data['right_id'] . '
				ORDER BY left_id ASC';
			$result = $db->sql_query($sql);

			while ($row = $db->sql_fetchrow($result))
			{
				$forum_parents[$row['forum_id']] = array($row['forum_name'], (int) $row['forum_type']);
			}
			$db->sql_freeresult($result);

			$forum_data['forum_parents'] = serialize($forum_parents);

			$sql = 'UPDATE ' . FORUMS_TABLE . "
				SET forum_parents = '" . $db->sql_escape($forum_data['forum_parents']) . "'
				WHERE parent_id = " . $forum_data['parent_id'];
			$db->sql_query($sql);
		}
		else
		{
			$forum_parents = unserialize($forum_data['forum_parents']);
		}
	}

	return $forum_parents;
}

/**
* Generate topic pagination
*/
function topic_generate_pagination($replies, $url)
{
	global $config, $user;

	// Make sure $per_page is a valid value
	$per_page = ($config['posts_per_page'] <= 0) ? 1 : $config['posts_per_page'];

	if (($replies + 1) > $per_page)
	{
		$total_pages = ceil(($replies + 1) / $per_page);
		$pagination = '';

		$times = 1;
		for ($j = 0; $j < $replies + 1; $j += $per_page)
		{
			$pagination .= '<a href="' . $url . '&amp;start=' . $j . '">' . $times . '</a>';
			if ($times == 1 && $total_pages > 5)
			{
				$pagination .= ' ... ';

				// Display the last three pages
				$times = $total_pages - 3;
				$j += ($total_pages - 4) * $per_page;
			}
			else if ($times < $total_pages)
			{
				$pagination .= '<span class="page-sep">' . $user->lang['COMMA_SEPARATOR'] . '</span>';
			}
			$times++;
		}
	}
	else
	{
		$pagination = '';
	}

	return $pagination;
}

/**
* Obtain list of moderators of each forum
*/
function get_moderators(&$forum_moderators, $forum_id = false)
{
	global $config, $template, $db, $phpbb_root_path, $phpEx;

	// Have we disabled the display of moderators? If so, then return
	// from whence we came ...
	if (!$config['load_moderators'])
	{
		return;
	}

	$forum_sql = '';

	if ($forum_id !== false)
	{
		if (!is_array($forum_id))
		{
			$forum_id = array($forum_id);
		}

		// If we don't have a forum then we can't have a moderator
		if (!sizeof($forum_id))
		{
			return;
		}

		$forum_sql = 'AND m.' . $db->sql_in_set('forum_id', $forum_id);
	}

	$sql_array = array(
		'SELECT'	=> 'm.*, u.user_colour, g.group_colour, g.group_type',

		'FROM'		=> array(
			MODERATOR_CACHE_TABLE	=> 'm',
		),

		'LEFT_JOIN'	=> array(
			array(
				'FROM'	=> array(USERS_TABLE => 'u'),
				'ON'	=> 'm.user_id = u.user_id',
			),
			array(
				'FROM'	=> array(GROUPS_TABLE => 'g'),
				'ON'	=> 'm.group_id = g.group_id',
			),
		),

		'WHERE'		=> "m.display_on_index = 1 $forum_sql",
	);

	$sql = $db->sql_build_query('SELECT', $sql_array);
	$result = $db->sql_query($sql, 3600);

	while ($row = $db->sql_fetchrow($result))
	{
		if (!empty($row['user_id']))
		{
			$forum_moderators[$row['forum_id']][] = get_username_string('full', $row['user_id'], $row['username'], $row['user_colour']);
		}
		else
		{
			$forum_moderators[$row['forum_id']][] = '<a' . (($row['group_colour']) ? ' style="color:#' . $row['group_colour'] . ';"' : '') . ' href="' . append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=group&amp;g=' . $row['group_id']) . '">' . (($row['group_type'] == GROUP_SPECIAL) ? $user->lang['G_' . $row['group_name']] : $row['group_name']) . '</a>';
		}
	}
	$db->sql_freeresult($result);

	return;
}

/**
* User authorisation levels output
*
* @param	string	$mode			Can be forum or topic. Not in use at the moment.
* @param	int		$forum_id		The current forum the user is in.
* @param	int		$forum_status	The forums status bit.
*/
function gen_forum_auth_level($mode, $forum_id, $forum_status)
{
	global $template, $auth, $user, $config;

	$locked = ($forum_status == ITEM_LOCKED && !$auth->acl_get('m_edit', $forum_id)) ? true : false;

	$rules = array(
		($auth->acl_get('f_post', $forum_id) && !$locked) ? $user->lang['RULES_POST_CAN'] : $user->lang['RULES_POST_CANNOT'],
		($auth->acl_get('f_reply', $forum_id) && !$locked) ? $user->lang['RULES_REPLY_CAN'] : $user->lang['RULES_REPLY_CANNOT'],
		($user->data['is_registered'] && $auth->acl_gets('f_edit', 'm_edit', $forum_id) && !$locked) ? $user->lang['RULES_EDIT_CAN'] : $user->lang['RULES_EDIT_CANNOT'],
		($user->data['is_registered'] && $auth->acl_gets('f_delete', 'm_delete', $forum_id) && !$locked) ? $user->lang['RULES_DELETE_CAN'] : $user->lang['RULES_DELETE_CANNOT'],
	);

	if ($config['allow_attachments'])
	{
		$rules[] = ($auth->acl_get('f_attach', $forum_id) && $auth->acl_get('u_attach') && !$locked) ? $user->lang['RULES_ATTACH_CAN'] : $user->lang['RULES_ATTACH_CANNOT'];
	}

	foreach ($rules as $rule)
	{
		$template->assign_block_vars('rules', array('RULE' => $rule));
	}

	return;
}

/**
* Generate topic status
*/
function topic_status(&$topic_row, $replies, $unread_topic, &$folder_img, &$folder_alt, &$topic_type)
{
	global $user, $config;

	$folder = $folder_new = '';

	if ($topic_row['topic_status'] == ITEM_MOVED)
	{
		$topic_type = $user->lang['VIEW_TOPIC_MOVED'];
		$folder_img = 'topic_moved';
		$folder_alt = 'TOPIC_MOVED';
	}
	else
	{
		switch ($topic_row['topic_type'])
		{
			case POST_GLOBAL:
				$topic_type = $user->lang['VIEW_TOPIC_GLOBAL'];
				$folder = 'global_read';
				$folder_new = 'global_unread';
			break;

			case POST_ANNOUNCE:
				$topic_type = $user->lang['VIEW_TOPIC_ANNOUNCEMENT'];
				$folder = 'announce_read';
				$folder_new = 'announce_unread';
			break;

			case POST_STICKY:
				$topic_type = $user->lang['VIEW_TOPIC_STICKY'];
				$folder = 'sticky_read';
				$folder_new = 'sticky_unread';
			break;

			default:
				$topic_type = '';
				$folder = 'topic_read';
				$folder_new = 'topic_unread';

				if ($config['hot_threshold'] && $replies >= $config['hot_threshold'] && $topic_row['topic_status'] != ITEM_LOCKED)
				{
					$folder .= '_hot';
					$folder_new .= '_hot';
				}
			break;
		}

		if ($topic_row['topic_status'] == ITEM_LOCKED)
		{
			$topic_type = $user->lang['VIEW_TOPIC_LOCKED'];
			$folder .= '_locked';
			$folder_new .= '_locked';
		}


		$folder_img = ($unread_topic) ? $folder_new : $folder;
		$folder_alt = ($unread_topic) ? 'NEW_POSTS' : (($topic_row['topic_status'] == ITEM_LOCKED) ? 'TOPIC_LOCKED' : 'NO_NEW_POSTS');

		// Posted image?
		if (!empty($topic_row['topic_posted']) && $topic_row['topic_posted'])
		{
			$folder_img .= '_mine';
		}
	}

	if ($topic_row['poll_start'] && $topic_row['topic_status'] != ITEM_MOVED)
	{
		$topic_type = $user->lang['VIEW_TOPIC_POLL'];
	}
}

/**
* Assign/Build custom bbcodes for display in screens supporting using of bbcodes
* The custom bbcodes buttons will be placed within the template block 'custom_codes'
*/
function display_custom_bbcodes()
{
	global $db, $template;

	// Start counting from 22 for the bbcode ids (every bbcode takes two ids - opening/closing)
	$num_predefined_bbcodes = 22;

	$sql = 'SELECT bbcode_id, bbcode_tag, bbcode_helpline
		FROM ' . BBCODES_TABLE . '
		WHERE display_on_posting = 1
		ORDER BY bbcode_tag';
	$result = $db->sql_query($sql);

	$i = 0;
	while ($row = $db->sql_fetchrow($result))
	{
		$template->assign_block_vars('custom_tags', array(
			'BBCODE_NAME'		=> "'[{$row['bbcode_tag']}]', '[/" . str_replace('=', '', $row['bbcode_tag']) . "]'",
			'BBCODE_ID'			=> $num_predefined_bbcodes + ($i * 2),
			'BBCODE_TAG'		=> $row['bbcode_tag'],
			'BBCODE_HELPLINE'	=> $row['bbcode_helpline'],
			'A_BBCODE_HELPLINE'	=> str_replace(array('&amp;', '&quot;', "'", '&lt;', '&gt;'), array('&', '"', "\'", '<', '>'), $row['bbcode_helpline']),
		));

		$i++;
	}
	$db->sql_freeresult($result);
}

/**
* Display reasons
*/
function display_reasons($reason_id = 0)
{
	global $db, $user, $template;

	$sql = 'SELECT *
		FROM ' . REPORTS_REASONS_TABLE . '
		ORDER BY reason_order ASC';
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		// If the reason is defined within the language file, we will use the localized version, else just use the database entry...
		if (isset($user->lang['report_reasons']['TITLE'][strtoupper($row['reason_title'])]) && isset($user->lang['report_reasons']['DESCRIPTION'][strtoupper($row['reason_title'])]))
		{
			$row['reason_description'] = $user->lang['report_reasons']['DESCRIPTION'][strtoupper($row['reason_title'])];
			$row['reason_title'] = $user->lang['report_reasons']['TITLE'][strtoupper($row['reason_title'])];
		}

		$template->assign_block_vars('reason', array(
			'ID'			=> $row['reason_id'],
			'TITLE'			=> $row['reason_title'],
			'DESCRIPTION'	=> $row['reason_description'],
			'S_SELECTED'	=> ($row['reason_id'] == $reason_id) ? true : false)
		);
	}
	$db->sql_freeresult($result);
}

/**
* Display user activity (action forum/topic)
*/
function display_user_activity(&$userdata)
{
	global $auth, $template, $db, $user;
	global $phpbb_root_path, $phpEx;

	// Do not display user activity for users having more than 5000 posts...
	if ($userdata['user_posts'] > 5000)
	{
		return;
	}

	$forum_ary = array();

	// Do not include those forums the user is not having read access to...
	$forum_read_ary = $auth->acl_getf('!f_read');

	foreach ($forum_read_ary as $forum_id => $not_allowed)
	{
		if ($not_allowed['f_read'])
		{
			$forum_ary[] = (int) $forum_id;
		}
	}

	$forum_ary = array_unique($forum_ary);
	$forum_sql = (sizeof($forum_ary)) ? 'AND ' . $db->sql_in_set('forum_id', $forum_ary, true) : '';

	// Obtain active forum
	$sql = 'SELECT forum_id, COUNT(post_id) AS num_posts
		FROM ' . POSTS_TABLE . '
		WHERE poster_id = ' . $userdata['user_id'] . "
			AND post_postcount = 1
			$forum_sql
		GROUP BY forum_id
		ORDER BY num_posts DESC";
	$result = $db->sql_query_limit($sql, 1);
	$active_f_row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if (!empty($active_f_row))
	{
		$sql = 'SELECT forum_name
			FROM ' . FORUMS_TABLE . '
			WHERE forum_id = ' . $active_f_row['forum_id'];
		$result = $db->sql_query($sql, 3600);
		$active_f_row['forum_name'] = (string) $db->sql_fetchfield('forum_name');
		$db->sql_freeresult($result);
	}

	// Obtain active topic
	$sql = 'SELECT topic_id, COUNT(post_id) AS num_posts
		FROM ' . POSTS_TABLE . '
		WHERE poster_id = ' . $userdata['user_id'] . "
			AND post_postcount = 1
			$forum_sql
		GROUP BY topic_id
		ORDER BY num_posts DESC";
	$result = $db->sql_query_limit($sql, 1);
	$active_t_row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if (!empty($active_t_row))
	{
		$sql = 'SELECT topic_title
			FROM ' . TOPICS_TABLE . '
			WHERE topic_id = ' . $active_t_row['topic_id'];
		$result = $db->sql_query($sql);
		$active_t_row['topic_title'] = (string) $db->sql_fetchfield('topic_title');
		$db->sql_freeresult($result);
	}

	$userdata['active_t_row'] = $active_t_row;
	$userdata['active_f_row'] = $active_f_row;

	$active_f_name = $active_f_id = $active_f_count = $active_f_pct = '';
	if (!empty($active_f_row['num_posts']))
	{
		$active_f_name = $active_f_row['forum_name'];
		$active_f_id = $active_f_row['forum_id'];
		$active_f_count = $active_f_row['num_posts'];
		$active_f_pct = ($userdata['user_posts']) ? ($active_f_count / $userdata['user_posts']) * 100 : 0;
	}

	$active_t_name = $active_t_id = $active_t_count = $active_t_pct = '';
	if (!empty($active_t_row['num_posts']))
	{
		$active_t_name = $active_t_row['topic_title'];
		$active_t_id = $active_t_row['topic_id'];
		$active_t_count = $active_t_row['num_posts'];
		$active_t_pct = ($userdata['user_posts']) ? ($active_t_count / $userdata['user_posts']) * 100 : 0;
	}

	$l_active_pct = ($userdata['user_id'] != ANONYMOUS && $userdata['user_id'] == $user->data['user_id']) ? $user->lang['POST_PCT_ACTIVE_OWN'] : $user->lang['POST_PCT_ACTIVE'];

	$template->assign_vars(array(
		'ACTIVE_FORUM'			=> $active_f_name,
		'ACTIVE_FORUM_POSTS'	=> ($active_f_count == 1) ? sprintf($user->lang['USER_POST'], 1) : sprintf($user->lang['USER_POSTS'], $active_f_count),
		'ACTIVE_FORUM_PCT'		=> sprintf($l_active_pct, $active_f_pct),
		'ACTIVE_TOPIC'			=> censor_text($active_t_name),
		'ACTIVE_TOPIC_POSTS'	=> ($active_t_count == 1) ? sprintf($user->lang['USER_POST'], 1) : sprintf($user->lang['USER_POSTS'], $active_t_count),
		'ACTIVE_TOPIC_PCT'		=> sprintf($l_active_pct, $active_t_pct),
		'U_ACTIVE_FORUM'		=> append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $active_f_id),
		'U_ACTIVE_TOPIC'		=> append_sid("{$phpbb_root_path}viewtopic.$phpEx", 't=' . $active_t_id),
		'S_SHOW_ACTIVITY'		=> true)
	);
}

/**
* Topic and forum watching common code
*/
function watch_topic_forum($mode, &$s_watching, $user_id, $forum_id, $topic_id, $notify_status = 'unset', $start = 0)
{
	global $template, $db, $user, $phpEx, $start, $phpbb_root_path;

	$table_sql = ($mode == 'forum') ? FORUMS_WATCH_TABLE : TOPICS_WATCH_TABLE;
	$where_sql = ($mode == 'forum') ? 'forum_id' : 'topic_id';
	$match_id = ($mode == 'forum') ? $forum_id : $topic_id;

	$u_url = ($mode == 'forum') ? 'f' : 'f=' . $forum_id . '&amp;t';

	// Is user watching this thread?
	if ($user_id != ANONYMOUS)
	{
		$can_watch = true;

		if ($notify_status == 'unset')
		{
			$sql = "SELECT notify_status
				FROM $table_sql
				WHERE $where_sql = $match_id
					AND user_id = $user_id";
			$result = $db->sql_query($sql);

			$notify_status = ($row = $db->sql_fetchrow($result)) ? $row['notify_status'] : NULL;
			$db->sql_freeresult($result);
		}

		if (!is_null($notify_status) && $notify_status !== '')
		{
			if (isset($_GET['unwatch']))
			{
				if ($_GET['unwatch'] == $mode)
				{
					$is_watching = 0;

					$sql = 'DELETE FROM ' . $table_sql . "
						WHERE $where_sql = $match_id
							AND user_id = $user_id";
					$db->sql_query($sql);
				}

				$redirect_url = append_sid("{$phpbb_root_path}view$mode.$phpEx", "$u_url=$match_id&amp;start=$start");

				meta_refresh(3, $redirect_url);

				$message = $user->lang['NOT_WATCHING_' . strtoupper($mode)] . '<br /><br />' . sprintf($user->lang['RETURN_' . strtoupper($mode)], '<a href="' . $redirect_url . '">', '</a>');
				trigger_error($message);
			}
			else
			{
				$is_watching = true;

				if ($notify_status)
				{
					$sql = 'UPDATE ' . $table_sql . "
						SET notify_status = 0
						WHERE $where_sql = $match_id
							AND user_id = $user_id";
					$db->sql_query($sql);
				}
			}
		}
		else
		{
			if (isset($_GET['watch']))
			{
				if ($_GET['watch'] == $mode)
				{
					$is_watching = true;

					$sql = 'INSERT INTO ' . $table_sql . " (user_id, $where_sql, notify_status)
						VALUES ($user_id, $match_id, 0)";
					$db->sql_query($sql);
				}

				$redirect_url = append_sid("{$phpbb_root_path}view$mode.$phpEx", "$u_url=$match_id&amp;start=$start");
				meta_refresh(3, $redirect_url);

				$message = $user->lang['ARE_WATCHING_' . strtoupper($mode)] . '<br /><br />' . sprintf($user->lang['RETURN_' . strtoupper($mode)], '<a href="' . $redirect_url . '">', '</a>');
				trigger_error($message);
			}
			else
			{
				$is_watching = 0;
			}
		}
	}
	else
	{
		if (isset($_GET['unwatch']) && $_GET['unwatch'] == $mode)
		{
			login_box();
		}
		else
		{
			$can_watch = 0;
			$is_watching = 0;
		}
	}

	if ($can_watch)
	{
		$s_watching['link'] = append_sid("{$phpbb_root_path}view$mode.$phpEx", "$u_url=$match_id&amp;" . (($is_watching) ? 'unwatch' : 'watch') . "=$mode&amp;start=$start");
		$s_watching['title'] = $user->lang[(($is_watching) ? 'STOP' : 'START') . '_WATCHING_' . strtoupper($mode)];
		$s_watching['is_watching'] = $is_watching;
	}

	return;
}

/**
* Get user rank title and image
*
* @param int $user_rank the current stored users rank id
* @param int $user_posts the users number of posts
* @param string &$rank_title the rank title will be stored here after execution
* @param string &$rank_img the rank image as full img tag is stored here after execution
* @param string &$rank_img_src the rank image source is stored here after execution
*
*/
function get_user_rank($user_rank, $user_posts, &$rank_title, &$rank_img, &$rank_img_src)
{
	global $ranks, $config, $phpbb_root_path;

	if (empty($ranks))
	{
		global $cache;
		$ranks = $cache->obtain_ranks();
	}

	if (!empty($user_rank))
	{
		$rank_title = (isset($ranks['special'][$user_rank]['rank_title'])) ? $ranks['special'][$user_rank]['rank_title'] : '';
		$rank_img = (!empty($ranks['special'][$user_rank]['rank_image'])) ? '<img src="' . $phpbb_root_path . $config['ranks_path'] . '/' . $ranks['special'][$user_rank]['rank_image'] . '" alt="' . $ranks['special'][$user_rank]['rank_title'] . '" title="' . $ranks['special'][$user_rank]['rank_title'] . '" />' : '';
		$rank_img_src = (!empty($ranks['special'][$user_rank]['rank_image'])) ? $phpbb_root_path . $config['ranks_path'] . '/' . $ranks['special'][$user_rank]['rank_image'] : '';
	}
	else
	{
		if (!empty($ranks['normal']))
		{
			foreach ($ranks['normal'] as $rank)
			{
				if ($user_posts >= $rank['rank_min'])
				{
					$rank_title = $rank['rank_title'];
					$rank_img = (!empty($rank['rank_image'])) ? '<img src="' . $phpbb_root_path . $config['ranks_path'] . '/' . $rank['rank_image'] . '" alt="' . $rank['rank_title'] . '" title="' . $rank['rank_title'] . '" />' : '';
					$rank_img_src = (!empty($rank['rank_image'])) ? $phpbb_root_path . $config['ranks_path'] . '/' . $rank['rank_image'] : '';
					break;
				}
			}
		}
	}
}

/**
* Get user avatar
*
* @param string $avatar Users assigned avatar name
* @param int $avatar_type Type of avatar
* @param string $avatar_width Width of users avatar
* @param string $avatar_height Height of users avatar
* @param string $alt Optional language string for alt tag within image, can be a language key or text
*
* @return string Avatar image
*/
function get_user_avatar($avatar, $avatar_type, $avatar_width, $avatar_height, $alt = 'USER_AVATAR')
{
	global $user, $config, $phpbb_root_path, $phpEx;

	if (empty($avatar) || !$avatar_type)
	{
		return '';
	}

	$avatar_img = '';

	switch ($avatar_type)
	{
		case AVATAR_UPLOAD:
			$avatar_img = $phpbb_root_path . "download/file.$phpEx?avatar=";
		break;

		case AVATAR_GALLERY:
			$avatar_img = $phpbb_root_path . $config['avatar_gallery_path'] . '/';
		break;
	}

	$avatar_img .= $avatar;
	return '<img src="' . $avatar_img . '" width="' . $avatar_width . '" height="' . $avatar_height . '" alt="' . ((!empty($user->lang[$alt])) ? $user->lang[$alt] : $alt) . '" />';
}

?>