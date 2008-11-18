<?php
/**
*
* @package install
* @version $Id: functions_phpbb20.php 8489 2008-04-03 14:04:10Z naderman $
* @copyright (c) 2006 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* Helper functions for phpBB 2.0.x to phpBB 3.0.x conversion
*/

/**
* Set forum flags - only prune old polls by default
*/
function phpbb_forum_flags()
{
	// Set forum flags
	$forum_flags = 0;

	// FORUM_FLAG_LINK_TRACK
	$forum_flags += 0;

	// FORUM_FLAG_PRUNE_POLL
	$forum_flags += FORUM_FLAG_PRUNE_POLL;

	// FORUM_FLAG_PRUNE_ANNOUNCE
	$forum_flags += 0;

	// FORUM_FLAG_PRUNE_STICKY
	$forum_flags += 0;

	// FORUM_FLAG_ACTIVE_TOPICS
	$forum_flags += 0;

	// FORUM_FLAG_POST_REVIEW
	$forum_flags += FORUM_FLAG_POST_REVIEW;

	return $forum_flags;
}

/**
* Insert/Convert forums
*/
function phpbb_insert_forums()
{
	global $db, $src_db, $same_db, $convert, $user, $config;

	$db->sql_query($convert->truncate_statement . FORUMS_TABLE);

	// Determine the highest id used within the old forums table (we add the categories after the forum ids)
	$sql = 'SELECT MAX(forum_id) AS max_forum_id
		FROM ' . $convert->src_table_prefix . 'forums';
	$result = $src_db->sql_query($sql);
	$max_forum_id = (int) $src_db->sql_fetchfield('max_forum_id');
	$src_db->sql_freeresult($result);

	$max_forum_id++;

	// pruning disabled globally?
	$sql = "SELECT config_value
		FROM {$convert->src_table_prefix}config
		WHERE config_name = 'prune_enable'";
	$result = $src_db->sql_query($sql);
	$prune_enabled = (int) $src_db->sql_fetchfield('config_value');
	$src_db->sql_freeresult($result);
	
	
	// Insert categories
	$sql = 'SELECT cat_id, cat_title
		FROM ' . $convert->src_table_prefix . 'categories
		ORDER BY cat_order';

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'binary'");
	}

	$result = $src_db->sql_query($sql);

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'utf8'");
	}

	switch ($db->sql_layer)
	{
		case 'mssql':
		case 'mssql_odbc':
			$db->sql_query('SET IDENTITY_INSERT ' . FORUMS_TABLE . ' ON');
		break;
	}

	$cats_added = array();
	while ($row = $src_db->sql_fetchrow($result))
	{
		$sql_ary = array(
			'forum_id'		=> (int) $max_forum_id,
			'forum_name'	=> ($row['cat_title']) ? htmlspecialchars(phpbb_set_default_encoding($row['cat_title']), ENT_COMPAT, 'UTF-8') : $user->lang['CATEGORY'],
			'parent_id'		=> 0,
			'forum_parents'	=> '',
			'forum_desc'	=> '',
			'forum_type'	=> FORUM_CAT,
			'forum_status'	=> ITEM_UNLOCKED,
			'forum_rules'	=> '',
		);

		$sql = 'SELECT MAX(right_id) AS right_id
			FROM ' . FORUMS_TABLE;
		$_result = $db->sql_query($sql);
		$cat_row = $db->sql_fetchrow($_result);
		$db->sql_freeresult($_result);

		$sql_ary['left_id'] = (int) ($cat_row['right_id'] + 1);
		$sql_ary['right_id'] = (int) ($cat_row['right_id'] + 2);

		$sql = 'INSERT INTO ' . FORUMS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
		$db->sql_query($sql);

		$cats_added[$row['cat_id']] = $max_forum_id;
		$max_forum_id++;
	}
	$src_db->sql_freeresult($result);

	// There may be installations having forums with non-existant category ids.
	// We try to catch them and add them to an "unknown" category instead of leaving them out.
	$sql = 'SELECT cat_id
		FROM ' . $convert->src_table_prefix . 'forums
		GROUP BY cat_id';
	$result = $src_db->sql_query($sql);

	$unknown_cat_id = false;
	while ($row = $src_db->sql_fetchrow($result))
	{
		// Catch those categories not been added before
		if (!isset($cats_added[$row['cat_id']]))
		{
			$unknown_cat_id = true;
		}
	}
	$src_db->sql_freeresult($result);

	// Is there at least one category not known?
	if ($unknown_cat_id === true)
	{
		$unknown_cat_id = 'ghost';

		$sql_ary = array(
			'forum_id'		=> (int) $max_forum_id,
			'forum_name'	=> (string) $user->lang['CATEGORY'],
			'parent_id'		=> 0,
			'forum_parents'	=> '',
			'forum_desc'	=> '',
			'forum_type'	=> FORUM_CAT,
			'forum_status'	=> ITEM_UNLOCKED,
			'forum_rules'	=> '',
		);

		$sql = 'SELECT MAX(right_id) AS right_id
			FROM ' . FORUMS_TABLE;
		$_result = $db->sql_query($sql);
		$cat_row = $db->sql_fetchrow($_result);
		$db->sql_freeresult($_result);

		$sql_ary['left_id'] = (int) ($cat_row['right_id'] + 1);
		$sql_ary['right_id'] = (int) ($cat_row['right_id'] + 2);

		$sql = 'INSERT INTO ' . FORUMS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
		$db->sql_query($sql);

		$cats_added[$unknown_cat_id] = $max_forum_id;
		$max_forum_id++;
	}

	// Now insert the forums
	$sql = 'SELECT f.forum_id, f.forum_name, f.cat_id, f.forum_desc, f.forum_status, f.prune_enable, f.prune_next, fp.prune_days, fp.prune_freq FROM ' . $convert->src_table_prefix . 'forums f
		LEFT JOIN ' . $convert->src_table_prefix . 'forum_prune fp ON f.forum_id = fp.forum_id
		GROUP BY f.forum_id, f.forum_name, f.cat_id, f.forum_desc, f.forum_status, f.prune_enable, f.prune_next, f.forum_order, fp.prune_days, fp.prune_freq
		ORDER BY f.cat_id, f.forum_order';

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'binary'");
	}

	$result = $src_db->sql_query($sql);

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'utf8'");
	}

	while ($row = $src_db->sql_fetchrow($result))
	{
		// Some might have forums here with an id not being "possible"...
		// To be somewhat friendly we "change" the category id for those to a previously created ghost category
		if (!isset($cats_added[$row['cat_id']]) && $unknown_cat_id !== false)
		{
			$row['cat_id'] = $unknown_cat_id;
		}

		if (!isset($cats_added[$row['cat_id']]))
		{
			continue;
		}

		// Define the new forums sql ary
		$sql_ary = array(
			'forum_id'			=> (int) $row['forum_id'],
			'forum_name'		=> htmlspecialchars(phpbb_set_default_encoding($row['forum_name']), ENT_COMPAT, 'UTF-8'),
			'parent_id'			=> (int) $cats_added[$row['cat_id']],
			'forum_parents'		=> '',
			'forum_desc'		=> htmlspecialchars(phpbb_set_default_encoding($row['forum_desc']), ENT_COMPAT, 'UTF-8'),
			'forum_type'		=> FORUM_POST,
			'forum_status'		=> is_item_locked($row['forum_status']),
			'enable_prune'		=> ($prune_enabled) ? (int)$row['prune_enable'] : 0,
			'prune_next'		=> (int) null_to_zero($row['prune_next']),
			'prune_days'		=> (int) null_to_zero($row['prune_days']),
			'prune_viewed'		=> 0,
			'prune_freq'		=> (int) null_to_zero($row['prune_freq']),

			'forum_flags'		=> phpbb_forum_flags(),

			// Default values
			'forum_desc_bitfield'		=> '',
			'forum_desc_options'		=> 7,
			'forum_desc_uid'			=> '',
			'forum_link'				=> '',
			'forum_password'			=> '',
			'forum_style'				=> 0,
			'forum_image'				=> '',
			'forum_rules'				=> '',
			'forum_rules_link'			=> '',
			'forum_rules_bitfield'		=> '',
			'forum_rules_options'		=> 7,
			'forum_rules_uid'			=> '',
			'forum_topics_per_page'		=> 0,
			'forum_posts'				=> 0,
			'forum_topics'				=> 0,
			'forum_topics_real'			=> 0,
			'forum_last_post_id'		=> 0,
			'forum_last_poster_id'		=> 0,
			'forum_last_post_subject'	=> '',
			'forum_last_post_time'		=> 0,
			'forum_last_poster_name'	=> '',
			'forum_last_poster_colour'	=> '',
			'display_on_index'			=> 1,
			'enable_indexing'			=> 1,
			'enable_icons'				=> 0,
		);

		// Now add the forums with proper left/right ids
		$sql = 'SELECT left_id, right_id
			FROM ' . FORUMS_TABLE . '
			WHERE forum_id = ' . $cats_added[$row['cat_id']];
		$_result = $db->sql_query($sql);
		$cat_row = $db->sql_fetchrow($_result);
		$db->sql_freeresult($_result);

		$sql = 'UPDATE ' . FORUMS_TABLE . '
			SET left_id = left_id + 2, right_id = right_id + 2
			WHERE left_id > ' . $cat_row['right_id'];
		$db->sql_query($sql);

		$sql = 'UPDATE ' . FORUMS_TABLE . '
			SET right_id = right_id + 2
			WHERE ' . $cat_row['left_id'] . ' BETWEEN left_id AND right_id';
		$db->sql_query($sql);

		$sql_ary['left_id'] = (int) $cat_row['right_id'];
		$sql_ary['right_id'] = (int) ($cat_row['right_id'] + 1);

		$sql = 'INSERT INTO ' . FORUMS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
		$db->sql_query($sql);
	}
	$src_db->sql_freeresult($result);

	switch ($db->sql_layer)
	{
		case 'postgres':
			$db->sql_query("SELECT SETVAL('" . FORUMS_TABLE . "_seq',(select case when max(forum_id)>0 then max(forum_id)+1 else 1 end from " . FORUMS_TABLE . '));');
		break;

		case 'mssql':
		case 'mssql_odbc':
			$db->sql_query('SET IDENTITY_INSERT ' . FORUMS_TABLE . ' OFF');
		break;

		case 'oracle':
			$result = $db->sql_query('SELECT MAX(forum_id) as max_id FROM ' . FORUMS_TABLE);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			$largest_id = (int) $row['max_id'];

			if ($largest_id)
			{
				$db->sql_query('DROP SEQUENCE ' . FORUMS_TABLE . '_seq');
				$db->sql_query('CREATE SEQUENCE ' . FORUMS_TABLE . '_seq START WITH ' . ($largest_id + 1));
			}
		break;
	}
}

/**
* Function for recoding text with the default language
*
* @param string $text text to recode to utf8
* @param bool $grab_user_lang if set to true the function tries to use $convert_row['user_lang'] (and falls back to $convert_row['poster_id']) instead of the boards default language
*/
function phpbb_set_encoding($text, $grab_user_lang = true)
{
	global $lang_enc_array, $convert_row;
	global $convert, $phpEx;

	/*static $lang_enc_array = array(
		'korean'						=> 'euc-kr',
		'serbian'						=> 'windows-1250',
		'polish'						=> 'iso-8859-2',
		'kurdish'						=> 'windows-1254',
		'slovak'						=> 'Windows-1250',
		'russian'						=> 'windows-1251',
		'estonian'						=> 'iso-8859-4',
		'chinese_simplified'			=> 'gb2312',
		'macedonian'					=> 'windows-1251',
		'azerbaijani'					=> 'UTF-8',
		'romanian'						=> 'iso-8859-2',
		'romanian_diacritice'			=> 'iso-8859-2',
		'lithuanian'					=> 'windows-1257',
		'turkish'						=> 'iso-8859-9',
		'ukrainian'						=> 'windows-1251',
		'japanese'						=> 'shift_jis',
		'hungarian'						=> 'ISO-8859-2',
		'romanian_no_diacritics'		=> 'iso-8859-2',
		'mongolian'						=> 'UTF-8',
		'slovenian'						=> 'windows-1250',
		'bosnian'						=> 'windows-1250',
		'czech'							=> 'Windows-1250',
		'farsi'							=> 'Windows-1256',
		'croatian'						=> 'windows-1250',
		'greek'							=> 'iso-8859-7',
		'russian_tu'					=> 'windows-1251',
		'sakha'							=> 'UTF-8',
		'serbian_cyrillic'				=> 'windows-1251',
		'bulgarian'						=> 'windows-1251',
		'chinese_traditional_taiwan'	=> 'big5',
		'chinese_traditional'			=> 'big5',
		'arabic'						=> 'windows-1256',
		'hebrew'						=> 'WINDOWS-1255',
		'thai'							=> 'windows-874',
		//'chinese_traditional_taiwan'	=> 'utf-8' // custom modified, we may have to do an include :-(
	);*/

	if (empty($lang_enc_array))
	{
		$lang_enc_array = array();
	}

	$get_lang = trim(get_config_value('default_lang'));

	// Do we need the users language encoding?
	if ($grab_user_lang && !empty($convert_row))
	{
		if (!empty($convert_row['user_lang']))
		{
			$get_lang = trim($convert_row['user_lang']);
		}
		else if (!empty($convert_row['poster_id']))
		{
			global $src_db, $same_db;

			if ($convert->mysql_convert && $same_db)
			{
				$src_db->sql_query("SET NAMES 'binary'");
			}

			$sql = 'SELECT user_lang
				FROM ' . $convert->src_table_prefix . 'users
				WHERE user_id = ' . (int) $convert_row['poster_id'];
			$result = $src_db->sql_query($sql);
			$get_lang = (string) $src_db->sql_fetchfield('user_lang');
			$src_db->sql_freeresult($result);

			if ($convert->mysql_convert && $same_db)
			{
				$src_db->sql_query("SET NAMES 'utf8'");
			}

			$get_lang = (!trim($get_lang)) ? trim(get_config_value('default_lang')) : trim($get_lang);
		}
	}

	if (!isset($lang_enc_array[$get_lang]))
	{
		$filename = $convert->options['forum_path'] . '/language/lang_' . $get_lang . '/lang_main.' . $phpEx;

		if (!file_exists($filename))
		{
			$get_lang = trim(get_config_value('default_lang'));
		}

		if (!isset($lang_enc_array[$get_lang]))
		{
			include($convert->options['forum_path'] . '/language/lang_' . $get_lang . '/lang_main.' . $phpEx);
			$lang_enc_array[$get_lang] = $lang['ENCODING'];
			unset($lang);
		}
	}

	$encoding = $lang_enc_array[$get_lang];

	return utf8_recode($text, $lang_enc_array[$get_lang]);
}

/**
* Same as phpbb_set_encoding, but forcing boards default language
*/
function phpbb_set_default_encoding($text)
{
	return phpbb_set_encoding($text, false);
}

/**
* Convert Birthday from Birthday MOD to phpBB Format
*/
function phpbb_get_birthday($birthday = '')
{
	if (defined('MOD_BIRTHDAY_TERRA'))
	{
		$birthday = (string) $birthday;

		// stored as month, day, year
		if (!$birthday)
		{
			return ' 0- 0-   0';
		}

		// We use the original mod code to retrieve the birthday (not ideal)
		preg_match('/(..)(..)(....)/', sprintf('%08d', $birthday), $birthday_parts);

		$month = $birthday_parts[1];
		$day = $birthday_parts[2];
		$year =  $birthday_parts[3];

		return sprintf('%2d-%2d-%4d', $day, $month, $year);
	}
	else
	{
		$birthday = (int) $birthday;

		if (!$birthday || $birthday == 999999 || ((version_compare(PHP_VERSION, '5.1.0') < 0) && $birthday < 0))
		{
			return ' 0- 0-   0';
		}

		// The birthday mod from niels is using this code to transform to day/month/year
		return gmdate('d-m-Y', $birthday * 86400 + 1);
	}
}

/**
* Return correct user id value
* Everyone's id will be one higher to allow the guest/anonymous user to have a positive id as well
*/
function phpbb_user_id($user_id)
{
	global $config;

	// Increment user id if the old forum is having a user with the id 1
	if (!isset($config['increment_user_id']))
	{
		global $src_db, $same_db, $convert;

		if ($convert->mysql_convert && $same_db)
		{
			$src_db->sql_query("SET NAMES 'binary'");
		}

		// Now let us set a temporary config variable for user id incrementing
		$sql = "SELECT user_id
			FROM {$convert->src_table_prefix}users
			WHERE user_id = 1";
		$result = $src_db->sql_query($sql);
		$id = (int) $src_db->sql_fetchfield('user_id');
		$src_db->sql_freeresult($result);

		// Try to get the maximum user id possible...
		$sql = "SELECT MAX(user_id) AS max_user_id
			FROM {$convert->src_table_prefix}users";
		$result = $src_db->sql_query($sql);
		$max_id = (int) $src_db->sql_fetchfield('max_user_id');
		$src_db->sql_freeresult($result);

		if ($convert->mysql_convert && $same_db)
		{
			$src_db->sql_query("SET NAMES 'utf8'");
		}

		// If there is a user id 1, we need to increment user ids. :/
		if ($id === 1)
		{
			set_config('increment_user_id', ($max_id + 1), true);
			$config['increment_user_id'] = $max_id + 1;
		}
		else
		{
			set_config('increment_user_id', 0, true);
			$config['increment_user_id'] = 0;
		}
	}

	// If the old user id is -1 in 2.0.x it is the anonymous user...
	if ($user_id == -1)
	{
		return ANONYMOUS;
	}

	if (!empty($config['increment_user_id']) && $user_id == 1)
	{
		return $config['increment_user_id'];
	}

	// A user id of 0 can happen, for example within the ban table if no user is banned...
	// Within the posts and topics table this can be "dangerous" but is the fault of the user
	// having mods installed (a poster id of 0 is not possible in 2.0.x).
	// Therefore, we return the user id "as is".

	return (int) $user_id;
}

/* Copy additional table fields from old forum to new forum if user wants this (for Mod compatibility for example)
function phpbb_copy_table_fields()
{
}
*/

/**
* Convert authentication
* user, group and forum table has to be filled in order to work
*/
function phpbb_convert_authentication($mode)
{
	global $db, $src_db, $same_db, $convert, $user, $config, $cache;

	if ($mode == 'start')
	{
		$db->sql_query($convert->truncate_statement . ACL_USERS_TABLE);
		$db->sql_query($convert->truncate_statement . ACL_GROUPS_TABLE);

		// What we will do is handling all 2.0.x admins as founder to replicate what is common in 2.0.x.
		// After conversion the main admin need to make sure he is removing permissions and the founder status if wanted.


		// Grab user ids of users with user_level of ADMIN
		$sql = "SELECT user_id
			FROM {$convert->src_table_prefix}users
			WHERE user_level = 1
			ORDER BY user_regdate ASC";
		$result = $src_db->sql_query($sql);

		while ($row = $src_db->sql_fetchrow($result))
		{
			$user_id = (int) phpbb_user_id($row['user_id']);
			// Set founder admin...
			$sql = 'UPDATE ' . USERS_TABLE . '
				SET user_type = ' . USER_FOUNDER . "
				WHERE user_id = $user_id";
			$db->sql_query($sql);
		}
		$src_db->sql_freeresult($result);

		$sql = 'SELECT group_id
			FROM ' . GROUPS_TABLE . "
			WHERE group_name = '" . $db->sql_escape('BOTS') . "'";
		$result = $db->sql_query($sql);
		$bot_group_id = (int) $db->sql_fetchfield('group_id');
		$db->sql_freeresult($result);
	}

	// Grab forum auth information
	$sql = "SELECT *
		FROM {$convert->src_table_prefix}forums";
	$result = $src_db->sql_query($sql);

	$forum_access = array();
	while ($row = $src_db->sql_fetchrow($result))
	{
		$forum_access[$row['forum_id']] = $row;
	}
	$src_db->sql_freeresult($result);

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'binary'");
	}
	// Grab user auth information from 2.0.x board
	$sql = "SELECT ug.user_id, aa.*
		FROM {$convert->src_table_prefix}auth_access aa, {$convert->src_table_prefix}user_group ug, {$convert->src_table_prefix}groups g, {$convert->src_table_prefix}forums f
		WHERE g.group_id = aa.group_id
			AND g.group_single_user = 1
			AND ug.group_id = g.group_id
			AND f.forum_id = aa.forum_id";
	$result = $src_db->sql_query($sql);

	$user_access = array();
	while ($row = $src_db->sql_fetchrow($result))
	{
		$user_access[$row['forum_id']][] = $row;
	}
	$src_db->sql_freeresult($result);

	// Grab group auth information
	$sql = "SELECT g.group_id, aa.*
		FROM {$convert->src_table_prefix}auth_access aa, {$convert->src_table_prefix}groups g
		WHERE g.group_id = aa.group_id
			AND g.group_single_user <> 1";
	$result = $src_db->sql_query($sql);

	$group_access = array();
	while ($row = $src_db->sql_fetchrow($result))
	{
		$group_access[$row['forum_id']][] = $row;
	}
	$src_db->sql_freeresult($result);

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'utf8'");
	}

	// Add Forum Access List
	$auth_map = array(
		'auth_view'			=> array('f_', 'f_list'),
		'auth_read'			=> array('f_read', 'f_search'),
		'auth_post'			=> array('f_post', 'f_bbcode', 'f_smilies', 'f_img', 'f_sigs', 'f_postcount', 'f_report', 'f_subscribe', 'f_print', 'f_email'),
		'auth_reply'		=> 'f_reply',
		'auth_edit'			=> 'f_edit',
		'auth_delete'		=> 'f_delete',
		'auth_pollcreate'	=> 'f_poll',
		'auth_vote'			=> 'f_vote',
		'auth_announce'		=> 'f_announce',
		'auth_sticky'		=> 'f_sticky',
		'auth_attachments'	=> array('f_attach', 'f_download'),
		'auth_download'		=> 'f_download',
	);

	// Define the ACL constants used in 2.0 to make the code slightly more readable
	define('AUTH_ALL', 0);
	define('AUTH_REG', 1);
	define('AUTH_ACL', 2);
	define('AUTH_MOD', 3);
	define('AUTH_ADMIN', 5);

	// A mapping of the simple permissions used by 2.0
	$simple_auth_ary = array(
		'public'			=> array(
			'auth_view'			=> AUTH_ALL,
			'auth_read'			=> AUTH_ALL,
			'auth_post'			=> AUTH_ALL,
			'auth_reply'		=> AUTH_ALL,
			'auth_edit'			=> AUTH_REG,
			'auth_delete'		=> AUTH_REG,
			'auth_sticky'		=> AUTH_MOD,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_REG,
			'auth_pollcreate'	=> AUTH_REG,
		),
		'registered'		=> array(
			'auth_view'			=> AUTH_ALL,
			'auth_read'			=> AUTH_ALL,
			'auth_post'			=> AUTH_REG,
			'auth_reply'		=> AUTH_REG,
			'auth_edit'			=> AUTH_REG,
			'auth_delete'		=> AUTH_REG,
			'auth_sticky'		=> AUTH_MOD,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_REG,
			'auth_pollcreate'	=> AUTH_REG,
		),
		'registered_hidden'	=> array(
			'auth_view'			=> AUTH_REG,
			'auth_read'			=> AUTH_REG,
			'auth_post'			=> AUTH_REG,
			'auth_reply'		=> AUTH_REG,
			'auth_edit'			=> AUTH_REG,
			'auth_delete'		=> AUTH_REG,
			'auth_sticky'		=> AUTH_MOD,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_REG,
			'auth_pollcreate'	=> AUTH_REG,
		),
		'private'			=> array(
			'auth_view'			=> AUTH_ALL,
			'auth_read'			=> AUTH_ACL,
			'auth_post'			=> AUTH_ACL,
			'auth_reply'		=> AUTH_ACL,
			'auth_edit'			=> AUTH_ACL,
			'auth_delete'		=> AUTH_ACL,
			'auth_sticky'		=> AUTH_ACL,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_ACL,
			'auth_pollcreate'	=> AUTH_ACL,
		),
		'private_hidden'	=> array(
			'auth_view'			=> AUTH_ACL,
			'auth_read'			=> AUTH_ACL,
			'auth_post'			=> AUTH_ACL,
			'auth_reply'		=> AUTH_ACL,
			'auth_edit'			=> AUTH_ACL,
			'auth_delete'		=> AUTH_ACL,
			'auth_sticky'		=> AUTH_ACL,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_ACL,
			'auth_pollcreate'	=> AUTH_ACL,
		),
		'moderator'			=> array(
			'auth_view'			=> AUTH_ALL,
			'auth_read'			=> AUTH_MOD,
			'auth_post'			=> AUTH_MOD,
			'auth_reply'		=> AUTH_MOD,
			'auth_edit'			=> AUTH_MOD,
			'auth_delete'		=> AUTH_MOD,
			'auth_sticky'		=> AUTH_MOD,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_MOD,
			'auth_pollcreate'	=> AUTH_MOD,
		),
		'moderator_hidden'	=> array(
			'auth_view'			=> AUTH_MOD,
			'auth_read'			=> AUTH_MOD,
			'auth_post'			=> AUTH_MOD,
			'auth_reply'		=> AUTH_MOD,
			'auth_edit'			=> AUTH_MOD,
			'auth_delete'		=> AUTH_MOD,
			'auth_sticky'		=> AUTH_MOD,
			'auth_announce'		=> AUTH_MOD,
			'auth_vote'			=> AUTH_MOD,
			'auth_pollcreate'	=> AUTH_MOD,
		),
	);

	if ($mode == 'start')
	{
		user_group_auth('guests', 'SELECT user_id, {GUESTS} FROM ' . USERS_TABLE . ' WHERE user_id = ' . ANONYMOUS, false);
		user_group_auth('registered', 'SELECT user_id, {REGISTERED} FROM ' . USERS_TABLE . ' WHERE user_id <> ' . ANONYMOUS . " AND group_id <> $bot_group_id", false);

		// Selecting from old table
		if (!empty($config['increment_user_id']))
		{
			$auth_sql = 'SELECT user_id, {ADMINISTRATORS} FROM ' . $convert->src_table_prefix . 'users WHERE user_level = 1 AND user_id <> 1';
			user_group_auth('administrators', $auth_sql, true);

			$auth_sql = 'SELECT ' . $config['increment_user_id'] . ' as user_id, {ADMINISTRATORS} FROM ' . $convert->src_table_prefix . 'users WHERE user_level = 1 AND user_id = 1';
			user_group_auth('administrators', $auth_sql, true);
		}
		else
		{
			$auth_sql = 'SELECT user_id, {ADMINISTRATORS} FROM ' . $convert->src_table_prefix . 'users WHERE user_level = 1';
			user_group_auth('administrators', $auth_sql, true);
		}

		if (!empty($config['increment_user_id']))
		{
			$auth_sql = 'SELECT user_id, {GLOBAL_MODERATORS} FROM ' . $convert->src_table_prefix . 'users WHERE user_level = 1 AND user_id <> 1';
			user_group_auth('global_moderators', $auth_sql, true);

			$auth_sql = 'SELECT ' . $config['increment_user_id'] . ' as user_id, {GLOBAL_MODERATORS} FROM ' . $convert->src_table_prefix . 'users WHERE user_level = 1 AND user_id = 1';
			user_group_auth('global_moderators', $auth_sql, true);
		}
		else
		{
			$auth_sql = 'SELECT user_id, {GLOBAL_MODERATORS} FROM ' . $convert->src_table_prefix . 'users WHERE user_level = 1';
			user_group_auth('global_moderators', $auth_sql, true);
		}
	}
	else if ($mode == 'first')
	{
		// Go through all 2.0.x forums
		foreach ($forum_access as $forum)
		{
			$new_forum_id = (int) $forum['forum_id'];

			// Administrators have full access to all forums whatever happens
			mass_auth('group_role', $new_forum_id, 'administrators', 'FORUM_FULL');

			$matched_type = '';
			foreach ($simple_auth_ary as $key => $auth_levels)
			{
				$matched = 1;
				foreach ($auth_levels as $k => $level)
				{
					if ($forum[$k] != $auth_levels[$k])
					{
						$matched = 0;
					}
				}

				if ($matched)
				{
					$matched_type = $key;
					break;
				}
			}

			switch ($matched_type)
			{
				case 'public':
					mass_auth('group_role', $new_forum_id, 'guests', 'FORUM_LIMITED');
					mass_auth('group_role', $new_forum_id, 'registered', 'FORUM_LIMITED_POLLS');
					mass_auth('group_role', $new_forum_id, 'bots', 'FORUM_BOT');
				break;

				case 'registered':
					mass_auth('group_role', $new_forum_id, 'guests', 'FORUM_READONLY');
					mass_auth('group_role', $new_forum_id, 'bots', 'FORUM_BOT');

				// no break;

				case 'registered_hidden':
					mass_auth('group_role', $new_forum_id, 'registered', 'FORUM_POLLS');
				break;

				case 'private':
				case 'private_hidden':
				case 'moderator':
				case 'moderator_hidden':
				default:
					// The permissions don't match a simple set, so we're going to have to map them directly

					// No post approval for all, in 2.0.x this feature does not exist
					mass_auth('group', $new_forum_id, 'guests', 'f_noapprove', ACL_YES);
					mass_auth('group', $new_forum_id, 'registered', 'f_noapprove', ACL_YES);

					// Go through authentication map
					foreach ($auth_map as $old_auth_key => $new_acl)
					{
						// If old authentication key does not exist we continue
						// This is helpful for mods adding additional authentication fields, we need to add them to the auth_map array
						if (!isset($forum[$old_auth_key]))
						{
							continue;
						}

						// Now set the new ACL correctly
						switch ($forum[$old_auth_key])
						{
							// AUTH_ALL
							case AUTH_ALL:
								mass_auth('group', $new_forum_id, 'guests', $new_acl, ACL_YES);
								mass_auth('group', $new_forum_id, 'bots', $new_acl, ACL_YES);
								mass_auth('group', $new_forum_id, 'registered', $new_acl, ACL_YES);
							break;

							// AUTH_REG
							case AUTH_REG:
								mass_auth('group', $new_forum_id, 'registered', $new_acl, ACL_YES);
							break;

							// AUTH_ACL
							case AUTH_ACL:
								// Go through the old group access list for this forum
								if (isset($group_access[$forum['forum_id']]))
								{
									foreach ($group_access[$forum['forum_id']] as $index => $access)
									{
										// We only check for ACL_YES equivalence entry
										if (isset($access[$old_auth_key]) && $access[$old_auth_key] == 1)
										{
											mass_auth('group', $new_forum_id, (int) $access['group_id'], $new_acl, ACL_YES);
										}
									}
								}

								if (isset($user_access[$forum['forum_id']]))
								{
									foreach ($user_access[$forum['forum_id']] as $index => $access)
									{
										// We only check for ACL_YES equivalence entry
										if (isset($access[$old_auth_key]) && $access[$old_auth_key] == 1)
										{
											mass_auth('user', $new_forum_id, (int) phpbb_user_id($access['user_id']), $new_acl, ACL_YES);
										}
									}
								}
							break;

							// AUTH_MOD
							case AUTH_MOD:
								if (isset($group_access[$forum['forum_id']]))
								{
									foreach ($group_access[$forum['forum_id']] as $index => $access)
									{
										// We only check for ACL_YES equivalence entry
										if (isset($access[$old_auth_key]) && $access[$old_auth_key] == 1)
										{
											mass_auth('group', $new_forum_id, (int) $access['group_id'], $new_acl, ACL_YES);
										}
									}
								}

								if (isset($user_access[$forum['forum_id']]))
								{
									foreach ($user_access[$forum['forum_id']] as $index => $access)
									{
										// We only check for ACL_YES equivalence entry
										if (isset($access[$old_auth_key]) && $access[$old_auth_key] == 1)
										{
											mass_auth('user', $new_forum_id, (int) phpbb_user_id($access['user_id']), $new_acl, ACL_YES);
										}
									}
								}
							break;
						}
					}
				break;
			}
		}
	}
	else if ($mode == 'second')
	{
		// Assign permission roles and other default permissions

		// guests having u_download and u_search ability
		$db->sql_query('INSERT INTO ' . ACL_GROUPS_TABLE . ' (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) SELECT ' . get_group_id('guests') . ', 0, auth_option_id, 0, 1 FROM ' . ACL_OPTIONS_TABLE . " WHERE auth_option IN ('u_', 'u_download', 'u_search')");

		// administrators/global mods having full user features
		mass_auth('group_role', 0, 'administrators', 'USER_FULL');
		mass_auth('group_role', 0, 'global_moderators', 'USER_FULL');

		// By default all converted administrators are given full access
		mass_auth('group_role', 0, 'administrators', 'ADMIN_FULL');

		// All registered users are assigned the standard user role
		mass_auth('group_role', 0, 'registered', 'USER_STANDARD');
		mass_auth('group_role', 0, 'registered_coppa', 'USER_STANDARD');

		// Instead of administrators being global moderators we give the MOD_FULL role to global mods (admins already assigned to this group)
		mass_auth('group_role', 0, 'global_moderators', 'MOD_FULL');

		// And now those who have had their avatar rights removed get assigned a more restrictive role
		$sql = 'SELECT user_id FROM ' . $convert->src_table_prefix . 'users
			WHERE user_allowavatar = 0
				AND user_id > 0';
		$result = $src_db->sql_query($sql);

		while ($row = $src_db->sql_fetchrow($result))
		{
			mass_auth('user_role', 0, (int) phpbb_user_id($row['user_id']), 'USER_NOAVATAR');
		}
		$src_db->sql_freeresult($result);

		// And the same for those who have had their PM rights removed
		$sql = 'SELECT user_id FROM ' . $convert->src_table_prefix . 'users
			WHERE user_allow_pm = 0
				AND user_id > 0';
		$result = $src_db->sql_query($sql);

		while ($row = $src_db->sql_fetchrow($result))
		{
			mass_auth('user_role', 0, (int) phpbb_user_id($row['user_id']), 'USER_NOPM');
		}
		$src_db->sql_freeresult($result);
	}
	else if ($mode == 'third')
	{
		// And now the moderators
		// We make sure that they have at least standard access to the forums they moderate in addition to the moderating permissions
		
		$mod_post_map = array(
			'auth_announce'		=> 'f_announce',
			'auth_sticky'		=> 'f_sticky'
		);
		
		foreach ($user_access as $forum_id => $access_map)
		{
			$forum_id = (int) $forum_id;

			foreach ($access_map as $access)
			{
				if (isset($access['auth_mod']) && $access['auth_mod'] == 1)
				{
					mass_auth('user_role', $forum_id, (int) phpbb_user_id($access['user_id']), 'MOD_STANDARD');
					mass_auth('user_role', $forum_id, (int) phpbb_user_id($access['user_id']), 'FORUM_STANDARD');
					foreach ($mod_post_map as $old => $new)
					{
						if (isset($forum_access[$forum_id]) && isset($forum_access[$forum_id][$old]) && $forum_access[$forum_id][$old] == AUTH_MOD)
						{
							mass_auth('user', $forum_id, (int) phpbb_user_id($access['user_id']), $new, ACL_YES);
						}
					}
				}
			}
		}

		foreach ($group_access as $forum_id => $access_map)
		{
			$forum_id = (int) $forum_id;

			foreach ($access_map as $access)
			{
				if (isset($access['auth_mod']) && $access['auth_mod'] == 1)
				{
					mass_auth('group_role', $forum_id, (int) $access['group_id'], 'MOD_STANDARD');
					mass_auth('group_role', $forum_id, (int) $access['group_id'], 'FORUM_STANDARD');
					foreach ($mod_post_map as $old => $new)
					{
						if (isset($forum_access[$forum_id]) && isset($forum_access[$forum_id][$old]) && $forum_access[$forum_id][$old] == AUTH_MOD)
						{
							mass_auth('group', $forum_id, (int) $access['group_id'], $new, ACL_YES);
						}
					}
				}
			}
		}

		// We grant everyone readonly access to the categories to ensure that the forums are visible
		$sql = 'SELECT forum_id, forum_name, parent_id, left_id, right_id
			FROM ' . FORUMS_TABLE . '
			ORDER BY left_id ASC';
		$result = $db->sql_query($sql);

		$parent_forums = $forums = array();
		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['parent_id'] == 0)
			{
				mass_auth('group_role', $row['forum_id'], 'administrators', 'FORUM_FULL');
				mass_auth('group_role', $row['forum_id'], 'global_moderators', 'FORUM_FULL');
				$parent_forums[] = $row;
			}
			else
			{
				$forums[] = $row;
			}
		}
		$db->sql_freeresult($result);

		global $auth;

		// Let us see which groups have access to these forums...
		foreach ($parent_forums as $row)
		{
			// Get the children
			$branch = $forum_ids = array();

			foreach ($forums as $key => $_row)
			{
				if ($_row['left_id'] > $row['left_id'] && $_row['left_id'] < $row['right_id'])
				{
					$branch[] = $_row;
					$forum_ids[] = $_row['forum_id'];
					continue;
				}
			}

			if (sizeof($forum_ids))
			{
				// Now make sure the user is able to read these forums
				$hold_ary = $auth->acl_group_raw_data(false, 'f_list', $forum_ids);

				if (empty($hold_ary))
				{
					continue;
				}

				foreach ($hold_ary as $g_id => $f_id_ary)
				{
					$set_group = false;

					foreach ($f_id_ary as $f_id => $auth_ary)
					{
						foreach ($auth_ary as $auth_option => $setting)
						{
							if ($setting == ACL_YES)
							{
								$set_group = true;
								break 2;
							}
						}
					}

					if ($set_group)
					{
						mass_auth('group', $row['forum_id'], $g_id, 'f_list', ACL_YES);
					}
				}
			}
		}
	}
}

/**
* Set primary group.
* Really simple and only based on user_level (remaining groups will be assigned later)
*/
function phpbb_set_primary_group($user_level)
{
	global $convert_row;

	if ($user_level == 1)
	{
		return get_group_id('administrators');
	}
/*	else if ($user_level == 2)
	{
		return get_group_id('global_moderators');
	}
	else if ($user_level == 0 && $convert_row['user_active'])*/
	else if ($convert_row['user_active'])
	{
		return get_group_id('registered');
	}

	return 0;
}

/**
* Convert the group name, making sure to avoid conflicts with 3.0 special groups
*/
function phpbb_convert_group_name($group_name)
{
	$default_groups = array(
		'GUESTS',
		'REGISTERED',
		'REGISTERED_COPPA',
		'GLOBAL_MODERATORS',
		'ADMINISTRATORS',
		'BOTS',
	);

	if (in_array(strtoupper($group_name), $default_groups))
	{
		return 'phpBB2 - ' . $group_name;
	}

	return phpbb_set_default_encoding($group_name);
}

/**
* Convert the group type constants
*/
function phpbb_convert_group_type($group_type)
{
	switch ($group_type)
	{
		case 0:
			return GROUP_OPEN;
		break;

		case 1:
			return GROUP_CLOSED;
		break;

		case 2:
			return GROUP_HIDDEN;
		break;
	}

	// Never return GROUP_SPECIAL here, because only phpBB3's default groups are allowed to have this type set.
	return GROUP_HIDDEN;
}

/**
* Convert the topic type constants
*/
function phpbb_convert_topic_type($topic_type)
{
	switch ($topic_type)
	{
		case 0:
			return POST_NORMAL;
		break;

		case 1:
			return POST_STICKY;
		break;

		case 2:
			return POST_ANNOUNCE;
		break;

		case 3:
			return POST_GLOBAL;
		break;
	}

	return POST_NORMAL;
}

function phpbb_replace_size($matches)
{
	return '[size=' . min(200, ceil(100.0 * (((double) $matches[1])/12.0))) . ':' . $matches[2] . ']';
}

/**
* Reparse the message stripping out the bbcode_uid values and adding new ones and setting the bitfield
* @todo What do we want to do about HTML in messages - currently it gets converted to the entities, but there may be some objections to this
*/
function phpbb_prepare_message($message)
{
	global $phpbb_root_path, $phpEx, $db, $convert, $user, $config, $cache, $convert_row, $message_parser;

	if (!$message)
	{
		$convert->row['mp_bbcode_bitfield'] = $convert_row['mp_bbcode_bitfield'] = 0;
		return '';
	}

	// Decode phpBB 2.0.x Message
	if (isset($convert->row['old_bbcode_uid']) && $convert->row['old_bbcode_uid'] != '')
	{
		// Adjust size...
		if (strpos($message, '[size=') !== false)
		{
			$message = preg_replace_callback('/\[size=(\d*):(' . $convert->row['old_bbcode_uid'] . ')\]/', 'phpbb_replace_size', $message);
		}

		$message = preg_replace('/\:(([a-z0-9]:)?)' . $convert->row['old_bbcode_uid'] . '/s', '', $message);
	}

	if (strpos($message, '[quote=') !== false)
	{
		$message = preg_replace('/\[quote="(.*?)"\]/s', '[quote=&quot;\1&quot;]', $message);
		$message = preg_replace('/\[quote=\\\"(.*?)\\\"\]/s', '[quote=&quot;\1&quot;]', $message);
		
		// let's hope that this solves more problems than it causes. Deal with escaped quotes.
		$message = str_replace('\"', '&quot;', $message);
		$message = str_replace('\&quot;', '&quot;', $message);
	}

	// Already the new user id ;)
	$user_id = $convert->row['poster_id'];

	$message = str_replace('<', '&lt;', $message);
	$message = str_replace('>', '&gt;', $message);
	$message = str_replace('<br />', "\n", $message);

	// make the post UTF-8
	$message = phpbb_set_encoding($message);

	$message_parser->warn_msg = array(); // Reset the errors from the previous message
	$message_parser->bbcode_uid = make_uid($convert->row['post_time']);
	$message_parser->message = $message;
	unset($message);

	// Make sure options are set.
//	$enable_html = (!isset($row['enable_html'])) ? false : $row['enable_html'];
	$enable_bbcode = (!isset($convert->row['enable_bbcode'])) ? true : $convert->row['enable_bbcode'];
	$enable_smilies = (!isset($convert->row['enable_smilies'])) ? true : $convert->row['enable_smilies'];
	$enable_magic_url = (!isset($convert->row['enable_magic_url'])) ? true : $convert->row['enable_magic_url'];

	// parse($allow_bbcode, $allow_magic_url, $allow_smilies, $allow_img_bbcode = true, $allow_flash_bbcode = true, $allow_quote_bbcode = true, $allow_url_bbcode = true, $update_this_message = true, $mode = 'post')
	$message_parser->parse($enable_bbcode, $enable_magic_url, $enable_smilies);
	
	if (sizeof($message_parser->warn_msg))
	{
		$msg_id = isset($convert->row['post_id']) ? $convert->row['post_id'] : $convert->row['privmsgs_id'];
		$convert->p_master->error('<span style="color:red">' . $user->lang['POST_ID'] . ': ' . $msg_id . ' ' . $user->lang['CONV_ERROR_MESSAGE_PARSER'] . ': <br /><br />' . implode('<br />', $message_parser->warn_msg), __LINE__, __FILE__, true);
	}

	$convert->row['mp_bbcode_bitfield'] = $convert_row['mp_bbcode_bitfield'] = $message_parser->bbcode_bitfield;

	$message = $message_parser->message;
	unset($message_parser->message);

	return $message;
}

/**
* Return the bitfield calculated by the previous function
*/
function get_bbcode_bitfield()
{
	global $convert_row;

	return $convert_row['mp_bbcode_bitfield'];
}

/**
* Determine the last user to edit a post
* In practice we only tracked edits by the original poster in 2.0.x so this will only be set if they had edited their own post
*/
function phpbb_post_edit_user()
{
	global $convert_row, $config;

	if (isset($convert_row['post_edit_count']))
	{
		return phpbb_user_id($convert_row['poster_id']);
	}

	return 0;
}

/**
* Obtain the path to uploaded files on the 2.0.x forum
* This is only used if the Attachment MOD was installed
*/
function phpbb_get_files_dir()
{
	if (!defined('MOD_ATTACHMENT'))
	{
		return;
	}

	global $src_db, $same_db, $convert, $user, $config, $cache;

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'binary'");
	}
	$sql = 'SELECT config_value AS upload_dir
		FROM ' . $convert->src_table_prefix . "attachments_config
		WHERE config_name = 'upload_dir'";
	$result = $src_db->sql_query($sql);
	$upload_path = $src_db->sql_fetchfield('upload_dir');
	$src_db->sql_freeresult($result);

	$sql = 'SELECT config_value AS ftp_upload
		FROM ' . $convert->src_table_prefix . "attachments_config
		WHERE config_name = 'allow_ftp_upload'";
	$result = $src_db->sql_query($sql);
	$ftp_upload = (int) $src_db->sql_fetchfield('ftp_upload');
	$src_db->sql_freeresult($result);

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'utf8'");
	}

	if ($ftp_upload)
	{
		$convert->p_master->error($user->lang['CONV_ERROR_ATTACH_FTP_DIR'], __LINE__, __FILE__);
	}

	return $upload_path;
}

/**
* Copy thumbnails of uploaded images from the 2.0.x forum
* This is only used if the Attachment MOD was installed
*/
function phpbb_copy_thumbnails()
{
	global $db, $convert, $user, $config, $cache, $phpbb_root_path;

	$src_path = $convert->options['forum_path'] . '/' . phpbb_get_files_dir() . '/thumbs/';
	
	if ($handle = @opendir($src_path))
	{
		while ($entry = readdir($handle))
		{
			if ($entry[0] == '.')
			{
				continue;
			}

			if (is_dir($src_path . $entry))
			{
				continue;
			}
			else
			{
				copy_file($src_path . $entry, $config['upload_path'] . '/' . preg_replace('/^t_/', 'thumb_', $entry));
				@unlink($phpbb_root_path . $config['upload_path'] . '/thumbs/' . $entry);
			}
		}
		closedir($handle);
	}
}

/**
* Convert the attachment category constants
* This is only used if the Attachment MOD was installed
*/
function phpbb_attachment_category($cat_id)
{
	switch ($cat_id)
	{
		case 1:
			return ATTACHMENT_CATEGORY_IMAGE;
		break;

		case 2:
			return ATTACHMENT_CATEGORY_WM;
		break;

		case 3:
			return ATTACHMENT_CATEGORY_FLASH;
		break;
	}

	return ATTACHMENT_CATEGORY_NONE;
}

/**
* Obtain list of forums in which different attachment categories can be used
*/
function phpbb_attachment_forum_perms($forum_permissions)
{
	if (empty($forum_permissions))
	{
		return '';
	}

	// Decode forum permissions
	$forum_ids = array();

	$one_char_encoding = '#';
	$two_char_encoding = '.';

	$auth_len = 1;
	for ($pos = 0; $pos < strlen($forum_permissions); $pos += $auth_len)
	{
		$forum_auth = substr($forum_permissions, $pos, 1);
		if ($forum_auth == $one_char_encoding)
		{
			$auth_len = 1;
			continue;
		}
		else if ($forum_auth == $two_char_encoding)
		{
			$auth_len = 2;
			$pos--;
			continue;
		}
		
		$forum_auth = substr($forum_permissions, $pos, $auth_len);
		$forum_id = base64_unpack($forum_auth);

		$forum_ids[] = (int) $forum_id;
	}
	
	if (sizeof($forum_ids))
	{
		return attachment_forum_perms($forum_ids);
	}

	return '';
}

/**
* Convert the avatar type constants
*/
function phpbb_avatar_type($type)
{
	switch ($type)
	{
		case 1:
			return AVATAR_UPLOAD;
		break;

		case 2:
			return AVATAR_REMOTE;
		break;

		case 3:
			return AVATAR_GALLERY;
		break;
	}

	return 0;
}


/**
* Just undos the replacing of '<' and '>'
*/
function  phpbb_smilie_html_decode($code)
{
	$code = str_replace('&lt;', '<', $code);
	return str_replace('&gt;', '>', $code);
}

/**
* Transfer avatars, copying the image if it was uploaded
*/
function phpbb_import_avatar($user_avatar)
{
	global $convert_row;

	if (!$convert_row['user_avatar_type'])
	{
		return '';
	}
	else if ($convert_row['user_avatar_type'] == 1)
	{
		// Uploaded avatar
		return import_avatar($user_avatar, false, $convert_row['user_id']);
	}
	else if ($convert_row['user_avatar_type'] == 2)
	{
		// Remote avatar
		return $user_avatar;
	}
	else if ($convert_row['user_avatar_type'] == 3)
	{
		// Gallery avatar
		return $user_avatar;
	}

	return '';
}


/**
* Find out about the avatar's dimensions
*/
function phpbb_get_avatar_height($user_avatar)
{
	global $convert_row;
	
	if (empty($convert_row['user_avatar_type']))
	{
		return 0;
	}
	return get_avatar_height($user_avatar, 'phpbb_avatar_type', $convert_row['user_avatar_type']);
}


/**
* Find out about the avatar's dimensions
*/
function phpbb_get_avatar_width($user_avatar)
{
	global $convert_row;

	if (empty($convert_row['user_avatar_type']))
	{
		return 0;
	}
	
	return get_avatar_width($user_avatar, 'phpbb_avatar_type', $convert_row['user_avatar_type']);
}


/**
* Calculate the correct to_address field for private messages
*/
function phpbb_privmsgs_to_userid($to_userid)
{
	global $config;

	return 'u_' . phpbb_user_id($to_userid);
}

/**
* Calculate whether a private message was unread using the bitfield
*/
function phpbb_unread_pm($pm_type)
{
	return ($pm_type == 5) ? 1 : 0;
}

/**
* Calculate whether a private message was new using the bitfield
*/
function phpbb_new_pm($pm_type)
{
	return ($pm_type == 1) ? 1 : 0;
}

/**
* Obtain the folder_id for the custom folder created to replace the savebox from 2.0.x (used to store saved private messages)
*/
function phpbb_get_savebox_id($user_id)
{
	global $db;

	$user_id = phpbb_user_id($user_id);

	// Only one custom folder, check only one
	$sql = 'SELECT folder_id
		FROM ' . PRIVMSGS_FOLDER_TABLE . '
		WHERE user_id = ' . $user_id;
	$result = $db->sql_query_limit($sql, 1);
	$folder_id = (int) $db->sql_fetchfield('folder_id');
	$db->sql_freeresult($result);

	return $folder_id;
}

/**
* Transfer attachment specific configuration options
* These were not stored in the main config table on 2.0.x
* This is only used if the Attachment MOD was installed
*/
function phpbb_import_attach_config()
{
	global $db, $src_db, $same_db, $convert, $config;

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'binary'");
	}

	$sql = 'SELECT *
		FROM ' . $convert->src_table_prefix . 'attachments_config';
	$result = $src_db->sql_query($sql);

	if ($convert->mysql_convert && $same_db)
	{
		$src_db->sql_query("SET NAMES 'utf8'");
	}

	$attach_config = array();
	while ($row = $src_db->sql_fetchrow($result))
	{
		$attach_config[$row['config_name']] = $row['config_value'];
	}
	$src_db->sql_freeresult($result);

	set_config('allow_attachments', 1);

	// old attachment mod? Must be very old if this entry do not exist...
	if (!empty($attach_config['display_order']))
	{
		set_config('display_order', $attach_config['display_order']);
	}
	set_config('max_filesize', $attach_config['max_filesize']);
	set_config('max_filesize_pm', $attach_config['max_filesize_pm']);
	set_config('attachment_quota', $attach_config['attachment_quota']);
	set_config('max_attachments', $attach_config['max_attachments']);
	set_config('max_attachments_pm', $attach_config['max_attachments_pm']);
	set_config('allow_pm_attach', $attach_config['allow_pm_attach']);

	set_config('img_display_inlined', $attach_config['img_display_inlined']);
	set_config('img_max_width', $attach_config['img_max_width']);
	set_config('img_max_height', $attach_config['img_max_height']);
	set_config('img_link_width', $attach_config['img_link_width']);
	set_config('img_link_height', $attach_config['img_link_height']);
	set_config('img_create_thumbnail', $attach_config['img_create_thumbnail']);
	set_config('img_max_thumb_width', 400);
	set_config('img_min_thumb_filesize', $attach_config['img_min_thumb_filesize']);
	set_config('img_imagick', $attach_config['img_imagick']);
}

/**
* Calculate the date a user became inactive
*/
function phpbb_inactive_time()
{
	global $convert_row;

	if ($convert_row['user_active'])
	{
		return 0;
	}

	if ($convert_row['user_lastvisit'])
	{
		return $convert_row['user_lastvisit'];
	}

	return $convert_row['user_regdate'];
}

/**
* Calculate the reason a user became inactive
* We can't actually tell the difference between a manual deactivation and one for profile changes
* from the data available to assume the latter
*/
function phpbb_inactive_reason()
{
	global $convert_row;

	if ($convert_row['user_active'])
	{
		return 0;
	}

	if ($convert_row['user_lastvisit'])
	{
		return INACTIVE_PROFILE;
	}

	return INACTIVE_REGISTER;
}

/**
* Adjust 2.0.x disallowed names to 3.0.x format
*/
function phpbb_disallowed_username($username)
{
	// Replace * with %
	$username = phpbb_set_default_encoding(str_replace('*', '%', $username));
	return utf8_htmlspecialchars($username);
}

/**
* Checks whether there are any usernames on the old board that would map to the same
* username_clean on phpBB3. Prints out a list if any exist and exits.
*/
function phpbb_check_username_collisions()
{
	global $db, $src_db, $convert, $table_prefix, $user, $lang;

	$map_dbms = '';
	switch ($db->sql_layer)
	{
		case 'mysql':
			$map_dbms = 'mysql_40';
		break;
	
		case 'mysql4':
			if (version_compare($db->mysql_version, '4.1.3', '>='))
			{
				$map_dbms = 'mysql_41';
			}
			else
			{
				$map_dbms = 'mysql_40';
			}
		break;
	
		case 'mysqli':
			$map_dbms = 'mysql_41';
		break;
	
		case 'mssql':
		case 'mssql_odbc':
			$map_dbms = 'mssql';
		break;
	
		default:
			$map_dbms = $db->sql_layer;
		break;
	}

	// create a temporary table in which we store the clean usernames
	$drop_sql = 'DROP TABLE ' . $table_prefix . 'userconv';
	switch ($map_dbms)
	{
		case 'firebird':
			$create_sql = 'CREATE TABLE ' . $table_prefix . 'userconv (
				user_id INTEGER NOT NULL,
				username_clean VARCHAR(255) CHARACTER SET UTF8 DEFAULT \'\' NOT NULL COLLATE UNICODE
			)';
		break;

		case 'mssql':
			$create_sql = 'CREATE TABLE [' . $table_prefix . 'userconv] (
				[user_id] [int] NOT NULL ,
				[username_clean] [varchar] (255) DEFAULT (\'\') NOT NULL
			)';
		break;

		case 'mysql_40':
			$create_sql = 'CREATE TABLE ' . $table_prefix . 'userconv (
				user_id mediumint(8) NOT NULL,
				username_clean blob NOT NULL
			)';
		break;

		case 'mysql_41':
			$create_sql = 'CREATE TABLE ' . $table_prefix . 'userconv (
				user_id mediumint(8) NOT NULL,
				username_clean varchar(255) DEFAULT \'\' NOT NULL
			) CHARACTER SET `utf8` COLLATE `utf8_bin`';
		break;

		case 'oracle':
			$create_sql = 'CREATE TABLE ' . $table_prefix . 'userconv (
				user_id number(8) NOT NULL,
				username_clean varchar2(255) DEFAULT \'\'
			)';
		break;

		case 'postgres':
			$create_sql = 'CREATE TABLE ' . $table_prefix . 'userconv (
				user_id INT4 DEFAULT \'0\',
				username_clean varchar_ci DEFAULT \'\' NOT NULL
			)';
		break;

		case 'sqlite':
			$create_sql = 'CREATE TABLE ' . $table_prefix . 'userconv (
				user_id INTEGER NOT NULL DEFAULT \'0\',
				username_clean varchar(255) NOT NULL DEFAULT \'\'
			)';
		break;
	}

	$db->sql_return_on_error(true);
	$db->sql_query($drop_sql);
	$db->sql_return_on_error(false);
	$db->sql_query($create_sql);

	// now select all user_ids and usernames and then convert the username (this can take quite a while!)
	$sql = 'SELECT user_id, username
		FROM ' . $convert->src_table_prefix . 'users';
	$result = $src_db->sql_query($sql);

	$insert_ary = array();
	$i = 0;
	while ($row = $src_db->sql_fetchrow($result))
	{
		$clean_name = utf8_clean_string(phpbb_set_default_encoding($row['username']));
		$insert_ary[] = array('user_id' => (int) $row['user_id'], 'username_clean' => (string) $clean_name);

		if ($i % 1000 == 999)
		{
			$db->sql_multi_insert($table_prefix . 'userconv', $insert_ary);
			$insert_ary = array();
		}
		$i++;
	}
	$src_db->sql_freeresult($result);

	if (sizeof($insert_ary))
	{
		$db->sql_multi_insert($table_prefix . 'userconv', $insert_ary);
	}
	unset($insert_ary);

	// now find the clean version of the usernames that collide
	$sql = 'SELECT username_clean
		FROM ' . $table_prefix . 'userconv
		GROUP BY username_clean
		HAVING COUNT(user_id) > 1';
	$result = $db->sql_query($sql);

	$colliding_names = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$colliding_names[] = $row['username_clean'];
	}
	$db->sql_freeresult($result);

	// there was at least one collision, the admin will have to solve it before conversion can continue
	if (sizeof($colliding_names))
	{
		$sql = 'SELECT user_id, username_clean
			FROM ' . $table_prefix . 'userconv
			WHERE ' . $db->sql_in_set('username_clean', $colliding_names);
		$result = $db->sql_query($sql);
		unset($colliding_names);

		$colliding_user_ids = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$colliding_user_ids[(int) $row['user_id']] = $row['username_clean'];
		}
		$db->sql_freeresult($result);

		$sql = 'SELECT username, user_id, user_posts
			FROM ' . $convert->src_table_prefix . 'users
			WHERE ' . $src_db->sql_in_set('user_id', array_keys($colliding_user_ids));
		$result = $src_db->sql_query($sql);

		$colliding_users = array();
		while ($row = $src_db->sql_fetchrow($result))
		{
			$row['user_id'] = (int) $row['user_id'];
			if (isset($colliding_user_ids[$row['user_id']]))
			{
				$colliding_users[$colliding_user_ids[$row['user_id']]][] = $row;
			}
		}
		$src_db->sql_freeresult($result);
		unset($colliding_user_ids);

		$list = '';
		foreach ($colliding_users as $username_clean => $users)
		{
			$list .= sprintf($user->lang['COLLIDING_CLEAN_USERNAME'], $username_clean) . "<br />\n";
			foreach ($users as $i => $row)
			{
				$list .= sprintf($user->lang['COLLIDING_USER'], $row['user_id'], phpbb_set_default_encoding($row['username']), $row['user_posts']) . "<br />\n";
			}
		}

		$lang['INST_ERR_FATAL'] = $user->lang['CONV_ERR_FATAL'];
		$convert->p_master->error('<span style="color:red">' . $user->lang['COLLIDING_USERNAMES_FOUND'] . '</span></b><br /><br />' . $list . '<b>', __LINE__, __FILE__);
	}

	$db->sql_query($drop_sql);
}

?>