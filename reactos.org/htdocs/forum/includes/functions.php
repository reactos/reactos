<?php
/**
*
* @package phpBB3
* @version $Id: functions.php 8491 2008-04-04 11:41:58Z acydburn $
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

// Common global functions

/**
* set_var
*
* Set variable, used by {@link request_var the request_var function}
*
* @access private
*/
function set_var(&$result, $var, $type, $multibyte = false)
{
	settype($var, $type);
	$result = $var;

	if ($type == 'string')
	{
		$result = trim(htmlspecialchars(str_replace(array("\r\n", "\r"), array("\n", "\n"), $result), ENT_COMPAT, 'UTF-8'));

		if (!empty($result))
		{
			// Make sure multibyte characters are wellformed
			if ($multibyte)
			{
				if (!preg_match('/^./u', $result))
				{
					$result = '';
				}
			}
			else
			{
				// no multibyte, allow only ASCII (0-127)
				$result = preg_replace('/[\x80-\xFF]/', '?', $result);
			}
		}

		$result = (STRIP) ? stripslashes($result) : $result;
	}
}

/**
* request_var
*
* Used to get passed variable
*/
function request_var($var_name, $default, $multibyte = false, $cookie = false)
{
	if (!$cookie && isset($_COOKIE[$var_name]))
	{
		if (!isset($_GET[$var_name]) && !isset($_POST[$var_name]))
		{
			return (is_array($default)) ? array() : $default;
		}
		$_REQUEST[$var_name] = isset($_POST[$var_name]) ? $_POST[$var_name] : $_GET[$var_name];
	}

	if (!isset($_REQUEST[$var_name]) || (is_array($_REQUEST[$var_name]) && !is_array($default)) || (is_array($default) && !is_array($_REQUEST[$var_name])))
	{
		return (is_array($default)) ? array() : $default;
	}

	$var = $_REQUEST[$var_name];
	if (!is_array($default))
	{
		$type = gettype($default);
	}
	else
	{
		list($key_type, $type) = each($default);
		$type = gettype($type);
		$key_type = gettype($key_type);
		if ($type == 'array')
		{
			reset($default);
			$default = current($default);
			list($sub_key_type, $sub_type) = each($default);
			$sub_type = gettype($sub_type);
			$sub_type = ($sub_type == 'array') ? 'NULL' : $sub_type;
			$sub_key_type = gettype($sub_key_type);
		}
	}

	if (is_array($var))
	{
		$_var = $var;
		$var = array();

		foreach ($_var as $k => $v)
		{
			set_var($k, $k, $key_type);
			if ($type == 'array' && is_array($v))
			{
				foreach ($v as $_k => $_v)
				{
					if (is_array($_v))
					{
						$_v = null;
					}
					set_var($_k, $_k, $sub_key_type);
					set_var($var[$k][$_k], $_v, $sub_type, $multibyte);
				}
			}
			else
			{
				if ($type == 'array' || is_array($v))
				{
					$v = null;
				}
				set_var($var[$k], $v, $type, $multibyte);
			}
		}
	}
	else
	{
		set_var($var, $var, $type, $multibyte);
	}

	return $var;
}

/**
* Set config value. Creates missing config entry.
*/
function set_config($config_name, $config_value, $is_dynamic = false)
{
	global $db, $cache, $config;

	$sql = 'UPDATE ' . CONFIG_TABLE . "
		SET config_value = '" . $db->sql_escape($config_value) . "'
		WHERE config_name = '" . $db->sql_escape($config_name) . "'";
	$db->sql_query($sql);

	if (!$db->sql_affectedrows() && !isset($config[$config_name]))
	{
		$sql = 'INSERT INTO ' . CONFIG_TABLE . ' ' . $db->sql_build_array('INSERT', array(
			'config_name'	=> $config_name,
			'config_value'	=> $config_value,
			'is_dynamic'	=> ($is_dynamic) ? 1 : 0));
		$db->sql_query($sql);
	}

	$config[$config_name] = $config_value;

	if (!$is_dynamic)
	{
		$cache->destroy('config');
	}
}

/**
* Generates an alphanumeric random string of given length
*/
function gen_rand_string($num_chars = 8)
{
	$rand_str = unique_id();
	$rand_str = str_replace('0', 'Z', strtoupper(base_convert($rand_str, 16, 35)));

	return substr($rand_str, 0, $num_chars);
}

/**
* Return unique id
* @param string $extra additional entropy
*/
function unique_id($extra = 'c')
{
	static $dss_seeded = false;
	global $config;

	$val = $config['rand_seed'] . microtime();
	$val = md5($val);
	$config['rand_seed'] = md5($config['rand_seed'] . $val . $extra);

	if ($dss_seeded !== true && ($config['rand_seed_last_update'] < time() - rand(1,10)))
	{
		set_config('rand_seed', $config['rand_seed'], true);
		set_config('rand_seed_last_update', time(), true);
		$dss_seeded = true;
	}

	return substr($val, 4, 16);
}

/**
* Return formatted string for filesizes
*/
function get_formatted_filesize($bytes, $add_size_lang = true)
{
	global $user;

	if ($bytes >= pow(2, 20))
	{
		return ($add_size_lang) ? round($bytes / 1024 / 1024, 2) . ' ' . $user->lang['MIB'] : round($bytes / 1024 / 1024, 2);
	}

	if ($bytes >= pow(2, 10))
	{
		return ($add_size_lang) ? round($bytes / 1024, 2) . ' ' . $user->lang['KIB'] : round($bytes / 1024, 2);
	}

	return ($add_size_lang) ? ($bytes) . ' ' . $user->lang['BYTES'] : ($bytes);
}

/**
* Determine whether we are approaching the maximum execution time. Should be called once
* at the beginning of the script in which it's used.
* @return	bool	Either true if the maximum execution time is nearly reached, or false
*					if some time is still left.
*/
function still_on_time($extra_time = 15)
{
	static $max_execution_time, $start_time;

	$time = explode(' ', microtime());
	$current_time = $time[0] + $time[1];

	if (empty($max_execution_time))
	{
		$max_execution_time = (function_exists('ini_get')) ? (int) @ini_get('max_execution_time') : (int) @get_cfg_var('max_execution_time');

		// If zero, then set to something higher to not let the user catch the ten seconds barrier.
		if ($max_execution_time === 0)
		{
			$max_execution_time = 50 + $extra_time;
		}

		$max_execution_time = min(max(10, ($max_execution_time - $extra_time)), 50);

		// For debugging purposes
		// $max_execution_time = 10;

		global $starttime;
		$start_time = (empty($starttime)) ? $current_time : $starttime;
	}

	return (ceil($current_time - $start_time) < $max_execution_time) ? true : false;
}

/**
*
* @version Version 0.1 / $Id: functions.php 8491 2008-04-04 11:41:58Z acydburn $
*
* Portable PHP password hashing framework.
*
* Written by Solar Designer <solar at openwall.com> in 2004-2006 and placed in
* the public domain.
*
* There's absolutely no warranty.
*
* The homepage URL for this framework is:
*
*	http://www.openwall.com/phpass/
*
* Please be sure to update the Version line if you edit this file in any way.
* It is suggested that you leave the main version number intact, but indicate
* your project name (after the slash) and add your own revision information.
*
* Please do not change the "private" password hashing method implemented in
* here, thereby making your hashes incompatible.  However, if you must, please
* change the hash type identifier (the "$P$") to something different.
*
* Obviously, since this code is in the public domain, the above are not
* requirements (there can be none), but merely suggestions.
*
*
* Hash the password
*/
function phpbb_hash($password)
{
	$itoa64 = './0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';

	$random_state = unique_id();
	$random = '';
	$count = 6;

	if (($fh = @fopen('/dev/urandom', 'rb')))
	{
		$random = fread($fh, $count);
		fclose($fh);
	}

	if (strlen($random) < $count)
	{
		$random = '';

		for ($i = 0; $i < $count; $i += 16)
		{
			$random_state = md5(unique_id() . $random_state);
			$random .= pack('H*', md5($random_state));
		}
		$random = substr($random, 0, $count);
	}

	$hash = _hash_crypt_private($password, _hash_gensalt_private($random, $itoa64), $itoa64);

	if (strlen($hash) == 34)
	{
		return $hash;
	}

	return md5($password);
}

/**
* Check for correct password
*/
function phpbb_check_hash($password, $hash)
{
	$itoa64 = './0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
	if (strlen($hash) == 34)
	{
		return (_hash_crypt_private($password, $hash, $itoa64) === $hash) ? true : false;
	}

	return (md5($password) === $hash) ? true : false;
}

/**
* Generate salt for hash generation
*/
function _hash_gensalt_private($input, &$itoa64, $iteration_count_log2 = 6)
{
	if ($iteration_count_log2 < 4 || $iteration_count_log2 > 31)
	{
		$iteration_count_log2 = 8;
	}

	$output = '$H$';
	$output .= $itoa64[min($iteration_count_log2 + ((PHP_VERSION >= 5) ? 5 : 3), 30)];
	$output .= _hash_encode64($input, 6, $itoa64);

	return $output;
}

/**
* Encode hash
*/
function _hash_encode64($input, $count, &$itoa64)
{
	$output = '';
	$i = 0;

	do
	{
		$value = ord($input[$i++]);
		$output .= $itoa64[$value & 0x3f];

		if ($i < $count)
		{
			$value |= ord($input[$i]) << 8;
		}

		$output .= $itoa64[($value >> 6) & 0x3f];

		if ($i++ >= $count)
		{
			break;
		}

		if ($i < $count)
		{
			$value |= ord($input[$i]) << 16;
		}

		$output .= $itoa64[($value >> 12) & 0x3f];

		if ($i++ >= $count)
		{
			break;
		}

		$output .= $itoa64[($value >> 18) & 0x3f];
	}
	while ($i < $count);

	return $output;
}

/**
* The crypt function/replacement
*/
function _hash_crypt_private($password, $setting, &$itoa64)
{
	$output = '*';

	// Check for correct hash
	if (substr($setting, 0, 3) != '$H$')
	{
		return $output;
	}

	$count_log2 = strpos($itoa64, $setting[3]);

	if ($count_log2 < 7 || $count_log2 > 30)
	{
		return $output;
	}

	$count = 1 << $count_log2;
	$salt = substr($setting, 4, 8);

	if (strlen($salt) != 8)
	{
		return $output;
	}

	/**
	* We're kind of forced to use MD5 here since it's the only
	* cryptographic primitive available in all versions of PHP
	* currently in use.  To implement our own low-level crypto
	* in PHP would result in much worse performance and
	* consequently in lower iteration counts and hashes that are
	* quicker to crack (by non-PHP code).
	*/
	if (PHP_VERSION >= 5)
	{
		$hash = md5($salt . $password, true);
		do
		{
			$hash = md5($hash . $password, true);
		}
		while (--$count);
	}
	else
	{
		$hash = pack('H*', md5($salt . $password));
		do
		{
			$hash = pack('H*', md5($hash . $password));
		}
		while (--$count);
	}

	$output = substr($setting, 0, 12);
	$output .= _hash_encode64($hash, 16, $itoa64);

	return $output;
}

// Compatibility functions

if (!function_exists('array_combine'))
{
	/**
	* A wrapper for the PHP5 function array_combine()
	* @param array $keys contains keys for the resulting array
	* @param array $values contains values for the resulting array
	*
	* @return Returns an array by using the values from the keys array as keys and the
	* 	values from the values array as the corresponding values. Returns false if the
	* 	number of elements for each array isn't equal or if the arrays are empty.
	*/
	function array_combine($keys, $values)
	{
		$keys = array_values($keys);
		$values = array_values($values);

		$n = sizeof($keys);
		$m = sizeof($values);
		if (!$n || !$m || ($n != $m))
		{
			return false;
		}

		$combined = array();
		for ($i = 0; $i < $n; $i++)
		{
			$combined[$keys[$i]] = $values[$i];
		}
		return $combined;
	}
}

if (!function_exists('str_split'))
{
	/**
	* A wrapper for the PHP5 function str_split()
	* @param array $string contains the string to be converted
	* @param array $split_length contains the length of each chunk
	*
	* @return  Converts a string to an array. If the optional split_length parameter is specified,
	*  	the returned array will be broken down into chunks with each being split_length in length,
	*  	otherwise each chunk will be one character in length. FALSE is returned if split_length is
	*  	less than 1. If the split_length length exceeds the length of string, the entire string is
	*  	returned as the first (and only) array element.
	*/
	function str_split($string, $split_length = 1)
	{
		if ($split_length < 1)
		{
			return false;
		}
		else if ($split_length >= strlen($string))
		{
			return array($string);
		}
		else
		{
			preg_match_all('#.{1,' . $split_length . '}#s', $string, $matches);
			return $matches[0];
		}
	}
}

if (!function_exists('stripos'))
{
	/**
	* A wrapper for the PHP5 function stripos
	* Find position of first occurrence of a case-insensitive string
	*
	* @param string $haystack is the string to search in
	* @param string $needle is the string to search for
	*
	* @return mixed Returns the numeric position of the first occurrence of needle in the haystack string. Unlike strpos(), stripos() is case-insensitive.
	* Note that the needle may be a string of one or more characters.
	* If needle is not found, stripos() will return boolean FALSE.
	*/
	function stripos($haystack, $needle)
	{
		if (preg_match('#' . preg_quote($needle, '#') . '#i', $haystack, $m))
		{
			return strpos($haystack, $m[0]);
		}

		return false;
	}
}

/**
* Checks if a path ($path) is absolute or relative
*
* @param string $path Path to check absoluteness of
* @return boolean
*/
function is_absolute($path)
{
	return ($path[0] == '/' || (DIRECTORY_SEPARATOR == '\\' && preg_match('#^[a-z]:/#i', $path))) ? true : false;
}

/**
* @author Chris Smith <chris@project-minerva.org>
* @copyright 2006 Project Minerva Team
* @param string $path The path which we should attempt to resolve.
* @return mixed
*/
function phpbb_own_realpath($path)
{
	// Now to perform funky shizzle

	// Switch to use UNIX slashes
	$path = str_replace(DIRECTORY_SEPARATOR, '/', $path);
	$path_prefix = '';

	// Determine what sort of path we have
	if (is_absolute($path))
	{
		$absolute = true;

		if ($path[0] == '/')
		{
			// Absolute path, *NIX style
			$path_prefix = '';
		}
		else
		{
			// Absolute path, Windows style
			// Remove the drive letter and colon
			$path_prefix = $path[0] . ':';
			$path = substr($path, 2);
		}
	}
	else
	{
		// Relative Path
		// Prepend the current working directory
		if (function_exists('getcwd'))
		{
			// This is the best method, hopefully it is enabled!
			$path = str_replace(DIRECTORY_SEPARATOR, '/', getcwd()) . '/' . $path;
			$absolute = true;
			if (preg_match('#^[a-z]:#i', $path))
			{
				$path_prefix = $path[0] . ':';
				$path = substr($path, 2);
			}
			else
			{
				$path_prefix = '';
			}
		}
		else if (isset($_SERVER['SCRIPT_FILENAME']) && !empty($_SERVER['SCRIPT_FILENAME']))
		{
			// Warning: If chdir() has been used this will lie!
			// Warning: This has some problems sometime (CLI can create them easily)
			$path = str_replace(DIRECTORY_SEPARATOR, '/', dirname($_SERVER['SCRIPT_FILENAME'])) . '/' . $path;
			$absolute = true;
			$path_prefix = '';
		}
		else
		{
			// We have no way of getting the absolute path, just run on using relative ones.
			$absolute = false;
			$path_prefix = '.';
		}
	}

	// Remove any repeated slashes
	$path = preg_replace('#/{2,}#', '/', $path);

	// Remove the slashes from the start and end of the path
	$path = trim($path, '/');

	// Break the string into little bits for us to nibble on
	$bits = explode('/', $path);

	// Remove any . in the path, renumber array for the loop below
	$bits = array_values(array_diff($bits, array('.')));

	// Lets get looping, run over and resolve any .. (up directory)
	for ($i = 0, $max = sizeof($bits); $i < $max; $i++)
	{
		// @todo Optimise
		if ($bits[$i] == '..' )
		{
			if (isset($bits[$i - 1]))
			{
				if ($bits[$i - 1] != '..')
				{
					// We found a .. and we are able to traverse upwards, lets do it!
					unset($bits[$i]);
					unset($bits[$i - 1]);
					$i -= 2;
					$max -= 2;
					$bits = array_values($bits);
				}
			}
			else if ($absolute) // ie. !isset($bits[$i - 1]) && $absolute
			{
				// We have an absolute path trying to descend above the root of the filesystem
				// ... Error!
				return false;
			}
		}
	}

	// Prepend the path prefix
	array_unshift($bits, $path_prefix);

	$resolved = '';

	$max = sizeof($bits) - 1;

	// Check if we are able to resolve symlinks, Windows cannot.
	$symlink_resolve = (function_exists('readlink')) ? true : false;

	foreach ($bits as $i => $bit)
	{
		if (@is_dir("$resolved/$bit") || ($i == $max && @is_file("$resolved/$bit")))
		{
			// Path Exists
			if ($symlink_resolve && is_link("$resolved/$bit") && ($link = readlink("$resolved/$bit")))
			{
				// Resolved a symlink.
				$resolved = $link . (($i == $max) ? '' : '/');
				continue;
			}
		}
		else
		{
			// Something doesn't exist here!
			// This is correct realpath() behaviour but sadly open_basedir and safe_mode make this problematic
			// return false;
		}
		$resolved .= $bit . (($i == $max) ? '' : '/');
	}

	// @todo If the file exists fine and open_basedir only has one path we should be able to prepend it
	// because we must be inside that basedir, the question is where...
	// @internal The slash in is_dir() gets around an open_basedir restriction
	if (!@file_exists($resolved) || (!is_dir($resolved . '/') && !is_file($resolved)))
	{
		return false;
	}

	// Put the slashes back to the native operating systems slashes
	$resolved = str_replace('/', DIRECTORY_SEPARATOR, $resolved);

	// Check for DIRECTORY_SEPARATOR at the end (and remove it!)
	if (substr($resolved, -1) == DIRECTORY_SEPARATOR)
	{
		return substr($resolved, 0, -1);
	}

	return $resolved; // We got here, in the end!
}

if (!function_exists('realpath'))
{
	/**
	* A wrapper for realpath
	* @ignore
	*/
	function phpbb_realpath($path)
	{
		return phpbb_own_realpath($path);
	}
}
else
{
	/**
	* A wrapper for realpath
	*/
	function phpbb_realpath($path)
	{
		$realpath = realpath($path);

		// Strangely there are provider not disabling realpath but returning strange values. :o
		// We at least try to cope with them.
		if ($realpath === $path || $realpath === false)
		{
			return phpbb_own_realpath($path);
		}

		// Check for DIRECTORY_SEPARATOR at the end (and remove it!)
		if (substr($realpath, -1) == DIRECTORY_SEPARATOR)
		{
			$realpath = substr($realpath, 0, -1);
		}

		return $realpath;
	}
}

if (!function_exists('htmlspecialchars_decode'))
{
	/**
	* A wrapper for htmlspecialchars_decode
	* @ignore
	*/
	function htmlspecialchars_decode($string, $quote_style = ENT_COMPAT)
	{
		return strtr($string, array_flip(get_html_translation_table(HTML_SPECIALCHARS, $quote_style)));
	}
}

// functions used for building option fields

/**
* Pick a language, any language ...
*/
function language_select($default = '')
{
	global $db;

	$sql = 'SELECT lang_iso, lang_local_name
		FROM ' . LANG_TABLE . '
		ORDER BY lang_english_name';
	$result = $db->sql_query($sql);

	$lang_options = '';
	while ($row = $db->sql_fetchrow($result))
	{
		$selected = ($row['lang_iso'] == $default) ? ' selected="selected"' : '';
		$lang_options .= '<option value="' . $row['lang_iso'] . '"' . $selected . '>' . $row['lang_local_name'] . '</option>';
	}
	$db->sql_freeresult($result);

	return $lang_options;
}

/**
* Pick a template/theme combo,
*/
function style_select($default = '', $all = false)
{
	global $db;

	$sql_where = (!$all) ? 'WHERE style_active = 1 ' : '';
	$sql = 'SELECT style_id, style_name
		FROM ' . STYLES_TABLE . "
		$sql_where
		ORDER BY style_name";
	$result = $db->sql_query($sql);

	$style_options = '';
	while ($row = $db->sql_fetchrow($result))
	{
		$selected = ($row['style_id'] == $default) ? ' selected="selected"' : '';
		$style_options .= '<option value="' . $row['style_id'] . '"' . $selected . '>' . $row['style_name'] . '</option>';
	}
	$db->sql_freeresult($result);

	return $style_options;
}

/**
* Pick a timezone
*/
function tz_select($default = '', $truncate = false)
{
	global $user;

	$tz_select = '';
	foreach ($user->lang['tz_zones'] as $offset => $zone)
	{
		if ($truncate)
		{
			$zone_trunc = truncate_string($zone, 50, false, '...');
		}
		else
		{
			$zone_trunc = $zone;
		}

		if (is_numeric($offset))
		{
			$selected = ($offset == $default) ? ' selected="selected"' : '';
			$tz_select .= '<option title="'.$zone.'" value="' . $offset . '"' . $selected . '>' . $zone_trunc . '</option>';
		}
	}

	return $tz_select;
}

// Functions handling topic/post tracking/marking

/**
* Marks a topic/forum as read
* Marks a topic as posted to
*
* @param int $user_id can only be used with $mode == 'post'
*/
function markread($mode, $forum_id = false, $topic_id = false, $post_time = 0, $user_id = 0)
{
	global $db, $user, $config;

	if ($mode == 'all')
	{
		if ($forum_id === false || !sizeof($forum_id))
		{
			if ($config['load_db_lastread'] && $user->data['is_registered'])
			{
				// Mark all forums read (index page)
				$db->sql_query('DELETE FROM ' . TOPICS_TRACK_TABLE . " WHERE user_id = {$user->data['user_id']}");
				$db->sql_query('DELETE FROM ' . FORUMS_TRACK_TABLE . " WHERE user_id = {$user->data['user_id']}");
				$db->sql_query('UPDATE ' . USERS_TABLE . ' SET user_lastmark = ' . time() . " WHERE user_id = {$user->data['user_id']}");
			}
			else if ($config['load_anon_lastread'] || $user->data['is_registered'])
			{
				$tracking_topics = (isset($_COOKIE[$config['cookie_name'] . '_track'])) ? ((STRIP) ? stripslashes($_COOKIE[$config['cookie_name'] . '_track']) : $_COOKIE[$config['cookie_name'] . '_track']) : '';
				$tracking_topics = ($tracking_topics) ? tracking_unserialize($tracking_topics) : array();

				unset($tracking_topics['tf']);
				unset($tracking_topics['t']);
				unset($tracking_topics['f']);
				$tracking_topics['l'] = base_convert(time() - $config['board_startdate'], 10, 36);

				$user->set_cookie('track', tracking_serialize($tracking_topics), time() + 31536000);
				$_COOKIE[$config['cookie_name'] . '_track'] = (STRIP) ? addslashes(tracking_serialize($tracking_topics)) : tracking_serialize($tracking_topics);

				unset($tracking_topics);

				if ($user->data['is_registered'])
				{
					$db->sql_query('UPDATE ' . USERS_TABLE . ' SET user_lastmark = ' . time() . " WHERE user_id = {$user->data['user_id']}");
				}
			}
		}

		return;
	}
	else if ($mode == 'topics')
	{
		// Mark all topics in forums read
		if (!is_array($forum_id))
		{
			$forum_id = array($forum_id);
		}

		// Add 0 to forums array to mark global announcements correctly
		$forum_id[] = 0;

		if ($config['load_db_lastread'] && $user->data['is_registered'])
		{
			$sql = 'DELETE FROM ' . TOPICS_TRACK_TABLE . "
				WHERE user_id = {$user->data['user_id']}
					AND " . $db->sql_in_set('forum_id', $forum_id);
			$db->sql_query($sql);

			$sql = 'SELECT forum_id
				FROM ' . FORUMS_TRACK_TABLE . "
				WHERE user_id = {$user->data['user_id']}
					AND " . $db->sql_in_set('forum_id', $forum_id);
			$result = $db->sql_query($sql);

			$sql_update = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$sql_update[] = $row['forum_id'];
			}
			$db->sql_freeresult($result);

			if (sizeof($sql_update))
			{
				$sql = 'UPDATE ' . FORUMS_TRACK_TABLE . '
					SET mark_time = ' . time() . "
					WHERE user_id = {$user->data['user_id']}
						AND " . $db->sql_in_set('forum_id', $sql_update);
				$db->sql_query($sql);
			}

			if ($sql_insert = array_diff($forum_id, $sql_update))
			{
				$sql_ary = array();
				foreach ($sql_insert as $f_id)
				{
					$sql_ary[] = array(
						'user_id'	=> (int) $user->data['user_id'],
						'forum_id'	=> (int) $f_id,
						'mark_time'	=> time()
					);
				}

				$db->sql_multi_insert(FORUMS_TRACK_TABLE, $sql_ary);
			}
		}
		else if ($config['load_anon_lastread'] || $user->data['is_registered'])
		{
			$tracking = (isset($_COOKIE[$config['cookie_name'] . '_track'])) ? ((STRIP) ? stripslashes($_COOKIE[$config['cookie_name'] . '_track']) : $_COOKIE[$config['cookie_name'] . '_track']) : '';
			$tracking = ($tracking) ? tracking_unserialize($tracking) : array();

			foreach ($forum_id as $f_id)
			{
				$topic_ids36 = (isset($tracking['tf'][$f_id])) ? $tracking['tf'][$f_id] : array();

				if (isset($tracking['tf'][$f_id]))
				{
					unset($tracking['tf'][$f_id]);
				}

				foreach ($topic_ids36 as $topic_id36)
				{
					unset($tracking['t'][$topic_id36]);
				}

				if (isset($tracking['f'][$f_id]))
				{
					unset($tracking['f'][$f_id]);
				}

				$tracking['f'][$f_id] = base_convert(time() - $config['board_startdate'], 10, 36);
			}

			if (isset($tracking['tf']) && empty($tracking['tf']))
			{
				unset($tracking['tf']);
			}

			$user->set_cookie('track', tracking_serialize($tracking), time() + 31536000);
			$_COOKIE[$config['cookie_name'] . '_track'] = (STRIP) ? addslashes(tracking_serialize($tracking)) : tracking_serialize($tracking);

			unset($tracking);
		}

		return;
	}
	else if ($mode == 'topic')
	{
		if ($topic_id === false || $forum_id === false)
		{
			return;
		}

		if ($config['load_db_lastread'] && $user->data['is_registered'])
		{
			$sql = 'UPDATE ' . TOPICS_TRACK_TABLE . '
				SET mark_time = ' . (($post_time) ? $post_time : time()) . "
				WHERE user_id = {$user->data['user_id']}
					AND topic_id = $topic_id";
			$db->sql_query($sql);

			// insert row
			if (!$db->sql_affectedrows())
			{
				$db->sql_return_on_error(true);

				$sql_ary = array(
					'user_id'		=> (int) $user->data['user_id'],
					'topic_id'		=> (int) $topic_id,
					'forum_id'		=> (int) $forum_id,
					'mark_time'		=> ($post_time) ? (int) $post_time : time(),
				);

				$db->sql_query('INSERT INTO ' . TOPICS_TRACK_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));

				$db->sql_return_on_error(false);
			}
		}
		else if ($config['load_anon_lastread'] || $user->data['is_registered'])
		{
			$tracking = (isset($_COOKIE[$config['cookie_name'] . '_track'])) ? ((STRIP) ? stripslashes($_COOKIE[$config['cookie_name'] . '_track']) : $_COOKIE[$config['cookie_name'] . '_track']) : '';
			$tracking = ($tracking) ? tracking_unserialize($tracking) : array();

			$topic_id36 = base_convert($topic_id, 10, 36);

			if (!isset($tracking['t'][$topic_id36]))
			{
				$tracking['tf'][$forum_id][$topic_id36] = true;
			}

			$post_time = ($post_time) ? $post_time : time();
			$tracking['t'][$topic_id36] = base_convert($post_time - $config['board_startdate'], 10, 36);

			// If the cookie grows larger than 10000 characters we will remove the smallest value
			// This can result in old topics being unread - but most of the time it should be accurate...
			if (isset($_COOKIE[$config['cookie_name'] . '_track']) && strlen($_COOKIE[$config['cookie_name'] . '_track']) > 10000)
			{
				//echo 'Cookie grown too large' . print_r($tracking, true);

				// We get the ten most minimum stored time offsets and its associated topic ids
				$time_keys = array();
				for ($i = 0; $i < 10 && sizeof($tracking['t']); $i++)
				{
					$min_value = min($tracking['t']);
					$m_tkey = array_search($min_value, $tracking['t']);
					unset($tracking['t'][$m_tkey]);

					$time_keys[$m_tkey] = $min_value;
				}

				// Now remove the topic ids from the array...
				foreach ($tracking['tf'] as $f_id => $topic_id_ary)
				{
					foreach ($time_keys as $m_tkey => $min_value)
					{
						if (isset($topic_id_ary[$m_tkey]))
						{
							$tracking['f'][$f_id] = $min_value;
							unset($tracking['tf'][$f_id][$m_tkey]);
						}
					}
				}

				if ($user->data['is_registered'])
				{
					$user->data['user_lastmark'] = intval(base_convert(max($time_keys) + $config['board_startdate'], 36, 10));
					$db->sql_query('UPDATE ' . USERS_TABLE . ' SET user_lastmark = ' . $user->data['user_lastmark'] . " WHERE user_id = {$user->data['user_id']}");
				}
				else
				{
					$tracking['l'] = max($time_keys);
				}
			}

			$user->set_cookie('track', tracking_serialize($tracking), time() + 31536000);
			$_COOKIE[$config['cookie_name'] . '_track'] = (STRIP) ? addslashes(tracking_serialize($tracking)) : tracking_serialize($tracking);
		}

		return;
	}
	else if ($mode == 'post')
	{
		if ($topic_id === false)
		{
			return;
		}

		$use_user_id = (!$user_id) ? $user->data['user_id'] : $user_id;

		if ($config['load_db_track'] && $use_user_id != ANONYMOUS)
		{
			$db->sql_return_on_error(true);

			$sql_ary = array(
				'user_id'		=> (int) $use_user_id,
				'topic_id'		=> (int) $topic_id,
				'topic_posted'	=> 1
			);

			$db->sql_query('INSERT INTO ' . TOPICS_POSTED_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));

			$db->sql_return_on_error(false);
		}

		return;
	}
}

/**
* Get topic tracking info by using already fetched info
*/
function get_topic_tracking($forum_id, $topic_ids, &$rowset, $forum_mark_time, $global_announce_list = false)
{
	global $config, $user;

	$last_read = array();

	if (!is_array($topic_ids))
	{
		$topic_ids = array($topic_ids);
	}

	foreach ($topic_ids as $topic_id)
	{
		if (!empty($rowset[$topic_id]['mark_time']))
		{
			$last_read[$topic_id] = $rowset[$topic_id]['mark_time'];
		}
	}

	$topic_ids = array_diff($topic_ids, array_keys($last_read));

	if (sizeof($topic_ids))
	{
		$mark_time = array();

		// Get global announcement info
		if ($global_announce_list && sizeof($global_announce_list))
		{
			if (!isset($forum_mark_time[0]))
			{
				global $db;

				$sql = 'SELECT mark_time
					FROM ' . FORUMS_TRACK_TABLE . "
					WHERE user_id = {$user->data['user_id']}
						AND forum_id = 0";
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if ($row)
				{
					$mark_time[0] = $row['mark_time'];
				}
			}
			else
			{
				if ($forum_mark_time[0] !== false)
				{
					$mark_time[0] = $forum_mark_time[0];
				}
			}
		}

		if (!empty($forum_mark_time[$forum_id]) && $forum_mark_time[$forum_id] !== false)
		{
			$mark_time[$forum_id] = $forum_mark_time[$forum_id];
		}

		$user_lastmark = (isset($mark_time[$forum_id])) ? $mark_time[$forum_id] : $user->data['user_lastmark'];

		foreach ($topic_ids as $topic_id)
		{
			if ($global_announce_list && isset($global_announce_list[$topic_id]))
			{
				$last_read[$topic_id] = (isset($mark_time[0])) ? $mark_time[0] : $user_lastmark;
			}
			else
			{
				$last_read[$topic_id] = $user_lastmark;
			}
		}
	}

	return $last_read;
}

/**
* Get topic tracking info from db (for cookie based tracking only this function is used)
*/
function get_complete_topic_tracking($forum_id, $topic_ids, $global_announce_list = false)
{
	global $config, $user;

	$last_read = array();

	if (!is_array($topic_ids))
	{
		$topic_ids = array($topic_ids);
	}

	if ($config['load_db_lastread'] && $user->data['is_registered'])
	{
		global $db;

		$sql = 'SELECT topic_id, mark_time
			FROM ' . TOPICS_TRACK_TABLE . "
			WHERE user_id = {$user->data['user_id']}
				AND " . $db->sql_in_set('topic_id', $topic_ids);
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$last_read[$row['topic_id']] = $row['mark_time'];
		}
		$db->sql_freeresult($result);

		$topic_ids = array_diff($topic_ids, array_keys($last_read));

		if (sizeof($topic_ids))
		{
			$sql = 'SELECT forum_id, mark_time
				FROM ' . FORUMS_TRACK_TABLE . "
				WHERE user_id = {$user->data['user_id']}
					AND forum_id " .
					(($global_announce_list && sizeof($global_announce_list)) ? "IN (0, $forum_id)" : "= $forum_id");
			$result = $db->sql_query($sql);

			$mark_time = array();
			while ($row = $db->sql_fetchrow($result))
			{
				$mark_time[$row['forum_id']] = $row['mark_time'];
			}
			$db->sql_freeresult($result);

			$user_lastmark = (isset($mark_time[$forum_id])) ? $mark_time[$forum_id] : $user->data['user_lastmark'];

			foreach ($topic_ids as $topic_id)
			{
				if ($global_announce_list && isset($global_announce_list[$topic_id]))
				{
					$last_read[$topic_id] = (isset($mark_time[0])) ? $mark_time[0] : $user_lastmark;
				}
				else
				{
					$last_read[$topic_id] = $user_lastmark;
				}
			}
		}
	}
	else if ($config['load_anon_lastread'] || $user->data['is_registered'])
	{
		global $tracking_topics;

		if (!isset($tracking_topics) || !sizeof($tracking_topics))
		{
			$tracking_topics = (isset($_COOKIE[$config['cookie_name'] . '_track'])) ? ((STRIP) ? stripslashes($_COOKIE[$config['cookie_name'] . '_track']) : $_COOKIE[$config['cookie_name'] . '_track']) : '';
			$tracking_topics = ($tracking_topics) ? tracking_unserialize($tracking_topics) : array();
		}

		if (!$user->data['is_registered'])
		{
			$user_lastmark = (isset($tracking_topics['l'])) ? base_convert($tracking_topics['l'], 36, 10) + $config['board_startdate'] : 0;
		}
		else
		{
			$user_lastmark = $user->data['user_lastmark'];
		}

		foreach ($topic_ids as $topic_id)
		{
			$topic_id36 = base_convert($topic_id, 10, 36);

			if (isset($tracking_topics['t'][$topic_id36]))
			{
				$last_read[$topic_id] = base_convert($tracking_topics['t'][$topic_id36], 36, 10) + $config['board_startdate'];
			}
		}

		$topic_ids = array_diff($topic_ids, array_keys($last_read));

		if (sizeof($topic_ids))
		{
			$mark_time = array();
			if ($global_announce_list && sizeof($global_announce_list))
			{
				if (isset($tracking_topics['f'][0]))
				{
					$mark_time[0] = base_convert($tracking_topics['f'][0], 36, 10) + $config['board_startdate'];
				}
			}

			if (isset($tracking_topics['f'][$forum_id]))
			{
				$mark_time[$forum_id] = base_convert($tracking_topics['f'][$forum_id], 36, 10) + $config['board_startdate'];
			}

			$user_lastmark = (isset($mark_time[$forum_id])) ? $mark_time[$forum_id] : $user_lastmark;

			foreach ($topic_ids as $topic_id)
			{
				if ($global_announce_list && isset($global_announce_list[$topic_id]))
				{
					$last_read[$topic_id] = (isset($mark_time[0])) ? $mark_time[0] : $user_lastmark;
				}
				else
				{
					$last_read[$topic_id] = $user_lastmark;
				}
			}
		}
	}

	return $last_read;
}

/**
* Check for read forums and update topic tracking info accordingly
*
* @param int $forum_id the forum id to check
* @param int $forum_last_post_time the forums last post time
* @param int $f_mark_time the forums last mark time if user is registered and load_db_lastread enabled
* @param int $mark_time_forum false if the mark time needs to be obtained, else the last users forum mark time
*
* @return true if complete forum got marked read, else false.
*/
function update_forum_tracking_info($forum_id, $forum_last_post_time, $f_mark_time = false, $mark_time_forum = false)
{
	global $db, $tracking_topics, $user, $config;

	// Determine the users last forum mark time if not given.
	if ($mark_time_forum === false)
	{
		if ($config['load_db_lastread'] && $user->data['is_registered'])
		{
			$mark_time_forum = (!empty($f_mark_time)) ? $f_mark_time : $user->data['user_lastmark'];
		}
		else if ($config['load_anon_lastread'] || $user->data['is_registered'])
		{
			$tracking_topics = (isset($_COOKIE[$config['cookie_name'] . '_track'])) ? ((STRIP) ? stripslashes($_COOKIE[$config['cookie_name'] . '_track']) : $_COOKIE[$config['cookie_name'] . '_track']) : '';
			$tracking_topics = ($tracking_topics) ? tracking_unserialize($tracking_topics) : array();

			if (!$user->data['is_registered'])
			{
				$user->data['user_lastmark'] = (isset($tracking_topics['l'])) ? (int) (base_convert($tracking_topics['l'], 36, 10) + $config['board_startdate']) : 0;
			}

			$mark_time_forum = (isset($tracking_topics['f'][$forum_id])) ? (int) (base_convert($tracking_topics['f'][$forum_id], 36, 10) + $config['board_startdate']) : $user->data['user_lastmark'];
		}
	}

	// Check the forum for any left unread topics.
	// If there are none, we mark the forum as read.
	if ($config['load_db_lastread'] && $user->data['is_registered'])
	{
		if ($mark_time_forum >= $forum_last_post_time)
		{
			// We do not need to mark read, this happened before. Therefore setting this to true
			$row = true;
		}
		else
		{
			$sql = 'SELECT t.forum_id FROM ' . TOPICS_TABLE . ' t
				LEFT JOIN ' . TOPICS_TRACK_TABLE . ' tt ON (tt.topic_id = t.topic_id AND tt.user_id = ' . $user->data['user_id'] . ')
				WHERE t.forum_id = ' . $forum_id . '
					AND t.topic_last_post_time > ' . $mark_time_forum . '
					AND t.topic_moved_id = 0
					AND (tt.topic_id IS NULL OR tt.mark_time < t.topic_last_post_time)
				GROUP BY t.forum_id';
			$result = $db->sql_query_limit($sql, 1);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
		}
	}
	else if ($config['load_anon_lastread'] || $user->data['is_registered'])
	{
		// Get information from cookie
		$row = false;

		if (!isset($tracking_topics['tf'][$forum_id]))
		{
			// We do not need to mark read, this happened before. Therefore setting this to true
			$row = true;
		}
		else
		{
			$sql = 'SELECT topic_id
				FROM ' . TOPICS_TABLE . '
				WHERE forum_id = ' . $forum_id . '
					AND topic_last_post_time > ' . $mark_time_forum . '
					AND topic_moved_id = 0';
			$result = $db->sql_query($sql);

			$check_forum = $tracking_topics['tf'][$forum_id];
			$unread = false;

			while ($row = $db->sql_fetchrow($result))
			{
				if (!isset($check_forum[base_convert($row['topic_id'], 10, 36)]))
				{
					$unread = true;
					break;
				}
			}
			$db->sql_freeresult($result);

			$row = $unread;
		}
	}
	else
	{
		$row = true;
	}

	if (!$row)
	{
		markread('topics', $forum_id);
		return true;
	}

	return false;
}

/**
* Transform an array into a serialized format
*/
function tracking_serialize($input)
{
	$out = '';
	foreach ($input as $key => $value)
	{
		if (is_array($value))
		{
			$out .= $key . ':(' . tracking_serialize($value) . ');';
		}
		else
		{
			$out .= $key . ':' . $value . ';';
		}
	}
	return $out;
}

/**
* Transform a serialized array into an actual array
*/
function tracking_unserialize($string, $max_depth = 3)
{
	$n = strlen($string);
	if ($n > 10010)
	{
		die('Invalid data supplied');
	}
	$data = $stack = array();
	$key = '';
	$mode = 0;
	$level = &$data;
	for ($i = 0; $i < $n; ++$i)
	{
		switch ($mode)
		{
			case 0:
				switch ($string[$i])
				{
					case ':':
						$level[$key] = 0;
						$mode = 1;
					break;
					case ')':
						unset($level);
						$level = array_pop($stack);
						$mode = 3;
					break;
					default:
						$key .= $string[$i];
				}
			break;

			case 1:
				switch ($string[$i])
				{
					case '(':
						if (sizeof($stack) >= $max_depth)
						{
							die('Invalid data supplied');
						}
						$stack[] = &$level;
						$level[$key] = array();
						$level = &$level[$key];
						$key = '';
						$mode = 0;
					break;
					default:
						$level[$key] = $string[$i];
						$mode = 2;
					break;
				}
			break;

			case 2:
				switch ($string[$i])
				{
					case ')':
						unset($level);
						$level = array_pop($stack);
						$mode = 3;
					break;
					case ';':
						$key = '';
						$mode = 0;
					break;
					default:
						$level[$key] .= $string[$i];
					break;
				}
			break;

			case 3:
				switch ($string[$i])
				{
					case ')':
						unset($level);
						$level = array_pop($stack);
					break;
					case ';':
						$key = '';
						$mode = 0;
					break;
					default:
						die('Invalid data supplied');
					break;
				}
			break;
		}
	}

	if (sizeof($stack) != 0 || ($mode != 0 && $mode != 3))
	{
		die('Invalid data supplied');
	}

	return $level;
}

// Pagination functions

/**
* Pagination routine, generates page number sequence
* tpl_prefix is for using different pagination blocks at one page
*/
function generate_pagination($base_url, $num_items, $per_page, $start_item, $add_prevnext_text = false, $tpl_prefix = '')
{
	global $template, $user;

	// Make sure $per_page is a valid value
	$per_page = ($per_page <= 0) ? 1 : $per_page;

	$seperator = '<span class="page-sep">' . $user->lang['COMMA_SEPARATOR'] . '</span>';
	$total_pages = ceil($num_items / $per_page);

	if ($total_pages == 1 || !$num_items)
	{
		return false;
	}

	$on_page = floor($start_item / $per_page) + 1;
	$url_delim = (strpos($base_url, '?') === false) ? '?' : '&amp;';

	$page_string = ($on_page == 1) ? '<strong>1</strong>' : '<a href="' . $base_url . '">1</a>';

	if ($total_pages > 5)
	{
		$start_cnt = min(max(1, $on_page - 4), $total_pages - 5);
		$end_cnt = max(min($total_pages, $on_page + 4), 6);

		$page_string .= ($start_cnt > 1) ? ' ... ' : $seperator;

		for ($i = $start_cnt + 1; $i < $end_cnt; $i++)
		{
			$page_string .= ($i == $on_page) ? '<strong>' . $i . '</strong>' : '<a href="' . $base_url . "{$url_delim}start=" . (($i - 1) * $per_page) . '">' . $i . '</a>';
			if ($i < $end_cnt - 1)
			{
				$page_string .= $seperator;
			}
		}

		$page_string .= ($end_cnt < $total_pages) ? ' ... ' : $seperator;
	}
	else
	{
		$page_string .= $seperator;

		for ($i = 2; $i < $total_pages; $i++)
		{
			$page_string .= ($i == $on_page) ? '<strong>' . $i . '</strong>' : '<a href="' . $base_url . "{$url_delim}start=" . (($i - 1) * $per_page) . '">' . $i . '</a>';
			if ($i < $total_pages)
			{
				$page_string .= $seperator;
			}
		}
	}

	$page_string .= ($on_page == $total_pages) ? '<strong>' . $total_pages . '</strong>' : '<a href="' . $base_url . "{$url_delim}start=" . (($total_pages - 1) * $per_page) . '">' . $total_pages . '</a>';

	if ($add_prevnext_text)
	{
		if ($on_page != 1)
		{
			$page_string = '<a href="' . $base_url . "{$url_delim}start=" . (($on_page - 2) * $per_page) . '">' . $user->lang['PREVIOUS'] . '</a>&nbsp;&nbsp;' . $page_string;
		}

		if ($on_page != $total_pages)
		{
			$page_string .= '&nbsp;&nbsp;<a href="' . $base_url . "{$url_delim}start=" . ($on_page * $per_page) . '">' . $user->lang['NEXT'] . '</a>';
		}
	}

	$template->assign_vars(array(
		$tpl_prefix . 'BASE_URL'		=> $base_url,
		'A_' . $tpl_prefix . 'BASE_URL'	=> addslashes($base_url),
		$tpl_prefix . 'PER_PAGE'		=> $per_page,

		$tpl_prefix . 'PREVIOUS_PAGE'	=> ($on_page == 1) ? '' : $base_url . "{$url_delim}start=" . (($on_page - 2) * $per_page),
		$tpl_prefix . 'NEXT_PAGE'		=> ($on_page == $total_pages) ? '' : $base_url . "{$url_delim}start=" . ($on_page * $per_page),
		$tpl_prefix . 'TOTAL_PAGES'		=> $total_pages,
	));

	return $page_string;
}

/**
* Return current page (pagination)
*/
function on_page($num_items, $per_page, $start)
{
	global $template, $user;

	// Make sure $per_page is a valid value
	$per_page = ($per_page <= 0) ? 1 : $per_page;

	$on_page = floor($start / $per_page) + 1;

	$template->assign_vars(array(
		'ON_PAGE'		=> $on_page)
	);

	return sprintf($user->lang['PAGE_OF'], $on_page, max(ceil($num_items / $per_page), 1));
}

// Server functions (building urls, redirecting...)

/**
* Append session id to url.
* This function supports hooks.
*
* @param string $url The url the session id needs to be appended to (can have params)
* @param mixed $params String or array of additional url parameters
* @param bool $is_amp Is url using &amp; (true) or & (false)
* @param string $session_id Possibility to use a custom session id instead of the global one
*
* Examples:
* <code>
* append_sid("{$phpbb_root_path}viewtopic.$phpEx?t=1&amp;f=2");
* append_sid("{$phpbb_root_path}viewtopic.$phpEx", 't=1&amp;f=2');
* append_sid("{$phpbb_root_path}viewtopic.$phpEx", 't=1&f=2', false);
* append_sid("{$phpbb_root_path}viewtopic.$phpEx", array('t' => 1, 'f' => 2));
* </code>
*
*/
function append_sid($url, $params = false, $is_amp = true, $session_id = false)
{
	global $_SID, $_EXTRA_URL, $phpbb_hook;

	// Developers using the hook function need to globalise the $_SID and $_EXTRA_URL on their own and also handle it appropiatly.
	// They could mimick most of what is within this function
	if (!empty($phpbb_hook) && $phpbb_hook->call_hook(__FUNCTION__, $url, $params, $is_amp, $session_id))
	{
		if ($phpbb_hook->hook_return(__FUNCTION__))
		{
			return $phpbb_hook->hook_return_result(__FUNCTION__);
		}
	}

	// Assign sid if session id is not specified
	if ($session_id === false)
	{
		$session_id = $_SID;
	}

	$amp_delim = ($is_amp) ? '&amp;' : '&';
	$url_delim = (strpos($url, '?') === false) ? '?' : $amp_delim;

	// Appending custom url parameter?
	$append_url = (!empty($_EXTRA_URL)) ? implode($amp_delim, $_EXTRA_URL) : '';

	$anchor = '';
	if (strpos($url, '#') !== false)
	{
		list($url, $anchor) = explode('#', $url, 2);
		$anchor = '#' . $anchor;
	}
	else if (!is_array($params) && strpos($params, '#') !== false)
	{
		list($params, $anchor) = explode('#', $params, 2);
		$anchor = '#' . $anchor;
	}

	// Use the short variant if possible ;)
	if ($params === false)
	{
		// Append session id
		if (!$session_id)
		{
			return $url . (($append_url) ? $url_delim . $append_url : '') . $anchor;
		}
		else
		{
			return $url . (($append_url) ? $url_delim . $append_url . $amp_delim : $url_delim) . 'sid=' . $session_id . $anchor;
		}
	}

	// Build string if parameters are specified as array
	if (is_array($params))
	{
		$output = array();

		foreach ($params as $key => $item)
		{
			if ($item === NULL)
			{
				continue;
			}

			if ($key == '#')
			{
				$anchor = '#' . $item;
				continue;
			}

			$output[] = $key . '=' . $item;
		}

		$params = implode($amp_delim, $output);
	}

	// Append session id and parameters (even if they are empty)
	// If parameters are empty, the developer can still append his/her parameters without caring about the delimiter
	return $url . (($append_url) ? $url_delim . $append_url . $amp_delim : $url_delim) . $params . ((!$session_id) ? '' : $amp_delim . 'sid=' . $session_id) . $anchor;
}

/**
* Generate board url (example: http://www.example.com/phpBB)
* @param bool $without_script_path if set to true the script path gets not appended (example: http://www.example.com)
*/
function generate_board_url($without_script_path = false)
{
	global $config, $user;

	$server_name = $user->host;
	$server_port = (!empty($_SERVER['SERVER_PORT'])) ? (int) $_SERVER['SERVER_PORT'] : (int) getenv('SERVER_PORT');

	// Forcing server vars is the only way to specify/override the protocol
	if ($config['force_server_vars'] || !$server_name)
	{
		$server_protocol = ($config['server_protocol']) ? $config['server_protocol'] : (($config['cookie_secure']) ? 'https://' : 'http://');
		$server_name = $config['server_name'];
		$server_port = (int) $config['server_port'];
		$script_path = $config['script_path'];

		$url = $server_protocol . $server_name;
	}
	else
	{
		// Do not rely on cookie_secure, users seem to think that it means a secured cookie instead of an encrypted connection
		$cookie_secure = (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] == 'on') ? 1 : 0;
		$url = (($cookie_secure) ? 'https://' : 'http://') . $server_name;

		$script_path = $user->page['root_script_path'];
	}

	if ($server_port && (($config['cookie_secure'] && $server_port <> 443) || (!$config['cookie_secure'] && $server_port <> 80)))
	{
		// HTTP HOST can carry a port number...
		if (strpos($server_name, ':') === false)
		{
			$url .= ':' . $server_port;
		}
	}

	if (!$without_script_path)
	{
		$url .= $script_path;
	}

	// Strip / from the end
	if (substr($url, -1, 1) == '/')
	{
		$url = substr($url, 0, -1);
	}

	return $url;
}

/**
* Redirects the user to another page then exits the script nicely
*/
function redirect($url, $return = false)
{
	global $db, $cache, $config, $user, $phpbb_root_path;

	if (empty($user->lang))
	{
		$user->add_lang('common');
	}

	if (!$return)
	{
		garbage_collection();
	}

	// Make sure no &amp;'s are in, this will break the redirect
	$url = str_replace('&amp;', '&', $url);

	// Determine which type of redirect we need to handle...
	$url_parts = parse_url($url);

	if ($url_parts === false)
	{
		// Malformed url, redirect to current page...
		$url = generate_board_url() . '/' . $user->page['page'];
	}
	else if (!empty($url_parts['scheme']) && !empty($url_parts['host']))
	{
		// Full URL
	}
	else if ($url[0] == '/')
	{
		// Absolute uri, prepend direct url...
		$url = generate_board_url(true) . $url;
	}
	else
	{
		// Relative uri
		$pathinfo = pathinfo($url);

		// Is the uri pointing to the current directory?
		if ($pathinfo['dirname'] == '.')
		{
			$url = str_replace('./', '', $url);

			// Strip / from the beginning
			if ($url && substr($url, 0, 1) == '/')
			{
				$url = substr($url, 1);
			}

			if ($user->page['page_dir'])
			{
				$url = generate_board_url() . '/' . $user->page['page_dir'] . '/' . $url;
			}
			else
			{
				$url = generate_board_url() . '/' . $url;
			}
		}
		else
		{
			// Used ./ before, but $phpbb_root_path is working better with urls within another root path
			$root_dirs = explode('/', str_replace('\\', '/', phpbb_realpath($phpbb_root_path)));
			$page_dirs = explode('/', str_replace('\\', '/', phpbb_realpath($pathinfo['dirname'])));
			$intersection = array_intersect_assoc($root_dirs, $page_dirs);

			$root_dirs = array_diff_assoc($root_dirs, $intersection);
			$page_dirs = array_diff_assoc($page_dirs, $intersection);

			$dir = str_repeat('../', sizeof($root_dirs)) . implode('/', $page_dirs);

			// Strip / from the end
			if ($dir && substr($dir, -1, 1) == '/')
			{
				$dir = substr($dir, 0, -1);
			}

			// Strip / from the beginning
			if ($dir && substr($dir, 0, 1) == '/')
			{
				$dir = substr($dir, 1);
			}

			$url = str_replace($pathinfo['dirname'] . '/', '', $url);

			// Strip / from the beginning
			if (substr($url, 0, 1) == '/')
			{
				$url = substr($url, 1);
			}

			$url = $dir . '/' . $url;
			$url = generate_board_url() . '/' . $url;
		}
	}

	// Make sure no linebreaks are there... to prevent http response splitting for PHP < 4.4.2
	if (strpos(urldecode($url), "\n") !== false || strpos(urldecode($url), "\r") !== false || strpos($url, ';') !== false)
	{
		trigger_error('Tried to redirect to potentially insecure url.', E_USER_ERROR);
	}

	// Now, also check the protocol and for a valid url the last time...
	$allowed_protocols = array('http', 'https', 'ftp', 'ftps');
	$url_parts = parse_url($url);

	if ($url_parts === false || empty($url_parts['scheme']) || !in_array($url_parts['scheme'], $allowed_protocols))
	{
		trigger_error('Tried to redirect to potentially insecure url.', E_USER_ERROR);
	}

	if ($return)
	{
		return $url;
	}

	// Redirect via an HTML form for PITA webservers
	if (@preg_match('#Microsoft|WebSTAR|Xitami#', getenv('SERVER_SOFTWARE')))
	{
		header('Refresh: 0; URL=' . $url);

		echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">';
		echo '<html xmlns="http://www.w3.org/1999/xhtml" dir="' . $user->lang['DIRECTION'] . '" lang="' . $user->lang['USER_LANG'] . '" xml:lang="' . $user->lang['USER_LANG'] . '">';
		echo '<head>';
		echo '<meta http-equiv="content-type" content="text/html; charset=utf-8" />';
		echo '<meta http-equiv="refresh" content="0; url=' . str_replace('&', '&amp;', $url) . '" />';
		echo '<title>' . $user->lang['REDIRECT'] . '</title>';
		echo '</head>';
		echo '<body>';
		echo '<div style="text-align: center;">' . sprintf($user->lang['URL_REDIRECT'], '<a href="' . str_replace('&', '&amp;', $url) . '">', '</a>') . '</div>';
		echo '</body>';
		echo '</html>';

		exit;
	}

	// Behave as per HTTP/1.1 spec for others
	header('Location: ' . $url);
	exit;
}

/**
* Re-Apply session id after page reloads
*/
function reapply_sid($url)
{
	global $phpEx, $phpbb_root_path;

	if ($url === "index.$phpEx")
	{
		return append_sid("index.$phpEx");
	}
	else if ($url === "{$phpbb_root_path}index.$phpEx")
	{
		return append_sid("{$phpbb_root_path}index.$phpEx");
	}

	// Remove previously added sid
	if (strpos($url, '?sid=') !== false)
	{
		$url = preg_replace('/(\?)sid=[a-z0-9]+(&amp;|&)?/', '\1', $url);
	}
	else if (strpos($url, '&sid=') !== false)
	{
		$url = preg_replace('/&sid=[a-z0-9]+(&)?/', '\1', $url);
	}
	else if (strpos($url, '&amp;sid=') !== false)
	{
		$url = preg_replace('/&amp;sid=[a-z0-9]+(&amp;)?/', '\1', $url);
	}

	return append_sid($url);
}

/**
* Returns url from the session/current page with an re-appended SID with optionally stripping vars from the url
*/
function build_url($strip_vars = false)
{
	global $user, $phpbb_root_path;

	// Append SID
	$redirect = append_sid($user->page['page'], false, false);

	// Add delimiter if not there...
	if (strpos($redirect, '?') === false)
	{
		$redirect .= '?';
	}

	// Strip vars...
	if ($strip_vars !== false && strpos($redirect, '?') !== false)
	{
		if (!is_array($strip_vars))
		{
			$strip_vars = array($strip_vars);
		}

		$query = $_query = array();

		$args = substr($redirect, strpos($redirect, '?') + 1);
		$args = ($args) ? explode('&', $args) : array();
		$redirect = substr($redirect, 0, strpos($redirect, '?'));

		foreach ($args as $argument)
		{
			$arguments = explode('=', $argument);
			$key = $arguments[0];
			unset($arguments[0]);

			$query[$key] = implode('=', $arguments);
		}

		// Strip the vars off
		foreach ($strip_vars as $strip)
		{
			if (isset($query[$strip]))
			{
				unset($query[$strip]);
			}
		}

		// Glue the remaining parts together... already urlencoded
		foreach ($query as $key => $value)
		{
			$_query[] = $key . '=' . $value;
		}
		$query = implode('&', $_query);

		$redirect .= ($query) ? '?' . $query : '';
	}

	return $phpbb_root_path . str_replace('&', '&amp;', $redirect);
}

/**
* Meta refresh assignment
*/
function meta_refresh($time, $url)
{
	global $template;

	$url = redirect($url, true);

	// For XHTML compatibility we change back & to &amp;
	$template->assign_vars(array(
		'META' => '<meta http-equiv="refresh" content="' . $time . ';url=' . str_replace('&', '&amp;', $url) . '" />')
	);
}

//Form validation

/**
* Add a secret token to the form (requires the S_FORM_TOKEN template variable)
* @param string  $form_name The name of the form; has to match the name used in check_form_key, otherwise no restrictions apply
*/
function add_form_key($form_name)
{
	global $config, $template, $user;
	$now = time();
	$token_sid = ($user->data['user_id'] == ANONYMOUS && !empty($config['form_token_sid_guests'])) ? $user->session_id : '';
	$token = sha1($now . $user->data['user_form_salt'] . $form_name . $token_sid);

	$s_fields = build_hidden_fields(array(
			'creation_time' => $now,
			'form_token'	=> $token,
	));
	$template->assign_vars(array(
			'S_FORM_TOKEN'	=> $s_fields,
	));
}

/**
* Check the form key. Required for all altering actions not secured by confirm_box
* @param string  $form_name The name of the form; has to match the name used in add_form_key, otherwise no restrictions apply
* @param int $timespan The maximum acceptable age for a submitted form in seconds. Defaults to the config setting.
* @param string $return_page The address for the return link
* @param bool $trigger If true, the function will triger an error when encountering an invalid form
*/
function check_form_key($form_name, $timespan = false, $return_page = '', $trigger = false)
{
	global $config, $user;

	if ($timespan === false)
	{
		// we enforce a minimum value of half a minute here.
		$timespan = ($config['form_token_lifetime'] == -1) ? -1 : max(30, $config['form_token_lifetime']);
	}

	if (isset($_POST['creation_time']) && isset($_POST['form_token']))
	{
		$creation_time	= abs(request_var('creation_time', 0));
		$token = request_var('form_token', '');

		$diff = (time() - $creation_time);

		if (($diff <= $timespan) || $timespan === -1)
		{
			$token_sid = ($user->data['user_id'] == ANONYMOUS && !empty($config['form_token_sid_guests'])) ? $user->session_id : '';

			$key = sha1($creation_time . $user->data['user_form_salt'] . $form_name . $token_sid);
			if ($key === $token)
			{
				return true;
			}
		}
	}
	if ($trigger)
	{
		trigger_error($user->lang['FORM_INVALID'] . $return_page);
	}
	return false;
}

// Message/Login boxes

/**
* Build Confirm box
* @param boolean $check True for checking if confirmed (without any additional parameters) and false for displaying the confirm box
* @param string $title Title/Message used for confirm box.
*		message text is _CONFIRM appended to title.
*		If title cannot be found in user->lang a default one is displayed
*		If title_CONFIRM cannot be found in user->lang the text given is used.
* @param string $hidden Hidden variables
* @param string $html_body Template used for confirm box
* @param string $u_action Custom form action
*/
function confirm_box($check, $title = '', $hidden = '', $html_body = 'confirm_body.html', $u_action = '')
{
	global $user, $template, $db;
	global $phpEx, $phpbb_root_path;

	if (isset($_POST['cancel']))
	{
		return false;
	}

	$confirm = false;
	if (isset($_POST['confirm']))
	{
		// language frontier
		if ($_POST['confirm'] === $user->lang['YES'])
		{
			$confirm = true;
		}
	}

	if ($check && $confirm)
	{
		$user_id = request_var('user_id', 0);
		$session_id = request_var('sess', '');
		$confirm_key = request_var('confirm_key', '');

		if ($user_id != $user->data['user_id'] || $session_id != $user->session_id || !$confirm_key || !$user->data['user_last_confirm_key'] || $confirm_key != $user->data['user_last_confirm_key'])
		{
			return false;
		}

		// Reset user_last_confirm_key
		$sql = 'UPDATE ' . USERS_TABLE . " SET user_last_confirm_key = ''
			WHERE user_id = " . $user->data['user_id'];
		$db->sql_query($sql);

		return true;
	}
	else if ($check)
	{
		return false;
	}

	$s_hidden_fields = build_hidden_fields(array(
		'user_id'	=> $user->data['user_id'],
		'sess'		=> $user->session_id,
		'sid'		=> $user->session_id)
	);

	// generate activation key
	$confirm_key = gen_rand_string(10);

	if (defined('IN_ADMIN') && isset($user->data['session_admin']) && $user->data['session_admin'])
	{
		adm_page_header((!isset($user->lang[$title])) ? $user->lang['CONFIRM'] : $user->lang[$title]);
	}
	else
	{
		page_header((!isset($user->lang[$title])) ? $user->lang['CONFIRM'] : $user->lang[$title]);
	}

	$template->set_filenames(array(
		'body' => $html_body)
	);

	// If activation key already exist, we better do not re-use the key (something very strange is going on...)
	if (request_var('confirm_key', ''))
	{
		// This should not occur, therefore we cancel the operation to safe the user
		return false;
	}

	// re-add sid / transform & to &amp; for user->page (user->page is always using &)
	$use_page = ($u_action) ? $phpbb_root_path . $u_action : $phpbb_root_path . str_replace('&', '&amp;', $user->page['page']);
	$u_action = reapply_sid($use_page);
	$u_action .= ((strpos($u_action, '?') === false) ? '?' : '&amp;') . 'confirm_key=' . $confirm_key;

	$template->assign_vars(array(
		'MESSAGE_TITLE'		=> (!isset($user->lang[$title])) ? $user->lang['CONFIRM'] : $user->lang[$title],
		'MESSAGE_TEXT'		=> (!isset($user->lang[$title . '_CONFIRM'])) ? $title : $user->lang[$title . '_CONFIRM'],

		'YES_VALUE'			=> $user->lang['YES'],
		'S_CONFIRM_ACTION'	=> $u_action,
		'S_HIDDEN_FIELDS'	=> $hidden . $s_hidden_fields)
	);

	$sql = 'UPDATE ' . USERS_TABLE . " SET user_last_confirm_key = '" . $db->sql_escape($confirm_key) . "'
		WHERE user_id = " . $user->data['user_id'];
	$db->sql_query($sql);

	if (defined('IN_ADMIN') && isset($user->data['session_admin']) && $user->data['session_admin'])
	{
		adm_page_footer();
	}
	else
	{
		page_footer();
	}
}

/**
* Generate login box or verify password
*/
function login_box($redirect = '', $l_explain = '', $l_success = '', $admin = false, $s_display = true)
{
	global $db, $user, $template, $auth, $phpEx, $phpbb_root_path, $config;

	$err = '';

	// Make sure user->setup() has been called
	if (empty($user->lang))
	{
		$user->setup();
	}

	// Print out error if user tries to authenticate as an administrator without having the privileges...
	if ($admin && !$auth->acl_get('a_'))
	{
		// Not authd
		// anonymous/inactive users are never able to go to the ACP even if they have the relevant permissions
		if ($user->data['is_registered'])
		{
			add_log('admin', 'LOG_ADMIN_AUTH_FAIL');
		}
		trigger_error('NO_AUTH_ADMIN');
	}
	
	// Don't pass anything here, login_roscms will call roscms_subsys_login on its own.
	$result = $auth->login('', '', false, true, $admin);

	// If admin authentication and login, we will log if it was a success or not...
	// We also break the operation on the first non-success login - it could be argued that the user already knows
	if ($admin)
	{
		if ($result['status'] == LOGIN_SUCCESS)
		{
			add_log('admin', 'LOG_ADMIN_AUTH_SUCCESS');
		}
		else
		{
			// Only log the failed attempt if a real user tried to.
			// anonymous/inactive users are never able to go to the ACP even if they have the relevant permissions
			if ($user->data['is_registered'])
			{
				add_log('admin', 'LOG_ADMIN_AUTH_FAIL');
			}
		}
	}

	// The result parameter is always an array, holding the relevant information...
	if ($result['status'] == LOGIN_SUCCESS)
	{
		$redirect = $admin ? "{$phpbb_root_path}adm/index.$phpEx" : "{$phpbb_root_path}index.$phpEx";
		
		// append/replace SID (may change during the session for AOL users)
		$redirect = reapply_sid($redirect);

		// Special case... the user is effectively banned, but we allow founders to login
		if (defined('IN_CHECK_BAN') && $result['user_row']['user_type'] != USER_FOUNDER)
		{
			return;
		}

		header("Location: $redirect");
	}
}

/**
* Generate forum login box
*/
function login_forum_box($forum_data)
{
	global $db, $config, $user, $template, $phpEx;

	$password = request_var('password', '', true);

	$sql = 'SELECT forum_id
		FROM ' . FORUMS_ACCESS_TABLE . '
		WHERE forum_id = ' . $forum_data['forum_id'] . '
			AND user_id = ' . $user->data['user_id'] . "
			AND session_id = '" . $db->sql_escape($user->session_id) . "'";
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if ($row)
	{
		return true;
	}

	if ($password)
	{
		// Remove expired authorised sessions
		$sql = 'SELECT f.session_id
			FROM ' . FORUMS_ACCESS_TABLE . ' f
			LEFT JOIN ' . SESSIONS_TABLE . ' s ON (f.session_id = s.session_id)
			WHERE s.session_id IS NULL';
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			$sql_in = array();
			do
			{
				$sql_in[] = (string) $row['session_id'];
			}
			while ($row = $db->sql_fetchrow($result));

			// Remove expired sessions
			$sql = 'DELETE FROM ' . FORUMS_ACCESS_TABLE . '
				WHERE ' . $db->sql_in_set('session_id', $sql_in);
			$db->sql_query($sql);
		}
		$db->sql_freeresult($result);

		if (phpbb_check_hash($password, $forum_data['forum_password']))
		{
			$sql_ary = array(
				'forum_id'		=> (int) $forum_data['forum_id'],
				'user_id'		=> (int) $user->data['user_id'],
				'session_id'	=> (string) $user->session_id,
			);

			$db->sql_query('INSERT INTO ' . FORUMS_ACCESS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));

			return true;
		}

		$template->assign_var('LOGIN_ERROR', $user->lang['WRONG_PASSWORD']);
	}

	page_header($user->lang['LOGIN']);

	$template->assign_vars(array(
		'S_HIDDEN_FIELDS'		=> build_hidden_fields(array('f' => $forum_data['forum_id'])))
	);

	$template->set_filenames(array(
		'body' => 'login_forum.html')
	);

	page_footer();
}

// Little helpers

/**
* Little helper for the build_hidden_fields function
*/
function _build_hidden_fields($key, $value, $specialchar, $stripslashes)
{
	$hidden_fields = '';

	if (!is_array($value))
	{
		$value = ($stripslashes) ? stripslashes($value) : $value;
		$value = ($specialchar) ? htmlspecialchars($value, ENT_COMPAT, 'UTF-8') : $value;

		$hidden_fields .= '<input type="hidden" name="' . $key . '" value="' . $value . '" />' . "\n";
	}
	else
	{
		foreach ($value as $_key => $_value)
		{
			$_key = ($stripslashes) ? stripslashes($_key) : $_key;
			$_key = ($specialchar) ? htmlspecialchars($_key, ENT_COMPAT, 'UTF-8') : $_key;

			$hidden_fields .= _build_hidden_fields($key . '[' . $_key . ']', $_value, $specialchar, $stripslashes);
		}
	}

	return $hidden_fields;
}

/**
* Build simple hidden fields from array
*
* @param array $field_ary an array of values to build the hidden field from
* @param bool $specialchar if true, keys and values get specialchared
* @param bool $stripslashes if true, keys and values get stripslashed
*
* @return string the hidden fields
*/
function build_hidden_fields($field_ary, $specialchar = false, $stripslashes = false)
{
	$s_hidden_fields = '';

	foreach ($field_ary as $name => $vars)
	{
		$name = ($stripslashes) ? stripslashes($name) : $name;
		$name = ($specialchar) ? htmlspecialchars($name, ENT_COMPAT, 'UTF-8') : $name;

		$s_hidden_fields .= _build_hidden_fields($name, $vars, $specialchar, $stripslashes);
	}

	return $s_hidden_fields;
}

/**
* Parse cfg file
*/
function parse_cfg_file($filename, $lines = false)
{
	$parsed_items = array();

	if ($lines === false)
	{
		$lines = file($filename);
	}

	foreach ($lines as $line)
	{
		$line = trim($line);

		if (!$line || $line[0] == '#' || ($delim_pos = strpos($line, '=')) === false)
		{
			continue;
		}

		// Determine first occurrence, since in values the equal sign is allowed
		$key = strtolower(trim(substr($line, 0, $delim_pos)));
		$value = trim(substr($line, $delim_pos + 1));

		if (in_array($value, array('off', 'false', '0')))
		{
			$value = false;
		}
		else if (in_array($value, array('on', 'true', '1')))
		{
			$value = true;
		}
		else if (!trim($value))
		{
			$value = '';
		}
		else if (($value[0] == "'" && $value[sizeof($value) - 1] == "'") || ($value[0] == '"' && $value[sizeof($value) - 1] == '"'))
		{
			$value = substr($value, 1, sizeof($value)-2);
		}

		$parsed_items[$key] = $value;
	}

	return $parsed_items;
}

/**
* Add log event
*/
function add_log()
{
	global $db, $user;

	$args = func_get_args();

	$mode			= array_shift($args);
	$reportee_id	= ($mode == 'user') ? intval(array_shift($args)) : '';
	$forum_id		= ($mode == 'mod') ? intval(array_shift($args)) : '';
	$topic_id		= ($mode == 'mod') ? intval(array_shift($args)) : '';
	$action			= array_shift($args);
	$data			= (!sizeof($args)) ? '' : serialize($args);

	$sql_ary = array(
		'user_id'		=> (empty($user->data)) ? ANONYMOUS : $user->data['user_id'],
		'log_ip'		=> $user->ip,
		'log_time'		=> time(),
		'log_operation'	=> $action,
		'log_data'		=> $data,
	);

	switch ($mode)
	{
		case 'admin':
			$sql_ary['log_type'] = LOG_ADMIN;
		break;

		case 'mod':
			$sql_ary += array(
				'log_type'	=> LOG_MOD,
				'forum_id'	=> $forum_id,
				'topic_id'	=> $topic_id
			);
		break;

		case 'user':
			$sql_ary += array(
				'log_type'		=> LOG_USERS,
				'reportee_id'	=> $reportee_id
			);
		break;

		case 'critical':
			$sql_ary['log_type'] = LOG_CRITICAL;
		break;

		default:
			return false;
	}

	$db->sql_query('INSERT INTO ' . LOG_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary));

	return $db->sql_nextid();
}

/**
* Return a nicely formatted backtrace (parts from the php manual by diz at ysagoon dot com)
*/
function get_backtrace()
{
	global $phpbb_root_path;

	$output = '<div style="font-family: monospace;">';
	$backtrace = debug_backtrace();
	$path = phpbb_realpath($phpbb_root_path);

	foreach ($backtrace as $number => $trace)
	{
		// We skip the first one, because it only shows this file/function
		if ($number == 0)
		{
			continue;
		}

		// Strip the current directory from path
		if (empty($trace['file']))
		{
			$trace['file'] = '';
		}
		else
		{
			$trace['file'] = str_replace(array($path, '\\'), array('', '/'), $trace['file']);
			$trace['file'] = substr($trace['file'], 1);
		}
		$args = array();

		// If include/require/include_once is not called, do not show arguments - they may contain sensible information
		if (!in_array($trace['function'], array('include', 'require', 'include_once')))
		{
			unset($trace['args']);
		}
		else
		{
			// Path...
			if (!empty($trace['args'][0]))
			{
				$argument = htmlspecialchars($trace['args'][0]);
				$argument = str_replace(array($path, '\\'), array('', '/'), $argument);
				$argument = substr($argument, 1);
				$args[] = "'{$argument}'";
			}
		}

		$trace['class'] = (!isset($trace['class'])) ? '' : $trace['class'];
		$trace['type'] = (!isset($trace['type'])) ? '' : $trace['type'];

		$output .= '<br />';
		$output .= '<b>FILE:</b> ' . htmlspecialchars($trace['file']) . '<br />';
		$output .= '<b>LINE:</b> ' . ((!empty($trace['line'])) ? $trace['line'] : '') . '<br />';

		$output .= '<b>CALL:</b> ' . htmlspecialchars($trace['class'] . $trace['type'] . $trace['function']) . '(' . ((sizeof($args)) ? implode(', ', $args) : '') . ')<br />';
	}
	$output .= '</div>';
	return $output;
}

/**
* This function returns a regular expression pattern for commonly used expressions
* Use with / as delimiter for email mode and # for url modes
* mode can be: email|bbcode_htm|url|url_inline|www_url|www_url_inline|relative_url|relative_url_inline|ipv4|ipv6
*/
function get_preg_expression($mode)
{
	switch ($mode)
	{
		case 'email':
			return '(?:[a-z0-9\'\.\-_\+\|]|&amp;)+@[a-z0-9\-]+\.(?:[a-z0-9\-]+\.)*[a-z]+';
		break;

		case 'bbcode_htm':
			return array(
				'#<!\-\- e \-\-><a href="mailto:(.*?)">.*?</a><!\-\- e \-\->#',
				'#<!\-\- l \-\-><a (?:class="[\w-]+" )?href="(.*?)(?:(&amp;|\?)sid=[0-9a-f]{32})?">.*?</a><!\-\- l \-\->#',
				'#<!\-\- ([mw]) \-\-><a (?:class="[\w-]+" )?href="(.*?)">.*?</a><!\-\- \1 \-\->#',
				'#<!\-\- s(.*?) \-\-><img src="\{SMILIES_PATH\}\/.*? \/><!\-\- s\1 \-\->#',
				'#<!\-\- .*? \-\->#s',
				'#<.*?>#s',
			);
		break;

		// Whoa these look impressive!
		// The code to generate the following two regular expressions which match valid IPv4/IPv6 addresses
		// can be found in the develop directory
		case 'ipv4':
			return '#^(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$#';
		break;

		case 'ipv6':
			return '#^(?:(?:(?:[\dA-F]{1,4}:){6}(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:::(?:[\dA-F]{1,4}:){5}(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:(?:[\dA-F]{1,4}:):(?:[\dA-F]{1,4}:){4}(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:(?:[\dA-F]{1,4}:){1,2}:(?:[\dA-F]{1,4}:){3}(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:(?:[\dA-F]{1,4}:){1,3}:(?:[\dA-F]{1,4}:){2}(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:(?:[\dA-F]{1,4}:){1,4}:(?:[\dA-F]{1,4}:)(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:(?:[\dA-F]{1,4}:){1,5}:(?:[\dA-F]{1,4}:[\dA-F]{1,4}|(?:(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(?:\d{1,2}|1\d\d|2[0-4]\d|25[0-5])))|(?:(?:[\dA-F]{1,4}:){1,6}:[\dA-F]{1,4})|(?:(?:[\dA-F]{1,4}:){1,7}:))$#i';
		break;

		case 'url':
		case 'url_inline':
			$inline = ($mode == 'url') ? ')' : '';
			$scheme = ($mode == 'url') ? '[a-z\d+\-.]' : '[a-z\d+]'; // avoid automatic parsing of "word" in "last word.http://..."
			// generated with regex generation file in the develop folder
			return "[a-z]$scheme*:/{2}(?:(?:[a-z0-9\-._~!$&'($inline*+,;=:@|]+|%[\dA-F]{2})+|[0-9.]+|\[[a-z0-9.]+:[a-z0-9.]+:[a-z0-9.:]+\])(?::\d*)?(?:/(?:[a-z0-9\-._~!$&'($inline*+,;=:@|]+|%[\dA-F]{2})*)*(?:\?(?:[a-z0-9\-._~!$&'($inline*+,;=:@/?|]+|%[\dA-F]{2})*)?(?:\#(?:[a-z0-9\-._~!$&'($inline*+,;=:@/?|]+|%[\dA-F]{2})*)?";
		break;

		case 'www_url':
		case 'www_url_inline':
			$inline = ($mode == 'www_url') ? ')' : '';
			return "www\.(?:[a-z0-9\-._~!$&'($inline*+,;=:@|]+|%[\dA-F]{2})+(?::\d*)?(?:/(?:[a-z0-9\-._~!$&'($inline*+,;=:@|]+|%[\dA-F]{2})*)*(?:\?(?:[a-z0-9\-._~!$&'($inline*+,;=:@/?|]+|%[\dA-F]{2})*)?(?:\#(?:[a-z0-9\-._~!$&'($inline*+,;=:@/?|]+|%[\dA-F]{2})*)?";
		break;

		case 'relative_url':
		case 'relative_url_inline':
			$inline = ($mode == 'relative_url') ? ')' : '';
			return "(?:[a-z0-9\-._~!$&'($inline*+,;=:@|]+|%[\dA-F]{2})*(?:/(?:[a-z0-9\-._~!$&'($inline*+,;=:@|]+|%[\dA-F]{2})*)*(?:\?(?:[a-z0-9\-._~!$&'($inline*+,;=:@/?|]+|%[\dA-F]{2})*)?(?:\#(?:[a-z0-9\-._~!$&'($inline*+,;=:@/?|]+|%[\dA-F]{2})*)?";
		break;
	}

	return '';
}

/**
* Returns the first block of the specified IPv6 address and as many additional
* ones as specified in the length paramater.
* If length is zero, then an empty string is returned.
* If length is greater than 3 the complete IP will be returned
*/
function short_ipv6($ip, $length)
{
	if ($length < 1)
	{
		return '';
	}

	// extend IPv6 addresses
	$blocks = substr_count($ip, ':') + 1;
	if ($blocks < 9)
	{
		$ip = str_replace('::', ':' . str_repeat('0000:', 9 - $blocks), $ip);
	}
	if ($ip[0] == ':')
	{
		$ip = '0000' . $ip;
	}
	if ($length < 4)
	{
		$ip = implode(':', array_slice(explode(':', $ip), 0, 1 + $length));
	}

	return $ip;
}

/**
* Wrapper for php's checkdnsrr function.
*
* The windows failover is from the php manual
* Please make sure to check the return value for === true and === false, since NULL could
* be returned too.
*
* @return true if entry found, false if not, NULL if this function is not supported by this environment
*/
function phpbb_checkdnsrr($host, $type = '')
{
	$type = (!$type) ? 'MX' : $type;

	if (DIRECTORY_SEPARATOR == '\\')
	{
		if (!function_exists('exec'))
		{
			return NULL;
		}

		// @exec('nslookup -retry=1 -timout=1 -type=' . escapeshellarg($type) . ' ' . escapeshellarg($host), $output);
		@exec('nslookup -type=' . escapeshellarg($type) . ' ' . escapeshellarg($host), $output);

		// If output is empty, the nslookup failed
		if (empty($output))
		{
			return NULL;
		}

		foreach ($output as $line)
		{
			if (!trim($line))
			{
				continue;
			}

			// Valid records begin with host name:
			if (strpos($line, $host) === 0)
			{
				return true;
			}
		}

		return false;
	}
	else if (function_exists('checkdnsrr'))
	{
		return (checkdnsrr($host, $type)) ? true : false;
	}

	return NULL;
}

// Handler, header and footer

/**
* Error and message handler, call with trigger_error if reqd
*/
function msg_handler($errno, $msg_text, $errfile, $errline)
{
	global $cache, $db, $auth, $template, $config, $user;
	global $phpEx, $phpbb_root_path, $msg_title, $msg_long_text;

	// Do not display notices if we suppress them via @
	if (error_reporting() == 0)
	{
		return;
	}

	// Message handler is stripping text. In case we need it, we are possible to define long text...
	if (isset($msg_long_text) && $msg_long_text && !$msg_text)
	{
		$msg_text = $msg_long_text;
	}

	switch ($errno)
	{
		case E_NOTICE:
		case E_WARNING:

			// Check the error reporting level and return if the error level does not match
			// If DEBUG is defined the default level is E_ALL
			if (($errno & ((defined('DEBUG')) ? E_ALL : error_reporting())) == 0)
			{
				return;
			}

			if (strpos($errfile, 'cache') === false && strpos($errfile, 'template.') === false)
			{
				// flush the content, else we get a white page if output buffering is on
				if ($config['gzip_compress'])
				{
					if (@extension_loaded('zlib') && !headers_sent())
					{
						@ob_flush();
					}
				}

				// remove complete path to installation, with the risk of changing backslashes meant to be there
				$errfile = str_replace(array(phpbb_realpath($phpbb_root_path), '\\'), array('', '/'), $errfile);
				$msg_text = str_replace(array(phpbb_realpath($phpbb_root_path), '\\'), array('', '/'), $msg_text);

				echo '<b>[phpBB Debug] PHP Notice</b>: in file <b>' . $errfile . '</b> on line <b>' . $errline . '</b>: <b>' . $msg_text . '</b><br />' . "\n";
			}

			return;

		break;

		case E_USER_ERROR:

			if (!empty($user) && !empty($user->lang))
			{
				$msg_text = (!empty($user->lang[$msg_text])) ? $user->lang[$msg_text] : $msg_text;
				$msg_title = (!isset($msg_title)) ? $user->lang['GENERAL_ERROR'] : ((!empty($user->lang[$msg_title])) ? $user->lang[$msg_title] : $msg_title);

				$l_return_index = sprintf($user->lang['RETURN_INDEX'], '<a href="' . $phpbb_root_path . '">', '</a>');
				$l_notify = '';

				if (!empty($config['board_contact']))
				{
					$l_notify = '<p>' . sprintf($user->lang['NOTIFY_ADMIN_EMAIL'], $config['board_contact']) . '</p>';
				}
			}
			else
			{
				$msg_title = 'General Error';
				$l_return_index = '<a href="' . $phpbb_root_path . '">Return to index page</a>';
				$l_notify = '';

				if (!empty($config['board_contact']))
				{
					$l_notify = '<p>Please notify the board administrator or webmaster: <a href="mailto:' . $config['board_contact'] . '">' . $config['board_contact'] . '</a></p>';
				}
			}

			garbage_collection();

			// Try to not call the adm page data...

			echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">';
			echo '<html xmlns="http://www.w3.org/1999/xhtml" dir="ltr">';
			echo '<head>';
			echo '<meta http-equiv="content-type" content="text/html; charset=utf-8" />';
			echo '<title>' . $msg_title . '</title>';
			echo '<style type="text/css">' . "\n" . '/* <![CDATA[ */' . "\n";
			echo '* { margin: 0; padding: 0; } html { font-size: 100%; height: 100%; margin-bottom: 1px; background-color: #E4EDF0; } body { font-family: "Lucida Grande", Verdana, Helvetica, Arial, sans-serif; color: #536482; background: #E4EDF0; font-size: 62.5%; margin: 0; } ';
			echo 'a:link, a:active, a:visited { color: #006699; text-decoration: none; } a:hover { color: #DD6900; text-decoration: underline; } ';
			echo '#wrap { padding: 0 20px 15px 20px; min-width: 615px; } #page-header { text-align: right; height: 40px; } #page-footer { clear: both; font-size: 1em; text-align: center; } ';
			echo '.panel { margin: 4px 0; background-color: #FFFFFF; border: solid 1px  #A9B8C2; } ';
			echo '#errorpage #page-header a { font-weight: bold; line-height: 6em; } #errorpage #content { padding: 10px; } #errorpage #content h1 { line-height: 1.2em; margin-bottom: 0; color: #DF075C; } ';
			echo '#errorpage #content div { margin-top: 20px; margin-bottom: 5px; border-bottom: 1px solid #CCCCCC; padding-bottom: 5px; color: #333333; font: bold 1.2em "Lucida Grande", Arial, Helvetica, sans-serif; text-decoration: none; line-height: 120%; text-align: left; } ';
			echo "\n" . '/* ]]> */' . "\n";
			echo '</style>';
			echo '</head>';
			echo '<body id="errorpage">';
			echo '<div id="wrap">';
			echo '	<div id="page-header">';
			echo '		' . $l_return_index;
			echo '	</div>';
			echo '	<div id="acp">';
			echo '	<div class="panel">';
			echo '		<div id="content">';
			echo '			<h1>' . $msg_title . '</h1>';

			echo '			<div>' . $msg_text . '</div>';

			echo $l_notify;

			echo '		</div>';
			echo '	</div>';
			echo '	</div>';
			echo '	<div id="page-footer">';
			echo '		Powered by phpBB &copy; 2000, 2002, 2005, 2007 <a href="http://www.phpbb.com/">phpBB Group</a>';
			echo '	</div>';
			echo '</div>';
			echo '</body>';
			echo '</html>';

			exit_handler();
		break;

		case E_USER_WARNING:
		case E_USER_NOTICE:

			define('IN_ERROR_HANDLER', true);

			if (empty($user->data))
			{
				$user->session_begin();
			}

			// We re-init the auth array to get correct results on login/logout
			$auth->acl($user->data);

			if (empty($user->lang))
			{
				$user->setup();
			}

			$msg_text = (!empty($user->lang[$msg_text])) ? $user->lang[$msg_text] : $msg_text;
			$msg_title = (!isset($msg_title)) ? $user->lang['INFORMATION'] : ((!empty($user->lang[$msg_title])) ? $user->lang[$msg_title] : $msg_title);

			if (!defined('HEADER_INC'))
			{
				if (defined('IN_ADMIN') && isset($user->data['session_admin']) && $user->data['session_admin'])
				{
					adm_page_header($msg_title);
				}
				else
				{
					page_header($msg_title);
				}
			}

			$template->set_filenames(array(
				'body' => 'message_body.html')
			);

			$template->assign_vars(array(
				'MESSAGE_TITLE'		=> $msg_title,
				'MESSAGE_TEXT'		=> $msg_text,
				'S_USER_WARNING'	=> ($errno == E_USER_WARNING) ? true : false,
				'S_USER_NOTICE'		=> ($errno == E_USER_NOTICE) ? true : false)
			);

			// We do not want the cron script to be called on error messages
			define('IN_CRON', true);

			if (defined('IN_ADMIN') && isset($user->data['session_admin']) && $user->data['session_admin'])
			{
				adm_page_footer();
			}
			else
			{
				page_footer();
			}

			exit_handler();
		break;
	}

	// If we notice an error not handled here we pass this back to PHP by returning false
	// This may not work for all php versions
	return false;
}

/**
* Queries the session table to get information about online guests
* @param int $forum_id Limits the search to the forum with this id
* @return int The number of active distinct guest sessions
*/
function obtain_guest_count($forum_id = 0)
{
	global $db, $config;
	
	if ($forum_id)
	{
		$reading_sql = ' AND s.session_forum_id = ' . (int) $forum_id;
	} 
	else
	{
		$reading_sql = '';
	}
	$time = (time() - (intval($config['load_online_time']) * 60)); 

	// Get number of online guests

	if ($db->sql_layer === 'sqlite')
	{
		$sql = 'SELECT COUNT(session_ip) as num_guests
			FROM (
				SELECT DISTINCT s.session_ip
				FROM ' . SESSIONS_TABLE . ' s
				WHERE s.session_user_id = ' . ANONYMOUS . '
					AND s.session_time >= ' . ($time - ((int) ($time % 60))) .
				$reading_sql .
			')';
	}
	else
	{
		$sql = 'SELECT COUNT(DISTINCT s.session_ip) as num_guests
			FROM ' . SESSIONS_TABLE . ' s
			WHERE s.session_user_id = ' . ANONYMOUS . '
				AND s.session_time >= ' . ($time - ((int) ($time % 60))) .
			$reading_sql;
	}
	$result = $db->sql_query($sql, 60);
	$guests_online = (int) $db->sql_fetchfield('num_guests');
	$db->sql_freeresult($result);
	
	return $guests_online;
}

/**
* Queries the session table to get information about online users
* @param int $forum_id Limits the search to the forum with this id
* @return array An array containing the ids of online, hidden and visible users, as well as statistical info
*/
function obtain_users_online($forum_id = 0)
{
	global $db, $config, $user;

	$reading_sql = '';
	if ($forum_id !== 0)
	{
		$reading_sql = ' AND s.session_forum_id = ' . (int) $forum_id;
	}

	$online_users = array(
		'online_users'			=> array(),
		'hidden_users'			=> array(),
		'total_online'			=> 0,
		'visible_online'		=> 0,
		'hidden_online'			=> 0,
		'guests_online'			=> 0,
	);

	if ($config['load_online_guests'])
	{
		$online_users['guests_online'] = obtain_guest_count($forum_id);
	}
	
	// a little discrete magic to cache this for 30 seconds
	$time = (time() - (intval($config['load_online_time']) * 60)); 

	$sql = 'SELECT s.session_user_id, s.session_ip, s.session_viewonline
		FROM ' . SESSIONS_TABLE . ' s
		WHERE s.session_time >= ' . ($time - ((int) ($time % 30))) .
			$reading_sql .
		' AND s.session_user_id <> ' . ANONYMOUS;
	$result = $db->sql_query($sql, 30); 

	while ($row = $db->sql_fetchrow($result))
	{
		// Skip multiple sessions for one user
		if (!isset($online_users['online_users'][$row['session_user_id']]))
		{
			$online_users['online_users'][$row['session_user_id']] = (int) $row['session_user_id'];
			if ($row['session_viewonline'])
			{
				$online_users['visible_online']++;
			}
			else
			{
				$online_users['hidden_users'][$row['session_user_id']] = (int) $row['session_user_id'];
				$online_users['hidden_online']++;
			}
		}
	}
	$online_users['total_online'] = $online_users['guests_online'] + $online_users['visible_online'] + $online_users['hidden_online'];
	$db->sql_freeresult($result);
	
	return $online_users;
}

/**
* Uses the result of obtain_users_online to generate a localized, readable representation.
* @param mixed $online_users result of obtain_users_online - array with user_id lists for total, hidden and visible users, and statistics
* @param int $forum_id Indicate that the data is limited to one forum and not global.
* @return array An array containing the string for output to the template
*/
function obtain_users_online_string($online_users, $forum_id = 0)
{
	global $config, $db, $user, $auth;

	$user_online_link = $online_userlist = '';

	if (sizeof($online_users['online_users']))
	{
		$sql = 'SELECT username, username_clean, user_id, user_type, user_allow_viewonline, user_colour
				FROM ' . USERS_TABLE . '
				WHERE ' . $db->sql_in_set('user_id', $online_users['online_users']) . '
				ORDER BY username_clean ASC';
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			// User is logged in and therefore not a guest
			if ($row['user_id'] != ANONYMOUS)
			{
				if (isset($online_users['hidden_users'][$row['user_id']]))
				{
					$row['username'] = '<em>' . $row['username'] . '</em>';
				}

				if (!isset($online_users['hidden_users'][$row['user_id']]) || $auth->acl_get('u_viewonline'))
				{
					$user_online_link = get_username_string(($row['user_type'] <> USER_IGNORE) ? 'full' : 'no_profile', $row['user_id'], $row['username'], $row['user_colour']);
					$online_userlist .= ($online_userlist != '') ? ', ' . $user_online_link : $user_online_link;
				}
			}
		}
		$db->sql_freeresult($result);
	}

	if (!$online_userlist)
	{
		$online_userlist = $user->lang['NO_ONLINE_USERS'];
	}

	if ($forum_id === 0)
	{
		$online_userlist = $user->lang['REGISTERED_USERS'] . ' ' . $online_userlist;
	}
	else if ($config['load_online_guests'])
	{
		$l_online = ($online_users['guests_online'] === 1) ? $user->lang['BROWSING_FORUM_GUEST'] : $user->lang['BROWSING_FORUM_GUESTS'];
		$online_userlist = sprintf($l_online, $online_userlist, $online_users['guests_online']);
	}
	else
	{
		$online_userlist = sprintf($user->lang['BROWSING_FORUM'], $online_userlist);
	}
	// Build online listing
	$vars_online = array(
		'ONLINE'	=> array('total_online', 'l_t_user_s', 0),
		'REG'		=> array('visible_online', 'l_r_user_s', !$config['load_online_guests']),
		'HIDDEN'	=> array('hidden_online', 'l_h_user_s', $config['load_online_guests']),
		'GUEST'		=> array('guests_online', 'l_g_user_s', 0)
	);

	foreach ($vars_online as $l_prefix => $var_ary)
	{
		if ($var_ary[2])
		{
			$l_suffix = '_AND';
		}
		else
		{
			$l_suffix = '';
		}
		switch ($online_users[$var_ary[0]])
		{
			case 0:
				${$var_ary[1]} = $user->lang[$l_prefix . '_USERS_ZERO_TOTAL' . $l_suffix];
			break;

			case 1:
				${$var_ary[1]} = $user->lang[$l_prefix . '_USER_TOTAL' . $l_suffix];
			break;

			default:
				${$var_ary[1]} = $user->lang[$l_prefix . '_USERS_TOTAL' . $l_suffix];
			break;
		}
	}
	unset($vars_online);

	$l_online_users = sprintf($l_t_user_s, $online_users['total_online']);
	$l_online_users .= sprintf($l_r_user_s, $online_users['visible_online']);
	$l_online_users .= sprintf($l_h_user_s, $online_users['hidden_online']);

	if ($config['load_online_guests'])
	{
		$l_online_users .= sprintf($l_g_user_s, $online_users['guests_online']);
	}



	return array(
		'online_userlist'	=> $online_userlist,
		'l_online_users'	=> $l_online_users,
	);
}


/**
* Generate page header
*/
function page_header($page_title = '', $display_online_list = true)
{
	global $db, $config, $template, $SID, $_SID, $user, $auth, $phpEx, $phpbb_root_path;

	if (defined('HEADER_INC'))
	{
		return;
	}

	define('HEADER_INC', true);

	// gzip_compression
	if ($config['gzip_compress'])
	{
		if (@extension_loaded('zlib') && !headers_sent())
		{
			ob_start('ob_gzhandler');
		}
	}

	// Generate logged in/logged out status
	if ($user->data['user_id'] != ANONYMOUS)
	{
		$u_login_logout = append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=logout', true, $user->session_id);
		$l_login_logout = sprintf($user->lang['LOGOUT_USER'], $user->data['username']);
	}
	else
	{
		$u_login_logout = append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=login');
		$l_login_logout = $user->lang['LOGIN'];
	}

	// Last visit date/time
	$s_last_visit = ($user->data['user_id'] != ANONYMOUS) ? $user->format_date($user->data['session_last_visit']) : '';

	// Get users online list ... if required
	$l_online_users = $online_userlist = $l_online_record = '';

	if ($config['load_online'] && $config['load_online_time'] && $display_online_list)
	{
		$f = request_var('f', 0);
		$f = max($f, 0);
		$online_users = obtain_users_online($f);
		$user_online_strings = obtain_users_online_string($online_users, $f);

		$l_online_users = $user_online_strings['l_online_users'];
		$online_userlist = $user_online_strings['online_userlist'];
		$total_online_users = $online_users['total_online'];

		if ($total_online_users > $config['record_online_users'])
		{
			set_config('record_online_users', $total_online_users, true);
			set_config('record_online_date', time(), true);
		}

		$l_online_record = sprintf($user->lang['RECORD_ONLINE_USERS'], $config['record_online_users'], $user->format_date($config['record_online_date']));

		$l_online_time = ($config['load_online_time'] == 1) ? 'VIEW_ONLINE_TIME' : 'VIEW_ONLINE_TIMES';
		$l_online_time = sprintf($user->lang[$l_online_time], $config['load_online_time']);
	}
	else
	{
		$l_online_time = '';
	}

	$l_privmsgs_text = $l_privmsgs_text_unread = '';
	$s_privmsg_new = false;

	// Obtain number of new private messages if user is logged in
	if (isset($user->data['is_registered']) && $user->data['is_registered'])
	{
		if ($user->data['user_new_privmsg'])
		{
			$l_message_new = ($user->data['user_new_privmsg'] == 1) ? $user->lang['NEW_PM'] : $user->lang['NEW_PMS'];
			$l_privmsgs_text = sprintf($l_message_new, $user->data['user_new_privmsg']);

			if (!$user->data['user_last_privmsg'] || $user->data['user_last_privmsg'] > $user->data['session_last_visit'])
			{
				$sql = 'UPDATE ' . USERS_TABLE . '
					SET user_last_privmsg = ' . $user->data['session_last_visit'] . '
					WHERE user_id = ' . $user->data['user_id'];
				$db->sql_query($sql);

				$s_privmsg_new = true;
			}
			else
			{
				$s_privmsg_new = false;
			}
		}
		else
		{
			$l_privmsgs_text = $user->lang['NO_NEW_PM'];
			$s_privmsg_new = false;
		}

		$l_privmsgs_text_unread = '';

		if ($user->data['user_unread_privmsg'] && $user->data['user_unread_privmsg'] != $user->data['user_new_privmsg'])
		{
			$l_message_unread = ($user->data['user_unread_privmsg'] == 1) ? $user->lang['UNREAD_PM'] : $user->lang['UNREAD_PMS'];
			$l_privmsgs_text_unread = sprintf($l_message_unread, $user->data['user_unread_privmsg']);
		}
	}

	// Which timezone?
	$tz = ($user->data['user_id'] != ANONYMOUS) ? strval(doubleval($user->data['user_timezone'])) : strval(doubleval($config['board_timezone']));

	// Send a proper content-language to the output
	$user_lang = $user->lang['USER_LANG'];
	if (strpos($user_lang, '-x-') !== false)
	{
		$user_lang = substr($user_lang, 0, strpos($user_lang, '-x-'));
	}

	// The following assigns all _common_ variables that may be used at any point in a template.
	$template->assign_vars(array(
		'SITENAME'						=> $config['sitename'],
		'SITE_DESCRIPTION'				=> $config['site_desc'],
		'PAGE_TITLE'					=> $page_title,
		'SCRIPT_NAME'					=> str_replace('.' . $phpEx, '', $user->page['page_name']),
		'LAST_VISIT_DATE'				=> sprintf($user->lang['YOU_LAST_VISIT'], $s_last_visit),
		'LAST_VISIT_YOU'				=> $s_last_visit,
		'CURRENT_TIME'					=> sprintf($user->lang['CURRENT_TIME'], $user->format_date(time(), false, true)),
		'TOTAL_USERS_ONLINE'			=> $l_online_users,
		'LOGGED_IN_USER_LIST'			=> $online_userlist,
		'RECORD_USERS'					=> $l_online_record,
		'PRIVATE_MESSAGE_INFO'			=> $l_privmsgs_text,
		'PRIVATE_MESSAGE_INFO_UNREAD'	=> $l_privmsgs_text_unread,

		'S_USER_NEW_PRIVMSG'			=> $user->data['user_new_privmsg'],
		'S_USER_UNREAD_PRIVMSG'			=> $user->data['user_unread_privmsg'],

		'SID'				=> $SID,
		'_SID'				=> $_SID,
		'SESSION_ID'		=> $user->session_id,
		'ROOT_PATH'			=> $phpbb_root_path,

		'L_LOGIN_LOGOUT'	=> $l_login_logout,
		'L_INDEX'			=> $user->lang['FORUM_INDEX'],
		'L_ONLINE_EXPLAIN'	=> $l_online_time,

		'U_PRIVATEMSGS'			=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;folder=inbox'),
		'U_RETURN_INBOX'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;folder=inbox'),
		'U_POPUP_PM'			=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;mode=popup'),
		'UA_POPUP_PM'			=> addslashes(append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=pm&amp;mode=popup')),
		'U_MEMBERLIST'			=> append_sid("{$phpbb_root_path}memberlist.$phpEx"),
		'U_VIEWONLINE'			=> ($auth->acl_gets('u_viewprofile', 'a_user', 'a_useradd', 'a_userdel')) ? append_sid("{$phpbb_root_path}viewonline.$phpEx") : '',
		'U_LOGIN_LOGOUT'		=> $u_login_logout,
		'U_INDEX'				=> append_sid("{$phpbb_root_path}index.$phpEx"),
		'U_SEARCH'				=> append_sid("{$phpbb_root_path}search.$phpEx"),
		'U_REGISTER'			=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=register'),
		'U_PROFILE'				=> append_sid("{$phpbb_root_path}ucp.$phpEx"),
		'U_MODCP'				=> append_sid("{$phpbb_root_path}mcp.$phpEx", false, true, $user->session_id),
		'U_FAQ'					=> append_sid("{$phpbb_root_path}faq.$phpEx"),
		'U_SEARCH_SELF'			=> append_sid("{$phpbb_root_path}search.$phpEx", 'search_id=egosearch'),
		'U_SEARCH_NEW'			=> append_sid("{$phpbb_root_path}search.$phpEx", 'search_id=newposts'),
		'U_SEARCH_UNANSWERED'	=> append_sid("{$phpbb_root_path}search.$phpEx", 'search_id=unanswered'),
		'U_SEARCH_ACTIVE_TOPICS'=> append_sid("{$phpbb_root_path}search.$phpEx", 'search_id=active_topics'),
		'U_DELETE_COOKIES'		=> append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=delete_cookies'),
		'U_TEAM'				=> append_sid("{$phpbb_root_path}memberlist.$phpEx", 'mode=leaders'),
		'U_RESTORE_PERMISSIONS'	=> ($user->data['user_perm_from'] && $auth->acl_get('a_switchperm')) ? append_sid("{$phpbb_root_path}ucp.$phpEx", 'mode=restore_perm') : '',

		'S_USER_LOGGED_IN'		=> ($user->data['user_id'] != ANONYMOUS) ? true : false,
		'S_AUTOLOGIN_ENABLED'	=> ($config['allow_autologin']) ? true : false,
		'S_BOARD_DISABLED'		=> ($config['board_disable']) ? true : false,
		'S_REGISTERED_USER'		=> $user->data['is_registered'],
		'S_IS_BOT'				=> $user->data['is_bot'],
		'S_USER_PM_POPUP'		=> $user->optionget('popuppm'),
		'S_USER_LANG'			=> $user_lang,
		'S_USER_BROWSER'		=> (isset($user->data['session_browser'])) ? $user->data['session_browser'] : $user->lang['UNKNOWN_BROWSER'],
		'S_USERNAME'			=> $user->data['username'],
		'S_CONTENT_DIRECTION'	=> $user->lang['DIRECTION'],
		'S_CONTENT_FLOW_BEGIN'	=> ($user->lang['DIRECTION'] == 'ltr') ? 'left' : 'right',
		'S_CONTENT_FLOW_END'	=> ($user->lang['DIRECTION'] == 'ltr') ? 'right' : 'left',
		'S_CONTENT_ENCODING'	=> 'UTF-8',
		'S_TIMEZONE'			=> ($user->data['user_dst'] || ($user->data['user_id'] == ANONYMOUS && $config['board_dst'])) ? sprintf($user->lang['ALL_TIMES'], $user->lang['tz'][$tz], $user->lang['tz']['dst']) : sprintf($user->lang['ALL_TIMES'], $user->lang['tz'][$tz], ''),
		'S_DISPLAY_ONLINE_LIST'	=> ($l_online_time) ? 1 : 0,
		'S_DISPLAY_SEARCH'		=> (!$config['load_search']) ? 0 : (isset($auth) ? ($auth->acl_get('u_search') && $auth->acl_getf_global('f_search')) : 1),
		'S_DISPLAY_PM'			=> ($config['allow_privmsg'] && $user->data['is_registered'] && ($auth->acl_get('u_readpm') || $auth->acl_get('u_sendpm'))) ? true : false,
		'S_DISPLAY_MEMBERLIST'	=> (isset($auth)) ? $auth->acl_get('u_viewprofile') : 0,
		'S_NEW_PM'				=> ($s_privmsg_new) ? 1 : 0,
		'S_REGISTER_ENABLED'	=> ($config['require_activation'] != USER_ACTIVATION_DISABLE) ? true : false,

		'T_THEME_PATH'			=> "{$phpbb_root_path}styles/" . $user->theme['theme_path'] . '/theme',
		'T_TEMPLATE_PATH'		=> "{$phpbb_root_path}styles/" . $user->theme['template_path'] . '/template',
		'T_IMAGESET_PATH'		=> "{$phpbb_root_path}styles/" . $user->theme['imageset_path'] . '/imageset',
		'T_IMAGESET_LANG_PATH'	=> "{$phpbb_root_path}styles/" . $user->theme['imageset_path'] . '/imageset/' . $user->data['user_lang'],
		'T_IMAGES_PATH'			=> "{$phpbb_root_path}images/",
		'T_SMILIES_PATH'		=> "{$phpbb_root_path}{$config['smilies_path']}/",
		'T_AVATAR_PATH'			=> "{$phpbb_root_path}{$config['avatar_path']}/",
		'T_AVATAR_GALLERY_PATH'	=> "{$phpbb_root_path}{$config['avatar_gallery_path']}/",
		'T_ICONS_PATH'			=> "{$phpbb_root_path}{$config['icons_path']}/",
		'T_RANKS_PATH'			=> "{$phpbb_root_path}{$config['ranks_path']}/",
		'T_UPLOAD_PATH'			=> "{$phpbb_root_path}{$config['upload_path']}/",
		'T_STYLESHEET_LINK'		=> (!$user->theme['theme_storedb']) ? "{$phpbb_root_path}styles/" . $user->theme['theme_path'] . '/theme/stylesheet.css' : "{$phpbb_root_path}style.$phpEx?sid=$user->session_id&amp;id=" . $user->theme['style_id'] . '&amp;lang=' . $user->data['user_lang'],
		'T_STYLESHEET_NAME'		=> $user->theme['theme_name'],

		'SITE_LOGO_IMG'			=> $user->img('site_logo'))
	);

	// application/xhtml+xml not used because of IE
	header('Content-type: text/html; charset=UTF-8');

	header('Cache-Control: private, no-cache="set-cookie"');
	header('Expires: 0');
	header('Pragma: no-cache');

	return;
}

/**
* Generate page footer
*/
function page_footer($run_cron = true)
{
	global $db, $config, $template, $user, $auth, $cache, $starttime, $phpbb_root_path, $phpEx;

	// Output page creation time
	if (defined('DEBUG'))
	{
		$mtime = explode(' ', microtime());
		$totaltime = $mtime[0] + $mtime[1] - $starttime;

		if (!empty($_REQUEST['explain']) && $auth->acl_get('a_') && defined('DEBUG_EXTRA') && method_exists($db, 'sql_report'))
		{
			$db->sql_report('display');
		}

		$debug_output = sprintf('Time : %.3fs | ' . $db->sql_num_queries() . ' Queries | GZIP : ' . (($config['gzip_compress']) ? 'On' : 'Off') . (($user->load) ? ' | Load : ' . $user->load : ''), $totaltime);

		if ($auth->acl_get('a_') && defined('DEBUG_EXTRA'))
		{
			if (function_exists('memory_get_usage'))
			{
				if ($memory_usage = memory_get_usage())
				{
					global $base_memory_usage;
					$memory_usage -= $base_memory_usage;
					$memory_usage = get_formatted_filesize($memory_usage);

					$debug_output .= ' | Memory Usage: ' . $memory_usage;
				}
			}

			$debug_output .= ' | <a href="' . build_url() . '&amp;explain=1">Explain</a>';
		}
	}

	$template->assign_vars(array(
		'DEBUG_OUTPUT'			=> (defined('DEBUG')) ? $debug_output : '',
		'TRANSLATION_INFO'		=> (!empty($user->lang['TRANSLATION_INFO'])) ? $user->lang['TRANSLATION_INFO'] : '',

		'U_ACP' => ($auth->acl_get('a_') && $user->data['is_registered']) ? append_sid("{$phpbb_root_path}adm/index.$phpEx", false, true, $user->session_id) : '')
	);

	// Call cron-type script
	if (!defined('IN_CRON') && $run_cron && !$config['board_disable'])
	{
		$cron_type = '';

		if (time() - $config['queue_interval'] > $config['last_queue_run'] && !defined('IN_ADMIN') && file_exists($phpbb_root_path . 'cache/queue.' . $phpEx))
		{
			// Process email queue
			$cron_type = 'queue';
		}
		else if (method_exists($cache, 'tidy') && time() - $config['cache_gc'] > $config['cache_last_gc'])
		{
			// Tidy the cache
			$cron_type = 'tidy_cache';
		}
		else if (time() - $config['warnings_gc'] > $config['warnings_last_gc'])
		{
			$cron_type = 'tidy_warnings';
		}
		else if (time() - $config['database_gc'] > $config['database_last_gc'])
		{
			// Tidy the database
			$cron_type = 'tidy_database';
		}
		else if (time() - $config['search_gc'] > $config['search_last_gc'])
		{
			// Tidy the search
			$cron_type = 'tidy_search';
		}
		else if (time() - $config['session_gc'] > $config['session_last_gc'])
		{
			$cron_type = 'tidy_sessions';
		}

		if ($cron_type)
		{
			$template->assign_var('RUN_CRON_TASK', '<img src="' . append_sid($phpbb_root_path . 'cron.' . $phpEx, 'cron_type=' . $cron_type) . '" width="1" height="1" alt="cron" />');
		}
	}

	$template->display('body');

	garbage_collection();
	exit_handler();
}

/**
* Closing the cache object and the database
* Cool function name, eh? We might want to add operations to it later
*/
function garbage_collection()
{
	global $cache, $db;

	// Unload cache, must be done before the DB connection if closed
	if (!empty($cache))
	{
		$cache->unload();
	}

	// Close our DB connection.
	if (!empty($db))
	{
		$db->sql_close();
	}
}

/**
* Handler for exit calls in phpBB.
* This function supports hooks.
*
* Note: This function is called after the template has been outputted.
*/
function exit_handler()
{
	global $phpbb_hook;

	if (!empty($phpbb_hook) && $phpbb_hook->call_hook(__FUNCTION__))
	{
		if ($phpbb_hook->hook_return(__FUNCTION__))
		{
			return $phpbb_hook->hook_return_result(__FUNCTION__);
		}
	}

	// As a pre-caution... some setups display a blank page if the flush() is not there.
	@flush();

	exit;
}

/**
* Handler for init calls in phpBB. This function is called in user::setup();
* This function supports hooks.
*/
function phpbb_user_session_handler()
{
	global $phpbb_hook;

	if (!empty($phpbb_hook) && $phpbb_hook->call_hook(__FUNCTION__))
	{
		if ($phpbb_hook->hook_return(__FUNCTION__))
		{
			return $phpbb_hook->hook_return_result(__FUNCTION__);
		}
	}

	return;
}

?>