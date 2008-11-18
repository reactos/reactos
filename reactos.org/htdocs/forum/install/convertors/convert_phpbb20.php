<?php
/**
*
* @package install
* @version $Id: convert_phpbb20.php 8492 2008-04-07 13:08:42Z acydburn $
* @copyright (c) 2006 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* NOTE to potential convertor authors. Please use this file to get
* familiar with the structure since we added some bare explanations here.
*
* Since this file gets included more than once on one page you are not able to add functions to it.
* Instead use a functions_ file.
*
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

include($phpbb_root_path . 'config.' . $phpEx);
unset($dbpasswd);

/**
* $convertor_data provides some basic information about this convertor which is
* used on the initial list of convertors and to populate the default settings
*/
$convertor_data = array(
	'forum_name'	=> 'phpBB 2.0.x',
	'version'		=> '1.0.1',
	'phpbb_version'	=> '3.0.1',
	'author'		=> '<a href="http://www.phpbb.com/">phpBB Group</a>',
	'dbms'			=> $dbms,
	'dbhost'		=> $dbhost,
	'dbport'		=> $dbport,
	'dbuser'		=> $dbuser,
	'dbpasswd'		=> '',
	'dbname'		=> $dbname,
	'table_prefix'	=> 'phpbb_',
	'forum_path'	=> '../forums',
	'author_notes'	=> '',
);

/**
* $tables is a list of the tables (minus prefix) which we expect to find in the
* source forum. It is used to guess the prefix if the specified prefix is incorrect
*/
$tables = array(
	'auth_access',
	'banlist',
	'categories',
	'disallow',
	'forum_prune',
	'forums',
	'groups',
	'posts',
	'posts_text',
	'privmsgs',
	'privmsgs_text',
	'ranks',
	'smilies',
	'topics',
	'topics_watch',
	'user_group',
	'users',
	'vote_desc',
	'vote_results',
	'vote_voters',
	'words'
);

/**
* $config_schema details how the board configuration information is stored in the source forum.
*
* 'table_format' can take the value 'file' to indicate a config file. In this case array_name
* is set to indicate the name of the array the config values are stored in
* 'table_format' can be an array if the values are stored in a table which is an assosciative array
* (as per phpBB 2.0.x)
* If left empty, values are assumed to be stored in a table where each config setting is
* a column (as per phpBB 1.x)
*
* In either of the latter cases 'table_name' indicates the name of the table in the database
*
* 'settings' is an array which maps the name of the config directive in the source forum
* to the config directive in phpBB3. It can either be a direct mapping or use a function.
* Please note that the contents of the old config value are passed to the function, therefore
* an in-built function requiring the variable passed by reference is not able to be used. Since
* empty() is such a function we created the function is_empty() to be used instead.
*/
$config_schema = array(
	'table_name'	=>	'config',
	'table_format'	=>	array('config_name' => 'config_value'),
	'settings'		=>	array(
		'allow_bbcode'			=> 'allow_bbcode',
		'allow_smilies'			=> 'allow_smilies',
		'allow_sig'				=> 'allow_sig',
		'allow_namechange'		=> 'allow_namechange',
		'allow_avatar_local'	=> 'allow_avatar_local',
		'allow_avatar_remote'	=> 'allow_avatar_remote',
		'allow_avatar_upload'	=> 'allow_avatar_upload',
		'board_disable'			=> 'board_disable',
		'sitename'				=> 'phpbb_set_encoding(sitename)',
		'site_desc'				=> 'phpbb_set_encoding(site_desc)',
		'session_length'		=> 'session_length',
		'board_email_sig'		=> 'phpbb_set_encoding(board_email_sig)',
		'posts_per_page'		=> 'posts_per_page',
		'topics_per_page'		=> 'topics_per_page',
		'enable_confirm'		=> 'enable_confirm',
		'board_email_form'		=> 'board_email_form',
		'override_user_style'	=> 'override_user_style',
		'hot_threshold'			=> 'hot_threshold',
		'max_poll_options'		=> 'max_poll_options',
		'max_sig_chars'			=> 'max_sig_chars',
		'pm_max_msgs'			=> 'max_inbox_privmsgs',
		'smtp_delivery'			=> 'smtp_delivery',
		'smtp_host'				=> 'smtp_host',
		'smtp_username'			=> 'smtp_username',
		'smtp_password'			=> 'smtp_password',
		'require_activation'	=> 'require_activation',
		'flood_interval'		=> 'flood_interval',
		'avatar_filesize'		=> 'avatar_filesize',
		'avatar_max_width'		=> 'avatar_max_width',
		'avatar_max_height'		=> 'avatar_max_height',
		'default_dateformat'	=> 'default_dateformat',
		'board_timezone'		=> 'board_timezone',
		'allow_privmsg'			=> 'not(privmsg_disable)',
		'gzip_compress'			=> 'gzip_compress',
		'coppa_enable'			=> 'is_empty(coppa_mail)',
		'coppa_fax'				=> 'coppa_fax',
		'coppa_mail'			=> 'coppa_mail',
		'record_online_users'	=> 'record_online_users',
		'record_online_date'	=> 'record_online_date',
		'board_startdate'		=> 'board_startdate',
	)
);

/**
* $test_file is the name of a file which is present on the source
* forum which can be used to check that the path specified by the
* user was correct
*/
$test_file = 'modcp.php';

/**
* If this is set then we are not generating the first page of information but getting the conversion information.
*/
if (!$get_info)
{
	// Test to see if the birthday MOD is installed on the source forum
	// Niels' birthday mod
	if (get_config_value('birthday_required') !== false || get_config_value('bday_require') !== false)
	{
		define('MOD_BIRTHDAY', true);
	}

	// TerraFrost's validated birthday mod
	if (get_config_value('bday_require') !== false)
	{
		define('MOD_BIRTHDAY_TERRA', true);
	}

	// Test to see if the attachment MOD is installed on the source forum
	// If it is, we will convert this data as well
	$src_db->sql_return_on_error(true);

	$sql = "SELECT config_value
		FROM {$convert->src_table_prefix}attachments_config
		WHERE config_name = 'upload_dir'";
	$result = $src_db->sql_query($sql);

	if ($result && $row = $src_db->sql_fetchrow($result))
	{
		// Here the constant is defined
		define('MOD_ATTACHMENT', true);

		// Here i add more tables to be checked in the old forum
		$tables += array(
			'attachments',
			'attachments_desc',
			'extensions',
			'extension_groups'
		);

		$src_db->sql_freeresult($result);
	}
	else if ($result)
	{
		$src_db->sql_freeresult($result);
	}
	

	/**
	* Tests for further MODs can be included here.
	* Please use constants for this, prefixing them with MOD_
	*/

	$src_db->sql_return_on_error(false);

	// Now let us set a temporary config variable for user id incrementing
	$sql = "SELECT user_id
		FROM {$convert->src_table_prefix}users
		WHERE user_id = 1";
	$result = $src_db->sql_query($sql);
	$user_id = (int) $src_db->sql_fetchfield('user_id');
	$src_db->sql_freeresult($result);

	// If there is a user id 1, we need to increment user ids. :/
	if ($user_id === 1)
	{
		// Try to get the maximum user id possible...
		$sql = "SELECT MAX(user_id) AS max_user_id
			FROM {$convert->src_table_prefix}users";
		$result = $src_db->sql_query($sql);
		$user_id = (int) $src_db->sql_fetchfield('max_user_id');
		$src_db->sql_freeresult($result);

		set_config('increment_user_id', ($user_id + 1), true);
	}
	else
	{
		set_config('increment_user_id', 0, true);
	}

	// Overwrite maximum avatar width/height
	@define('DEFAULT_AVATAR_X_CUSTOM', get_config_value('avatar_max_width'));
	@define('DEFAULT_AVATAR_Y_CUSTOM', get_config_value('avatar_max_height'));

/**
*	Description on how to use the convertor framework.
*
*	'schema' Syntax Description
*		-> 'target'			=> Target Table. If not specified the next table will be handled
*		-> 'primary'		=> Primary Key. If this is specified then this table is processed in batches
*		-> 'query_first'	=> array('target' or 'src', Query to execute before beginning the process
*								(if more than one then specified as array))
*		-> 'function_first'	=> Function to execute before beginning the process (if more than one then specified as array)
*								(This is mostly useful if variables need to be given to the converting process)
*		-> 'test_file'		=> This is not used at the moment but should be filled with a file from the old installation
*
*		// DB Functions
*		'distinct'	=> Add DISTINCT to the select query
*		'where'		=> Add WHERE to the select query
*		'group_by'	=> Add GROUP BY to the select query
*		'left_join'	=> Add LEFT JOIN to the select query (if more than one joins specified as array)
*		'having'	=> Add HAVING to the select query
*
*		// DB INSERT array
*		This one consist of three parameters
*		First Parameter:
*							The key need to be filled within the target table
*							If this is empty, the target table gets not assigned the source value
*		Second Parameter:
*							Source value. If the first parameter is specified, it will be assigned this value.
*							If the first parameter is empty, this only gets added to the select query
*		Third Parameter:
*							Custom Function. Function to execute while storing source value into target table.
*							The functions return value get stored.
*							The function parameter consist of the value of the second parameter.
*
*							types:
*								- empty string == execute nothing
*								- string == function to execute
*								- array == complex execution instructions
*		
*		Complex execution instructions:
*		@todo test complex execution instructions - in theory they will work fine
*
*							By defining an array as the third parameter you are able to define some statements to be executed. The key
*							is defining what to execute, numbers can be appended...
*
*							'function' => execute function
*							'execute' => run code, whereby all occurrences of {VALUE} get replaced by the last returned value.
*										The result *must* be assigned/stored to {RESULT}.
*							'typecast'	=> typecast value
*
*							The returned variables will be made always available to the next function to continue to work with.
*
*							example (variable inputted is an integer of 1):
*
*							array(
*								'function1'		=> 'increment_by_one',		// returned variable is 2
*								'typecast'		=> 'string',				// typecast variable to be a string
*								'execute'		=> '{RESULT} = {VALUE} . ' is good';', // returned variable is '2 is good'
*								'function2'		=> 'replace_good_with_bad',				// returned variable is '2 is bad'
*							),
*
*/

	$convertor = array(
		'test_file'				=> 'viewtopic.php',

		'avatar_path'			=> get_config_value('avatar_path') . '/',
		'avatar_gallery_path'	=> get_config_value('avatar_gallery_path') . '/',
		'smilies_path'			=> get_config_value('smilies_path') . '/',
		'upload_path'			=> (defined('MOD_ATTACHMENT')) ? phpbb_get_files_dir() . '/' : '',
		'thumbnails'			=> (defined('MOD_ATTACHMENT')) ? array('thumbs/', 't_') : '',
		'ranks_path'			=> false, // phpBB 2.0.x had no config value for a ranks path

		// We empty some tables to have clean data available
		'query_first'			=> array(
			array('target', $convert->truncate_statement . SEARCH_RESULTS_TABLE),
			array('target', $convert->truncate_statement . SEARCH_WORDLIST_TABLE),
			array('target', $convert->truncate_statement . SEARCH_WORDMATCH_TABLE),
			array('target', $convert->truncate_statement . LOG_TABLE),
		),
		
//	with this you are able to import all attachment files on the fly. For large boards this is not an option, therefore commented out by default.
//	Instead every file gets copied while processing the corresponding attachment entry.
//		if (defined("MOD_ATTACHMENT")) { import_attachment_files(); phpbb_copy_thumbnails(); }

		// phpBB2 allowed some similar usernames to coexist which would have the same
		// username_clean in phpBB3 which is not possible, so we'll give the admin a list
		// of user ids and usernames and let him deicde what he wants to do with them
		'execute_first'	=> '
			phpbb_check_username_collisions();
			import_avatar_gallery();
			if (defined("MOD_ATTACHMENT")) phpbb_import_attach_config();
			phpbb_insert_forums();
		',

		'execute_last'	=> array('
			add_bots();
		', '
			update_folder_pm_count();
		', '
			update_unread_count();
		', '
			phpbb_convert_authentication(\'start\');
		', '
			phpbb_convert_authentication(\'first\');
		', '
			phpbb_convert_authentication(\'second\');
		', '
			phpbb_convert_authentication(\'third\');
		'),

		'schema' => array(

			array(
				'target'		=> (defined('MOD_ATTACHMENT')) ? ATTACHMENTS_TABLE : '',
				'primary'		=> 'attachments.attach_id',
				'query_first'	=> (defined('MOD_ATTACHMENT')) ? array('target', $convert->truncate_statement . ATTACHMENTS_TABLE) : '',
				'autoincrement'	=> 'attach_id',

				array('attach_id',				'attachments.attach_id',				''),
				array('post_msg_id',			'attachments.post_id',					''),
				array('topic_id',				'posts.topic_id',						''),
				array('in_message',				0,										''),
				array('is_orphan',				0,										''),
				array('poster_id',				'attachments.user_id_1 AS poster_id',	'phpbb_user_id'),
				array('physical_filename',		'attachments_desc.physical_filename',	'import_attachment'),
				array('real_filename',			'attachments_desc.real_filename',		'phpbb_set_encoding'),
				array('download_count',			'attachments_desc.download_count',		''),
				array('attach_comment',			'attachments_desc.comment',				array('function1' => 'phpbb_set_encoding', 'function2' => 'utf8_htmlspecialchars')),
				array('extension',				'attachments_desc.extension',			''),
				array('mimetype',				'attachments_desc.mimetype',			''),
				array('filesize',				'attachments_desc.filesize',			''),
				array('filetime',				'attachments_desc.filetime',			''),
				array('thumbnail',				'attachments_desc.thumbnail',			''),

				'where'			=> 'attachments_desc.attach_id = attachments.attach_id AND attachments.privmsgs_id = 0 AND posts.post_id = attachments.post_id',
				'group_by'		=> 'attachments.attach_id'
			),

			array(
				'target'		=> (defined('MOD_ATTACHMENT')) ? ATTACHMENTS_TABLE : '',
				'primary'		=> 'attachments.attach_id',
				'autoincrement'	=> 'attach_id',

				array('attach_id',				'attachments.attach_id',				''),
				array('post_msg_id',			'attachments.privmsgs_id',				''),
				array('topic_id',				0,										''),
				array('in_message',				1,										''),
				array('is_orphan',				0,										''),
				array('poster_id',				'attachments.user_id_1 AS poster_id',	'phpbb_user_id'),
				array('physical_filename',		'attachments_desc.physical_filename',	'import_attachment'),
				array('real_filename',			'attachments_desc.real_filename',		''),
				array('download_count',			'attachments_desc.download_count',		''),
				array('attach_comment',			'attachments_desc.comment',				array('function1' => 'phpbb_set_encoding', 'function2' => 'utf8_htmlspecialchars')),
				array('extension',				'attachments_desc.extension',			''),
				array('mimetype',				'attachments_desc.mimetype',			''),
				array('filesize',				'attachments_desc.filesize',			''),
				array('filetime',				'attachments_desc.filetime',			''),
				array('thumbnail',				'attachments_desc.thumbnail',			''),

				'where'			=> 'attachments_desc.attach_id = attachments.attach_id AND attachments.post_id = 0',
				'group_by'		=> 'attachments.attach_id'
			),

			array(
				'target'		=> (defined('MOD_ATTACHMENT')) ? EXTENSIONS_TABLE : '',
				'query_first'	=> (defined('MOD_ATTACHMENT')) ? array('target', $convert->truncate_statement . EXTENSIONS_TABLE) : '',
				'autoincrement'	=> 'extension_id',

				array('extension_id',			'extensions.ext_id',				''),
				array('group_id',				'extensions.group_id',				''),
				array('extension',				'extensions.extension',				''),
			),

			array(
				'target'		=> (defined('MOD_ATTACHMENT')) ? EXTENSION_GROUPS_TABLE : '',
				'query_first'	=> (defined('MOD_ATTACHMENT')) ? array('target', $convert->truncate_statement . EXTENSION_GROUPS_TABLE) : '',
				'autoincrement'	=> 'group_id',

				array('group_id',				'extension_groups.group_id',			''),
				array('group_name',				'extension_groups.group_name',			array('function1' => 'phpbb_set_encoding', 'function2' => 'utf8_htmlspecialchars')),
				array('cat_id',					'extension_groups.cat_id',				'phpbb_attachment_category'),
				array('allow_group',			'extension_groups.allow_group',			''),
				array('download_mode',			1,										''),
				array('upload_icon',			'',										''),
				array('max_filesize',			'extension_groups.max_filesize',		''),
				array('allowed_forums',			'extension_groups.forum_permissions',	'phpbb_attachment_forum_perms'),
				array('allow_in_pm',			1,										''),
			),

			array(
				'target'		=> BANLIST_TABLE,
				'query_first'	=> array('target', $convert->truncate_statement . BANLIST_TABLE),

				array('ban_ip',					'banlist.ban_ip',					'decode_ban_ip'),
				array('ban_userid',				'banlist.ban_userid',				'phpbb_user_id'),
				array('ban_email',				'banlist.ban_email',				''),
				array('ban_reason',				'',									''),
				array('ban_give_reason',		'',									''),

				'where'			=> "banlist.ban_ip NOT LIKE '%.%'",
			),

			array(
				'target'		=> BANLIST_TABLE,

				array('ban_ip',					'banlist.ban_ip',	''),
				array('ban_userid',				0,					''),
				array('ban_email',				'',					''),
				array('ban_reason',				'',					''),
				array('ban_give_reason',		'',					''),

				'where'			=> "banlist.ban_ip LIKE '%.%'",
			),

			array(
				'target'		=> DISALLOW_TABLE,
				'query_first'	=> array('target', $convert->truncate_statement . DISALLOW_TABLE),

				array('disallow_username',		'disallow.disallow_username',				'phpbb_disallowed_username'),
			),

			array(
				'target'		=> RANKS_TABLE,
				'query_first'	=> array('target', $convert->truncate_statement . RANKS_TABLE),
				'autoincrement'	=> 'rank_id',

				array('rank_id',					'ranks.rank_id',				''),
				array('rank_title',					'ranks.rank_title',				array('function1' => 'phpbb_set_default_encoding', 'function2' => 'utf8_htmlspecialchars')),
				array('rank_min',					'ranks.rank_min',				array('typecast' => 'int', 'execute' => '{RESULT} = ({VALUE}[0] < 0) ? 0 : {VALUE}[0];')),
				array('rank_special',				'ranks.rank_special',			''),
				array('rank_image',					'ranks.rank_image',				'import_rank'),
			),

			array(
				'target'		=> TOPICS_TABLE,
				'query_first'	=> array('target', $convert->truncate_statement . TOPICS_TABLE),
				'primary'		=> 'topics.topic_id',
				'autoincrement'	=> 'topic_id',

				array('topic_id',				'topics.topic_id',					''),
				array('forum_id',				'topics.forum_id',					''),
				array('icon_id',				0,									''),
				array('topic_poster',			'topics.topic_poster AS poster_id',	'phpbb_user_id'),
				array('topic_attachment',		((defined('MOD_ATTACHMENT')) ? 'topics.topic_attachment' : 0), ''),
				array('topic_title',			'topics.topic_title',				'phpbb_set_encoding'),
				array('topic_time',				'topics.topic_time',				''),
				array('topic_views',			'topics.topic_views',				''),
				array('topic_replies',			'topics.topic_replies',				''),
				array('topic_replies_real',		'topics.topic_replies',				''),
				array('topic_last_post_id',		'topics.topic_last_post_id',		''),
				array('topic_status',			'topics.topic_status',				'is_topic_locked'),
				array('topic_moved_id',			0,									''),
				array('topic_type',				'topics.topic_type',				'phpbb_convert_topic_type'),
				array('topic_first_post_id',	'topics.topic_first_post_id',		''),
				array('topic_last_view_time',	'posts.post_time',					''),
				array('poll_title',				'vote_desc.vote_text',				array('function1' => 'null_to_str', 'function2' => 'phpbb_set_encoding', 'function3' => 'utf8_htmlspecialchars')),
				array('poll_start',				'vote_desc.vote_start',				'null_to_zero'),
				array('poll_length',			'vote_desc.vote_length',			'null_to_zero'),
				array('poll_max_options',		1,									''),
				array('poll_vote_change',		0,									''),

				'left_join'		=>	array (	'topics LEFT JOIN vote_desc ON topics.topic_id = vote_desc.topic_id AND topics.topic_vote = 1', 
											'topics LEFT JOIN posts ON topics.topic_last_post_id = posts.post_id',
									),
				'where'			=> 'topics.topic_moved_id = 0',
			),

			array(
				'target'		=> TOPICS_TABLE,
				'primary'		=> 'topics.topic_id',
				'autoincrement'	=> 'topic_id',

				array('topic_id',				'topics.topic_id',					''),
				array('forum_id',				'topics.forum_id',					''),
				array('icon_id',				0,									''),
				array('topic_poster',			'topics.topic_poster AS poster_id',	'phpbb_user_id'),
				array('topic_attachment',		((defined('MOD_ATTACHMENT')) ? 'topics.topic_attachment' : 0), ''),
				array('topic_title',			'topics.topic_title',				'phpbb_set_encoding'),
				array('topic_time',				'topics.topic_time',				''),
				array('topic_views',			'topics.topic_views',				''),
				array('topic_replies',			'topics.topic_replies',				''),
				array('topic_replies_real',		'topics.topic_replies',				''),
				array('topic_last_post_id',		'topics.topic_last_post_id',		''),
				array('topic_status',			ITEM_MOVED,							''),
				array('topic_moved_id',			'topics.topic_moved_id',			''),
				array('topic_type',				'topics.topic_type',				'phpbb_convert_topic_type'),
				array('topic_first_post_id',	'topics.topic_first_post_id',		''),

				array('poll_title',				'vote_desc.vote_text',				array('function1' => 'null_to_str', 'function2' => 'phpbb_set_encoding', 'function3' => 'utf8_htmlspecialchars')),
				array('poll_start',				'vote_desc.vote_start',				'null_to_zero'),
				array('poll_length',			'vote_desc.vote_length',			'null_to_zero'),
				array('poll_max_options',		1,									''),
				array('poll_vote_change',		0,									''),

				'left_join'		=> 'topics LEFT JOIN vote_desc ON topics.topic_id = vote_desc.topic_id AND topics.topic_vote = 1',
				'where'			=> 'topics.topic_moved_id <> 0',
			),

			array(
				'target'		=> TOPICS_WATCH_TABLE,
				'primary'		=> 'topics_watch.topic_id',
				'query_first'	=> array('target', $convert->truncate_statement . TOPICS_WATCH_TABLE),

				array('topic_id',				'topics_watch.topic_id',			''),
				array('user_id',				'topics_watch.user_id',				'phpbb_user_id'),
				array('notify_status',			'topics_watch.notify_status',		''),
			),

			array(
				'target'		=> SMILIES_TABLE,
				'query_first'	=> array('target', $convert->truncate_statement . SMILIES_TABLE),
				'autoincrement'	=> 'smiley_id',

				array('smiley_id',				'smilies.smilies_id',				''),
				array('code',					'smilies.code',						array('function1' => 'phpbb_smilie_html_decode', 'function2' => 'phpbb_set_encoding', 'function3' => 'utf8_htmlspecialchars')),
				array('emotion',				'smilies.emoticon',					'phpbb_set_encoding'),
				array('smiley_url',				'smilies.smile_url',				'import_smiley'),
				array('smiley_width',			'smilies.smile_url',				'get_smiley_width'),
				array('smiley_height',			'smilies.smile_url',				'get_smiley_height'),
				array('smiley_order',			'smilies.smilies_id',				''),
				array('display_on_posting',		'smilies.smilies_id',				'get_smiley_display'),

				'order_by'		=> 'smilies.smilies_id ASC',
			),

			array(
				'target'		=> POLL_OPTIONS_TABLE,
				'primary'		=> 'vote_results.vote_option_id',
				'query_first'	=> array('target', $convert->truncate_statement . POLL_OPTIONS_TABLE),

				array('poll_option_id',			'vote_results.vote_option_id',		''),
				array('topic_id',				'vote_desc.topic_id',				''),
				array('',						'topics.topic_poster AS poster_id',	'phpbb_user_id'),
				array('poll_option_text',		'vote_results.vote_option_text',	array('function1' => 'phpbb_set_encoding', 'function2' => 'utf8_htmlspecialchars')),
				array('poll_option_total',		'vote_results.vote_result',			''),

				'where'			=> 'vote_results.vote_id = vote_desc.vote_id',
				'left_join'		=> 'vote_desc LEFT JOIN topics ON topics.topic_id = vote_desc.topic_id',
			),

			array(
				'target'		=> POLL_VOTES_TABLE,
				'primary'		=> 'vote_desc.topic_id',
				'query_first'	=> array('target', $convert->truncate_statement . POLL_VOTES_TABLE),

				array('poll_option_id',			VOTE_CONVERTED,						''),
				array('topic_id',				'vote_desc.topic_id',				''),
				array('vote_user_id',			'vote_voters.vote_user_id',			'phpbb_user_id'),
				array('vote_user_ip',			'vote_voters.vote_user_ip',			'decode_ip'),

				'where'			=> 'vote_voters.vote_id = vote_desc.vote_id',
			),

			array(
				'target'		=> WORDS_TABLE,
				'primary'		=> 'words.word_id',
				'query_first'	=> array('target', $convert->truncate_statement . WORDS_TABLE),
				'autoincrement'	=> 'word_id',

				array('word_id',				'words.word_id',					''),
				array('word',					'words.word',						'phpbb_set_encoding'),
				array('replacement',			'words.replacement',				'phpbb_set_encoding'),
			),

			array(
				'target'		=> POSTS_TABLE,
				'primary'		=> 'posts.post_id',
				'autoincrement'	=> 'post_id',
				'query_first'	=> array('target', $convert->truncate_statement . POSTS_TABLE),
				'execute_first'	=> '
					$config["max_post_chars"] = 0;
					$config["max_quote_depth"] = 0;
				',

				array('post_id',				'posts.post_id',					''),
				array('topic_id',				'posts.topic_id',					''),
				array('forum_id',				'posts.forum_id',					''),
				array('poster_id',				'posts.poster_id',					'phpbb_user_id'),
				array('icon_id',				0,									''),
				array('poster_ip',				'posts.poster_ip',					'decode_ip'),
				array('post_time',				'posts.post_time',					''),
				array('enable_bbcode',			'posts.enable_bbcode',				''),
				array('',						'posts.enable_html',				''),
				array('enable_smilies',			'posts.enable_smilies',				''),
				array('enable_sig',				'posts.enable_sig',					''),
				array('enable_magic_url',		1,									''),
				array('post_username',			'posts.post_username',				'phpbb_set_encoding'),
				array('post_subject',			'posts_text.post_subject',			'phpbb_set_encoding'),
				array('post_attachment',		((defined('MOD_ATTACHMENT')) ? 'posts.post_attachment' : 0), ''),
				array('post_edit_time',			'posts.post_edit_time',				array('typecast' => 'int')),
				array('post_edit_count',		'posts.post_edit_count',			''),
				array('post_edit_reason',		'',									''),
				array('post_edit_user',			'',									'phpbb_post_edit_user'),

				array('bbcode_uid',				'posts.post_time',					'make_uid'),
				array('post_text',				'posts_text.post_text',				'phpbb_prepare_message'),
				array('',						'posts_text.bbcode_uid AS old_bbcode_uid',			''),
				array('bbcode_bitfield',		'',									'get_bbcode_bitfield'),
				array('post_checksum',			'',									''),

				// Commented out inline search indexing, this takes up a LOT of time. :D
				// @todo We either need to enable this or call the rebuild search functionality post convert
/*				array('',						'',									'search_indexing'),
				array('',						'posts_text.post_text AS message',	''),
				array('',						'posts_text.post_subject AS title',	''),*/

				'where'			=>	'posts.post_id = posts_text.post_id'
			),

			array(
				'target'		=> PRIVMSGS_TABLE,
				'primary'		=> 'privmsgs.privmsgs_id',
				'autoincrement'	=> 'msg_id',
				'query_first'	=> array(
					array('target', $convert->truncate_statement . PRIVMSGS_TABLE),
					array('target', $convert->truncate_statement . PRIVMSGS_RULES_TABLE),
				),

				'execute_first'	=> '
					$config["max_post_chars"] = 0;
					$config["max_quote_depth"] = 0;
				',

				array('msg_id',					'privmsgs.privmsgs_id',				''),
				array('root_level',				0,									''),
				array('author_id',				'privmsgs.privmsgs_from_userid AS poster_id',	'phpbb_user_id'),
				array('icon_id',				0,									''),
				array('author_ip',				'privmsgs.privmsgs_ip',				'decode_ip'),
				array('message_time',			'privmsgs.privmsgs_date',			''),
				array('enable_bbcode',			'privmsgs.privmsgs_enable_bbcode AS enable_bbcode',	''),
				array('',						'privmsgs.privmsgs_enable_html AS enable_html',	''),
				array('enable_smilies',			'privmsgs.privmsgs_enable_smilies AS enable_smilies',	''),
				array('enable_magic_url',		1,									''),
				array('enable_sig',				'privmsgs.privmsgs_attach_sig',		''),
				array('message_subject',		'privmsgs.privmsgs_subject',		'phpbb_set_encoding'), // Already specialchared in 2.0.x
				array('message_attachment',		((defined('MOD_ATTACHMENT')) ? 'privmsgs.privmsgs_attachment' : 0), ''),
				array('message_edit_reason',	'',									''),
				array('message_edit_user',		0,									''),
				array('message_edit_time',		0,									''),
				array('message_edit_count',		0,									''),

				array('bbcode_uid',				'privmsgs.privmsgs_date AS post_time',	'make_uid'),
				array('message_text',			'privmsgs_text.privmsgs_text',			'phpbb_prepare_message'),
				array('',						'privmsgs_text.privmsgs_bbcode_uid AS old_bbcode_uid',			''),
				array('bbcode_bitfield',		'',										'get_bbcode_bitfield'),
				array('to_address',				'privmsgs.privmsgs_to_userid',			'phpbb_privmsgs_to_userid'),
				array('bcc_address',			'',										''),

				'where'			=>	'privmsgs.privmsgs_id = privmsgs_text.privmsgs_text_id'
			),

			array(
				'target'		=> PRIVMSGS_FOLDER_TABLE,
				'primary'		=> 'users.user_id',
				'query_first'	=> array('target', $convert->truncate_statement . PRIVMSGS_FOLDER_TABLE),

				array('user_id',				'users.user_id',						'phpbb_user_id'),
				array('folder_name',			$user->lang['CONV_SAVED_MESSAGES'],		''),
				array('pm_count',				0,										''),
			
				'where'			=> 'users.user_id <> -1',
			),

			// Inbox
			array(
				'target'		=> PRIVMSGS_TO_TABLE,
				'primary'		=> 'privmsgs.privmsgs_id',
				'query_first'	=> array('target', $convert->truncate_statement . PRIVMSGS_TO_TABLE),

				array('msg_id',					'privmsgs.privmsgs_id',					''),
				array('user_id',				'privmsgs.privmsgs_to_userid',			'phpbb_user_id'),
				array('author_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('pm_deleted',				0,										''),
				array('pm_new',					'privmsgs.privmsgs_type',				'phpbb_new_pm'),
				array('pm_unread',				'privmsgs.privmsgs_type',				'phpbb_unread_pm'),
				array('pm_replied',				0,										''),
				array('pm_marked',				0,										''),
				array('pm_forwarded',			0,										''),
				array('folder_id',				PRIVMSGS_INBOX,							''),

				'where'			=> 'privmsgs.privmsgs_id = privmsgs_text.privmsgs_text_id
										AND (privmsgs.privmsgs_type = 0 OR privmsgs.privmsgs_type = 1 OR privmsgs.privmsgs_type = 5)',
			),
			
			// Outbox
			array(
				'target'		=> PRIVMSGS_TO_TABLE,
				'primary'		=> 'privmsgs.privmsgs_id',

				array('msg_id',					'privmsgs.privmsgs_id',					''),
				array('user_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('author_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('pm_deleted',				0,										''),
				array('pm_new',					0,										''),
				array('pm_unread',				0,										''),
				array('pm_replied',				0,										''),
				array('pm_marked',				0,										''),
				array('pm_forwarded',			0,										''),
				array('folder_id',				PRIVMSGS_OUTBOX,						''),

				'where'			=> 'privmsgs.privmsgs_id = privmsgs_text.privmsgs_text_id
										AND (privmsgs.privmsgs_type = 1 OR privmsgs.privmsgs_type = 5)',
			),

			// Sentbox
			array(
				'target'		=> PRIVMSGS_TO_TABLE,
				'primary'		=> 'privmsgs.privmsgs_id',

				array('msg_id',					'privmsgs.privmsgs_id',					''),
				array('user_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('author_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('pm_deleted',				0,										''),
				array('pm_new',					'privmsgs.privmsgs_type',				'phpbb_new_pm'),
				array('pm_unread',				'privmsgs.privmsgs_type',				'phpbb_unread_pm'),
				array('pm_replied',				0,										''),
				array('pm_marked',				0,										''),
				array('pm_forwarded',			0,										''),
				array('folder_id',				PRIVMSGS_SENTBOX,						''),

				'where'			=> 'privmsgs.privmsgs_id = privmsgs_text.privmsgs_text_id
										AND privmsgs.privmsgs_type = 2',
			),

			// Savebox (SAVED IN)
			array(
				'target'		=> PRIVMSGS_TO_TABLE,
				'primary'		=> 'privmsgs.privmsgs_id',

				array('msg_id',					'privmsgs.privmsgs_id',					''),
				array('user_id',				'privmsgs.privmsgs_to_userid',			'phpbb_user_id'),
				array('author_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('pm_deleted',				0,										''),
				array('pm_new',					'privmsgs.privmsgs_type',				'phpbb_new_pm'),
				array('pm_unread',				'privmsgs.privmsgs_type',				'phpbb_unread_pm'),
				array('pm_replied',				0,										''),
				array('pm_marked',				0,										''),
				array('pm_forwarded',			0,										''),
				array('folder_id',				'privmsgs.privmsgs_to_userid',			'phpbb_get_savebox_id'),

				'where'			=> 'privmsgs.privmsgs_id = privmsgs_text.privmsgs_text_id
										AND privmsgs.privmsgs_type = 3',
			),

			// Savebox (SAVED OUT)
			array(
				'target'		=> PRIVMSGS_TO_TABLE,
				'primary'		=> 'privmsgs.privmsgs_id',

				array('msg_id',					'privmsgs.privmsgs_id',					''),
				array('user_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('author_id',				'privmsgs.privmsgs_from_userid',		'phpbb_user_id'),
				array('pm_deleted',				0,										''),
				array('pm_new',					'privmsgs.privmsgs_type',				'phpbb_new_pm'),
				array('pm_unread',				'privmsgs.privmsgs_type',				'phpbb_unread_pm'),
				array('pm_replied',				0,										''),
				array('pm_marked',				0,										''),
				array('pm_forwarded',			0,										''),
				array('folder_id',				'privmsgs.privmsgs_from_userid',		'phpbb_get_savebox_id'),

				'where'			=> 'privmsgs.privmsgs_id = privmsgs_text.privmsgs_text_id
										AND privmsgs.privmsgs_type = 4',
			),

			array(
				'target'		=> GROUPS_TABLE,
				'autoincrement'	=> 'group_id',
				'query_first'	=> array('target', $convert->truncate_statement . GROUPS_TABLE),

				array('group_id',				'groups.group_id',					''),
				array('group_type',				'groups.group_type',				'phpbb_convert_group_type'),
				array('group_display',			0,									''),
				array('group_legend',			0,									''),
				array('group_name',				'groups.group_name',				'phpbb_convert_group_name'), // phpbb_set_encoding called in phpbb_convert_group_name
				array('group_desc',				'groups.group_description',			'phpbb_set_encoding'),

				'where'			=> 'groups.group_single_user = 0',
			),

			array(
				'target'		=> USER_GROUP_TABLE,
				'query_first'	=> array('target', $convert->truncate_statement . USER_GROUP_TABLE),
				'execute_first'	=> '
					add_default_groups();
				',

				array('group_id',		'groups.group_id',					''),
				array('user_id',		'groups.group_moderator',			'phpbb_user_id'),
				array('group_leader',	1,									''),
				array('user_pending',	0,									''),

				'where'			=> 'groups.group_single_user = 0 AND groups.group_moderator <> 0',
			),

			array(
				'target'		=> USER_GROUP_TABLE,

				array('group_id',		'user_group.group_id',				''),
				array('user_id',		'user_group.user_id',				'phpbb_user_id'),
				array('group_leader',	0,									''),
				array('user_pending',	'user_group.user_pending',			''),

				'where'			=> 'user_group.group_id = groups.group_id AND groups.group_single_user = 0 AND groups.group_moderator <> user_group.user_id',
			),

			array(
				'target'		=> USERS_TABLE,
				'primary'		=> 'users.user_id',
				'autoincrement'	=> 'user_id',
				'query_first'	=> array(
					array('target', 'DELETE FROM ' . USERS_TABLE . ' WHERE user_id <> ' . ANONYMOUS),
					array('target', $convert->truncate_statement . BOTS_TABLE)
				),

				'execute_last'	=> '
					remove_invalid_users();
				',

				array('user_id',				'users.user_id',					'phpbb_user_id'),
				array('',						'users.user_id AS poster_id',		'phpbb_user_id'),
				array('user_type',				'users.user_active',				'set_user_type'),
				array('group_id',				'users.user_level',					'phpbb_set_primary_group'),
				array('user_regdate',			'users.user_regdate',				''),
				array('username',				'users.username',					'phpbb_set_default_encoding'), // recode to utf8 with default lang
				array('username_clean',			'users.username',					array('function1' => 'phpbb_set_default_encoding', 'function2' => 'utf8_clean_string')),
				array('user_password',			'users.user_password',				''),
				array('user_pass_convert',		1,									''),
				array('user_posts',				'users.user_posts',					'intval'),
				array('user_email',				'users.user_email',					'strtolower'),
				array('user_email_hash',		'users.user_email',					'gen_email_hash'),
				array('user_birthday',			((defined('MOD_BIRTHDAY')) ? 'users.user_birthday' : ''),	'phpbb_get_birthday'),
				array('user_lastvisit',			'users.user_lastvisit',				'intval'),
				array('user_lastmark',			'users.user_lastvisit',				'intval'),
				array('user_lang',				$config['default_lang'],			''),
				array('',						'users.user_lang',					''),
				array('user_timezone',			'users.user_timezone',				'floatval'),
				array('user_dateformat',		'users.user_dateformat',			array('function1' => 'phpbb_set_encoding', 'function2' => 'fill_dateformat')),
				array('user_inactive_reason',	'',									'phpbb_inactive_reason'),
				array('user_inactive_time',		'',									'phpbb_inactive_time'),

				array('user_interests',			'users.user_interests',				array('function1' => 'phpbb_set_encoding')),
				array('user_occ',				'users.user_occ',					array('function1' => 'phpbb_set_encoding')),
				array('user_website',			'users.user_website',				'validate_website'),
				array('user_jabber',			'',									''),
				array('user_msnm',				'users.user_msnm',					array('function1' => 'phpbb_set_encoding')),
				array('user_yim',				'users.user_yim',					array('function1' => 'phpbb_set_encoding')),
				array('user_aim',				'users.user_aim',					array('function1' => 'phpbb_set_encoding')),
				array('user_icq',				'users.user_icq',					array('function1' => 'phpbb_set_encoding')),
				array('user_from',				'users.user_from',					array('function1' => 'phpbb_set_encoding')),
				array('user_rank',				'users.user_rank',					'intval'),
				array('user_permissions',		'',									''),

				array('user_avatar',			'users.user_avatar',				'phpbb_import_avatar'),
				array('user_avatar_type',		'users.user_avatar_type',			'phpbb_avatar_type'),
				array('user_avatar_width',		'users.user_avatar',				'phpbb_get_avatar_width'),
				array('user_avatar_height',		'users.user_avatar',				'phpbb_get_avatar_height'),

				array('user_new_privmsg',		'users.user_new_privmsg',			''),
				array('user_unread_privmsg',	0,									''), //'users.user_unread_privmsg'
				array('user_last_privmsg',		'users.user_last_privmsg',			'intval'),
				array('user_emailtime',			'users.user_emailtime',				'null_to_zero'),
				array('user_notify',			'users.user_notify',				'intval'),
				array('user_notify_pm',			'users.user_notify_pm',				'intval'),
				array('user_notify_type',		NOTIFY_EMAIL,						''),
				array('user_allow_pm',			'users.user_allow_pm',				'intval'),
				array('user_allow_viewonline',	'users.user_allow_viewonline',		'intval'),
				array('user_allow_viewemail',	'users.user_viewemail',				'intval'),
				array('user_actkey',			'users.user_actkey',				''),
				array('user_newpasswd',			'',									''), // Users need to re-request their password...
				array('user_style',				$config['default_style'],			''),

				array('user_options',			'',									'set_user_options'),
				array('',						'users.user_popup_pm AS popuppm',	''),
				array('',						'users.user_allowhtml AS html',		''),
				array('',						'users.user_allowbbcode AS bbcode',	''),
				array('',						'users.user_allowsmile AS smile',	''),
				array('',						'users.user_attachsig AS attachsig',''),

				array('user_sig_bbcode_uid',		'users.user_regdate',							'make_uid'),
				array('user_sig',					'users.user_sig',								'phpbb_prepare_message'),
				array('',							'users.user_sig_bbcode_uid AS old_bbcode_uid',	''),
				array('user_sig_bbcode_bitfield',	'',												'get_bbcode_bitfield'),
				array('',							'users.user_regdate AS post_time',				''),

				'where'			=> 'users.user_id <> -1',
			),
		),
	);
}

?>