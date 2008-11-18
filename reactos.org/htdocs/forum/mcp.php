<?php
/**
*
* @package mcp
* @version $Id: mcp.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
define('IN_PHPBB', true);
$phpbb_root_path = (defined('PHPBB_ROOT_PATH')) ? PHPBB_ROOT_PATH : './';
$phpEx = substr(strrchr(__FILE__, '.'), 1);
include($phpbb_root_path . 'common.' . $phpEx);
include($phpbb_root_path . 'includes/functions_admin.' . $phpEx);
require($phpbb_root_path . 'includes/functions_module.' . $phpEx);

// Start session management
$user->session_begin();
$auth->acl($user->data);
$user->setup('mcp');

$module = new p_master();

// Setting a variable to let the style designer know where he is...
$template->assign_var('S_IN_MCP', true);

// Basic parameter data
$id = request_var('i', '');

if (isset($_REQUEST['mode']) && is_array($_REQUEST['mode']))
{
	$mode = request_var('mode', array(''));
	list($mode, ) = each($mode);
}
else
{
	$mode = request_var('mode', '');
}

// Only Moderators can go beyond this point
if (!$user->data['is_registered'])
{
	if ($user->data['is_bot'])
	{
		redirect(append_sid("{$phpbb_root_path}index.$phpEx"));
	}

	login_box('', $user->lang['LOGIN_EXPLAIN_MCP']);
}

$quickmod = (isset($_REQUEST['quickmod'])) ? true : false;
$action = request_var('action', '');
$action_ary = request_var('action', array('' => 0));

$forum_action = request_var('forum_action', '');
if ($forum_action !== '' && !empty($_POST['sort']))
{
	$action = $forum_action;
}

if (sizeof($action_ary))
{
	list($action, ) = each($action_ary);
}
unset($action_ary);

if ($mode == 'topic_logs')
{
	$id = 'logs';
	$quickmod = false;
}

$post_id = request_var('p', 0);
$topic_id = request_var('t', 0);
$forum_id = request_var('f', 0);
$user_id = request_var('u', 0);
$username = utf8_normalize_nfc(request_var('username', '', true));

if ($post_id)
{
	// We determine the topic and forum id here, to make sure the moderator really has moderative rights on this post
	$sql = 'SELECT topic_id, forum_id
		FROM ' . POSTS_TABLE . "
		WHERE post_id = $post_id";
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	$topic_id = (int) $row['topic_id'];
	$forum_id = (int) ($row['forum_id']) ? $row['forum_id'] : $forum_id;
}
else if ($topic_id)
{
	$sql = 'SELECT forum_id
		FROM ' . TOPICS_TABLE . "
		WHERE topic_id = $topic_id";
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	$forum_id = (int) $row['forum_id'];
}

// If the user doesn't have any moderator powers (globally or locally) he can't access the mcp
if (!$auth->acl_getf_global('m_'))
{
	// Except he is using one of the quickmod tools for users
	$user_quickmod_actions = array(
		'lock'			=> 'f_user_lock',
		'make_sticky'	=> 'f_sticky',
		'make_announce'	=> 'f_announce',
		'make_global'	=> 'f_announce',
		'make_normal'	=> array('f_announce', 'f_sticky')
	);

	$allow_user = false;
	if ($quickmod && isset($user_quickmod_actions[$action]) && $user->data['is_registered'] && $auth->acl_gets($user_quickmod_actions[$action], $forum_id))
	{
		$topic_info = get_topic_data(array($topic_id));
		if ($topic_info[$topic_id]['topic_poster'] == $user->data['user_id'])
		{
			$allow_user = true;
		}
	}

	if (!$allow_user)
	{
		trigger_error('NOT_AUTHORISED');
	}
}

// if the user cannot read the forum he tries to access then we won't allow mcp access either
if ($forum_id && !$auth->acl_get('f_read', $forum_id))
{
	trigger_error('NOT_AUTHORISED');
}

if ($forum_id)
{
	$module->acl_forum_id = $forum_id;
}

// Instantiate module system and generate list of available modules
$module->list_modules('mcp');

if ($quickmod)
{
	$mode = 'quickmod';

	switch ($action)
	{
		case 'lock':
		case 'unlock':
		case 'lock_post':
		case 'unlock_post':
		case 'make_sticky':
		case 'make_announce':
		case 'make_global':
		case 'make_normal':
		case 'fork':
		case 'move':
		case 'delete_post':
		case 'delete_topic':
			$module->load('mcp', 'main', 'quickmod');
			exit_handler();
		break;

		case 'topic_logs':
			$module->set_active('logs', 'topic_logs');
		break;

		case 'merge_topic':
			$module->set_active('main', 'forum_view');
		break;

		case 'split':
		case 'merge':
			$module->set_active('main', 'topic_view');
		break;

		default:
			trigger_error("$action not allowed as quickmod");
	}
}
else
{
	// Select the active module
	$module->set_active($id, $mode);
}

// Hide some of the options if we don't have the relevant information to use them
if (!$post_id)
{
	$module->set_display('main', 'post_details', false);
	$module->set_display('warn', 'warn_post', false);
}

if ($mode == '' || $mode == 'unapproved_topics' || $mode == 'unapproved_posts')
{
	$module->set_display('queue', 'approve_details', false);
}

if ($mode == '' || $mode == 'reports' || $mode == 'reports_closed')
{
	$module->set_display('reports', 'report_details', false);
}

if (!$topic_id)
{
	$module->set_display('main', 'topic_view', false);
	$module->set_display('logs', 'topic_logs', false);
}

if (!$forum_id)
{
	$module->set_display('main', 'forum_view', false);
	$module->set_display('logs', 'forum_logs', false);
}

if (!$user_id && $username == '')
{
	$module->set_display('notes', 'user_notes', false);
	$module->set_display('warn', 'warn_user', false);
}

// Load and execute the relevant module
$module->load_active();

// Assign data to the template engine for the list of modules
$module->assign_tpl_vars(append_sid("{$phpbb_root_path}mcp.$phpEx"));

// Generate urls for letting the moderation control panel being accessed in different modes
$template->assign_vars(array(
	'U_MCP'			=> append_sid("{$phpbb_root_path}mcp.$phpEx", 'i=main'),
	'U_MCP_FORUM'	=> ($forum_id) ? append_sid("{$phpbb_root_path}mcp.$phpEx", "i=main&amp;mode=forum_view&amp;f=$forum_id") : '',
	'U_MCP_TOPIC'	=> ($forum_id && $topic_id) ? append_sid("{$phpbb_root_path}mcp.$phpEx", "i=main&amp;mode=topic_view&amp;t=$topic_id") : '',
	'U_MCP_POST'	=> ($forum_id && $topic_id && $post_id) ? append_sid("{$phpbb_root_path}mcp.$phpEx", "i=main&amp;mode=post_details&amp;t=$topic_id&amp;p=$post_id") : '',
));

// Generate the page, do not display/query online list
$module->display($module->get_page_title(), false);

/**
* Functions used to generate additional URL paramters
*/
function _module__url($mode, &$module_row)
{
	return extra_url();
}

function _module_notes_url($mode, &$module_row)
{
	if ($mode == 'front')
	{
		return '';
	}

	global $user_id;
	return ($user_id) ? "&amp;u=$user_id" : '';
}

function _module_warn_url($mode, &$module_row)
{
	if ($mode == 'front' || $mode == 'list')
	{
		global $forum_id;

		return ($forum_id) ? "&amp;f=$forum_id" : '';
	}

	if ($mode == 'warn_post')
	{
		global $forum_id, $post_id;

		$url_extra = ($forum_id) ? "&amp;f=$forum_id" : '';
		$url_extra .= ($post_id) ? "&amp;p=$post_id" : '';

		return $url_extra;
	}
	else
	{
		global $user_id;

		return ($user_id) ? "&amp;u=$user_id" : '';
	}
}

function _module_main_url($mode, &$module_row)
{
	return extra_url();
}

function _module_logs_url($mode, &$module_row)
{
	return extra_url();
}

function _module_ban_url($mode, &$module_row)
{
	return extra_url();
}

function _module_queue_url($mode, &$module_row)
{
	return extra_url();
}

function _module_reports_url($mode, &$module_row)
{
	return extra_url();
}

function extra_url()
{
	global $forum_id, $topic_id, $post_id, $user_id;

	$url_extra = '';
	$url_extra .= ($forum_id) ? "&amp;f=$forum_id" : '';
	$url_extra .= ($topic_id) ? "&amp;t=$topic_id" : '';
	$url_extra .= ($post_id) ? "&amp;p=$post_id" : '';
	$url_extra .= ($user_id) ? "&amp;u=$user_id" : '';

	return $url_extra;
}

/**
* Get simple topic data
*/
function get_topic_data($topic_ids, $acl_list = false, $read_tracking = false)
{
	global $auth, $db, $config, $user;
	static $rowset = array();

	$topics = array();

	if (!sizeof($topic_ids))
	{
		return array();
	}

	// cache might not contain read tracking info, so we can't use it if read
	// tracking information is requested
	if (!$read_tracking)
	{
		$cache_topic_ids = array_intersect($topic_ids, array_keys($rowset));
		$topic_ids = array_diff($topic_ids, array_keys($rowset));
	}
	else
	{
		$cache_topic_ids = array();
	}

	if (sizeof($topic_ids))
	{
		$sql_array = array(
			'SELECT'	=> 't.*, f.*',

			'FROM'		=> array(
				TOPICS_TABLE	=> 't',
			),

			'LEFT_JOIN'	=> array(
				array(
					'FROM'	=> array(FORUMS_TABLE => 'f'),
					'ON'	=> 'f.forum_id = t.forum_id'
				)
			),

			'WHERE'		=> $db->sql_in_set('t.topic_id', $topic_ids)
		);

		if ($read_tracking && $config['load_db_lastread'])
		{
			$sql_array['SELECT'] .= ', tt.mark_time, ft.mark_time as forum_mark_time';

			$sql_array['LEFT_JOIN'][] = array(
				'FROM'	=> array(TOPICS_TRACK_TABLE => 'tt'),
				'ON'	=> 'tt.user_id = ' . $user->data['user_id'] . ' AND t.topic_id = tt.topic_id'
			);

			$sql_array['LEFT_JOIN'][] = array(
				'FROM'	=> array(FORUMS_TRACK_TABLE => 'ft'),
				'ON'	=> 'ft.user_id = ' . $user->data['user_id'] . ' AND t.forum_id = ft.forum_id'
			);
		}

		$sql = $db->sql_build_query('SELECT', $sql_array);
		$result = $db->sql_query($sql);
	
		while ($row = $db->sql_fetchrow($result))
		{
			if (!$row['forum_id'])
			{
				// Global Announcement?
				$row['forum_id'] = request_var('f', 0);
			}

			$rowset[$row['topic_id']] = $row;

			if ($acl_list && !$auth->acl_gets($acl_list, $row['forum_id']))
			{
				continue;
			}

			$topics[$row['topic_id']] = $row;
		}
		$db->sql_freeresult($result);
	}

	foreach ($cache_topic_ids as $id)
	{
		if (!$acl_list || $auth->acl_gets($acl_list, $rowset[$id]['forum_id']))
		{
			$topics[$id] = $rowset[$id];
		}
	}

	return $topics;
}

/**
* Get simple post data
*/
function get_post_data($post_ids, $acl_list = false, $read_tracking = false)
{
	global $db, $auth, $config, $user;

	$rowset = array();

	if (!sizeof($post_ids))
	{
		return array();
	}

	$sql_array = array(
		'SELECT'	=> 'p.*, u.*, t.*, f.*',

		'FROM'		=> array(
			USERS_TABLE		=> 'u',
			POSTS_TABLE		=> 'p',
			TOPICS_TABLE	=> 't',
		),

		'LEFT_JOIN'	=> array(
			array(
				'FROM'	=> array(FORUMS_TABLE => 'f'),
				'ON'	=> 'f.forum_id = t.forum_id'
			)
		),

		'WHERE'		=> $db->sql_in_set('p.post_id', $post_ids) . '
			AND u.user_id = p.poster_id
			AND t.topic_id = p.topic_id',
	);

	if ($read_tracking && $config['load_db_lastread'])
	{
		$sql_array['SELECT'] .= ', tt.mark_time, ft.mark_time as forum_mark_time';

		$sql_array['LEFT_JOIN'][] = array(
			'FROM'	=> array(TOPICS_TRACK_TABLE => 'tt'),
			'ON'	=> 'tt.user_id = ' . $user->data['user_id'] . ' AND t.topic_id = tt.topic_id'
		);

		$sql_array['LEFT_JOIN'][] = array(
			'FROM'	=> array(FORUMS_TRACK_TABLE => 'ft'),
			'ON'	=> 'ft.user_id = ' . $user->data['user_id'] . ' AND t.forum_id = ft.forum_id'
		);
	}

	$sql = $db->sql_build_query('SELECT', $sql_array);
	$result = $db->sql_query($sql);
	unset($sql_array);

	while ($row = $db->sql_fetchrow($result))
	{
		if (!$row['forum_id'])
		{
			// Global Announcement?
			$row['forum_id'] = request_var('f', 0);
		}

		if ($acl_list && !$auth->acl_gets($acl_list, $row['forum_id']))
		{
			continue;
		}

		if (!$row['post_approved'] && !$auth->acl_get('m_approve', $row['forum_id']))
		{
			// Moderators without the permission to approve post should at least not see them. ;)
			continue;
		}

		$rowset[$row['post_id']] = $row;
	}
	$db->sql_freeresult($result);

	return $rowset;
}

/**
* Get simple forum data
*/
function get_forum_data($forum_id, $acl_list = 'f_list', $read_tracking = false)
{
	global $auth, $db, $user, $config;

	$rowset = array();

	if (!is_array($forum_id))
	{
		$forum_id = array($forum_id);
	}

	if (!sizeof($forum_id))
	{
		return array();
	}

	if ($read_tracking && $config['load_db_lastread'])
	{
		$read_tracking_join = ' LEFT JOIN ' . FORUMS_TRACK_TABLE . ' ft ON (ft.user_id = ' . $user->data['user_id'] . '
			AND ft.forum_id = f.forum_id)';
		$read_tracking_select = ', ft.mark_time';
	}
	else
	{
		$read_tracking_join = $read_tracking_select = '';
	}

	$sql = "SELECT f.* $read_tracking_select
		FROM " . FORUMS_TABLE . " f$read_tracking_join
		WHERE " . $db->sql_in_set('f.forum_id', $forum_id);
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		if ($acl_list && !$auth->acl_gets($acl_list, $row['forum_id']))
		{
			continue;
		}

		if ($auth->acl_get('m_approve', $row['forum_id']))
		{
			$row['forum_topics'] = $row['forum_topics_real'];
		}

		$rowset[$row['forum_id']] = $row;
	}
	$db->sql_freeresult($result);

	return $rowset;
}

/**
* sorting in mcp
*
* @param string $where_sql should either be WHERE (default if ommited) or end with AND or OR
*/
function mcp_sorting($mode, &$sort_days, &$sort_key, &$sort_dir, &$sort_by_sql, &$sort_order_sql, &$total, $forum_id = 0, $topic_id = 0, $where_sql = 'WHERE')
{
	global $db, $user, $auth, $template;

	$sort_days = request_var('st', 0);
	$min_time = ($sort_days) ? time() - ($sort_days * 86400) : 0;

	switch ($mode)
	{
		case 'viewforum':
			$type = 'topics';
			$default_key = 't';
			$default_dir = 'd';

			$sql = 'SELECT COUNT(topic_id) AS total
				FROM ' . TOPICS_TABLE . "
				$where_sql forum_id = $forum_id
					AND topic_type NOT IN (" . POST_ANNOUNCE . ', ' . POST_GLOBAL . ")
					AND topic_last_post_time >= $min_time";

			if (!$auth->acl_get('m_approve', $forum_id))
			{
				$sql .= 'AND topic_approved = 1';
			}
		break;

		case 'viewtopic':
			$type = 'posts';
			$default_key = 't';
			$default_dir = 'a';

			$sql = 'SELECT COUNT(post_id) AS total
				FROM ' . POSTS_TABLE . "
				$where_sql topic_id = $topic_id
					AND post_time >= $min_time";

			if (!$auth->acl_get('m_approve', $forum_id))
			{
				$sql .= 'AND post_approved = 1';
			}
		break;

		case 'unapproved_posts':
			$type = 'posts';
			$default_key = 't';
			$default_dir = 'd';
			$where_sql .= ($topic_id) ? ' topic_id = ' . $topic_id . ' AND' : '';

			$sql = 'SELECT COUNT(post_id) AS total
				FROM ' . POSTS_TABLE . "
				$where_sql " . $db->sql_in_set('forum_id', ($forum_id) ? array($forum_id) : get_forum_list('m_approve')) . '
					AND post_approved = 0';

			if ($min_time)
			{
				$sql .= ' AND post_time >= ' . $min_time;
			}
		break;

		case 'unapproved_topics':
			$type = 'topics';
			$default_key = 't';
			$default_dir = 'd';

			$sql = 'SELECT COUNT(topic_id) AS total
				FROM ' . TOPICS_TABLE . "
				$where_sql " . $db->sql_in_set('forum_id', ($forum_id) ? array($forum_id) : get_forum_list('m_approve')) . '
					AND topic_approved = 0';

			if ($min_time)
			{
				$sql .= ' AND topic_time >= ' . $min_time;
			}
		break;

		case 'reports':
		case 'reports_closed':
			$type = 'reports';
			$default_key = 't';
			$default_dir = 'd';
			$limit_time_sql = ($min_time) ? "AND r.report_time >= $min_time" : '';

			if ($topic_id)
			{
				$where_sql .= ' p.topic_id = ' . $topic_id;
			}
			else if ($forum_id)
			{
				$where_sql .= ' p.forum_id = ' . $forum_id;
			}
			else
			{
				$where_sql .= ' ' . $db->sql_in_set('p.forum_id', get_forum_list('!m_report'), true, true);
			}

			if ($mode == 'reports')
			{
				$where_sql .= ' AND r.report_closed = 0';
			}
			else
			{
				$where_sql .= ' AND r.report_closed = 1';
			}

			$sql = 'SELECT COUNT(r.report_id) AS total
				FROM ' . REPORTS_TABLE . ' r, ' . POSTS_TABLE . " p
				$where_sql
					AND p.post_id = r.post_id
					$limit_time_sql";
		break;

		case 'viewlogs':
			$type = 'logs';
			$default_key = 't';
			$default_dir = 'd';

			$sql = 'SELECT COUNT(log_id) AS total
				FROM ' . LOG_TABLE . "
				$where_sql " . $db->sql_in_set('forum_id', ($forum_id) ? array($forum_id) : get_forum_list('m_')) . '
					AND log_time >= ' . $min_time . '
					AND log_type = ' . LOG_MOD;
		break;
	}

	$sort_key = request_var('sk', $default_key);
	$sort_dir = request_var('sd', $default_dir);
	$sort_dir_text = array('a' => $user->lang['ASCENDING'], 'd' => $user->lang['DESCENDING']);

	switch ($type)
	{
		case 'topics':
			$limit_days = array(0 => $user->lang['ALL_TOPICS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
			$sort_by_text = array('a' => $user->lang['AUTHOR'], 't' => $user->lang['POST_TIME'], 'tt' => $user->lang['TOPIC_TIME'], 'r' => $user->lang['REPLIES'], 's' => $user->lang['SUBJECT'], 'v' => $user->lang['VIEWS']);

			$sort_by_sql = array('a' => 't.topic_first_poster_name', 't' => 't.topic_last_post_time', 'tt' => 't.topic_time', 'r' => (($auth->acl_get('m_approve', $forum_id)) ? 't.topic_replies_real' : 't.topic_replies'), 's' => 't.topic_title', 'v' => 't.topic_views');
			$limit_time_sql = ($min_time) ? "AND t.topic_last_post_time >= $min_time" : '';
		break;

		case 'posts':
			$limit_days = array(0 => $user->lang['ALL_POSTS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
			$sort_by_text = array('a' => $user->lang['AUTHOR'], 't' => $user->lang['POST_TIME'], 's' => $user->lang['SUBJECT']);
			$sort_by_sql = array('a' => 'u.username_clean', 't' => 'p.post_time', 's' => 'p.post_subject');
			$limit_time_sql = ($min_time) ? "AND p.post_time >= $min_time" : '';
		break;

		case 'reports':
			$limit_days = array(0 => $user->lang['ALL_REPORTS'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
			$sort_by_text = array('a' => $user->lang['AUTHOR'], 'r' => $user->lang['REPORTER'], 'p' => $user->lang['POST_TIME'], 't' => $user->lang['REPORT_TIME'], 's' => $user->lang['SUBJECT']);
			$sort_by_sql = array('a' => 'u.username_clean', 'r' => 'ru.username', 'p' => 'p.post_time', 't' => 'r.report_time', 's' => 'p.post_subject');
		break;

		case 'logs':
			$limit_days = array(0 => $user->lang['ALL_ENTRIES'], 1 => $user->lang['1_DAY'], 7 => $user->lang['7_DAYS'], 14 => $user->lang['2_WEEKS'], 30 => $user->lang['1_MONTH'], 90 => $user->lang['3_MONTHS'], 180 => $user->lang['6_MONTHS'], 365 => $user->lang['1_YEAR']);
			$sort_by_text = array('u' => $user->lang['SORT_USERNAME'], 't' => $user->lang['SORT_DATE'], 'i' => $user->lang['SORT_IP'], 'o' => $user->lang['SORT_ACTION']);

			$sort_by_sql = array('u' => 'u.username_clean', 't' => 'l.log_time', 'i' => 'l.log_ip', 'o' => 'l.log_operation');
			$limit_time_sql = ($min_time) ? "AND l.log_time >= $min_time" : '';
		break;
	}

	if (!isset($sort_by_sql[$sort_key]))
	{
		$sort_key = $default_key;
	}

	$sort_order_sql = $sort_by_sql[$sort_key] . ' ' . (($sort_dir == 'd') ? 'DESC' : 'ASC');

	$s_limit_days = $s_sort_key = $s_sort_dir = $sort_url = '';
	gen_sort_selects($limit_days, $sort_by_text, $sort_days, $sort_key, $sort_dir, $s_limit_days, $s_sort_key, $s_sort_dir, $sort_url);

	$template->assign_vars(array(
		'S_SELECT_SORT_DIR'		=> $s_sort_dir,
		'S_SELECT_SORT_KEY'		=> $s_sort_key,
		'S_SELECT_SORT_DAYS'	=> $s_limit_days)
	);

	if (($sort_days && $mode != 'viewlogs') || in_array($mode, array('reports', 'unapproved_topics', 'unapproved_posts')) || $where_sql != 'WHERE')
	{
		$result = $db->sql_query($sql);
		$total = (int) $db->sql_fetchfield('total');
		$db->sql_freeresult($result);
	}
	else
	{
		$total = -1;
	}
}

/**
* Validate ids
*
* @param	array	&$ids			The relevant ids to check
* @param	string	$table			The table to find the ids in
* @param	string	$sql_id			The ids relevant column name
* @param	array	$acl_list		A list of permissions the user need to have
* @param	mixed	$singe_forum	Limit to one forum id (int) or the first forum found (true)
*
* @return	mixed	False if no ids were able to be retrieved, true if at least one id left.
*					Additionally, this value can be the forum_id assigned if $single_forum was set.
*					Therefore checking the result for with !== false is the best method.
*/
function check_ids(&$ids, $table, $sql_id, $acl_list = false, $single_forum = false)
{
	global $db, $auth;

	if (!is_array($ids) || empty($ids))
	{
		return false;
	}

	$sql = "SELECT $sql_id, forum_id FROM $table
		WHERE " . $db->sql_in_set($sql_id, $ids);
	$result = $db->sql_query($sql);

	$ids = array();
	$forum_id = false;

	while ($row = $db->sql_fetchrow($result))
	{
		if ($acl_list && $row['forum_id'] && !$auth->acl_gets($acl_list, $row['forum_id']))
		{
			continue;
		}

		if ($acl_list && !$row['forum_id'] && !$auth->acl_getf_global($acl_list))
		{
			continue;
		}

		// Limit forum? If not, just assign the id.
		if ($single_forum === false)
		{
			$ids[] = $row[$sql_id];
			continue;
		}

		// Limit forum to a specific forum id?
		// This can get really tricky, because we do not want to create a failure on global topics. :)
		if ($row['forum_id'])
		{
			if ($single_forum !== true && $row['forum_id'] == (int) $single_forum)
			{
				$forum_id = (int) $single_forum;
			}
			else if ($forum_id === false)
			{
				$forum_id = $row['forum_id'];
			}

			if ($row['forum_id'] == $forum_id)
			{
				$ids[] = $row[$sql_id];
			}
		}
		else
		{
			// Always add a global topic
			$ids[] = $row[$sql_id];
		}
	}
	$db->sql_freeresult($result);

	if (!sizeof($ids))
	{
		return false;
	}

	// If forum id is false and ids populated we may have only global announcements selected (returning 0 because of (int) $forum_id)

	return ($single_forum === false) ? true : (int) $forum_id;
}

?>