<?php
/**
*
* @package install
* @version $Id: functions_convert.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2006 phpBB Group
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
* Default avatar width/height
* @ignore
*/
define('DEFAULT_AVATAR_X', 80);
define('DEFAULT_AVATAR_Y', 80);

// Global functions - all functions can be used by convertors

// SIMPLE FUNCTIONS

/**
* Return the preceding value
*/
function dec($var)
{
	return --$var;
}

/**
* Return the next value
*/
function inc($var)
{
	return ++$var;
}

/**
* Return whether the value is positive
*/
function is_positive($n)
{
	return ($n > 0) ? 1 : 0;
}

/**
* Boolean inverse of the value
*/
function not($var)
{
	return ($var) ? 0 : 1;
}

/**
* Convert a textual value to it's equivalent boolean value
*
* @param string $str String to convert (converts yes, on, y, 1 and true to boolean true)
* @return boolean The equivalent value
*/
function str_to_bool($str)
{
	$str = strtolower($str);
	return ($str == 'yes' || $str == 'on' || $str == 'y' || $str == 'true' || $str == '1') ? true : false;
}

/**
* Function to mimic php's empty() function (it is the same)
*/
function is_empty($mixed)
{
	return empty($mixed);
}

/**
* Convert the name of a user's primary group to the appropriate equivalent phpBB group id
*
* @param string $status The name of the group
* @return int The group_id corresponding to the equivalent group
*/
function str_to_primary_group($status)
{
	switch (ucfirst(strtolower($status)))
	{
		case 'Administrator':
			return get_group_id('administrators');
		break;

		case 'Super moderator':
		case 'Global moderator':
		case 'Moderator':
			return get_group_id('global_moderators');
		break;

		case 'Guest':
		case 'Anonymous':
			return get_group_id('guests');
		break;

		default:
			return get_group_id('registered');
		break;
	}
}

/**
* Convert a boolean into the appropriate phpBB constant indicating whether the item is locked
*/
function is_item_locked($bool)
{
	return ($bool) ? ITEM_LOCKED : ITEM_UNLOCKED;
}

/**
* Convert a value from days to seconds
*/
function days_to_seconds($days)
{
	return ($days * 86400);
}

/**
* Determine whether a user is anonymous and return the appropriate new user_id
*/
function is_user_anonymous($user_id)
{
	return ($user_id > ANONYMOUS) ? $user_id : ANONYMOUS;
}

/**
* Generate a key value based on existing values
*
* @param int $pad Amount to add to the maximum value
* @return int Key value
*/
function auto_id($pad = 0)
{
	global $auto_id, $convert_row;

	if (!empty($convert_row['max_id']))
	{
		return $convert_row['max_id'] + $pad;
	}
	
	return $auto_id + $pad;
}

/**
* Convert a boolean into the appropriate phpBB constant indicating whether the user is active
*/
function set_user_type($user_active)
{
	return ($user_active) ? USER_NORMAL : USER_INACTIVE;
}

/**
* Convert a value from minutes to hours
*/
function minutes_to_hours($minutes)
{
	return ($minutes / 3600);
}

/**
* Return the group_id for a given group name
*/
function get_group_id($group_name)
{
	global $db, $group_mapping;

	if (empty($group_mapping))
	{
		$sql = 'SELECT group_name, group_id
			FROM ' . GROUPS_TABLE;
		$result = $db->sql_query($sql);

		$group_mapping = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$group_mapping[strtoupper($row['group_name'])] = (int) $row['group_id'];
		}
		$db->sql_freeresult($result);
	}

	if (!sizeof($group_mapping))
	{
		add_default_groups();
		return get_group_id($group_name);
	}

	if (isset($group_mapping[strtoupper($group_name)]))
	{
		return $group_mapping[strtoupper($group_name)];
	}

	return $group_mapping['REGISTERED'];
}

/**
* Generate the email hash stored in the users table
*/
function gen_email_hash($email)
{
	return (crc32(strtolower($email)) . strlen($email));
}

/**
* Convert a boolean into the appropriate phpBB constant indicating whether the topic is locked
*/
function is_topic_locked($bool)
{
	return (!empty($bool)) ? ITEM_LOCKED : ITEM_UNLOCKED;
}

/**
* Generate a bbcode_uid value
*/
function make_uid($timestamp)
{
	static $last_timestamp, $last_uid;

	if (empty($last_timestamp) || $timestamp != $last_timestamp)
	{
		$last_uid = substr(base_convert(unique_id(), 16, 36), 0, BBCODE_UID_LEN);
	}
	$last_timestamp = $timestamp;
	return $last_uid;
}


/**
* Validate a website address
*/
function validate_website($url)
{
	if ($url === 'http://')
	{
		return '';
	}
	else if (!preg_match('#^[a-z0-9]+://#i', $url) && strlen($url) > 0)
	{
		return 'http://' . $url;
	}
	return $url;
}

/**
* Convert nulls to zeros for fields which allowed a NULL value in the source but not the destination
*/
function null_to_zero($value)
{
	return ($value === NULL) ? 0 : $value;
}

/**
* Convert nulls to empty strings for fields which allowed a NULL value in the source but not the destination
*/
function null_to_str($value)
{
	return ($value === NULL) ? '' : $value;
}

// EXTENDED FUNCTIONS

/**
* Get old config value
*/
function get_config_value($config_name)
{
	static $convert_config;

	if (!isset($convert_config))
	{
		$convert_config = get_config();
	}
	
	if (!isset($convert_config[$config_name]))
	{
		return false;
	}

	return (empty($convert_config[$config_name])) ? '' : $convert_config[$config_name];
}

/**
* Convert an IP address from the hexadecimal notation to normal dotted-quad notation
*/
function decode_ip($int_ip)
{
	if (!$int_ip)
	{
		return $int_ip;
	}

	$hexipbang = explode('.', chunk_split($int_ip, 2, '.'));

	// Any mod changing the way ips are stored? Then we are not able to convert and enter the ip "as is" to not "destroy" anything...
	if (sizeof($hexipbang) < 4)
	{
		return $int_ip;
	}

	return hexdec($hexipbang[0]) . '.' . hexdec($hexipbang[1]) . '.' . hexdec($hexipbang[2]) . '.' . hexdec($hexipbang[3]);
}

/**
* Reverse the encoding of wild-carded bans
*/
function decode_ban_ip($int_ip)
{
	return str_replace('255', '*', decode_ip($int_ip));
}

/**
* Determine the MIME-type of a specified filename
* This does not actually inspect the file, but simply uses the file extension
*/
function mimetype($filename)
{
	if (!preg_match('/\.([a-z0-9]+)$/i', $filename, $m))
	{
		return 'application/octet-stream';
	}

	switch (strtolower($m[1]))
	{
		case 'zip':		return 'application/zip';
		case 'jpeg':	return 'image/jpeg';
		case 'jpg':		return 'image/jpeg';
		case 'jpe':		return 'image/jpeg';
		case 'png':		return 'image/png';
		case 'gif':		return 'image/gif';
		case 'htm':
		case 'html':	return 'text/html';
		case 'tif':		return 'image/tiff';
		case 'tiff':	return 'image/tiff';
		case 'ras':		return 'image/x-cmu-raster';
		case 'pnm':		return 'image/x-portable-anymap';
		case 'pbm':		return 'image/x-portable-bitmap';
		case 'pgm':		return 'image/x-portable-graymap';
		case 'ppm':		return 'image/x-portable-pixmap';
		case 'rgb':		return 'image/x-rgb';
		case 'xbm':		return 'image/x-xbitmap';
		case 'xpm':		return 'image/x-xpixmap';
		case 'xwd':		return 'image/x-xwindowdump';
		case 'z':		return 'application/x-compress';
		case 'gtar':	return 'application/x-gtar';
		case 'tgz':		return 'application/x-gtar';
		case 'gz':		return 'application/x-gzip';
		case 'tar':		return 'application/x-tar';
		case 'xls':		return 'application/excel';
		case 'pdf':		return 'application/pdf';
		case 'ppt':		return 'application/powerpoint';
		case 'rm':		return 'application/vnd.rn-realmedia';
		case 'wma':		return 'audio/x-ms-wma';
		case 'swf':		return 'application/x-shockwave-flash';
		case 'ief':		return 'image/ief';
		case 'doc':
		case 'dot':
		case 'wrd':		return 'application/msword';
		case 'ai':
		case 'eps':
		case 'ps':		return 'application/postscript';
		case 'asc':
		case 'txt':
		case 'c':
		case 'cc':
		case 'h':
		case 'hh':
		case 'cpp':
		case 'hpp':
		case 'php':
		case 'php3':	return 'text/plain';
		default: 		return 'application/octet-stream';
	}
}

/**
* Obtain the dimensions of all remotely hosted avatars
* This should only be called from execute_last
* There can be significant network overhead if there are a large number of remote avatars
* @todo Look at the option of allowing the user to decide whether this is called or to force the dimensions
*/
function remote_avatar_dims()
{
	global $db;

	$sql = 'SELECT user_id, user_avatar
		FROM ' . USERS_TABLE . '
		WHERE user_avatar_type = ' . AVATAR_REMOTE;
	$result = $db->sql_query($sql);

	$remote_avatars = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$remote_avatars[(int) $row['user_id']] = $row['user_avatar'];
	}
	$db->sql_freeresult($result);

	foreach ($remote_avatars as $user_id => $avatar)
	{
		$width = (int) get_remote_avatar_dim($avatar, 0);
		$height = (int) get_remote_avatar_dim($avatar, 1);

		$sql = 'UPDATE ' . USERS_TABLE . '
			SET user_avatar_width = ' . (int) $width . ', user_avatar_height = ' . (int) $height . '
			WHERE user_id = ' . $user_id;
		$db->sql_query($sql);
	}
}

function import_avatar_gallery($gallery_name = '', $subdirs_as_galleries = false)
{
	global $config, $convert, $phpbb_root_path, $user;

	$relative_path = empty($convert->convertor['source_path_absolute']);

	if (empty($convert->convertor['avatar_gallery_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_GALLERY_PATH'], 'import_avatar_gallery()'), __LINE__, __FILE__);
	}

	$src_path = relative_base(path($convert->convertor['avatar_gallery_path'], $relative_path), $relative_path);

	if (is_dir($src_path))
	{
		// Do not die on failure... safe mode restrictions may be in effect.
		copy_dir($convert->convertor['avatar_gallery_path'], path($config['avatar_gallery_path']) . $gallery_name, !$subdirs_as_galleries, false, false, $relative_path);

		// only doing 1 level deep. (ibf 1.x)
		// notes: ibf has 2 tiers: directly in the avatar directory for base gallery (handled in the above statement), plus subdirs(handled below).
		// recursive subdirs ignored. -- i don't know if other forums support recursive galleries. if they do, this following code could be upgraded to be recursive.
		if ($subdirs_as_galleries)
		{
			$dirlist = array();
			if ($handle = @opendir($src_path))
			{
				while ($entry = readdir($handle))
				{
					if ($entry[0] == '.' || $entry == 'CVS' || $entry == 'index.htm')
					{
						continue;
					}

					if (is_dir($src_path . $entry))
					{
						$dirlist[] = $entry;
					}
				}
				closedir($handle);
			}
			else if ($dir = @dir($src_path))
			{
				while ($entry = $dir->read())
				{
					if ($entry[0] == '.' || $entry == 'CVS' || $entry == 'index.htm')
					{
						continue;
					}

					if (is_dir($src_path . $entry))
					{
						$dirlist[] = $entry;
					}
				}
				$dir->close();
			}

			for ($i = 0; $i < sizeof($dirlist); ++$i)
			{
				$dir = $dirlist[$i];

				// Do not die on failure... safe mode restrictions may be in effect.
				copy_dir(path($convert->convertor['avatar_gallery_path'], $relative_path) . $dir, path($config['avatar_gallery_path']) . $dir, true, false, false, $relative_path);
			}
		}
	}
}

function import_attachment_files($category_name = '')
{
	global $config, $convert, $phpbb_root_path, $db, $user;

	$sql = 'SELECT config_value AS upload_path
		FROM ' . CONFIG_TABLE . "
		WHERE config_name = 'upload_path'";
	$result = $db->sql_query($sql);
	$config['upload_path'] = $db->sql_fetchfield('upload_path');
	$db->sql_freeresult($result);

	$relative_path = empty($convert->convertor['source_path_absolute']);

	if (empty($convert->convertor['upload_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_UPLOAD_DIR'], 'import_attachment_files()'), __LINE__, __FILE__);
	}

	if (is_dir(relative_base(path($convert->convertor['upload_path'], $relative_path), $relative_path)))
	{
		copy_dir($convert->convertor['upload_path'], path($config['upload_path']) . $category_name, true, false, true, $relative_path);
	}
}

function attachment_forum_perms($forum_id)
{
	if (!is_array($forum_id))
	{
		$forum_id = array($forum_id);
	}

	return serialize($forum_id);
}

// base64todec function
// -> from php manual?
function base64_unpack($string)
{
	$chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-';
	$base = strlen($chars);

	$length = strlen($string);
	$number = 0;

	for ($i = 1; $i <= $length; $i++)
	{
		$pos = $length - $i;
		$operand = strpos($chars, substr($string, $pos, 1));
		$exponent = pow($base, $i-1);
		$dec_value = $operand * $exponent;
		$number += $dec_value;
	}

	return $number;
}

function _import_check($config_var, $source, $use_target)
{
	global $convert, $config;

	$result = array(
		'orig_source'	=> $source,
		'copied'		=> false,
		'relative_path'	=> (empty($convert->convertor['source_path_absolute'])) ? true : false,
	);

	// copy file will prepend $phpBB_root_path
	$target = $config[$config_var] . '/' . basename(($use_target === false) ? $source : $use_target);

	if (!empty($convert->convertor[$config_var]) && strpos($source, $convert->convertor[$config_var]) !== 0)
	{
		$source = $convert->convertor[$config_var] . $source;
	}

	$result['source'] = $source;

	if (file_exists(relative_base($source, $result['relative_path'], __LINE__, __FILE__)))
	{
		$result['copied'] = copy_file($source, $target, false, false, $result['relative_path']);
	}

	if ($result['copied'])
	{
		$result['target'] = basename($target);
	}
	else
	{
		$result['target'] = ($use_target !== false) ? $result['orig_source'] : basename($target);
	}

	return $result;
}

function import_attachment($source, $use_target = false)
{
	if (empty($source))
	{
		return '';
	}

	global $convert, $phpbb_root_path, $config, $user;

	if (empty($convert->convertor['upload_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_UPLOAD_DIR'], 'import_attachment()'), __LINE__, __FILE__);
	}

	$result = _import_check('upload_path', $source, $use_target);

	if ($result['copied'])
	{
		// Thumbnails?
		if (is_array($convert->convertor['thumbnails']))
		{
			$thumb_dir = $convert->convertor['thumbnails'][0];
			$thumb_prefix = $convert->convertor['thumbnails'][1];
			$thumb_source = $thumb_dir . $thumb_prefix . basename($result['source']);

			if (strpos($thumb_source, $convert->convertor['upload_path']) !== 0)
			{
				$thumb_source = $convert->convertor['upload_path'] . $thumb_source;
			}
			$thumb_target = $config['upload_path'] . '/thumb_' . $result['target'];

			if (file_exists(relative_base($thumb_source, $result['relative_path'], __LINE__, __FILE__)))
			{
				copy_file($thumb_source, $thumb_target, false, false, $result['relative_path']);
			}
		}
	}

	return $result['target'];
}

function import_rank($source, $use_target = false)
{
	if (empty($source))
	{
		return '';
	}

	global $convert, $phpbb_root_path, $config, $user;

	if (!isset($convert->convertor['ranks_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_RANKS_PATH'], 'import_rank()'), __LINE__, __FILE__);
	}

	$result = _import_check('ranks_path', $source, $use_target);
	return $result['target'];
}

function import_smiley($source, $use_target = false)
{
	if (empty($source))
	{
		return '';
	}

	global $convert, $phpbb_root_path, $config, $user;

	if (!isset($convert->convertor['smilies_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_SMILIES_PATH'], 'import_smiley()'), __LINE__, __FILE__);
	}

	$result = _import_check('smilies_path', $source, $use_target);
	return $result['target'];
}

/*
*/
function import_avatar($source, $use_target = false, $user_id = false)
{
	if (empty($source) || preg_match('#^https?:#i', $source) || preg_match('#blank\.(gif|png)$#i', $source))
	{
		return;
	}

	global $convert, $phpbb_root_path, $config, $user;

	if (!isset($convert->convertor['avatar_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_AVATAR_PATH'], 'import_avatar()'), __LINE__, __FILE__);
	}
	
	if ($use_target === false && $user_id !== false)
	{
		$use_target = $config['avatar_salt'] . '_' . $user_id . '.' . substr(strrchr($source, '.'), 1);
	}
	
	$result = _import_check('avatar_path', $source, $use_target);

	return ((!empty($user_id)) ? $user_id : $use_target) . '.' . substr(strrchr($source, '.'), 1);
}

/**
* @todo all image dimension functions below (there are a *lot*) should get revisited and converted to one or two functions (no more needed, really).
*/

/**
* Calculate the size of the specified image
* Called from the following functions for calculating the size of specific image types
*/
function get_image_dim($source)
{
	if (empty($source))
	{
		return array(0, 0);
	}

	global $convert;

	$relative_path = empty($convert->convertor['source_path_absolute']);

	if (file_exists(relative_base($source, $relative_path)))
	{
		$image = relative_base($source, $relative_path);
		return @getimagesize($image);
	}

	return false;
}

/**
* Obtain the width of the specified smilie
*/
function get_smiley_width($src)
{
	return get_smiley_dim($src, 0);
}

/**
* Obtain the height of the specified smilie
*/
function get_smiley_height($src)
{
	return get_smiley_dim($src, 1);
}

/**
* Obtain the size of the specified smilie (using the cache if possible) and cache the value
*/
function get_smiley_dim($source, $axis)
{
	if (empty($source))
	{
		return 15;
	}

	static $smiley_cache = array();

	if (isset($smiley_cache[$source]))
	{
		return $smiley_cache[$source][$axis];
	}

	global $convert, $phpbb_root_path, $config, $user;

	$orig_source = $source;

	if (!isset($convert->convertor['smilies_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_SMILIES_PATH'], 'get_smiley_dim()'), __LINE__, __FILE__);
	}

	if (!empty($convert->convertor['smilies_path']) && strpos($source, $convert->convertor['smilies_path']) !== 0)
	{
		$source = $convert->convertor['smilies_path'] . $source;
	}

	$smiley_cache[$orig_source] = get_image_dim($source);

	if (empty($smiley_cache[$orig_source]) || empty($smiley_cache[$orig_source][0]) || empty($smiley_cache[$orig_source][1]))
	{
		$smiley_cache[$orig_source] = array(15, 15);
		return 15;
	}

	return $smiley_cache[$orig_source][$axis];
}

/**
* Obtain the width of the specified avatar
*/
function get_avatar_width($src, $func = false, $arg1 = false, $arg2 = false)
{
	return get_avatar_dim($src, 0, $func, $arg1, $arg2);
}

/**
* Obtain the height of the specified avatar
*/
function get_avatar_height($src, $func = false, $arg1 = false, $arg2 = false)
{
	return get_avatar_dim($src, 1, $func, $arg1, $arg2);
}

/**
*/
function get_avatar_dim($src, $axis, $func = false, $arg1 = false, $arg2 = false)
{
	$avatar_type = AVATAR_UPLOAD;

	if ($func)
	{
		if ($arg1 || $arg2)
		{
			$ary = array($arg1);

			if ($arg2)
			{
				$ary[] = $arg2;
			}

			$avatar_type = call_user_func_array($func, $ary);
		}
		else
		{
			$avatar_type = call_user_func($func);
		}
	}

	switch ($avatar_type)
	{
		case AVATAR_UPLOAD:
			return get_upload_avatar_dim($src, $axis);
		break;

		case AVATAR_GALLERY:
			return get_gallery_avatar_dim($src, $axis);
		break;

		case AVATAR_REMOTE:
			 // see notes on this functions usage and (hopefully) model $func to avoid this accordingly
			return get_remote_avatar_dim($src, $axis);
		break;

		default:
			$default_x = (defined('DEFAULT_AVATAR_X_CUSTOM')) ? DEFAULT_AVATAR_X_CUSTOM : DEFAULT_AVATAR_X;
			$default_y = (defined('DEFAULT_AVATAR_Y_CUSTOM')) ? DEFAULT_AVATAR_Y_CUSTOM : DEFAULT_AVATAR_Y;

			return $axis ? $default_y : $default_x;
		break;
	}
}

/**
* Obtain the size of the specified uploaded avatar (using the cache if possible) and cache the value
*/
function get_upload_avatar_dim($source, $axis)
{
	static $cachedims = false;
	static $cachekey = false;

	if (empty($source))
	{
		return 0;
	}

	if ($cachekey == $source)
	{
		return $cachedims[$axis];
	}

	$orig_source = $source;

	if (substr($source, 0, 7) == 'upload:')
	{
		$source = substr($source, 7);
	}

	global $convert, $phpbb_root_path, $config, $user;

	if (!isset($convert->convertor['avatar_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_AVATAR_PATH'], 'get_upload_avatar_dim()'), __LINE__, __FILE__);
	}

	if (!empty($convert->convertor['avatar_path']) && strpos($source, $convert->convertor['avatar_path']) !== 0)
	{
		$source = path($convert->convertor['avatar_path'], empty($convert->convertor['source_path_absolute'])) . $source;
	}

	$cachedims = get_image_dim($source);

	if (empty($cachedims) || empty($cachedims[0]) || empty($cachedims[1]))
	{
		$default_x = (defined('DEFAULT_AVATAR_X_CUSTOM')) ? DEFAULT_AVATAR_X_CUSTOM : DEFAULT_AVATAR_X;
		$default_y = (defined('DEFAULT_AVATAR_Y_CUSTOM')) ? DEFAULT_AVATAR_Y_CUSTOM : DEFAULT_AVATAR_Y;

		$cachedims = array($default_x, $default_y);
	}

	return $cachedims[$axis];
}

/**
* Obtain the size of the specified gallery avatar (using the cache if possible) and cache the value
*/
function get_gallery_avatar_dim($source, $axis)
{
	if (empty($source))
	{
		return 0;
	}

	static $avatar_cache = array();

	if (isset($avatar_cache[$source]))
	{
		return $avatar_cache[$source][$axis];
	}

	global $convert, $phpbb_root_path, $config, $user;

	$orig_source = $source;

	if (!isset($convert->convertor['avatar_gallery_path']))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_GALLERY_PATH'], 'get_gallery_avatar_dim()'), __LINE__, __FILE__);
	}

	if (!empty($convert->convertor['avatar_gallery_path']) && strpos($source, $convert->convertor['avatar_gallery_path']) !== 0)
	{
		$source = path($convert->convertor['avatar_gallery_path'], empty($convert->convertor['source_path_absolute'])) . $source;
	}

	$avatar_cache[$orig_source] = get_image_dim($source);

	if (empty($avatar_cache[$orig_source]) || empty($avatar_cache[$orig_source][0]) || empty($avatar_cache[$orig_source][1]))
	{
		$default_x = (defined('DEFAULT_AVATAR_X_CUSTOM')) ? DEFAULT_AVATAR_X_CUSTOM : DEFAULT_AVATAR_X;
		$default_y = (defined('DEFAULT_AVATAR_Y_CUSTOM')) ? DEFAULT_AVATAR_Y_CUSTOM : DEFAULT_AVATAR_Y;

		$avatar_cache[$orig_source] = array($default_x, $default_y);
	}

	return $avatar_cache[$orig_source][$axis];
}

/**
* Obtain the size of the specified remote avatar (using the cache if possible) and cache the value
* Whilst it's unlikely that remote avatars will be duplicated, it is possible so caching seems the best option
* This should only be called from a post processing step due to the possibility of network timeouts
*/
function get_remote_avatar_dim($src, $axis)
{
	if (empty($src))
	{
		return 0;
	}

	static $remote_avatar_cache = array();

	// an ugly hack: we assume that the dimensions of each remote avatar are accessed exactly twice (x and y)
	if (isset($remote_avatar_cache[$src]))
	{
		$retval = $remote_avatar_cache[$src][$axis];
		unset($remote_avatar_cache);
		return $retval;
	}
	
	$url_info = @parse_url($src);
	if (empty($url_info['host']))
	{
		return 0;
	}
	$host = $url_info['host'];
	$port = (isset($url_info['port'])) ? $url_info['port'] : 0;
	$protocol = (isset($url_info['scheme'])) ? $url_info['scheme'] : 'http';
	if (empty($port))
	{
		switch(strtolower($protocol))
		{
			case 'ftp':
				$port = 21;
				break;
				
			case 'https':
				$port = 443;
				break;
			
			default:
				$port = 80;
		}
	}
	
	$timeout = @ini_get('default_socket_timeout');
	@ini_set('default_socket_timeout', 2);
	
	// We're just trying to reach the server to avoid timeouts
	$fp = @fsockopen($host, $port, $errno, $errstr, 1);
	if ($fp)
	{
		$remote_avatar_cache[$src] = @getimagesize($src);
		fclose($fp);
	}
	
	$default_x 	= (defined('DEFAULT_AVATAR_X_CUSTOM')) ? DEFAULT_AVATAR_X_CUSTOM : DEFAULT_AVATAR_X;
	$default_y 	= (defined('DEFAULT_AVATAR_Y_CUSTOM')) ? DEFAULT_AVATAR_Y_CUSTOM : DEFAULT_AVATAR_Y;
	$default 	= array($default_x, $default_y);
	
	if (empty($remote_avatar_cache[$src]) || empty($remote_avatar_cache[$src][0]) || empty($remote_avatar_cache[$src][1]))
	{
		$remote_avatar_cache[$src] = $default;
	}
	else
	{
		// We trust gallery and uploaded avatars to conform to the size settings; we might have to adjust here
		if ($remote_avatar_cache[$src][0] > $default_x || $remote_avatar_cache[$src][1] > $default_y)
		{
			$bigger = ($remote_avatar_cache[$src][0] > $remote_avatar_cache[$src][1]) ? 0 : 1;
			$ratio = $default[$bigger] / $remote_avatar_cache[$src][$bigger];
			$remote_avatar_cache[$src][0] = (int)($remote_avatar_cache[$src][0] * $ratio);
			$remote_avatar_cache[$src][1] = (int)($remote_avatar_cache[$src][1] * $ratio);
		}
	}
	
	@ini_set('default_socket_timeout', $timeout);
	return $remote_avatar_cache[$src][$axis];
}

function set_user_options()
{
	global $convert_row;

	// Key need to be set in row, else default value is chosen
	$keyoptions = array(
		'viewimg'		=> array('bit' => 0, 'default' => 1),
		'viewflash'		=> array('bit' => 1, 'default' => 1),
		'viewsmilies'	=> array('bit' => 2, 'default' => 1),
		'viewsigs'		=> array('bit' => 3, 'default' => 1),
		'viewavatars'	=> array('bit' => 4, 'default' => 1),
		'viewcensors'	=> array('bit' => 5, 'default' => 1),
		'attachsig'		=> array('bit' => 6, 'default' => 0),
		'bbcode'		=> array('bit' => 8, 'default' => 1),
		'smilies'		=> array('bit' => 9, 'default' => 1),
		'popuppm'		=> array('bit' => 10, 'default' => 0),
	);

	$option_field = 0;

	foreach ($keyoptions as $key => $key_ary)
	{
		$value = (isset($convert_row[$key])) ? (int) $convert_row[$key] : $key_ary['default'];

		if ($value && !($option_field & 1 << $key_ary['bit']))
		{
			$option_field += 1 << $key_ary['bit'];
		}
	}

	return $option_field;
}

/**
* Index messages on the fly as we convert them
* @todo naderman, can you check that this works with the new search plugins as it's use is currently disabled (and thus untested)
function search_indexing($message = '')
{
	global $fulltext_search, $convert_row;

	if (!isset($convert_row['post_id']))
	{
		return;
	}

	if (!$message)
	{
		if (!isset($convert_row['message']))
		{
			return;
		}

		$message = $convert_row['message'];
	}

	$title = (isset($convert_row['title'])) ? $convert_row['title'] : '';

	$fulltext_search->index('post', $convert_row['post_id'], $message, $title, $convert_row['poster_id'], $convert_row['forum_id']);
}
*/

function make_unique_filename($filename)
{
	if (!strlen($filename))
	{
		$filename = md5(unique_id()) . '.dat';
	}
	else if ($filename[0] == '.')
	{
		$filename = md5(unique_id()) . $filename;
	}
	else if (preg_match('/\.([a-z]+)$/i', $filename, $m))
	{
		$filename = preg_replace('/\.([a-z]+)$/i', '_' . md5(unique_id()) . '.\1', $filename);
	}
	else
	{
		$filename .= '_' . md5(unique_id()) . '.dat';
	}

	return $filename;
}

function words_unique(&$words)
{
	reset($words);
	$return_array = array();

	$word = current($words);
	do
	{
		$return_array[$word] = $word;
	}
	while ($word = next($words));

	return $return_array;
}

/**
* Adds a user to the specified group and optionally makes them a group leader
* This function does not create the group if it does not exist and so should only be called after the groups have been created
*/
function add_user_group($group_id, $user_id, $group_leader=false)
{
	global $convert, $phpbb_root_path, $config, $user, $db;
	
	$sql = 'INSERT INTO ' . USER_GROUP_TABLE . ' ' . $db->sql_build_array('INSERT', array(
		'group_id'		=> $group_id,
		'user_id'		=> $user_id,
		'group_leader'	=> ($group_leader) ? 1 : 0,
		'user_pending'	=> 0));
	$db->sql_query($sql);
}

// STANDALONE FUNCTIONS

/**
* Add users to the pre-defined "special" groups
*
* @param string $group The name of the special group to add to
* @param string $select_query An SQL query to retrieve the user(s) to add to the group
*/
function user_group_auth($group, $select_query, $use_src_db)
{
	global $convert, $phpbb_root_path, $config, $user, $db, $src_db, $same_db;

	if (!in_array($group, array('guests', 'registered', 'registered_coppa', 'global_moderators', 'administrators', 'bots')))
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_WRONG_GROUP'], $group, 'user_group_auth()'), __LINE__, __FILE__, true);
		return;
	}

	$sql = 'SELECT group_id
		FROM ' . GROUPS_TABLE . "
		WHERE group_name = '" . $db->sql_escape(strtoupper($group)) . "'";
	$result = $db->sql_query($sql);
	$group_id = (int) $db->sql_fetchfield('group_id');
	$db->sql_freeresult($result);

	if (!$group_id)
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_NO_GROUP'], $group, 'user_group_auth()'), __LINE__, __FILE__, true);
		return;
	}

	if ($same_db || !$use_src_db)
	{
		$sql = 'INSERT INTO ' . USER_GROUP_TABLE . ' (user_id, group_id, user_pending)
			' . str_replace('{' . strtoupper($group) . '}', $group_id . ', 0', $select_query);
		$db->sql_query($sql);
	}
	else
	{
		$result = $src_db->sql_query(str_replace('{' . strtoupper($group) . '}', $group_id . ' ', $select_query));
		while ($row = $src_db->sql_fetchrow($result))
		{
			// this might become quite a lot of INSERTS unfortunately
			$sql = 'INSERT INTO ' . USER_GROUP_TABLE . " (user_id, group_id, user_pending)
				VALUES ({$row['user_id']}, $group_id, 0)";
			$db->sql_query($sql);
		}
		$src_db->sql_freeresult($result);
	}
}

/**
* Retrieves configuration information from the source forum and caches it as an array
* Both database and file driven configuration formats can be handled
* (the type used is specified in $config_schema, see convert_phpbb20.php for more details)
*/
function get_config()
{
	static $convert_config;
	global $user;

	if (isset($convert_config))
	{
		return $convert_config;
	}

	global $src_db, $same_db, $phpbb_root_path, $config;
	global $convert;

	if ($convert->config_schema['table_format'] != 'file')
	{
		if ($convert->mysql_convert && $same_db)
		{
			$src_db->sql_query("SET NAMES 'binary'");
		}

		$sql = 'SELECT * FROM ' . $convert->src_table_prefix . $convert->config_schema['table_name'];
		$result = $src_db->sql_query($sql);
		$row = $src_db->sql_fetchrow($result);

		if (!$row)
		{
			$convert->p_master->error($user->lang['CONV_ERROR_GET_CONFIG'], __LINE__, __FILE__);
		}
	}

	if (is_array($convert->config_schema['table_format']))
	{
		$convert_config = array();
		list($key, $val) = each($convert->config_schema['table_format']);

		do
		{
			$convert_config[$row[$key]] = $row[$val];
		}
		while ($row = $src_db->sql_fetchrow($result));
		$src_db->sql_freeresult($result);

		if ($convert->mysql_convert && $same_db)
		{
			$src_db->sql_query("SET NAMES 'utf8'");
		}
	}
	else if ($convert->config_schema['table_format'] == 'file')
	{
		$filename = $convert->options['forum_path'] . '/' . $convert->config_schema['filename'];
		if (!file_exists($filename))
		{
			$convert->p_master->error($user->lang['FILE_NOT_FOUND'] . ': ' . $filename, __LINE__, __FILE__);
		}

		$convert_config = extract_variables_from_file($filename);
		if (!empty($convert->config_schema['array_name']))
		{
			$convert_config = $convert_config[$convert->config_schema['array_name']];
		}
	}
	else
	{
		$convert_config = $row;
		if ($convert->mysql_convert && $same_db)
		{
			$src_db->sql_query("SET NAMES 'utf8'");
		}
	}

	if (!sizeof($convert_config))
	{
		$convert->p_master->error($user->lang['CONV_ERROR_CONFIG_EMPTY'], __LINE__, __FILE__);
	}

	return $convert_config;
}

/**
* Transfers the relevant configuration information from the source forum
* The mapping of fields is specified in $config_schema, see convert_phpbb20.php for more details
*/
function restore_config($schema)
{
	global $db, $config;

	$convert_config = get_config();
	foreach ($schema['settings'] as $config_name => $src)
	{
		if (preg_match('/(.*)\((.*)\)/', $src, $m))
		{
			$var = (empty($m[2]) || empty($convert_config[$m[2]])) ? "''" : "'" . addslashes($convert_config[$m[2]]) . "'";
			$exec = '$config_value = ' . $m[1] . '(' . $var . ');';
			eval($exec);
		}
		else
		{
			$config_value = (isset($convert_config[$src])) ? $convert_config[$src] : '';
		}

		if ($config_value !== '')
		{
			// Most are...
			if (is_string($config_value))
			{
				$config_value = truncate_string(utf8_htmlspecialchars($config_value), 255, false);
			}

			set_config($config_name, $config_value);
		}
	}
}

/**
* Update the count of PM's in custom folders for all users
*/
function update_folder_pm_count()
{
	global $db, $convert, $user;

	$sql = 'SELECT user_id, folder_id, COUNT(msg_id) as num_messages
		FROM ' . PRIVMSGS_TO_TABLE . '
		WHERE folder_id NOT IN (' . PRIVMSGS_NO_BOX . ', ' . PRIVMSGS_HOLD_BOX . ', ' . PRIVMSGS_INBOX . ', ' . PRIVMSGS_OUTBOX . ', ' . PRIVMSGS_SENTBOX . ')
		GROUP BY folder_id, user_id';
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$db->sql_query('UPDATE ' . PRIVMSGS_FOLDER_TABLE . ' SET pm_count = ' . $row['num_messages'] . '
			WHERE user_id = ' . $row['user_id'] . ' AND folder_id = ' . $row['folder_id']);
	}
	$db->sql_freeresult($result);
}

// Functions mainly used by the main convertor script

function path($path, $path_relative = true)
{
	if ($path === false)
	{
		return '';
	}

	if (substr($path, -1) != '/')
	{
		$path .= '/';
	}

	if (!$path_relative)
	{
		return $path;
	}

	if (substr($path, 0, 1) == '/')
	{
		$path = substr($path, 1);
	}

	return $path;
}

/**
* Extract the variables defined in a configuration file
* @todo As noted by Xore we need to look at this from a security perspective
*/
function extract_variables_from_file($_filename)
{
	include($_filename);

	$vars = get_defined_vars();
	unset($vars['_filename']);

	return $vars;
}

function get_path($src_path, $src_url, $test_file)
{
	global $config, $phpbb_root_path, $phpEx;

	$board_config = get_config();

	$test_file = preg_replace('/\.php$/i', ".$phpEx", $test_file);
	$src_path = path($src_path);

	if (@file_exists($phpbb_root_path . $src_path . $test_file))
	{
		return $src_path;
	}

	if (!empty($src_url) && !empty($board_config['server_name']))
	{
		if (!preg_match('#https?://([^/]+)(.*)#i', $src_url, $m))
		{
			return false;
		}

		if ($m[1] != $board_config['server_name'])
		{
			return false;
		}

		$url_parts = explode('/', $m[2]);
		if (substr($src_url, -1) != '/')
		{
			if (preg_match('/.*\.([a-z0-9]{3,4})$/i', $url_parts[sizeof($url_parts) - 1]))
			{
				$url_parts[sizeof($url_parts) - 1] = '';
			}
			else
			{
				$url_parts[] = '';
			}
		}

		$script_path = $board_config['script_path'];
		if (substr($script_path, -1) == '/')
		{
			$script_path = substr($script_path, 0, -1);
		}

		$path_array = array();

		$phpbb_parts = explode('/', $script_path);
		for ($i = 0; $i < sizeof($url_parts); ++$i)
		{
			if ($i < sizeof($phpbb_parts[$i]) && $url_parts[$i] == $phpbb_parts[$i])
			{
				$path_array[] = $url_parts[$i];
				unset($url_parts[$i]);
			}
			else
			{
				$path = '';
				for ($j = $i; $j < sizeof($phpbb_parts); ++$j)
				{
					$path .= '../';
				}
				$path .= implode('/', $url_parts);
				break;
			}
		}

		if (!empty($path))
		{
			if (@file_exists($phpbb_root_path . $path . $test_file))
			{
				return $path;
			}
		}
	}

	return false;
}

function compare_table($tables, $tablename, &$prefixes)
{
	for ($i = 0, $table_size = sizeof($tables); $i < $table_size; ++$i)
	{
		if (preg_match('/(.*)' . $tables[$i] . '$/', $tablename, $m))
		{
			if (empty($m[1]))
			{
				$m[1] = '*';
			}

			if (isset($prefixes[$m[1]]))
			{
				$prefixes[$m[1]]++;
			}
			else
			{
				$prefixes[$m[1]] = 1;
			}
		}
	}
}

/**
* Grant permissions to a specified user or group
*
* @param string $ug_type user|group|user_role|group_role
* @param mixed $forum_id forum ids (array|int|0) -> 0 == all forums
* @param mixed $ug_id [int] user_id|group_id : [string] usergroup name
* @param mixed $acl_list [string] acl entry : [array] acl entries : [string] role entry
* @param int $setting ACL_YES|ACL_NO|ACL_NEVER
*/
function mass_auth($ug_type, $forum_id, $ug_id, $acl_list, $setting = ACL_NO)
{
	global $db, $convert, $user, $config;
	static $acl_option_ids, $group_ids;

	if (($ug_type == 'group' || $ug_type == 'group_role') && is_string($ug_id))
	{
		if (!isset($group_ids[$ug_id]))
		{
			$sql = 'SELECT group_id
				FROM ' . GROUPS_TABLE . "
				WHERE group_name = '" . $db->sql_escape(strtoupper($ug_id)) . "'";
			$result = $db->sql_query_limit($sql, 1);
			$id = (int) $db->sql_fetchfield('group_id');
			$db->sql_freeresult($result);

			if (!$id)
			{
				return;
			}

			$group_ids[$ug_id] = $id;
		}

		$ug_id = (int) $group_ids[$ug_id];
	}

	$table = ($ug_type == 'user' || $ug_type == 'user_role') ? ACL_USERS_TABLE : ACL_GROUPS_TABLE;
	$id_field = ($ug_type == 'user' || $ug_type == 'user_role') ? 'user_id' : 'group_id';

	// Role based permissions are the simplest to handle so check for them first
	if ($ug_type == 'user_role' || $ug_type == 'group_role')
	{
		if (is_numeric($forum_id))
		{
			$sql = 'SELECT role_id
				FROM ' . ACL_ROLES_TABLE . "
				WHERE role_name = 'ROLE_" . $db->sql_escape($acl_list) . "'";
			$result = $db->sql_query_limit($sql, 1);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// If we have no role id there is something wrong here
			if ($row)
			{
				$sql = "INSERT INTO $table ($id_field, forum_id, auth_role_id) VALUES ($ug_id, $forum_id, " . $row['role_id'] . ')';
				$db->sql_query($sql);
			}
		}

		return;
	}

	// Build correct parameters
	$auth = array();

	if (!is_array($acl_list))
	{
		$auth = array($acl_list => $setting);
	}
	else
	{
		foreach ($acl_list as $auth_option)
		{
			$auth[$auth_option] = $setting;
		}
	}
	unset($acl_list);

	if (!is_array($forum_id))
	{
		$forum_id = array($forum_id);
	}

	// Set any flags as required
	foreach ($auth as $auth_option => $acl_setting)
	{
		$flag = substr($auth_option, 0, strpos($auth_option, '_') + 1);
		if (empty($auth[$flag]))
		{
			$auth[$flag] = $acl_setting;
		}
	}

	if (!is_array($acl_option_ids) || empty($acl_option_ids))
	{
		$sql = 'SELECT auth_option_id, auth_option
			FROM ' . ACL_OPTIONS_TABLE;
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$acl_option_ids[$row['auth_option']] = $row['auth_option_id'];
		}
		$db->sql_freeresult($result);
	}

	$sql_forum = 'AND ' . $db->sql_in_set('a.forum_id', array_map('intval', $forum_id), false, true);

	$sql = ($ug_type == 'user') ? 'SELECT o.auth_option_id, o.auth_option, a.forum_id, a.auth_setting FROM ' . ACL_USERS_TABLE . ' a, ' . ACL_OPTIONS_TABLE . " o WHERE a.auth_option_id = o.auth_option_id $sql_forum AND a.user_id = $ug_id" : 'SELECT o.auth_option_id, o.auth_option, a.forum_id, a.auth_setting FROM ' . ACL_GROUPS_TABLE . ' a, ' . ACL_OPTIONS_TABLE . " o WHERE a.auth_option_id = o.auth_option_id $sql_forum AND a.group_id = $ug_id";
	$result = $db->sql_query($sql);

	$cur_auth = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$cur_auth[$row['forum_id']][$row['auth_option_id']] = $row['auth_setting'];
	}
	$db->sql_freeresult($result);

	$sql_ary = array();
	foreach ($forum_id as $forum)
	{
		foreach ($auth as $auth_option => $setting)
		{
			$auth_option_id = $acl_option_ids[$auth_option];

			if (!$auth_option_id)
			{
				continue;
			}

			switch ($setting)
			{
				case ACL_NO:
					if (isset($cur_auth[$forum][$auth_option_id]))
					{
						$sql_ary['delete'][] = "DELETE FROM $table
							WHERE forum_id = $forum
								AND auth_option_id = $auth_option_id
								AND $id_field = $ug_id";
					}
				break;

				default:
					if (!isset($cur_auth[$forum][$auth_option_id]))
					{
						$sql_ary['insert'][] = "$ug_id, $forum, $auth_option_id, $setting";
					}
					else if ($cur_auth[$forum][$auth_option_id] != $setting)
					{
						$sql_ary['update'][] = "UPDATE " . $table . "
							SET auth_setting = $setting
							WHERE $id_field = $ug_id
								AND forum_id = $forum
								AND auth_option_id = $auth_option_id";
					}
			}
		}
	}
	unset($cur_auth);

	$sql = '';
	foreach ($sql_ary as $sql_type => $sql_subary)
	{
		switch ($sql_type)
		{
			case 'insert':
				switch ($db->sql_layer)
				{
					case 'mysql':
					case 'mysql4':
						$sql = 'VALUES ' . implode(', ', preg_replace('#^(.*?)$#', '(\1)', $sql_subary));
					break;

					case 'mssql':
					case 'sqlite':
						$sql = implode(' UNION ALL ', preg_replace('#^(.*?)$#', 'SELECT \1', $sql_subary));
					break;

					default:
						foreach ($sql_subary as $sql)
						{
							$sql = "INSERT INTO $table ($id_field, forum_id, auth_option_id, auth_setting) VALUES ($sql)";
							$db->sql_query($sql);
							$sql = '';
						}
				}

				if ($sql != '')
				{
					$sql = "INSERT INTO $table ($id_field, forum_id, auth_option_id, auth_setting) $sql";
					$db->sql_query($sql);
				}
			break;

			case 'update':
			case 'delete':
				foreach ($sql_subary as $sql)
				{
					$db->sql_query($sql);
					$sql = '';
				}
			break;
		}
		unset($sql_ary[$sql_type]);
	}
	unset($sql_ary);

}

/**
* Update the count of unread private messages for all users
*/
function update_unread_count()
{
	global $db;

	$sql = 'SELECT user_id, COUNT(msg_id) as num_messages
		FROM ' . PRIVMSGS_TO_TABLE . '
		WHERE pm_unread = 1
			AND folder_id <> ' . PRIVMSGS_OUTBOX . '
		GROUP BY user_id';
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$db->sql_query('UPDATE ' . USERS_TABLE . ' SET user_unread_privmsg = ' . $row['num_messages'] . '
			WHERE user_id = ' . $row['user_id']);
	}
	$db->sql_freeresult($result);
}

/**
* Add any of the pre-defined "special" groups which are missing from the database
*/
function add_default_groups()
{
	global $db;

	$default_groups = array(
		'GUESTS'			=> array('', 0, 0),
		'REGISTERED'		=> array('', 0, 0),
		'REGISTERED_COPPA'	=> array('', 0, 0),
		'GLOBAL_MODERATORS'	=> array('00AA00', 1, 0),
		'ADMINISTRATORS'	=> array('AA0000', 1, 1),
		'BOTS'				=> array('9E8DA7', 0, 0)
	);

	$sql = 'SELECT *
		FROM ' . GROUPS_TABLE . '
		WHERE ' . $db->sql_in_set('group_name', array_keys($default_groups));
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		unset($default_groups[strtoupper($row['group_name'])]);
	}
	$db->sql_freeresult($result);

	$sql_ary = array();

	foreach ($default_groups as $name => $data)
	{
		$sql_ary[] = array(
			'group_name'			=> (string) $name,
			'group_desc'			=> '',
			'group_desc_uid'		=> '',
			'group_desc_bitfield'	=> '',
			'group_type'			=> GROUP_SPECIAL,
			'group_colour'			=> (string) $data[0],
			'group_legend'			=> (int) $data[1],
			'group_founder_manage'	=> (int) $data[2]
		);
	}

	if (sizeof($sql_ary))
	{
		$db->sql_multi_insert(GROUPS_TABLE, $sql_ary);
	}
}


/**
* Sync post count. We might need to do this in batches.
*/
function sync_post_count($offset, $limit)
{
	global $db;
	$sql = 'SELECT COUNT(post_id) AS num_posts, poster_id
			FROM ' . POSTS_TABLE . '
			WHERE post_postcount = 1
			GROUP BY poster_id
			ORDER BY poster_id';
	$result = $db->sql_query_limit($sql, $limit, $offset);

	while ($row = $db->sql_fetchrow($result))
	{
		$db->sql_query('UPDATE ' . USERS_TABLE . " SET user_posts = {$row['num_posts']} WHERE user_id = {$row['poster_id']}");
	}
	$db->sql_freeresult($result);
}

/**
* Add the search bots into the database
* This code should be used in execute_last if the source database did not have bots
* If you are converting bots this function should not be called
* @todo We might want to look at sharing the bot list between the install code and this code for consistancy
*/
function add_bots()
{
	global $db, $convert, $user, $config, $phpbb_root_path, $phpEx;

	$db->sql_query($convert->truncate_statement . BOTS_TABLE);

	$sql = 'SELECT group_id FROM ' . GROUPS_TABLE . " WHERE group_name = 'BOTS'";
	$result = $db->sql_query($sql);
	$group_id = (int) $db->sql_fetchfield('group_id', false, $result);
	$db->sql_freeresult($result);

	if (!$group_id)
	{
		add_default_groups();

		$sql = 'SELECT group_id FROM ' . GROUPS_TABLE . " WHERE group_name = 'BOTS'";
		$result = $db->sql_query($sql);
		$group_id = (int) $db->sql_fetchfield('group_id', false, $result);
		$db->sql_freeresult($result);

		if (!$group_id)
		{
			global $install;
			$install->error($user->lang['CONV_ERROR_INCONSISTENT_GROUPS'], __LINE__, __FILE__);
		}
	}

	$bots = array(
		'AdsBot [Google]'			=> array('AdsBot-Google', ''),
		'Alexa [Bot]'				=> array('ia_archiver', ''),
		'Alta Vista [Bot]'			=> array('Scooter/', ''),
		'Ask Jeeves [Bot]'			=> array('Ask Jeeves', ''),
		'Baidu [Spider]'			=> array('Baiduspider+(', ''),
		'Exabot [Bot]'				=> array('Exabot/', ''),
		'FAST Enterprise [Crawler]'	=> array('FAST Enterprise Crawler', ''),
		'FAST WebCrawler [Crawler]'	=> array('FAST-WebCrawler/', ''),
		'Francis [Bot]'				=> array('http://www.neomo.de/', ''),
		'Gigabot [Bot]'				=> array('Gigabot/', ''),
		'Google Adsense [Bot]'		=> array('Mediapartners-Google', ''),
		'Google Desktop'			=> array('Google Desktop', ''),
		'Google Feedfetcher'		=> array('Feedfetcher-Google', ''),
		'Google [Bot]'				=> array('Googlebot', ''),
		'Heise IT-Markt [Crawler]'	=> array('heise-IT-Markt-Crawler', ''),
		'Heritrix [Crawler]'		=> array('heritrix/1.', ''),
		'IBM Research [Bot]'		=> array('ibm.com/cs/crawler', ''),
		'ICCrawler - ICjobs'		=> array('ICCrawler - ICjobs', ''),
		'ichiro [Crawler]'			=> array('ichiro/2', ''),
		'Majestic-12 [Bot]'			=> array('MJ12bot/', ''),
		'Metager [Bot]'				=> array('MetagerBot/', ''),
		'MSN NewsBlogs'				=> array('msnbot-NewsBlogs/', ''),
		'MSN [Bot]'					=> array('msnbot/', ''),
		'MSNbot Media'				=> array('msnbot-media/', ''),
		'NG-Search [Bot]'			=> array('NG-Search/', ''),
		'Nutch [Bot]'				=> array('http://lucene.apache.org/nutch/', ''),
		'Nutch/CVS [Bot]'			=> array('NutchCVS/', ''),
		'OmniExplorer [Bot]'		=> array('OmniExplorer_Bot/', ''),
		'Online link [Validator]'	=> array('online link validator', ''),
		'psbot [Picsearch]'			=> array('psbot/0', ''),
		'Seekport [Bot]'			=> array('Seekbot/', ''),
		'Sensis [Crawler]'			=> array('Sensis Web Crawler', ''),
		'SEO Crawler'				=> array('SEO search Crawler/', ''),
		'Seoma [Crawler]'			=> array('Seoma [SEO Crawler]', ''),
		'SEOSearch [Crawler]'		=> array('SEOsearch/', ''),
		'Snappy [Bot]'				=> array('Snappy/1.1 ( http://www.urltrends.com/ )', ''),
		'Steeler [Crawler]'			=> array('http://www.tkl.iis.u-tokyo.ac.jp/~crawler/', ''),
		'Synoo [Bot]'				=> array('SynooBot/', ''),
		'Telekom [Bot]'				=> array('crawleradmin.t-info@telekom.de', ''),
		'TurnitinBot [Bot]'			=> array('TurnitinBot/', ''),
		'Voyager [Bot]'				=> array('voyager/1.0', ''),
		'W3 [Sitesearch]'			=> array('W3 SiteSearch Crawler', ''),
		'W3C [Linkcheck]'			=> array('W3C-checklink/', ''),
		'W3C [Validator]'			=> array('W3C_*Validator', ''),
		'WiseNut [Bot]'				=> array('http://www.WISEnutbot.com', ''),
		'YaCy [Bot]'				=> array('yacybot', ''),
		'Yahoo MMCrawler [Bot]'		=> array('Yahoo-MMCrawler/', ''),
		'Yahoo Slurp [Bot]'			=> array('Yahoo! DE Slurp', ''),
		'Yahoo [Bot]'				=> array('Yahoo! Slurp', ''),
		'YahooSeeker [Bot]'			=> array('YahooSeeker/', ''),
	);

	if (!function_exists('user_add'))
	{
		include($phpbb_root_path . 'includes/functions_user.' . $phpEx);
	}

	foreach ($bots as $bot_name => $bot_ary)
	{
		$user_row = array(
			'user_type'				=> USER_IGNORE,
			'group_id'				=> $group_id,
			'username'				=> $bot_name,
			'user_regdate'			=> time(),
			'user_password'			=> '',
			'user_colour'			=> '9E8DA7',
			'user_email'			=> '',
			'user_lang'				=> $config['default_lang'],
			'user_style'			=> 1,
			'user_timezone'			=> 0,
			'user_allow_massemail'	=> 0,
		);

		$user_id = user_add($user_row);

		if ($user_id)
		{
			$sql = 'INSERT INTO ' . BOTS_TABLE . ' ' . $db->sql_build_array('INSERT', array(
				'bot_active'	=> 1,
				'bot_name'		=> $bot_name,
				'user_id'		=> $user_id,
				'bot_agent'		=> $bot_ary[0],
				'bot_ip'		=> $bot_ary[1])
			);
			$db->sql_query($sql);
		}
	}
}

/**
* Update any dynamic configuration variables after the conversion is finished
* @todo Confirm that this updates all relevant values since it has not necessarily been kept in sync with all changes
*/
function update_dynamic_config()
{
	global $db, $config;

	// Get latest username
	$sql = 'SELECT user_id, username, user_colour
		FROM ' . USERS_TABLE . '
		WHERE user_type IN (' . USER_NORMAL . ', ' . USER_FOUNDER . ')';

	if (!empty($config['increment_user_id']))
	{
		$sql .= ' AND user_id <> ' . $config['increment_user_id'];
	}

	$sql .= ' ORDER BY user_id DESC';

	$result = $db->sql_query_limit($sql, 1);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if ($row)
	{
		set_config('newest_user_id', $row['user_id'], true);
		set_config('newest_username', $row['username'], true);
		set_config('newest_user_colour', $row['user_colour'], true);
	}

//	Also do not reset record online user/date. There will be old data or the fresh data from the schema.
//	set_config('record_online_users', 1, true);
//	set_config('record_online_date', time(), true);

	$sql = 'SELECT COUNT(post_id) AS stat
		FROM ' . POSTS_TABLE . '
		WHERE post_approved = 1';
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	set_config('num_posts', (int) $row['stat'], true);

	$sql = 'SELECT COUNT(topic_id) AS stat
		FROM ' . TOPICS_TABLE . '
		WHERE topic_approved = 1';
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	set_config('num_topics', (int) $row['stat'], true);

	$sql = 'SELECT COUNT(user_id) AS stat
		FROM ' . USERS_TABLE . '
		WHERE user_type IN (' . USER_NORMAL . ',' . USER_FOUNDER . ')';
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	set_config('num_users', (int) $row['stat'], true);

	$sql = 'SELECT COUNT(attach_id) as stat
		FROM ' . ATTACHMENTS_TABLE . '
		WHERE is_orphan = 0';
	$result = $db->sql_query($sql);
	set_config('num_files', (int) $db->sql_fetchfield('stat'), true);
	$db->sql_freeresult($result);

	$sql = 'SELECT SUM(filesize) as stat
		FROM ' . ATTACHMENTS_TABLE . '
		WHERE is_orphan = 0';
	$result = $db->sql_query($sql);
	set_config('upload_dir_size', (int) $db->sql_fetchfield('stat'), true);
	$db->sql_freeresult($result);

	/**
	* We do not resync users post counts - this can be done by the admin after conversion if wanted.
	$sql = 'SELECT COUNT(post_id) AS num_posts, poster_id
		FROM ' . POSTS_TABLE . '
		WHERE post_postcount = 1
		GROUP BY poster_id';
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$db->sql_query('UPDATE ' . USERS_TABLE . " SET user_posts = {$row['num_posts']} WHERE user_id = {$row['poster_id']}");
	}
	$db->sql_freeresult($result);
	*/
}

/**
* Updates topics_posted entries
*/
function update_topics_posted()
{
	global $db, $config;

	switch ($db->sql_layer)
	{
		case 'sqlite':
		case 'firebird':
			$db->sql_query('DELETE FROM ' . TOPICS_POSTED_TABLE);
		break;

		default:
			$db->sql_query('TRUNCATE TABLE ' . TOPICS_POSTED_TABLE);
		break;
	}

	// This can get really nasty... therefore we only do the last six months
	$get_from_time = time() - (6 * 4 * 7 * 24 * 60 * 60);

	// Select forum ids, do not include categories
	$sql = 'SELECT forum_id
		FROM ' . FORUMS_TABLE . '
		WHERE forum_type <> ' . FORUM_CAT;
	$result = $db->sql_query($sql);

	$forum_ids = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$forum_ids[] = $row['forum_id'];
	}
	$db->sql_freeresult($result);

	// Any global announcements? ;)
	$forum_ids[] = 0;

	// Now go through the forums and get us some topics...
	foreach ($forum_ids as $forum_id)
	{
		$sql = 'SELECT p.poster_id, p.topic_id
			FROM ' . POSTS_TABLE . ' p, ' . TOPICS_TABLE . ' t
			WHERE t.forum_id = ' . $forum_id . '
				AND t.topic_moved_id = 0
				AND t.topic_last_post_time > ' . $get_from_time . '
				AND t.topic_id = p.topic_id
				AND p.poster_id <> ' . ANONYMOUS . '
			GROUP BY p.poster_id, p.topic_id';
		$result = $db->sql_query($sql);

		$posted = array();
		while ($row = $db->sql_fetchrow($result))
		{
			$posted[$row['poster_id']][] = $row['topic_id'];
		}
		$db->sql_freeresult($result);

		$sql_ary = array();
		foreach ($posted as $user_id => $topic_row)
		{
			foreach ($topic_row as $topic_id)
			{
				$sql_ary[] = array(
					'user_id'		=> (int) $user_id,
					'topic_id'		=> (int) $topic_id,
					'topic_posted'	=> 1,
				);
			}
		}
		unset($posted);

		if (sizeof($sql_ary))
		{
			$db->sql_multi_insert(TOPICS_POSTED_TABLE, $sql_ary);
		}
	}
}

/**
* Ensure that all users have a default group specified and update related information such as their colour
*/
function fix_empty_primary_groups()
{
	global $db;

	// Set group ids for users not already having it
	$sql = 'UPDATE ' . USERS_TABLE . ' SET group_id = ' . get_group_id('registered') . '
		WHERE group_id = 0 AND user_type = ' . USER_INACTIVE;
	$db->sql_query($sql);

	$sql = 'UPDATE ' . USERS_TABLE . ' SET group_id = ' . get_group_id('registered') . '
		WHERE group_id = 0 AND user_type = ' . USER_NORMAL;
	$db->sql_query($sql);

	$db->sql_query('UPDATE ' . USERS_TABLE . ' SET group_id = ' . get_group_id('guests') . ' WHERE user_id = ' . ANONYMOUS);

	$sql = 'SELECT user_id FROM ' . USER_GROUP_TABLE . ' WHERE group_id = ' . get_group_id('administrators');
	$result = $db->sql_query($sql);

	$user_ids = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$user_ids[] = $row['user_id'];
	}
	$db->sql_freeresult($result);

	if (sizeof($user_ids))
	{
		$db->sql_query('UPDATE ' . USERS_TABLE . ' SET group_id = ' . get_group_id('administrators') . '
			WHERE group_id = 0 AND ' . $db->sql_in_set('user_id', $user_ids));
	}

	$sql = 'SELECT user_id FROM ' . USER_GROUP_TABLE . ' WHERE group_id = ' . get_group_id('global_moderators');

	$user_ids = array();
	while ($row = $db->sql_fetchrow($result))
	{
		$user_ids[] = $row['user_id'];
	}
	$db->sql_freeresult($result);

	if (sizeof($user_ids))
	{
		$db->sql_query('UPDATE ' . USERS_TABLE . ' SET group_id = ' . get_group_id('global_moderators') . '
			WHERE group_id = 0 AND ' . $db->sql_in_set('user_id', $user_ids));
	}

	// Set user colour
	$sql = 'SELECT group_id, group_colour FROM ' . GROUPS_TABLE . "
		WHERE group_colour <> ''";
	$result = $db->sql_query($sql);

	while ($row = $db->sql_fetchrow($result))
	{
		$db->sql_query('UPDATE ' . USERS_TABLE . " SET user_colour = '{$row['group_colour']}' WHERE group_id = {$row['group_id']}");
	}
	$db->sql_freeresult($result);
}

/**
* Cleanly remove invalid user entries after converting the users table...
*/
function remove_invalid_users()
{
	global $convert, $db, $phpEx, $phpbb_root_path;

	// username_clean is UNIQUE
	$sql = 'SELECT user_id
		FROM ' . USERS_TABLE . "
		WHERE username_clean = ''";
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if ($row)
	{
		if (!function_exists('user_delete'))
		{
			include($phpbb_root_path . 'includes/functions_user.' . $phpEx);
		}

		user_delete('remove', $row['user_id']);
	}
}

function convert_bbcode($message, $convert_size = true, $extended_bbcodes = false)
{
	static $orig, $repl, $origx, $replx, $str_from, $str_to;

	if (empty($orig))
	{
		$orig = $repl = array();

		$orig[] = '#\[(php|sql)\](.*?)\[/(php|sql)\]#is';
		$repl[] = '[code]\2[/code]';

		$orig[] = '#\[font=[^\]]+\](.*?)\[/font\]#is';
		$repl[] = '\1';

		$orig[] = '#\[align=[a-z]+\](.*?)\[/align\]#is';
		$repl[] = '\1';

		$orig[] = '#\[/list=.*?\]#is';
		$repl[] = '[/list]';

		$origx = array(
			'#\[glow[^\]]+\](.*?)\[/glow\]#is',
			'#\[shadow[^\]]+\](.*?)\[/shadow\]#is',
			'#\[flash[^\]]+\](.*?)\[/flash\]#is'
		);

		$replx = array(
			'\1',
			'\1',
			'[url=\1]Flash[/url]'
		);

		$str_from = array(
			'[ftp]',	'[/ftp]',
			'[ftp=',	'[/ftp]',
			'[pre]',	'[/pre]',
			'[table]',	'[/table]',
			'[td]',		'[/td]',
			'[tr]',		'[/tr]',
			'[s]',		'[/s]',
			'[left]',	'[/left]',
			'[right]',	'[/right]',
			'[center]',	'[/center]',
			'[sub]',	'[/sub]',
			'[sup]',	'[/sup]',
			'[tt]',		'[/tt]',
			'[move]',	'[/move]',
			'[hr]'
		);

		$str_to = array(
			'[url]',	'[/url]',
			'[url=',	'[/url]',
			'[code]',	'[/code]',
			"\n",		'',
			'',			'',
			"\n",		'',
			'',			'',
			'',			'',
			'',			'',
			'',			'',
			'',			'',
			'',			'',
			'',			'',
			'',			'',
			"\n\n"
		);

		for ($i = 0; $i < sizeof($str_from); ++$i)
		{
			$origx[] = '#\\' . str_replace(']', '\\]', $str_from[$i]) . '#is';
			$replx[] = $str_to[$i];
		}
	}

	if (preg_match_all('#\[email=([^\]]+)\](.*?)\[/email\]#i', $message, $m))
	{
		for ($i = 0; $i < sizeof($m[1]); ++$i)
		{
			if ($m[1][$i] == $m[2][$i])
			{
				$message = str_replace($m[0][$i], '[email]' . $m[1][$i] . '[/email]', $message);
			}
			else
			{
				$message = str_replace($m[0][$i], $m[2][$i] . ' ([email]' . $m[1][$i] . '[/email])', $message);
			}
		}
	}

	if ($convert_size && preg_match('#\[size=[0-9]+\].*?\[/size\]#i', $message))
	{
		$size = array(9, 9, 12, 15, 18, 24, 29, 29, 29, 29);
		$message = preg_replace('#\[size=([0-9]+)\](.*?)\[/size\]#i', '[size=\1]\2[/size]', $message);
		$message = preg_replace('#\[size=[0-9]{2,}\](.*?)\[/size\]#i', '[size=29]\1[/size]', $message);

		for ($i = sizeof($size); $i; )
		{
			$i--;
			$message = str_replace('[size=' . $i . ']', '[size=' . $size[$i] . ']', $message);
		}
	}

	if ($extended_bbcodes)
	{
		$message = preg_replace($origx, $replx, $message);
	}

	$message = preg_replace($orig, $repl, $message);
	return $message;
}


function copy_file($src, $trg, $overwrite = false, $die_on_failure = true, $source_relative_path = true)
{
	global $convert, $phpbb_root_path, $config, $user, $db;

	if (substr($trg, -1) == '/')
	{
		$trg .= basename($src);
	}
	$src_path = relative_base($src, $source_relative_path, __LINE__, __FILE__);
	$trg_path = $trg;

	if (!$overwrite && @file_exists($trg_path))
	{
		return true;
	}

	if (!@file_exists($src_path))
	{
		return;
	}

	$path = $phpbb_root_path;
	$parts = explode('/', $trg);
	unset($parts[sizeof($parts) - 1]);

	for ($i = 0; $i < sizeof($parts); ++$i)
	{
		$path .= $parts[$i] . '/';

		if (!is_dir($path))
		{
			@mkdir($path, 0777);
		}
	}

	if (!is_writable($path))
	{
		@chmod($path, 0777);
	}

	if (!@copy($src_path, $phpbb_root_path . $trg_path))
	{
		$convert->p_master->error(sprintf($user->lang['COULD_NOT_COPY'], $src_path, $phpbb_root_path . $trg_path), __LINE__, __FILE__, !$die_on_failure);
		return;
	}

	if ($perm = @fileperms($src_path))
	{
		@chmod($phpbb_root_path . $trg_path, $perm);
	}

	return true;
}

function copy_dir($src, $trg, $copy_subdirs = true, $overwrite = false, $die_on_failure = true, $source_relative_path = true)
{
	global $convert, $phpbb_root_path, $config, $user, $db;

	$dirlist = $filelist = $bad_dirs = array();
	$src = path($src, $source_relative_path);
	$trg = path($trg);
	$src_path = relative_base($src, $source_relative_path, __LINE__, __FILE__);
	$trg_path = $phpbb_root_path . $trg;

	if (!is_dir($trg_path))
	{
		@mkdir($trg_path, 0777);
		@chmod($trg_path, 0777);
	}

	if (!@is_writable($trg_path))
	{
		$bad_dirs[] = path($config['script_path']) . $trg;
	}

	if ($handle = @opendir($src_path))
	{
		while ($entry = readdir($handle))
		{
			if ($entry[0] == '.' || $entry == 'CVS' || $entry == 'index.htm')
			{
				continue;
			}

			if (is_dir($src_path . $entry))
			{
				$dirlist[] = $entry;
			}
			else
			{
				$filelist[] = $entry;
			}
		}
		closedir($handle);
	}
	else if ($dir = @dir($src_path))
	{
		while ($entry = $dir->read())
		{
			if ($entry[0] == '.' || $entry == 'CVS' || $entry == 'index.htm')
			{
				continue;
			}

			if (is_dir($src_path . $entry))
			{
				$dirlist[] = $entry;
			}
			else
			{
				$filelist[] = $entry;
			}
		}
		$dir->close();
	}
	else
	{
		$convert->p_master->error(sprintf($user->lang['CONV_ERROR_COULD_NOT_READ'], relative_base($src, $source_relative_path)), __LINE__, __FILE__);
	}

	if ($copy_subdirs)
	{
		for ($i = 0; $i < sizeof($dirlist); ++$i)
		{
			$dir = $dirlist[$i];

			if ($dir == 'CVS')
			{
				continue;
			}

			if (!is_dir($trg_path . $dir))
			{
				@mkdir($trg_path . $dir, 0777);
				@chmod($trg_path . $dir, 0777);
			}

			if (!@is_writable($trg_path . $dir))
			{
				$bad_dirs[] = $trg . $dir;
				$bad_dirs[] = $trg_path . $dir;
			}

			if (!sizeof($bad_dirs))
			{
				copy_dir($src . $dir, $trg . $dir, true, $overwrite, $die_on_failure, $source_relative_path);
			}
		}
	}

	if (sizeof($bad_dirs))
	{
		$str = (sizeof($bad_dirs) == 1) ? $user->lang['MAKE_FOLDER_WRITABLE'] : $user->lang['MAKE_FOLDERS_WRITABLE'];
		sort($bad_dirs);
		$convert->p_master->error(sprintf($str, implode('<br />', $bad_dirs)), __LINE__, __FILE__);
	}

	for ($i = 0; $i < sizeof($filelist); ++$i)
	{
		copy_file($src . $filelist[$i], $trg . $filelist[$i], $overwrite, $die_on_failure, $source_relative_path);
	}
}

function relative_base($path, $is_relative = true, $line = false, $file = false)
{
	global $convert, $phpbb_root_path, $config, $user, $db;

	if (!$is_relative)
	{
		return $path;
	}

	if (empty($convert->options['forum_path']) && $is_relative)
	{
		$line = $line ? $line : __LINE__;
		$file = $file ? $file : __FILE__;

		$convert->p_master->error($user->lang['CONV_ERROR_NO_FORUM_PATH'], $line, $file);
	}

	return $convert->options['forum_path'] . '/' . $path;
}

function get_smiley_display()
{
	static $smiley_count = 0;
	$smiley_count++;
	return ($smiley_count < 50) ? 1 : 0;
}


function fill_dateformat($user_dateformat)
{
	global $config;
	
	return ((empty($user_dateformat)) ? $config['default_dateformat'] : $user_dateformat);
}



?>