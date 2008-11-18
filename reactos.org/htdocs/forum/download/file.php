<?php
/**
*
* @package phpBB3
* @version $Id: file.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
define('IN_PHPBB', true);
$phpbb_root_path = (defined('PHPBB_ROOT_PATH')) ? PHPBB_ROOT_PATH : './../';
$phpEx = substr(strrchr(__FILE__, '.'), 1);

if (isset($_GET['avatar']))
{
	require($phpbb_root_path . 'config.' . $phpEx);
	require($phpbb_root_path . 'includes/acm/acm_' . $acm_type . '.' . $phpEx);
	require($phpbb_root_path . 'includes/cache.' . $phpEx);
	require($phpbb_root_path . 'includes/db/' . $dbms . '.' . $phpEx);
	require($phpbb_root_path . 'includes/constants.' . $phpEx);

	$db = new $sql_db();
	$cache = new cache();

	// Connect to DB
	if (!@$db->sql_connect($dbhost, $dbuser, $dbpasswd, $dbname, $dbport, false, false))
	{
		exit;
	}
	unset($dbpasswd);

	// worst-case default
	$browser = (!empty($_SERVER['HTTP_USER_AGENT'])) ? htmlspecialchars((string) $_SERVER['HTTP_USER_AGENT']) : 'msie 6.0';

	$config = $cache->obtain_config();
	$filename = $_GET['avatar'];
	$avatar_group = false;
	if ($filename[0] === 'g')
	{
		$avatar_group = true;
		$filename = substr($filename, 1);
	}

	// '==' is not a bug - . as the first char is as bad as no dot at all
	if (strpos($filename, '.') == false)
	{
		header('HTTP/1.0 403 Forbidden');
		if (!empty($cache))
		{
			$cache->unload();
		}
		$db->sql_close();
		exit;
	}

	$ext		= substr(strrchr($filename, '.'), 1);
	$stamp		= (int) substr(stristr($filename, '_'), 1);
	$filename	= (int) $filename;

	// let's see if we have to send the file at all
	$last_load 	=  isset($_SERVER['HTTP_IF_MODIFIED_SINCE']) ? strtotime(trim($_SERVER['HTTP_IF_MODIFIED_SINCE'])) : false;
	if (strpos(strtolower($browser), 'msie 6.0') === false)
	{
		if ($last_load !== false && $last_load <= $stamp)
		{
			if (@php_sapi_name() === 'CGI')
			{
				header('Status: 304 Not Modified', true, 304);
			}
			else
			{
				header('HTTP/1.0 304 Not Modified', true, 304);
			}
			// seems that we need those too ... browsers
			header('Pragma: public');
			header('Expires: ' . gmdate('D, d M Y H:i:s \G\M\T', time() + 31536000));
			exit();
		}
		else
		{
			header('Last-Modified: ' . gmdate('D, d M Y H:i:s', $stamp) . ' GMT');
		}
	}

	if (!in_array($ext, array('png', 'gif', 'jpg', 'jpeg')))
	{
		// no way such an avatar could exist. They are not following the rules, stop the show.
		header("HTTP/1.0 403 Forbidden");
		if (!empty($cache))
		{
			$cache->unload();
		}
		$db->sql_close();
		exit;
	}

	if (!$filename)
	{
		// no way such an avatar could exist. They are not following the rules, stop the show.
		header("HTTP/1.0 403 Forbidden");
		if (!empty($cache))
		{
			$cache->unload();
		}
		$db->sql_close();
		exit;
	}

	send_avatar_to_browser(($avatar_group ? 'g' : '') . $filename . '.' . $ext, $browser);

	if (!empty($cache))
	{
		$cache->unload();
	}
	$db->sql_close();
	exit;
}

// implicit else: we are not in avatar mode
include($phpbb_root_path . 'common.' . $phpEx);

$download_id = request_var('id', 0);
$mode = request_var('mode', '');
$thumbnail = request_var('t', false);

// Start session management, do not update session page.
$user->session_begin(false);
$auth->acl($user->data);
$user->setup('viewtopic');

if (!$download_id)
{
	trigger_error('NO_ATTACHMENT_SELECTED');
}

if (!$config['allow_attachments'] && !$config['allow_pm_attach'])
{
	trigger_error('ATTACHMENT_FUNCTIONALITY_DISABLED');
}

$sql = 'SELECT attach_id, in_message, post_msg_id, extension, is_orphan, poster_id
	FROM ' . ATTACHMENTS_TABLE . "
	WHERE attach_id = $download_id";
$result = $db->sql_query_limit($sql, 1);
$attachment = $db->sql_fetchrow($result);
$db->sql_freeresult($result);

if (!$attachment)
{
	trigger_error('ERROR_NO_ATTACHMENT');
}

if ((!$attachment['in_message'] && !$config['allow_attachments']) || ($attachment['in_message'] && !$config['allow_pm_attach']))
{
	trigger_error('ATTACHMENT_FUNCTIONALITY_DISABLED');
}

$row = array();

if ($attachment['is_orphan'])
{
	// We allow admins having attachment permissions to see orphan attachments...
	$own_attachment = ($auth->acl_get('a_attach') || $attachment['poster_id'] == $user->data['user_id']) ? true : false;

	if (!$own_attachment || ($attachment['in_message'] && !$auth->acl_get('u_pm_download')) || (!$attachment['in_message'] && !$auth->acl_get('u_download')))
	{
		trigger_error('ERROR_NO_ATTACHMENT');
	}

	// Obtain all extensions...
	$extensions = $cache->obtain_attach_extensions(true);
}
else
{
	if (!$attachment['in_message'])
	{
		//
		$sql = 'SELECT p.forum_id, f.forum_password, f.parent_id
			FROM ' . POSTS_TABLE . ' p, ' . FORUMS_TABLE . ' f
			WHERE p.post_id = ' . $attachment['post_msg_id'] . '
				AND p.forum_id = f.forum_id';
		$result = $db->sql_query_limit($sql, 1);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		// Global announcement?
		$f_download = (!$row) ? $auth->acl_getf_global('f_download') : $auth->acl_get('f_download', $row['forum_id']);

		if ($auth->acl_get('u_download') && $f_download)
		{
			if ($row && $row['forum_password'])
			{
				// Do something else ... ?
				login_forum_box($row);
			}
		}
		else
		{
			trigger_error('SORRY_AUTH_VIEW_ATTACH');
		}
	}
	else
	{
		$row['forum_id'] = false;
		if (!$auth->acl_get('u_pm_download'))
		{
			header('HTTP/1.0 403 Forbidden');
			trigger_error('SORRY_AUTH_VIEW_ATTACH');
		}

		// Check if the attachment is within the users scope...
		$sql = 'SELECT user_id, author_id
			FROM ' . PRIVMSGS_TO_TABLE . '
			WHERE msg_id = ' . $attachment['post_msg_id'];
		$result = $db->sql_query($sql);

		$allowed = false;
		while ($user_row = $db->sql_fetchrow($result))
		{
			if ($user->data['user_id'] == $user_row['user_id'] || $user->data['user_id'] == $user_row['author_id'])
			{
				$allowed = true;
				break;
			}
		}
		$db->sql_freeresult($result);

		if (!$allowed)
		{
			header('HTTP/1.0 403 Forbidden');
			trigger_error('ERROR_NO_ATTACHMENT');
		}
	}

	// disallowed?
	$extensions = array();
	if (!extension_allowed($row['forum_id'], $attachment['extension'], $extensions))
	{
		trigger_error(sprintf($user->lang['EXTENSION_DISABLED_AFTER_POSTING'], $attachment['extension']));
	}
}

if (!download_allowed())
{
	header('HTTP/1.0 403 Forbidden');
	trigger_error($user->lang['LINKAGE_FORBIDDEN']);
}

$download_mode = (int) $extensions[$attachment['extension']]['download_mode'];

// Fetching filename here to prevent sniffing of filename
$sql = 'SELECT attach_id, is_orphan, in_message, post_msg_id, extension, physical_filename, real_filename, mimetype
	FROM ' . ATTACHMENTS_TABLE . "
	WHERE attach_id = $download_id";
$result = $db->sql_query_limit($sql, 1);
$attachment = $db->sql_fetchrow($result);
$db->sql_freeresult($result);

if (!$attachment)
{
	trigger_error('ERROR_NO_ATTACHMENT');
}

$attachment['physical_filename'] = basename($attachment['physical_filename']);
$display_cat = $extensions[$attachment['extension']]['display_cat'];

if (($display_cat == ATTACHMENT_CATEGORY_IMAGE || $display_cat == ATTACHMENT_CATEGORY_THUMB) && !$user->optionget('viewimg'))
{
	$display_cat = ATTACHMENT_CATEGORY_NONE;
}

if ($display_cat == ATTACHMENT_CATEGORY_FLASH && !$user->optionget('viewflash'))
{
	$display_cat = ATTACHMENT_CATEGORY_NONE;
}

if ($thumbnail)
{
	$attachment['physical_filename'] = 'thumb_' . $attachment['physical_filename'];
}
else if (($display_cat == ATTACHMENT_CATEGORY_NONE || $display_cat == ATTACHMENT_CATEGORY_IMAGE) && !$attachment['is_orphan'])
{
	// Update download count
	$sql = 'UPDATE ' . ATTACHMENTS_TABLE . '
		SET download_count = download_count + 1
		WHERE attach_id = ' . $attachment['attach_id'];
	$db->sql_query($sql);
}

if ($display_cat == ATTACHMENT_CATEGORY_IMAGE && $mode === 'view' && (strpos($attachment['mimetype'], 'image') === 0) && strpos(strtolower($user->browser), 'msie') !== false)
{
	wrap_img_in_html(append_sid($phpbb_root_path . 'download/file.' . $phpEx, 'id=' . $attachment['attach_id']), $attachment['real_filename']);
}
else
{
	// Determine the 'presenting'-method
	if ($download_mode == PHYSICAL_LINK)
	{
		// This presenting method should no longer be used
		if (!@is_dir($phpbb_root_path . $config['upload_path']))
		{
			trigger_error($user->lang['PHYSICAL_DOWNLOAD_NOT_POSSIBLE']);
		}

		redirect($phpbb_root_path . $config['upload_path'] . '/' . $attachment['physical_filename']);
		exit;
	}
	else
	{
		send_file_to_browser($attachment, $config['upload_path'], $display_cat);
		exit;
	}
}


/**
* A simplified function to deliver avatars
* The argument needs to be checked before calling this function.
*/
function send_avatar_to_browser($file, $browser)
{
	global $config, $phpbb_root_path;

	$prefix = $config['avatar_salt'] . '_';
	$image_dir = $config['avatar_path'];

	// Adjust image_dir path (no trailing slash)
	if (substr($image_dir, -1, 1) == '/' || substr($image_dir, -1, 1) == '\\')
	{
		$image_dir = substr($image_dir, 0, -1) . '/';
	}
	$image_dir = str_replace(array('../', '..\\', './', '.\\'), '', $image_dir);

	if ($image_dir && ($image_dir[0] == '/' || $image_dir[0] == '\\'))
	{
		$image_dir = '';
	}
	$file_path = $phpbb_root_path . $image_dir . '/' . $prefix . $file;

	if ((@file_exists($file_path) && @is_readable($file_path)) && !headers_sent())
	{
		header('Pragma: public');

		$image_data = @getimagesize($file_path);
		header('Content-Type: ' . image_type_to_mime_type($image_data[2]));

		if (strpos(strtolower($browser), 'msie') !== false)
		{
			header('Content-Disposition: attachment; ' . header_filename($file));

			if (strpos(strtolower($browser), 'msie 6.0') !== false)
			{
				header('Expires: -1');
			}
			else
			{
				header('Expires: ' . gmdate('D, d M Y H:i:s \G\M\T', time() + 31536000));
			}
		}
		else
		{
			header('Content-Disposition: inline; ' . header_filename($file));
			header('Expires: ' . gmdate('D, d M Y H:i:s \G\M\T', time() + 31536000));
		}

		$size = @filesize($file_path);
		if ($size)
		{
			header("Content-Length: $size");
		}

		if (@readfile($file_path) === false)
		{
			$fp = @fopen($file_path, 'rb');

			if ($fp !== false)
			{
				while (!feof($fp))
				{
					echo fread($fp, 8192);
				}
				fclose($fp);
			}
		}

		flush();
	}
	else
	{
		header('HTTP/1.0 404 not found');
	}
}

/**
* Wraps an url into a simple html page. Used to display attachments in IE.
* this is a workaround for now; might be moved to template system later
* direct any complaints to 1 Microsoft Way, Redmond
*/
function wrap_img_in_html($src, $title)
{
	echo '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-Strict.dtd">';
	echo '<html>';
	echo '<head>';
	echo '<meta http-equiv="content-type" content="text/html; charset=UTF-8" />';
	echo '<title>' . $title . '</title>';
	echo '</head>';
	echo '<body>';
	echo '<div>';
	echo '<img src="' . $src . '" alt="' . $title . '" />';
	echo '</div>';
	echo '</body>';
	echo '</html>';
}

/**
* Send file to browser
*/
function send_file_to_browser($attachment, $upload_dir, $category)
{
	global $user, $db, $config, $phpbb_root_path;

	$filename = $phpbb_root_path . $upload_dir . '/' . $attachment['physical_filename'];

	if (!@file_exists($filename))
	{
		trigger_error($user->lang['ERROR_NO_ATTACHMENT'] . '<br /><br />' . sprintf($user->lang['FILE_NOT_FOUND_404'], $filename));
	}

	// Correct the mime type - we force application/octetstream for all files, except images
	// Please do not change this, it is a security precaution
	if ($category != ATTACHMENT_CATEGORY_IMAGE || strpos($attachment['mimetype'], 'image') !== 0)
	{
		$attachment['mimetype'] = (strpos(strtolower($user->browser), 'msie') !== false || strpos(strtolower($user->browser), 'opera') !== false) ? 'application/octetstream' : 'application/octet-stream';
	}

	if (@ob_get_length())
	{
		@ob_end_clean();
	}

	// Now send the File Contents to the Browser
	$size = @filesize($filename);

	// To correctly display further errors we need to make sure we are using the correct headers for both (unsetting content-length may not work)

	// Check if headers already sent or not able to get the file contents.
	if (headers_sent() || !@file_exists($filename) || !@is_readable($filename))
	{
		// PHP track_errors setting On?
		if (!empty($php_errormsg))
		{
			trigger_error($user->lang['UNABLE_TO_DELIVER_FILE'] . '<br />' . sprintf($user->lang['TRACKED_PHP_ERROR'], $php_errormsg));
		}

		trigger_error('UNABLE_TO_DELIVER_FILE');
	}

	// Now the tricky part... let's dance
	header('Pragma: public');

	/**
	* Commented out X-Sendfile support. To not expose the physical filename within the header if xsendfile is absent we need to look into methods of checking it's status.
	*
	* Try X-Sendfile since it is much more server friendly - only works if the path is *not* outside of the root path...
	* lighttpd has core support for it. An apache2 module is available at http://celebnamer.celebworld.ws/stuff/mod_xsendfile/
	*
	* Not really ideal, but should work fine...
	* <code>
	*	if (strpos($upload_dir, '/') !== 0 && strpos($upload_dir, '../') === false)
	*	{
	*		header('X-Sendfile: ' . $filename);
	*	}
	* </code>
	*/

	// Send out the Headers. Do not set Content-Disposition to inline please, it is a security measure for users using the Internet Explorer.
	header('Content-Type: ' . $attachment['mimetype']);

	if (empty($user->browser) || (strpos(strtolower($user->browser), 'msie') !== false))
	{
		header('Content-Disposition: attachment; ' . header_filename(htmlspecialchars_decode($attachment['real_filename'])));
		if (empty($user->browser) || (strpos(strtolower($user->browser), 'msie 6.0') !== false))
		{
			header('expires: -1');
		}
	}
	else
	{
		header('Content-Disposition: ' . ((strpos($attachment['mimetype'], 'image') === 0) ? 'inline' : 'attachment') . '; ' . header_filename(htmlspecialchars_decode($attachment['real_filename'])));
	}

	if ($size)
	{
		header("Content-Length: $size");
	}

	// Try to deliver in chunks
	@set_time_limit(0);

	$fp = @fopen($filename, 'rb');

	if ($fp !== false)
	{
		while (!feof($fp))
		{
			echo fread($fp, 8192);
		}
		fclose($fp);
	}
	else
	{
		@readfile($filename);
	}

	flush();
	exit;
}

/**
* Get a browser friendly UTF-8 encoded filename
*/
function header_filename($file)
{
	$user_agent = (!empty($_SERVER['HTTP_USER_AGENT'])) ? htmlspecialchars((string) $_SERVER['HTTP_USER_AGENT']) : '';

	// There be dragons here.
	// Not many follows the RFC...
	if (strpos($user_agent, 'MSIE') !== false || strpos($user_agent, 'Safari') !== false || strpos($user_agent, 'Konqueror') !== false)
	{
		return "filename=" . rawurlencode($file);
	}

	// follow the RFC for extended filename for the rest
	return "filename*=UTF-8''" . rawurlencode($file);
}

/**
* Check if downloading item is allowed
*/
function download_allowed()
{
	global $config, $user, $db;

	if (!$config['secure_downloads'])
	{
		return true;
	}

	$url = (!empty($_SERVER['HTTP_REFERER'])) ? trim($_SERVER['HTTP_REFERER']) : trim(getenv('HTTP_REFERER'));

	if (!$url)
	{
		return ($config['secure_allow_empty_referer']) ? true : false;
	}

	// Split URL into domain and script part
	$url = @parse_url($url);

	if ($url === false)
	{
		return ($config['secure_allow_empty_referer']) ? true : false;
	}

	$hostname = $url['host'];
	unset($url);

	$allowed = ($config['secure_allow_deny']) ? false : true;
	$iplist = array();

	if (($ip_ary = @gethostbynamel($hostname)) !== false)
	{
		foreach ($ip_ary as $ip)
		{
			if ($ip)
			{
				$iplist[] = $ip;
			}
		}
	}

	// Check for own server...
	$server_name = $user->host;

	// Forcing server vars is the only way to specify/override the protocol
	if ($config['force_server_vars'] || !$server_name)
	{
		$server_name = $config['server_name'];
	}

	if (preg_match('#^.*?' . preg_quote($server_name, '#') . '.*?$#i', $hostname))
	{
		$allowed = true;
	}

	// Get IP's and Hostnames
	if (!$allowed)
	{
		$sql = 'SELECT site_ip, site_hostname, ip_exclude
			FROM ' . SITELIST_TABLE;
		$result = $db->sql_query($sql);

		while ($row = $db->sql_fetchrow($result))
		{
			$site_ip = trim($row['site_ip']);
			$site_hostname = trim($row['site_hostname']);

			if ($site_ip)
			{
				foreach ($iplist as $ip)
				{
					if (preg_match('#^' . str_replace('\*', '.*?', preg_quote($site_ip, '#')) . '$#i', $ip))
					{
						if ($row['ip_exclude'])
						{
							$allowed = ($config['secure_allow_deny']) ? false : true;
							break 2;
						}
						else
						{
							$allowed = ($config['secure_allow_deny']) ? true : false;
						}
					}
				}
			}

			if ($site_hostname)
			{
				if (preg_match('#^' . str_replace('\*', '.*?', preg_quote($site_hostname, '#')) . '$#i', $hostname))
				{
					if ($row['ip_exclude'])
					{
						$allowed = ($config['secure_allow_deny']) ? false : true;
						break;
					}
					else
					{
						$allowed = ($config['secure_allow_deny']) ? true : false;
					}
				}
			}
		}
		$db->sql_freeresult($result);
	}

	return $allowed;
}

?>