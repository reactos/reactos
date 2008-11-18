<?php
/**
*
* @package mcp
* @version $Id: mcp_main.php 8479 2008-03-29 00:22:48Z naderman $
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
* mcp_main
* Handling mcp actions
* @package mcp
*/
class mcp_main
{
	var $p_master;
	var $u_action;

	function mcp_main(&$p_master)
	{
		$this->p_master = &$p_master;
	}

	function main($id, $mode)
	{
		global $auth, $db, $user, $template, $action;
		global $config, $phpbb_root_path, $phpEx;

		$quickmod = ($mode == 'quickmod') ? true : false;

		switch ($action)
		{
			case 'lock':
			case 'unlock':
				$topic_ids = (!$quickmod) ? request_var('topic_id_list', array(0)) : array(request_var('t', 0));

				if (!sizeof($topic_ids))
				{
					trigger_error('NO_TOPIC_SELECTED');
				}

				lock_unlock($action, $topic_ids);
			break;

			case 'lock_post':
			case 'unlock_post':

				$post_ids = (!$quickmod) ? request_var('post_id_list', array(0)) : array(request_var('p', 0));

				if (!sizeof($post_ids))
				{
					trigger_error('NO_POST_SELECTED');
				}

				lock_unlock($action, $post_ids);
			break;

			case 'make_announce':
			case 'make_sticky':
			case 'make_global':
			case 'make_normal':

				$topic_ids = (!$quickmod) ? request_var('topic_id_list', array(0)) : array(request_var('t', 0));

				if (!sizeof($topic_ids))
				{
					trigger_error('NO_TOPIC_SELECTED');
				}

				change_topic_type($action, $topic_ids);
			break;

			case 'move':
				$user->add_lang('viewtopic');

				$topic_ids = (!$quickmod) ? request_var('topic_id_list', array(0)) : array(request_var('t', 0));

				if (!sizeof($topic_ids))
				{
					trigger_error('NO_TOPIC_SELECTED');
				}

				mcp_move_topic($topic_ids);
			break;

			case 'fork':
				$user->add_lang('viewtopic');

				$topic_ids = (!$quickmod) ? request_var('topic_id_list', array(0)) : array(request_var('t', 0));

				if (!sizeof($topic_ids))
				{
					trigger_error('NO_TOPIC_SELECTED');
				}

				mcp_fork_topic($topic_ids);
			break;

			case 'delete_topic':
				$user->add_lang('viewtopic');

				$topic_ids = (!$quickmod) ? request_var('topic_id_list', array(0)) : array(request_var('t', 0));

				if (!sizeof($topic_ids))
				{
					trigger_error('NO_TOPIC_SELECTED');
				}

				mcp_delete_topic($topic_ids);
			break;

			case 'delete_post':
				$user->add_lang('posting');

				$post_ids = (!$quickmod) ? request_var('post_id_list', array(0)) : array(request_var('p', 0));

				if (!sizeof($post_ids))
				{
					trigger_error('NO_POST_SELECTED');
				}

				mcp_delete_post($post_ids);
			break;
		}

		switch ($mode)
		{
			case 'front':
				include($phpbb_root_path . 'includes/mcp/mcp_front.' . $phpEx);

				$user->add_lang('acp/common');

				mcp_front_view($id, $mode, $action);

				$this->tpl_name = 'mcp_front';
				$this->page_title = 'MCP_MAIN';
			break;

			case 'forum_view':
				include($phpbb_root_path . 'includes/mcp/mcp_forum.' . $phpEx);

				$user->add_lang('viewforum');

				$forum_id = request_var('f', 0);

				$forum_info = get_forum_data($forum_id, 'm_', true);

				if (!sizeof($forum_info))
				{
					$this->main('main', 'front');
					return;
				}

				$forum_info = $forum_info[$forum_id];

				mcp_forum_view($id, $mode, $action, $forum_info);

				$this->tpl_name = 'mcp_forum';
				$this->page_title = 'MCP_MAIN_FORUM_VIEW';
			break;

			case 'topic_view':
				include($phpbb_root_path . 'includes/mcp/mcp_topic.' . $phpEx);

				mcp_topic_view($id, $mode, $action);

				$this->tpl_name = 'mcp_topic';
				$this->page_title = 'MCP_MAIN_TOPIC_VIEW';
			break;

			case 'post_details':
				include($phpbb_root_path . 'includes/mcp/mcp_post.' . $phpEx);

				mcp_post_details($id, $mode, $action);

				$this->tpl_name = ($action == 'whois') ? 'mcp_whois' : 'mcp_post';
				$this->page_title = 'MCP_MAIN_POST_DETAILS';
			break;

			default:
				trigger_error('NO_MODE', E_USER_ERROR);
			break;
		}
	}
}

/**
* Lock/Unlock Topic/Post
*/
function lock_unlock($action, $ids)
{
	global $auth, $user, $db, $phpEx, $phpbb_root_path;

	if ($action == 'lock' || $action == 'unlock')
	{
		$table = TOPICS_TABLE;
		$sql_id = 'topic_id';
		$set_id = 'topic_status';
		$l_prefix = 'TOPIC';
	}
	else
	{
		$table = POSTS_TABLE;
		$sql_id = 'post_id';
		$set_id = 'post_edit_locked';
		$l_prefix = 'POST';
	}

	$orig_ids = $ids;

	if (!check_ids($ids, $table, $sql_id, array('m_lock')))
	{
		// Make sure that for f_user_lock only the lock action is triggered.
		if ($action != 'lock')
		{
			return;
		}

		$ids = $orig_ids;

		if (!check_ids($ids, $table, $sql_id, array('f_user_lock')))
		{
			return;
		}
	}
	unset($orig_ids);

	$redirect = request_var('redirect', build_url(array('_f_', 'action', 'quickmod')));

	$s_hidden_fields = build_hidden_fields(array(
		$sql_id . '_list'	=> $ids,
		'action'			=> $action,
		'redirect'			=> $redirect)
	);
	$success_msg = '';

	if (confirm_box(true))
	{
		$sql = "UPDATE $table
			SET $set_id = " . (($action == 'lock' || $action == 'lock_post') ? ITEM_LOCKED : ITEM_UNLOCKED) . '
			WHERE ' . $db->sql_in_set($sql_id, $ids);
		$db->sql_query($sql);

		$data = ($action == 'lock' || $action == 'unlock') ? get_topic_data($ids) : get_post_data($ids);

		foreach ($data as $id => $row)
		{
			add_log('mod', $row['forum_id'], $row['topic_id'], 'LOG_' . strtoupper($action), $row['topic_title']);
		}

		$success_msg = $l_prefix . ((sizeof($ids) == 1) ? '' : 'S') . '_' . (($action == 'lock' || $action == 'lock_post') ? 'LOCKED' : 'UNLOCKED') . '_SUCCESS';
	}
	else
	{
		confirm_box(false, strtoupper($action) . '_' . $l_prefix . ((sizeof($ids) == 1) ? '' : 'S'), $s_hidden_fields);
	}

	$redirect = request_var('redirect', "index.$phpEx");
	$redirect = reapply_sid($redirect);

	if (!$success_msg)
	{
		redirect($redirect);
	}
	else
	{
		meta_refresh(2, $redirect);
		trigger_error($user->lang[$success_msg] . '<br /><br />' . sprintf($user->lang['RETURN_PAGE'], '<a href="' . $redirect . '">', '</a>'));
	}
}

/**
* Change Topic Type
*/
function change_topic_type($action, $topic_ids)
{
	global $auth, $user, $db, $phpEx, $phpbb_root_path;

	// For changing topic types, we only allow operations in one forum.
	$forum_id = check_ids($topic_ids, TOPICS_TABLE, 'topic_id', array('f_announce', 'f_sticky', 'm_'), true);

	if ($forum_id === false)
	{
		return;
	}

	switch ($action)
	{
		case 'make_announce':
			$new_topic_type = POST_ANNOUNCE;
			$check_acl = 'f_announce';
			$l_new_type = (sizeof($topic_ids) == 1) ? 'MCP_MAKE_ANNOUNCEMENT' : 'MCP_MAKE_ANNOUNCEMENTS';
		break;

		case 'make_global':
			$new_topic_type = POST_GLOBAL;
			$check_acl = 'f_announce';
			$l_new_type = (sizeof($topic_ids) == 1) ? 'MCP_MAKE_GLOBAL' : 'MCP_MAKE_GLOBALS';
		break;

		case 'make_sticky':
			$new_topic_type = POST_STICKY;
			$check_acl = 'f_sticky';
			$l_new_type = (sizeof($topic_ids) == 1) ? 'MCP_MAKE_STICKY' : 'MCP_MAKE_STICKIES';
		break;

		default:
			$new_topic_type = POST_NORMAL;
			$check_acl = '';
			$l_new_type = (sizeof($topic_ids) == 1) ? 'MCP_MAKE_NORMAL' : 'MCP_MAKE_NORMALS';
		break;
	}

	$redirect = request_var('redirect', build_url(array('_f_', 'action', 'quickmod')));

	$s_hidden_fields = array(
		'topic_id_list'	=> $topic_ids,
		'f'				=> $forum_id,
		'action'		=> $action,
		'redirect'		=> $redirect,
	);
	$success_msg = '';

	if (confirm_box(true))
	{
		if ($new_topic_type != POST_GLOBAL)
		{
			$sql = 'UPDATE ' . TOPICS_TABLE . "
				SET topic_type = $new_topic_type
				WHERE " . $db->sql_in_set('topic_id', $topic_ids) . '
					AND forum_id <> 0';
			$db->sql_query($sql);

			// Reset forum id if a global topic is within the array
			$to_forum_id = request_var('to_forum_id', 0);

			if ($to_forum_id)
			{
				$sql = 'UPDATE ' . TOPICS_TABLE . "
					SET topic_type = $new_topic_type, forum_id = $to_forum_id
					WHERE " . $db->sql_in_set('topic_id', $topic_ids) . '
						AND forum_id = 0';
				$db->sql_query($sql);

				// Update forum_ids for all posts
				$sql = 'UPDATE ' . POSTS_TABLE . "
					SET forum_id = $to_forum_id
					WHERE " . $db->sql_in_set('topic_id', $topic_ids) . '
						AND forum_id = 0';
				$db->sql_query($sql);

				// Do a little forum sync stuff
				$sql = 'SELECT SUM(t.topic_replies + t.topic_approved) as topic_posts, COUNT(t.topic_approved) as topics_authed
					FROM ' . TOPICS_TABLE . ' t
					WHERE ' . $db->sql_in_set('t.topic_id', $topic_ids);
				$result = $db->sql_query($sql);
				$row_data = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$sync_sql = array();

				if ($row_data['topic_posts'])
				{
					$sync_sql[$to_forum_id][]	= 'forum_posts = forum_posts + ' . (int) $row_data['topic_posts'];
				}

				if ($row_data['topics_authed'])
				{
					$sync_sql[$to_forum_id][]	= 'forum_topics = forum_topics + ' . (int) $row_data['topics_authed'];
				}

				$sync_sql[$to_forum_id][]	= 'forum_topics_real = forum_topics_real + ' . (int) sizeof($topic_ids);

				foreach ($sync_sql as $forum_id_key => $array)
				{
					$sql = 'UPDATE ' . FORUMS_TABLE . '
						SET ' . implode(', ', $array) . '
						WHERE forum_id = ' . $forum_id_key;
					$db->sql_query($sql);
				}

				sync('forum', 'forum_id', $to_forum_id);
			}
		}
		else
		{
			// Get away with those topics already being a global announcement by re-calculating $topic_ids
			$sql = 'SELECT topic_id
				FROM ' . TOPICS_TABLE . '
				WHERE ' . $db->sql_in_set('topic_id', $topic_ids) . '
					AND forum_id <> 0';
			$result = $db->sql_query($sql);

			$topic_ids = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$topic_ids[] = $row['topic_id'];
			}
			$db->sql_freeresult($result);

			if (sizeof($topic_ids))
			{
				// Delete topic shadows for global announcements
				$sql = 'DELETE FROM ' . TOPICS_TABLE . '
					WHERE ' . $db->sql_in_set('topic_moved_id', $topic_ids);
				$db->sql_query($sql);

				$sql = 'UPDATE ' . TOPICS_TABLE . "
					SET topic_type = $new_topic_type, forum_id = 0
						WHERE " . $db->sql_in_set('topic_id', $topic_ids);
				$db->sql_query($sql);

				// Update forum_ids for all posts
				$sql = 'UPDATE ' . POSTS_TABLE . '
					SET forum_id = 0
					WHERE ' . $db->sql_in_set('topic_id', $topic_ids);
				$db->sql_query($sql);

				// Do a little forum sync stuff
				$sql = 'SELECT SUM(t.topic_replies + t.topic_approved) as topic_posts, COUNT(t.topic_approved) as topics_authed
					FROM ' . TOPICS_TABLE . ' t
					WHERE ' . $db->sql_in_set('t.topic_id', $topic_ids);
				$result = $db->sql_query($sql);
				$row_data = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				$sync_sql = array();

				if ($row_data['topic_posts'])
				{
					$sync_sql[$forum_id][]	= 'forum_posts = forum_posts - ' . (int) $row_data['topic_posts'];
				}

				if ($row_data['topics_authed'])
				{
					$sync_sql[$forum_id][]	= 'forum_topics = forum_topics - ' . (int) $row_data['topics_authed'];
				}

				$sync_sql[$forum_id][]	= 'forum_topics_real = forum_topics_real - ' . (int) sizeof($topic_ids);

				foreach ($sync_sql as $forum_id_key => $array)
				{
					$sql = 'UPDATE ' . FORUMS_TABLE . '
						SET ' . implode(', ', $array) . '
						WHERE forum_id = ' . $forum_id_key;
					$db->sql_query($sql);
				}

				sync('forum', 'forum_id', $forum_id);
			}
		}

		$success_msg = (sizeof($topic_ids) == 1) ? 'TOPIC_TYPE_CHANGED' : 'TOPICS_TYPE_CHANGED';

		if (sizeof($topic_ids))
		{
			$data = get_topic_data($topic_ids);

			foreach ($data as $topic_id => $row)
			{
				add_log('mod', $forum_id, $topic_id, 'LOG_TOPIC_TYPE_CHANGED', $row['topic_title']);
			}
		}
	}
	else
	{
		// Global topic involved?
		$global_involved = false;

		if ($new_topic_type != POST_GLOBAL)
		{
			$sql = 'SELECT forum_id
				FROM ' . TOPICS_TABLE . '
				WHERE ' . $db->sql_in_set('topic_id', $topic_ids) . '
					AND forum_id = 0';
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ($row)
			{
				$global_involved = true;
			}
		}

		if ($global_involved)
		{
			global $template;

			$template->assign_vars(array(
				'S_FORUM_SELECT'		=> make_forum_select(request_var('f', $forum_id), false, false, true, true),
				'S_CAN_LEAVE_SHADOW'	=> false,
				'ADDITIONAL_MSG'		=> (sizeof($topic_ids) == 1) ? $user->lang['SELECT_FORUM_GLOBAL_ANNOUNCEMENT'] : $user->lang['SELECT_FORUM_GLOBAL_ANNOUNCEMENTS'])
			);

			confirm_box(false, $l_new_type, build_hidden_fields($s_hidden_fields), 'mcp_move.html');
		}
		else
		{
			confirm_box(false, $l_new_type, build_hidden_fields($s_hidden_fields));
		}
	}

	$redirect = request_var('redirect', "index.$phpEx");
	$redirect = reapply_sid($redirect);

	if (!$success_msg)
	{
		redirect($redirect);
	}
	else
	{
		meta_refresh(2, $redirect);
		trigger_error($user->lang[$success_msg] . '<br /><br />' . sprintf($user->lang['RETURN_PAGE'], '<a href="' . $redirect . '">', '</a>'));
	}
}

/**
* Move Topic
*/
function mcp_move_topic($topic_ids)
{
	global $auth, $user, $db, $template;
	global $phpEx, $phpbb_root_path;

	// Here we limit the operation to one forum only
	$forum_id = check_ids($topic_ids, TOPICS_TABLE, 'topic_id', array('m_move'), true);

	if ($forum_id === false)
	{
		return;
	}

	$to_forum_id = request_var('to_forum_id', 0);
	$redirect = request_var('redirect', build_url(array('_f_', 'action', 'quickmod')));
	$additional_msg = $success_msg = '';

	$s_hidden_fields = build_hidden_fields(array(
		'topic_id_list'	=> $topic_ids,
		'f'				=> $forum_id,
		'action'		=> 'move',
		'redirect'		=> $redirect)
	);

	if ($to_forum_id)
	{
		$forum_data = get_forum_data($to_forum_id);

		if (!sizeof($forum_data))
		{
			$additional_msg = $user->lang['FORUM_NOT_EXIST'];
		}
		else
		{
			$forum_data = $forum_data[$to_forum_id];

			if ($forum_data['forum_type'] != FORUM_POST)
			{
				$additional_msg = $user->lang['FORUM_NOT_POSTABLE'];
			}
			else if (!$auth->acl_get('f_post', $to_forum_id))
			{
				$additional_msg = $user->lang['USER_CANNOT_POST'];
			}
			else if ($forum_id == $to_forum_id)
			{
				$additional_msg = $user->lang['CANNOT_MOVE_SAME_FORUM'];
			}
		}
	}
	else if (isset($_POST['confirm']))
	{
		$additional_msg = $user->lang['FORUM_NOT_EXIST'];
	}

	if (!$to_forum_id || $additional_msg)
	{
		unset($_POST['confirm']);
		unset($_REQUEST['confirm_key']);
	}

	if (confirm_box(true))
	{
		$topic_data = get_topic_data($topic_ids);
		$leave_shadow = (isset($_POST['move_leave_shadow'])) ? true : false;

		$topics_moved = sizeof($topic_ids);
		$topics_authed_moved = 0;
		$forum_sync_data = array();

		$forum_sync_data[$forum_id] = current($topic_data);
		$forum_sync_data[$to_forum_id] = $forum_data;

		foreach ($topic_data as $topic_id => $topic_info)
		{
			if ($topic_info['topic_approved'] == '1')
			{
				$topics_authed_moved++;
			}
		}

		$db->sql_transaction('begin');

		$sql = 'SELECT SUM(t.topic_replies + t.topic_approved) as topic_posts
			FROM ' . TOPICS_TABLE . ' t
			WHERE ' . $db->sql_in_set('t.topic_id', $topic_ids);
		$result = $db->sql_query($sql);
		$row_data = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		$sync_sql = array();

		if ($row_data['topic_posts'])
		{
			$sync_sql[$forum_id][]		= 'forum_posts = forum_posts - ' . (int) $row_data['topic_posts'];
			$sync_sql[$to_forum_id][]	= 'forum_posts = forum_posts + ' . (int) $row_data['topic_posts'];
		}

		if ($topics_authed_moved)
		{
			$sync_sql[$to_forum_id][]	= 'forum_topics = forum_topics + ' . (int) $topics_authed_moved;
		}

		$sync_sql[$to_forum_id][]	= 'forum_topics_real = forum_topics_real + ' . (int) $topics_moved;

		// Move topics, but do not resync yet
		move_topics($topic_ids, $to_forum_id, false);

		$forum_ids = array($to_forum_id);
		foreach ($topic_data as $topic_id => $row)
		{
			// Get the list of forums to resync, add a log entry
			$forum_ids[] = $row['forum_id'];
			add_log('mod', $to_forum_id, $topic_id, 'LOG_MOVE', $row['forum_name']);

			// If we have moved a global announcement, we need to correct the topic type
			if ($row['topic_type'] == POST_GLOBAL)
			{
				$sql = 'UPDATE ' . TOPICS_TABLE . '
					SET topic_type = ' . POST_ANNOUNCE . '
					WHERE topic_id = ' . (int) $row['topic_id'];
				$db->sql_query($sql);
			}

			// Leave a redirection if required and only if the topic is visible to users
			if ($leave_shadow && $row['topic_approved'] && $row['topic_type'] != POST_GLOBAL)
			{
				$shadow = array(
					'forum_id'				=>	(int) $row['forum_id'],
					'icon_id'				=>	(int) $row['icon_id'],
					'topic_attachment'		=>	(int) $row['topic_attachment'],
					'topic_approved'		=>	1,
					'topic_reported'		=>	(int) $row['topic_reported'],
					'topic_title'			=>	(string) $row['topic_title'],
					'topic_poster'			=>	(int) $row['topic_poster'],
					'topic_time'			=>	(int) $row['topic_time'],
					'topic_time_limit'		=>	(int) $row['topic_time_limit'],
					'topic_views'			=>	(int) $row['topic_views'],
					'topic_replies'			=>	(int) $row['topic_replies'],
					'topic_replies_real'	=>	(int) $row['topic_replies_real'],
					'topic_status'			=>	ITEM_MOVED,
					'topic_type'			=>	POST_NORMAL,
					'topic_first_post_id'	=>	(int) $row['topic_first_post_id'],
					'topic_first_poster_colour'=>(string) $row['topic_first_poster_colour'],
					'topic_first_poster_name'=>	(string) $row['topic_first_poster_name'],
					'topic_last_post_id'	=>	(int) $row['topic_last_post_id'],
					'topic_last_poster_id'	=>	(int) $row['topic_last_poster_id'],
					'topic_last_poster_colour'=>(string) $row['topic_last_poster_colour'],
					'topic_last_poster_name'=>	(string) $row['topic_last_poster_name'],
					'topic_last_post_subject'=>	(string)  $row['topic_last_post_subject'],
					'topic_last_post_time'	=>	(int) $row['topic_last_post_time'],
					'topic_last_view_time'	=>	(int) $row['topic_last_view_time'],
					'topic_moved_id'		=>	(int) $row['topic_id'],
					'topic_bumped'			=>	(int) $row['topic_bumped'],
					'topic_bumper'			=>	(int) $row['topic_bumper'],
					'poll_title'			=>	(string) $row['poll_title'],
					'poll_start'			=>	(int) $row['poll_start'],
					'poll_length'			=>	(int) $row['poll_length'],
					'poll_max_options'		=>	(int) $row['poll_max_options'],
					'poll_last_vote'		=>	(int) $row['poll_last_vote']
				);

				$db->sql_query('INSERT INTO ' . TOPICS_TABLE . $db->sql_build_array('INSERT', $shadow));

				$topics_authed_moved--;
				$topics_moved--;
			}
		}
		unset($topic_data);

		$sync_sql[$forum_id][]	= 'forum_topics_real = forum_topics_real - ' . (int) $topics_moved;

		if ($topics_authed_moved)
		{
			$sync_sql[$forum_id][]	= 'forum_topics = forum_topics - ' . (int) $topics_authed_moved;
		}

		$success_msg = (sizeof($topic_ids) == 1) ? 'TOPIC_MOVED_SUCCESS' : 'TOPICS_MOVED_SUCCESS';

		foreach ($sync_sql as $forum_id_key => $array)
		{
			$sql = 'UPDATE ' . FORUMS_TABLE . '
				SET ' . implode(', ', $array) . '
				WHERE forum_id = ' . $forum_id_key;
			$db->sql_query($sql);
		}

		$db->sql_transaction('commit');

		sync('forum', 'forum_id', array($forum_id, $to_forum_id));
	}
	else
	{
		$template->assign_vars(array(
			'S_FORUM_SELECT'		=> make_forum_select($to_forum_id, $forum_id, false, true, true, true),
			'S_CAN_LEAVE_SHADOW'	=> true,
			'ADDITIONAL_MSG'		=> $additional_msg)
		);

		confirm_box(false, 'MOVE_TOPIC' . ((sizeof($topic_ids) == 1) ? '' : 'S'), $s_hidden_fields, 'mcp_move.html');
	}

	$redirect = request_var('redirect', "index.$phpEx");
	$redirect = reapply_sid($redirect);

	if (!$success_msg)
	{
		redirect($redirect);
	}
	else
	{
		meta_refresh(3, $redirect);

		$message = $user->lang[$success_msg];
		$message .= '<br /><br />' . sprintf($user->lang['RETURN_PAGE'], '<a href="' . $redirect . '">', '</a>');
		$message .= '<br /><br />' . sprintf($user->lang['RETURN_FORUM'], '<a href="' . append_sid("{$phpbb_root_path}viewforum.$phpEx", "f=$forum_id") . '">', '</a>');
		$message .= '<br /><br />' . sprintf($user->lang['RETURN_NEW_FORUM'], '<a href="' . append_sid("{$phpbb_root_path}viewforum.$phpEx", "f=$to_forum_id") . '">', '</a>');

		trigger_error($message);
	}
}

/**
* Delete Topics
*/
function mcp_delete_topic($topic_ids)
{
	global $auth, $user, $db, $phpEx, $phpbb_root_path;

	if (!check_ids($topic_ids, TOPICS_TABLE, 'topic_id', array('m_delete')))
	{
		return;
	}

	$redirect = request_var('redirect', build_url(array('_f_', 'action', 'quickmod')));
	$forum_id = request_var('f', 0);

	$s_hidden_fields = build_hidden_fields(array(
		'topic_id_list'	=> $topic_ids,
		'f'				=> $forum_id,
		'action'		=> 'delete_topic',
		'redirect'		=> $redirect)
	);
	$success_msg = '';

	if (confirm_box(true))
	{
		$success_msg = (sizeof($topic_ids) == 1) ? 'TOPIC_DELETED_SUCCESS' : 'TOPICS_DELETED_SUCCESS';

		$data = get_topic_data($topic_ids);

		foreach ($data as $topic_id => $row)
		{
			add_log('mod', $row['forum_id'], 0, 'LOG_TOPIC_DELETED', $row['topic_title']);
		}

		$return = delete_topics('topic_id', $topic_ids);
	}
	else
	{
		confirm_box(false, (sizeof($topic_ids) == 1) ? 'DELETE_TOPIC' : 'DELETE_TOPICS', $s_hidden_fields);
	}

	$redirect = request_var('redirect', "index.$phpEx");
	$redirect = reapply_sid($redirect);

	if (!$success_msg)
	{
		redirect($redirect);
	}
	else
	{
		$redirect_url = append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $forum_id);
		meta_refresh(3, $redirect_url);
		trigger_error($user->lang[$success_msg] . '<br /><br />' . sprintf($user->lang['RETURN_FORUM'], '<a href="' . $redirect_url . '">', '</a>'));
	}
}

/**
* Delete Posts
*/
function mcp_delete_post($post_ids)
{
	global $auth, $user, $db, $phpEx, $phpbb_root_path;

	if (!check_ids($post_ids, POSTS_TABLE, 'post_id', array('m_delete')))
	{
		return;
	}

	$redirect = request_var('redirect', build_url(array('_f_', 'action', 'quickmod')));
	$forum_id = request_var('f', 0);

	$s_hidden_fields = build_hidden_fields(array(
		'post_id_list'	=> $post_ids,
		'f'				=> $forum_id,
		'action'		=> 'delete_post',
		'redirect'		=> $redirect)
	);
	$success_msg = '';

	if (confirm_box(true))
	{
		if (!function_exists('delete_posts'))
		{
			include($phpbb_root_path . 'includes/functions_admin.' . $phpEx);
		}

		// Count the number of topics that are affected
		// I did not use COUNT(DISTINCT ...) because I remember having problems
		// with it on older versions of MySQL -- Ashe

		$sql = 'SELECT DISTINCT topic_id
			FROM ' . POSTS_TABLE . '
			WHERE ' . $db->sql_in_set('post_id', $post_ids);
		$result = $db->sql_query($sql);

		$topic_id_list = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$topic_id_list[] = $row['topic_id'];
		}
		$affected_topics = sizeof($topic_id_list);
		$db->sql_freeresult($result);

		$post_data = get_post_data($post_ids);

		foreach ($post_data as $id => $row)
		{
			add_log('mod', $row['forum_id'], $row['topic_id'], 'LOG_DELETE_POST', $row['post_subject']);
		}

		// Now delete the posts, topics and forums are automatically resync'ed
		delete_posts('post_id', $post_ids);

		$sql = 'SELECT COUNT(topic_id) AS topics_left
			FROM ' . TOPICS_TABLE . '
			WHERE ' . $db->sql_in_set('topic_id', $topic_id_list);
		$result = $db->sql_query_limit($sql, 1);

		$deleted_topics = ($row = $db->sql_fetchrow($result)) ? ($affected_topics - $row['topics_left']) : $affected_topics;
		$db->sql_freeresult($result);

		$topic_id = request_var('t', 0);

		// Return links
		$return_link = array();
		if ($affected_topics == 1 && !$deleted_topics && $topic_id)
		{
			$return_link[] = sprintf($user->lang['RETURN_TOPIC'], '<a href="' . append_sid("{$phpbb_root_path}viewtopic.$phpEx", "f=$forum_id&amp;t=$topic_id") . '">', '</a>');
		}
		$return_link[] = sprintf($user->lang['RETURN_FORUM'], '<a href="' . append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $forum_id) . '">', '</a>');

		if (sizeof($post_ids) == 1)
		{
			if ($deleted_topics)
			{
				// We deleted the only post of a topic, which in turn has
				// been removed from the database
				$success_msg = $user->lang['TOPIC_DELETED_SUCCESS'];
			}
			else
			{
				$success_msg = $user->lang['POST_DELETED_SUCCESS'];
			}
		}
		else
		{
			if ($deleted_topics)
			{
				// Some of topics disappeared
				$success_msg = $user->lang['POSTS_DELETED_SUCCESS'] . '<br /><br />' . $user->lang['EMPTY_TOPICS_REMOVED_WARNING'];
			}
			else
			{
				$success_msg = $user->lang['POSTS_DELETED_SUCCESS'];
			}
		}
	}
	else
	{
		confirm_box(false, (sizeof($post_ids) == 1) ? 'DELETE_POST' : 'DELETE_POSTS', $s_hidden_fields);
	}

	$redirect = request_var('redirect', "index.$phpEx");
	$redirect = reapply_sid($redirect);

	if (!$success_msg)
	{
		redirect($redirect);
	}
	else
	{
		meta_refresh(3, $redirect);
		trigger_error($success_msg . '<br /><br />' . sprintf($user->lang['RETURN_PAGE'], '<a href="' . $redirect . '">', '</a>') . '<br /><br />' . implode('<br /><br />', $return_link));
	}
}

/**
* Fork Topic
*/
function mcp_fork_topic($topic_ids)
{
	global $auth, $user, $db, $template, $config;
	global $phpEx, $phpbb_root_path;

	if (!check_ids($topic_ids, TOPICS_TABLE, 'topic_id', array('m_')))
	{
		return;
	}

	$to_forum_id = request_var('to_forum_id', 0);
	$forum_id = request_var('f', 0);
	$redirect = request_var('redirect', build_url(array('_f_', 'action', 'quickmod')));
	$additional_msg = $success_msg = '';

	$s_hidden_fields = build_hidden_fields(array(
		'topic_id_list'	=> $topic_ids,
		'f'				=> $forum_id,
		'action'		=> 'fork',
		'redirect'		=> $redirect)
	);

	if ($to_forum_id)
	{
		$forum_data = get_forum_data($to_forum_id);

		if (!sizeof($topic_ids))
		{
			$additional_msg = $user->lang['NO_TOPIC_SELECTED'];
		}
		else if (!sizeof($forum_data))
		{
			$additional_msg = $user->lang['FORUM_NOT_EXIST'];
		}
		else
		{
			$forum_data = $forum_data[$to_forum_id];

			if ($forum_data['forum_type'] != FORUM_POST)
			{
				$additional_msg = $user->lang['FORUM_NOT_POSTABLE'];
			}
			else if (!$auth->acl_get('f_post', $to_forum_id))
			{
				$additional_msg = $user->lang['USER_CANNOT_POST'];
			}
		}
	}
	else if (isset($_POST['confirm']))
	{
		$additional_msg = $user->lang['FORUM_NOT_EXIST'];
	}

	if ($additional_msg)
	{
		unset($_POST['confirm']);
		unset($_REQUEST['confirm_key']);
	}

	if (confirm_box(true))
	{
		$topic_data = get_topic_data($topic_ids);

		$total_posts = 0;
		$new_topic_id_list = array();

		foreach ($topic_data as $topic_id => $topic_row)
		{
			$sql_ary = array(
				'forum_id'					=> (int) $to_forum_id,
				'icon_id'					=> (int) $topic_row['icon_id'],
				'topic_attachment'			=> (int) $topic_row['topic_attachment'],
				'topic_approved'			=> 1,
				'topic_reported'			=> 0,
				'topic_title'				=> (string) $topic_row['topic_title'],
				'topic_poster'				=> (int) $topic_row['topic_poster'],
				'topic_time'				=> (int) $topic_row['topic_time'],
				'topic_replies'				=> (int) $topic_row['topic_replies_real'],
				'topic_replies_real'		=> (int) $topic_row['topic_replies_real'],
				'topic_status'				=> (int) $topic_row['topic_status'],
				'topic_type'				=> (int) $topic_row['topic_type'],
				'topic_first_poster_name'	=> (string) $topic_row['topic_first_poster_name'],
				'topic_last_poster_id'		=> (int) $topic_row['topic_last_poster_id'],
				'topic_last_poster_name'	=> (string) $topic_row['topic_last_poster_name'],
				'topic_last_post_time'		=> (int) $topic_row['topic_last_post_time'],
				'topic_last_view_time'		=> (int) $topic_row['topic_last_view_time'],
				'topic_bumped'				=> (int) $topic_row['topic_bumped'],
				'topic_bumper'				=> (int) $topic_row['topic_bumper'],
				'poll_title'				=> (string) $topic_row['poll_title'],
				'poll_start'				=> (int) $topic_row['poll_start'],
				'poll_length'				=> (int) $topic_row['poll_length']
			);

			$db->sql_query('INSERT INTO ' . TOPICS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
			$new_topic_id = $db->sql_nextid();
			$new_topic_id_list[$topic_id] = $new_topic_id;

			if ($topic_row['poll_start'])
			{
				$poll_rows = array();

				$sql = 'SELECT *
					FROM ' . POLL_OPTIONS_TABLE . "
					WHERE topic_id = $topic_id";
				$result = $db->sql_query($sql);

				while ($row = $db->sql_fetchrow($result))
				{
					$sql_ary = array(
						'poll_option_id'	=> (int) $row['poll_option_id'],
						'topic_id'			=> (int) $new_topic_id,
						'poll_option_text'	=> (string) $row['poll_option_text'],
						'poll_option_total'	=> 0
					);

					$db->sql_query('INSERT INTO ' . POLL_OPTIONS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
				}
			}

			$sql = 'SELECT *
				FROM ' . POSTS_TABLE . "
				WHERE topic_id = $topic_id
				ORDER BY post_time ASC";
			$result = $db->sql_query($sql);

			$post_rows = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$post_rows[] = $row;
			}
			$db->sql_freeresult($result);

			if (!sizeof($post_rows))
			{
				continue;
			}

			$total_posts += sizeof($post_rows);
			foreach ($post_rows as $row)
			{
				$sql_ary = array(
					'topic_id'			=> (int) $new_topic_id,
					'forum_id'			=> (int) $to_forum_id,
					'poster_id'			=> (int) $row['poster_id'],
					'icon_id'			=> (int) $row['icon_id'],
					'poster_ip'			=> (string) $row['poster_ip'],
					'post_time'			=> (int) $row['post_time'],
					'post_approved'		=> 1,
					'post_reported'		=> 0,
					'enable_bbcode'		=> (int) $row['enable_bbcode'],
					'enable_smilies'	=> (int) $row['enable_smilies'],
					'enable_magic_url'	=> (int) $row['enable_magic_url'],
					'enable_sig'		=> (int) $row['enable_sig'],
					'post_username'		=> (string) $row['post_username'],
					'post_subject'		=> (string) $row['post_subject'],
					'post_text'			=> (string) $row['post_text'],
					'post_edit_reason'	=> (string) $row['post_edit_reason'],
					'post_edit_user'	=> (int) $row['post_edit_user'],
					'post_checksum'		=> (string) $row['post_checksum'],
					'post_attachment'	=> (int) $row['post_attachment'],
					'bbcode_bitfield'	=> $row['bbcode_bitfield'],
					'bbcode_uid'		=> (string) $row['bbcode_uid'],
					'post_edit_time'	=> (int) $row['post_edit_time'],
					'post_edit_count'	=> (int) $row['post_edit_count'],
					'post_edit_locked'	=> (int) $row['post_edit_locked'],
					'post_postcount'	=> 0,
				);

				$db->sql_query('INSERT INTO ' . POSTS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));
				$new_post_id = $db->sql_nextid();

				// Copy whether the topic is dotted
				markread('post', $to_forum_id, $new_topic_id, 0, $row['poster_id']);

				// Copy Attachments
				if ($row['post_attachment'])
				{
					$sql = 'SELECT * FROM ' . ATTACHMENTS_TABLE . "
						WHERE post_msg_id = {$row['post_id']}
							AND topic_id = $topic_id
							AND in_message = 0";
					$result = $db->sql_query($sql);

					$sql_ary = array();
					while ($attach_row = $db->sql_fetchrow($result))
					{
						$sql_ary[] = array(
							'post_msg_id'		=> (int) $new_post_id,
							'topic_id'			=> (int) $new_topic_id,
							'in_message'		=> 0,
							'is_orphan'			=> (int) $attach_row['is_orphan'],
							'poster_id'			=> (int) $attach_row['poster_id'],
							'physical_filename'	=> (string) basename($attach_row['physical_filename']),
							'real_filename'		=> (string) basename($attach_row['real_filename']),
							'download_count'	=> (int) $attach_row['download_count'],
							'attach_comment'	=> (string) $attach_row['attach_comment'],
							'extension'			=> (string) $attach_row['extension'],
							'mimetype'			=> (string) $attach_row['mimetype'],
							'filesize'			=> (int) $attach_row['filesize'],
							'filetime'			=> (int) $attach_row['filetime'],
							'thumbnail'			=> (int) $attach_row['thumbnail']
						);
					}
					$db->sql_freeresult($result);

					if (sizeof($sql_ary))
					{
						$db->sql_multi_insert(ATTACHMENTS_TABLE, $sql_ary);
					}
				}
			}

			$sql = 'SELECT user_id, notify_status
				FROM ' . TOPICS_WATCH_TABLE . '
				WHERE topic_id = ' . $topic_id;
			$result = $db->sql_query($sql);

			$sql_ary = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$sql_ary[] = array(
					'topic_id'		=> (int) $new_topic_id,
					'user_id'		=> (int) $row['user_id'],
					'notify_status'	=> (int) $row['notify_status'],
				);
			}
			$db->sql_freeresult($result);

			if (sizeof($sql_ary))
			{
				$db->sql_multi_insert(TOPICS_WATCH_TABLE, $sql_ary);
			}
		}

		// Sync new topics, parent forums and board stats
		sync('topic', 'topic_id', $new_topic_id_list);

		$sync_sql = array();

		$sync_sql[$to_forum_id][]	= 'forum_posts = forum_posts + ' . $total_posts;
		$sync_sql[$to_forum_id][]	= 'forum_topics = forum_topics + ' . sizeof($new_topic_id_list);
		$sync_sql[$to_forum_id][]	= 'forum_topics_real = forum_topics_real + ' . sizeof($new_topic_id_list);

		foreach ($sync_sql as $forum_id_key => $array)
		{
			$sql = 'UPDATE ' . FORUMS_TABLE . '
				SET ' . implode(', ', $array) . '
				WHERE forum_id = ' . $forum_id_key;
			$db->sql_query($sql);
		}

		sync('forum', 'forum_id', $to_forum_id);
		set_config('num_topics', $config['num_topics'] + sizeof($new_topic_id_list), true);
		set_config('num_posts', $config['num_posts'] + $total_posts, true);

		foreach ($new_topic_id_list as $topic_id => $new_topic_id)
		{
			add_log('mod', $to_forum_id, $new_topic_id, 'LOG_FORK', $topic_row['forum_name']);
		}

		$success_msg = (sizeof($topic_ids) == 1) ? 'TOPIC_FORKED_SUCCESS' : 'TOPICS_FORKED_SUCCESS';
	}
	else
	{
		$template->assign_vars(array(
			'S_FORUM_SELECT'		=> make_forum_select($to_forum_id, false, false, true, true, true),
			'S_CAN_LEAVE_SHADOW'	=> false,
			'ADDITIONAL_MSG'		=> $additional_msg)
		);

		confirm_box(false, 'FORK_TOPIC' . ((sizeof($topic_ids) == 1) ? '' : 'S'), $s_hidden_fields, 'mcp_move.html');
	}

	$redirect = request_var('redirect', "index.$phpEx");
	$redirect = reapply_sid($redirect);

	if (!$success_msg)
	{
		redirect($redirect);
	}
	else
	{
		$redirect_url = append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $forum_id);
		meta_refresh(3, $redirect_url);
		$return_link = sprintf($user->lang['RETURN_FORUM'], '<a href="' . $redirect_url . '">', '</a>');

		if ($forum_id != $to_forum_id)
		{
			$return_link .= '<br /><br />' . sprintf($user->lang['RETURN_NEW_FORUM'], '<a href="' . append_sid("{$phpbb_root_path}viewforum.$phpEx", 'f=' . $to_forum_id) . '">', '</a>');
		}

		trigger_error($user->lang[$success_msg] . '<br /><br />' . $return_link);
	}
}

?>