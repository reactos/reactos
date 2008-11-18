<?php
/**
*
* @package phpBB3
* @version $Id: session.php 8479 2008-03-29 00:22:48Z naderman $
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
* Session class
* @package phpBB3
*/
class session
{
	var $cookie_data = array();
	var $page = array();
	var $data = array();
	var $browser = '';
	var $forwarded_for = '';
	var $host = '';
	var $session_id = '';
	var $ip = '';
	var $load = 0;
	var $time_now = 0;
	var $update_session_page = true;

	/**
	* Extract current session page
	*
	* @param string $root_path current root path (phpbb_root_path)
	*/
	function extract_current_page($root_path)
	{
		$page_array = array();

		// First of all, get the request uri...
		$script_name = (!empty($_SERVER['PHP_SELF'])) ? $_SERVER['PHP_SELF'] : getenv('PHP_SELF');
		$args = (!empty($_SERVER['QUERY_STRING'])) ? explode('&', $_SERVER['QUERY_STRING']) : explode('&', getenv('QUERY_STRING'));

		// If we are unable to get the script name we use REQUEST_URI as a failover and note it within the page array for easier support...
		if (!$script_name)
		{
			$script_name = (!empty($_SERVER['REQUEST_URI'])) ? $_SERVER['REQUEST_URI'] : getenv('REQUEST_URI');
			$script_name = (($pos = strpos($script_name, '?')) !== false) ? substr($script_name, 0, $pos) : $script_name;
			$page_array['failover'] = 1;
		}

		// Replace backslashes and doubled slashes (could happen on some proxy setups)
		$script_name = str_replace(array('\\', '//'), '/', $script_name);

		// Now, remove the sid and let us get a clean query string...
		$use_args = array();

		// Since some browser do not encode correctly we need to do this with some "special" characters...
		// " -> %22, ' => %27, < -> %3C, > -> %3E
		$find = array('"', "'", '<', '>');
		$replace = array('%22', '%27', '%3C', '%3E');

		foreach ($args as $key => $argument)
		{
			if (strpos($argument, 'sid=') === 0 || strpos($argument, '_f_=') === 0)
			{
				continue;
			}

			$use_args[str_replace($find, $replace, $key)] = str_replace($find, $replace, $argument);
		}
		unset($args);

		// The following examples given are for an request uri of {path to the phpbb directory}/adm/index.php?i=10&b=2

		// The current query string
		$query_string = trim(implode('&', $use_args));

		// basenamed page name (for example: index.php)
		$page_name = basename($script_name);
		$page_name = urlencode(htmlspecialchars($page_name));

		// current directory within the phpBB root (for example: adm)
		$root_dirs = explode('/', str_replace('\\', '/', phpbb_realpath($root_path)));
		$page_dirs = explode('/', str_replace('\\', '/', phpbb_realpath('./')));
		$intersection = array_intersect_assoc($root_dirs, $page_dirs);

		$root_dirs = array_diff_assoc($root_dirs, $intersection);
		$page_dirs = array_diff_assoc($page_dirs, $intersection);

		$page_dir = str_repeat('../', sizeof($root_dirs)) . implode('/', $page_dirs);

		if ($page_dir && substr($page_dir, -1, 1) == '/')
		{
			$page_dir = substr($page_dir, 0, -1);
		}

		// Current page from phpBB root (for example: adm/index.php?i=10&b=2)
		$page = (($page_dir) ? $page_dir . '/' : '') . $page_name . (($query_string) ? "?$query_string" : '');

		// The script path from the webroot to the current directory (for example: /phpBB3/adm/) : always prefixed with / and ends in /
		$script_path = trim(str_replace('\\', '/', dirname($script_name)));

		// The script path from the webroot to the phpBB root (for example: /phpBB3/)
		$script_dirs = explode('/', $script_path);
		array_splice($script_dirs, -sizeof($page_dirs));
		$root_script_path = implode('/', $script_dirs) . (sizeof($root_dirs) ? '/' . implode('/', $root_dirs) : '');

		// We are on the base level (phpBB root == webroot), lets adjust the variables a bit...
		if (!$root_script_path)
		{
			$root_script_path = ($page_dir) ? str_replace($page_dir, '', $script_path) : $script_path;
		}

		$script_path .= (substr($script_path, -1, 1) == '/') ? '' : '/';
		$root_script_path .= (substr($root_script_path, -1, 1) == '/') ? '' : '/';

		$page_array += array(
			'page_name'			=> $page_name,
			'page_dir'			=> $page_dir,

			'query_string'		=> $query_string,
			'script_path'		=> str_replace(' ', '%20', htmlspecialchars($script_path)),
			'root_script_path'	=> str_replace(' ', '%20', htmlspecialchars($root_script_path)),

			'page'				=> $page,
			'forum'				=> (isset($_REQUEST['f']) && $_REQUEST['f'] > 0) ? (int) $_REQUEST['f'] : 0,
		);

		return $page_array;
	}

	/**
	* Start session management
	*
	* This is where all session activity begins. We gather various pieces of
	* information from the client and server. We test to see if a session already
	* exists. If it does, fine and dandy. If it doesn't we'll go on to create a
	* new one ... pretty logical heh? We also examine the system load (if we're
	* running on a system which makes such information readily available) and
	* halt if it's above an admin definable limit.
	*
	* @param bool $update_session_page if true the session page gets updated.
	*			This can be set to circumvent certain scripts to update the users last visited page.
	*/
	function session_begin($update_session_page = true)
	{
		global $phpEx, $SID, $_SID, $_EXTRA_URL, $db, $config, $phpbb_root_path;

		// Give us some basic information
		$this->time_now				= time();
		$this->cookie_data			= array('u' => 0, 'k' => '');
		$this->update_session_page	= $update_session_page;
		$this->browser				= (!empty($_SERVER['HTTP_USER_AGENT'])) ? htmlspecialchars((string) $_SERVER['HTTP_USER_AGENT']) : '';
		$this->forwarded_for		= (!empty($_SERVER['HTTP_X_FORWARDED_FOR'])) ? (string) $_SERVER['HTTP_X_FORWARDED_FOR'] : '';
		$this->host					= (!empty($_SERVER['HTTP_HOST'])) ? (string) strtolower($_SERVER['HTTP_HOST']) : ((!empty($_SERVER['SERVER_NAME'])) ? $_SERVER['SERVER_NAME'] : getenv('SERVER_NAME'));
		$this->page					= $this->extract_current_page($phpbb_root_path);

		// if the forwarded for header shall be checked we have to validate its contents
		if ($config['forwarded_for_check'])
		{
			$this->forwarded_for = preg_replace('#, +#', ', ', $this->forwarded_for);

			// split the list of IPs
			$ips = explode(', ', $this->forwarded_for);
			foreach ($ips as $ip)
			{
				// check IPv4 first, the IPv6 is hopefully only going to be used very seldomly
				if (!empty($ip) && !preg_match(get_preg_expression('ipv4'), $ip) && !preg_match(get_preg_expression('ipv6'), $ip))
				{
					// contains invalid data, don't use the forwarded for header
					$this->forwarded_for = '';
					break;
				}
			}
		}
		else
		{
			$this->forwarded_for = '';
		}

		if (isset($_COOKIE[$config['cookie_name'] . '_sid']) || isset($_COOKIE[$config['cookie_name'] . '_u']))
		{
			$this->cookie_data['u'] = request_var($config['cookie_name'] . '_u', 0, false, true);
			$this->cookie_data['k'] = request_var($config['cookie_name'] . '_k', '', false, true);
			$this->session_id 		= request_var($config['cookie_name'] . '_sid', '', false, true);

			$SID = (defined('NEED_SID')) ? '?sid=' . $this->session_id : '?sid=';
			$_SID = (defined('NEED_SID')) ? $this->session_id : '';

			if (empty($this->session_id))
			{
				$this->session_id = $_SID = request_var('sid', '');
				$SID = '?sid=' . $this->session_id;
				$this->cookie_data = array('u' => 0, 'k' => '');
			}
		}
		else
		{
			$this->session_id = $_SID = request_var('sid', '');
			$SID = '?sid=' . $this->session_id;
		}

		$_EXTRA_URL = array();

		// Why no forwarded_for et al? Well, too easily spoofed. With the results of my recent requests
		// it's pretty clear that in the majority of cases you'll at least be left with a proxy/cache ip.
		$this->ip = (!empty($_SERVER['REMOTE_ADDR'])) ? htmlspecialchars($_SERVER['REMOTE_ADDR']) : '';
		$this->load = false;

		// Load limit check (if applicable)
		if ($config['limit_load'] || $config['limit_search_load'])
		{
			if ($load = @file_get_contents('/proc/loadavg'))
			{
				$this->load = array_slice(explode(' ', $load), 0, 1);
				$this->load = floatval($this->load[0]);
			}
			else
			{
				set_config('limit_load', '0');
				set_config('limit_search_load', '0');
			}
		}

		// Is session_id is set or session_id is set and matches the url param if required
		if (!empty($this->session_id) && (!defined('NEED_SID') || (isset($_GET['sid']) && $this->session_id === $_GET['sid'])))
		{
			$sql = 'SELECT u.*, s.*
				FROM ' . SESSIONS_TABLE . ' s, ' . USERS_TABLE . " u
				WHERE s.session_id = '" . $db->sql_escape($this->session_id) . "'
					AND u.user_id = s.session_user_id";
			$result = $db->sql_query($sql);
			$this->data = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			// Did the session exist in the DB?
			if (isset($this->data['user_id']))
			{
				// Validate IP length according to admin ... enforces an IP
				// check on bots if admin requires this
//				$quadcheck = ($config['ip_check_bot'] && $this->data['user_type'] & USER_BOT) ? 4 : $config['ip_check'];

				if (strpos($this->ip, ':') !== false && strpos($this->data['session_ip'], ':') !== false)
				{
					$s_ip = short_ipv6($this->data['session_ip'], $config['ip_check']);
					$u_ip = short_ipv6($this->ip, $config['ip_check']);
				}
				else
				{
					$s_ip = implode('.', array_slice(explode('.', $this->data['session_ip']), 0, $config['ip_check']));
					$u_ip = implode('.', array_slice(explode('.', $this->ip), 0, $config['ip_check']));
				}

				$s_browser = ($config['browser_check']) ? trim(strtolower(substr($this->data['session_browser'], 0, 149))) : '';
				$u_browser = ($config['browser_check']) ? trim(strtolower(substr($this->browser, 0, 149))) : '';

				$s_forwarded_for = ($config['forwarded_for_check']) ? substr($this->data['session_forwarded_for'], 0, 254) : '';
				$u_forwarded_for = ($config['forwarded_for_check']) ? substr($this->forwarded_for, 0, 254) : '';

				if ($u_ip === $s_ip && $s_browser === $u_browser && $s_forwarded_for === $u_forwarded_for)
				{
					$session_expired = false;

					// Check whether the session is still valid if we have one
					$method = basename(trim($config['auth_method']));
					include_once($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx);

					$method = 'validate_session_' . $method;
					if (function_exists($method))
					{
						if (!$method($this->data))
						{
							$session_expired = true;
						}
					}

					if (!$session_expired)
					{
						// Check the session length timeframe if autologin is not enabled.
						// Else check the autologin length... and also removing those having autologin enabled but no longer allowed board-wide.
						if (!$this->data['session_autologin'])
						{
							if ($this->data['session_time'] < $this->time_now - ($config['session_length'] + 60))
							{
								$session_expired = true;
							}
						}
						else if (!$config['allow_autologin'] || ($config['max_autologin_time'] && $this->data['session_time'] < $this->time_now - (86400 * (int) $config['max_autologin_time']) + 60))
						{
							$session_expired = true;
						}
					}

					if (!$session_expired)
					{
						// Only update session DB a minute or so after last update or if page changes
						if ($this->time_now - $this->data['session_time'] > 60 || ($this->update_session_page && $this->data['session_page'] != $this->page['page']))
						{
							$sql_ary = array('session_time' => $this->time_now);

							if ($this->update_session_page)
							{
								$sql_ary['session_page'] = substr($this->page['page'], 0, 199);
								$sql_ary['session_forum_id'] = $this->page['forum'];
							}

							$db->sql_return_on_error(true);

							$sql = 'UPDATE ' . SESSIONS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
								WHERE session_id = '" . $db->sql_escape($this->session_id) . "'";
							$result = $db->sql_query($sql);

							$db->sql_return_on_error(false);

							// If the database is not yet updated, there will be an error due to the session_forum_id
							// @todo REMOVE for 3.0.2
							if ($result === false)
							{
								unset($sql_ary['session_forum_id']);

								$sql = 'UPDATE ' . SESSIONS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
									WHERE session_id = '" . $db->sql_escape($this->session_id) . "'";
								$db->sql_query($sql);
							}
						}

						$this->data['is_registered'] = ($this->data['user_id'] != ANONYMOUS && ($this->data['user_type'] == USER_NORMAL || $this->data['user_type'] == USER_FOUNDER)) ? true : false;
						$this->data['is_bot'] = (!$this->data['is_registered'] && $this->data['user_id'] != ANONYMOUS) ? true : false;
						$this->data['user_lang'] = basename($this->data['user_lang']);

						return true;
					}
				}
				else
				{
					// Added logging temporarly to help debug bugs...
					if (defined('DEBUG_EXTRA') && $this->data['user_id'] != ANONYMOUS)
					{
						add_log('critical', 'LOG_IP_BROWSER_FORWARDED_CHECK', $u_ip, $s_ip, $u_browser, $s_browser, htmlspecialchars($u_forwarded_for), htmlspecialchars($s_forwarded_for));
					}
				}
			}
		}

		// If we reach here then no (valid) session exists. So we'll create a new one
		return $this->session_create();
	}

	/**
	* Create a new session
	*
	* If upon trying to start a session we discover there is nothing existing we
	* jump here. Additionally this method is called directly during login to regenerate
	* the session for the specific user. In this method we carry out a number of tasks;
	* garbage collection, (search)bot checking, banned user comparison. Basically
	* though this method will result in a new session for a specific user.
	*/
	function session_create($user_id = false, $set_admin = false, $persist_login = false, $viewonline = true)
	{
		global $SID, $_SID, $db, $config, $cache, $phpbb_root_path, $phpEx;

		$this->data = array();

		/* Garbage collection ... remove old sessions updating user information
		// if necessary. It means (potentially) 11 queries but only infrequently
		if ($this->time_now > $config['session_last_gc'] + $config['session_gc'])
		{
			$this->session_gc();
		}*/

		// Do we allow autologin on this board? No? Then override anything
		// that may be requested here
		if (!$config['allow_autologin'])
		{
			$this->cookie_data['k'] = $persist_login = false;
		}

		/**
		* Here we do a bot check, oh er saucy! No, not that kind of bot
		* check. We loop through the list of bots defined by the admin and
		* see if we have any useragent and/or IP matches. If we do, this is a
		* bot, act accordingly
		*/
		$bot = false;
		$active_bots = $cache->obtain_bots();

		foreach ($active_bots as $row)
		{
			if ($row['bot_agent'] && preg_match('#' . str_replace('\*', '.*?', preg_quote($row['bot_agent'], '#')) . '#i', $this->browser))
			{
				$bot = $row['user_id'];
			}

			// If ip is supplied, we will make sure the ip is matching too...
			if ($row['bot_ip'] && ($bot || !$row['bot_agent']))
			{
				// Set bot to false, then we only have to set it to true if it is matching
				$bot = false;

				foreach (explode(',', $row['bot_ip']) as $bot_ip)
				{
					if (strpos($this->ip, $bot_ip) === 0)
					{
						$bot = (int) $row['user_id'];
						break;
					}
				}
			}

			if ($bot)
			{
				break;
			}
		}

		$method = basename(trim($config['auth_method']));
		include_once($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx);

		$method = 'autologin_' . $method;
		if (function_exists($method))
		{
			$this->data = $method();

			if (sizeof($this->data))
			{
				$this->cookie_data['k'] = '';
				$this->cookie_data['u'] = $this->data['user_id'];
			}
		}

		// If we're presented with an autologin key we'll join against it.
		// Else if we've been passed a user_id we'll grab data based on that
		if (isset($this->cookie_data['k']) && $this->cookie_data['k'] && $this->cookie_data['u'] && !sizeof($this->data))
		{
			$sql = 'SELECT u.*
				FROM ' . USERS_TABLE . ' u, ' . SESSIONS_KEYS_TABLE . ' k
				WHERE u.user_id = ' . (int) $this->cookie_data['u'] . '
					AND u.user_type IN (' . USER_NORMAL . ', ' . USER_FOUNDER . ")
					AND k.user_id = u.user_id
					AND k.key_id = '" . $db->sql_escape(md5($this->cookie_data['k'])) . "'";
			$result = $db->sql_query($sql);
			$this->data = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
			$bot = false;
		}
		else if ($user_id !== false && !sizeof($this->data))
		{
			$this->cookie_data['k'] = '';
			$this->cookie_data['u'] = $user_id;

			$sql = 'SELECT *
				FROM ' . USERS_TABLE . '
				WHERE user_id = ' . (int) $this->cookie_data['u'] . '
					AND user_type IN (' . USER_NORMAL . ', ' . USER_FOUNDER . ')';
			$result = $db->sql_query($sql);
			$this->data = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
			$bot = false;
		}

		// If no data was returned one or more of the following occurred:
		// Key didn't match one in the DB
		// User does not exist
		// User is inactive
		// User is bot
		if (!sizeof($this->data) || !is_array($this->data))
		{
			$this->cookie_data['k'] = '';
			$this->cookie_data['u'] = ($bot) ? $bot : ANONYMOUS;

			if (!$bot)
			{
				$sql = 'SELECT *
					FROM ' . USERS_TABLE . '
					WHERE user_id = ' . (int) $this->cookie_data['u'];
			}
			else
			{
				// We give bots always the same session if it is not yet expired.
				$sql = 'SELECT u.*, s.*
					FROM ' . USERS_TABLE . ' u
					LEFT JOIN ' . SESSIONS_TABLE . ' s ON (s.session_user_id = u.user_id)
					WHERE u.user_id = ' . (int) $bot;
			}

			$result = $db->sql_query($sql);
			$this->data = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
		}

		if ($this->data['user_id'] != ANONYMOUS && !$bot)
		{
			$this->data['session_last_visit'] = (isset($this->data['session_time']) && $this->data['session_time']) ? $this->data['session_time'] : (($this->data['user_lastvisit']) ? $this->data['user_lastvisit'] : time());
		}
		else
		{
			$this->data['session_last_visit'] = $this->time_now;
		}

		// Force user id to be integer...
		$this->data['user_id'] = (int) $this->data['user_id'];

		// At this stage we should have a filled data array, defined cookie u and k data.
		// data array should contain recent session info if we're a real user and a recent
		// session exists in which case session_id will also be set

		// Is user banned? Are they excluded? Won't return on ban, exists within method
		if ($this->data['user_type'] != USER_FOUNDER)
		{
			if (!$config['forwarded_for_check'])
			{
				$this->check_ban($this->data['user_id'], $this->ip);
			}
			else
			{
				$ips = explode(', ', $this->forwarded_for);
				$ips[] = $this->ip;
				$this->check_ban($this->data['user_id'], $ips);
			}
		}

		$this->data['is_registered'] = (!$bot && $this->data['user_id'] != ANONYMOUS && ($this->data['user_type'] == USER_NORMAL || $this->data['user_type'] == USER_FOUNDER)) ? true : false;
		$this->data['is_bot'] = ($bot) ? true : false;

		// If our friend is a bot, we re-assign a previously assigned session
		if ($this->data['is_bot'] && $bot == $this->data['user_id'] && $this->data['session_id'])
		{
			// Only assign the current session if the ip, browser and forwarded_for match...
			if (strpos($this->ip, ':') !== false && strpos($this->data['session_ip'], ':') !== false)
			{
				$s_ip = short_ipv6($this->data['session_ip'], $config['ip_check']);
				$u_ip = short_ipv6($this->ip, $config['ip_check']);
			}
			else
			{
				$s_ip = implode('.', array_slice(explode('.', $this->data['session_ip']), 0, $config['ip_check']));
				$u_ip = implode('.', array_slice(explode('.', $this->ip), 0, $config['ip_check']));
			}

			$s_browser = ($config['browser_check']) ? trim(strtolower(substr($this->data['session_browser'], 0, 149))) : '';
			$u_browser = ($config['browser_check']) ? trim(strtolower(substr($this->browser, 0, 149))) : '';

			$s_forwarded_for = ($config['forwarded_for_check']) ? substr($this->data['session_forwarded_for'], 0, 254) : '';
			$u_forwarded_for = ($config['forwarded_for_check']) ? substr($this->forwarded_for, 0, 254) : '';

			if ($u_ip === $s_ip && $s_browser === $u_browser && $s_forwarded_for === $u_forwarded_for)
			{
				$this->session_id = $this->data['session_id'];

				// Only update session DB a minute or so after last update or if page changes
				if ($this->time_now - $this->data['session_time'] > 60 || ($this->update_session_page && $this->data['session_page'] != $this->page['page']))
				{
					$this->data['session_time'] = $this->data['session_last_visit'] = $this->time_now;

					$sql_ary = array('session_time' => $this->time_now, 'session_last_visit' => $this->time_now, 'session_admin' => 0);

					if ($this->update_session_page)
					{
						$sql_ary['session_page'] = substr($this->page['page'], 0, 199);
						$sql_ary['session_forum_id'] = $this->page['forum'];
					}

					$sql = 'UPDATE ' . SESSIONS_TABLE . ' SET ' . $db->sql_build_array('UPDATE', $sql_ary) . "
						WHERE session_id = '" . $db->sql_escape($this->session_id) . "'";
					$db->sql_query($sql);

					// Update the last visit time
					$sql = 'UPDATE ' . USERS_TABLE . '
						SET user_lastvisit = ' . (int) $this->data['session_time'] . '
						WHERE user_id = ' . (int) $this->data['user_id'];
					$db->sql_query($sql);
				}

				$SID = '?sid=';
				$_SID = '';
				return true;
			}
			else
			{
				// If the ip and browser does not match make sure we only have one bot assigned to one session
				$db->sql_query('DELETE FROM ' . SESSIONS_TABLE . ' WHERE session_user_id = ' . $this->data['user_id']);
			}
		}

		$session_autologin = (($this->cookie_data['k'] || $persist_login) && $this->data['is_registered']) ? true : false;
		$set_admin = ($set_admin && $this->data['is_registered']) ? true : false;

		// Create or update the session
		$sql_ary = array(
			'session_user_id'		=> (int) $this->data['user_id'],
			'session_start'			=> (int) $this->time_now,
			'session_last_visit'	=> (int) $this->data['session_last_visit'],
			'session_time'			=> (int) $this->time_now,
			'session_browser'		=> (string) trim(substr($this->browser, 0, 149)),
			'session_forwarded_for'	=> (string) $this->forwarded_for,
			'session_ip'			=> (string) $this->ip,
			'session_autologin'		=> ($session_autologin) ? 1 : 0,
			'session_admin'			=> ($set_admin) ? 1 : 0,
			'session_viewonline'	=> ($viewonline) ? 1 : 0,
		);

		if ($this->update_session_page)
		{
			$sql_ary['session_page'] = (string) substr($this->page['page'], 0, 199);
			$sql_ary['session_forum_id'] = $this->page['forum'];
		}

		$db->sql_return_on_error(true);

		$sql = 'DELETE
			FROM ' . SESSIONS_TABLE . '
			WHERE session_id = \'' . $db->sql_escape($this->session_id) . '\'
				AND session_user_id = ' . ANONYMOUS;

		if (!defined('IN_ERROR_HANDLER') && (!$this->session_id || !$db->sql_query($sql) || !$db->sql_affectedrows()))
		{
			// Limit new sessions in 1 minute period (if required)
			if (empty($this->data['session_time']) && $config['active_sessions'])
			{
//				$db->sql_return_on_error(false);

				$sql = 'SELECT COUNT(session_id) AS sessions
					FROM ' . SESSIONS_TABLE . '
					WHERE session_time >= ' . ($this->time_now - 60);
				$result = $db->sql_query($sql);
				$row = $db->sql_fetchrow($result);
				$db->sql_freeresult($result);

				if ((int) $row['sessions'] > (int) $config['active_sessions'])
				{
					header('HTTP/1.1 503 Service Unavailable');
					trigger_error('BOARD_UNAVAILABLE');
				}
			}
		}

		// Since we re-create the session id here, the inserted row must be unique. Therefore, we display potential errors.
		// Commented out because it will not allow forums to update correctly
//		$db->sql_return_on_error(false);

		$this->session_id = $this->data['session_id'] = md5(unique_id());

		$sql_ary['session_id'] = (string) $this->session_id;
		$sql_ary['session_page'] = (string) substr($this->page['page'], 0, 199);
		$sql_ary['session_forum_id'] = $this->page['forum'];

		$sql = 'INSERT INTO ' . SESSIONS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
		$db->sql_query($sql);

		$db->sql_return_on_error(false);

		// Regenerate autologin/persistent login key
		if ($session_autologin)
		{
			$this->set_login_key();
		}

		// refresh data
		$SID = '?sid=' . $this->session_id;
		$_SID = $this->session_id;
		$this->data = array_merge($this->data, $sql_ary);

		if (!$bot)
		{
			$cookie_expire = $this->time_now + (($config['max_autologin_time']) ? 86400 * (int) $config['max_autologin_time'] : 31536000);

			$this->set_cookie('u', $this->cookie_data['u'], $cookie_expire);
			$this->set_cookie('k', $this->cookie_data['k'], $cookie_expire);
			$this->set_cookie('sid', $this->session_id, $cookie_expire);

			unset($cookie_expire);

			$sql = 'SELECT COUNT(session_id) AS sessions
					FROM ' . SESSIONS_TABLE . '
					WHERE session_user_id = ' . (int) $this->data['user_id'] . '
					AND session_time >= ' . (int) ($this->time_now - (max($config['session_length'], $config['form_token_lifetime'])));
			$result = $db->sql_query($sql);
			$row = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);

			if ((int) $row['sessions'] <= 1 || empty($this->data['user_form_salt']))
			{
				$this->data['user_form_salt'] = unique_id();
				// Update the form key
				$sql = 'UPDATE ' . USERS_TABLE . '
					SET user_form_salt = \'' . $db->sql_escape($this->data['user_form_salt']) . '\'
					WHERE user_id = ' . (int) $this->data['user_id'];
				$db->sql_query($sql);
			}
		}
		else
		{
			$this->data['session_time'] = $this->data['session_last_visit'] = $this->time_now;

			// Update the last visit time
			$sql = 'UPDATE ' . USERS_TABLE . '
				SET user_lastvisit = ' . (int) $this->data['session_time'] . '
				WHERE user_id = ' . (int) $this->data['user_id'];
			$db->sql_query($sql);

			$SID = '?sid=';
			$_SID = '';
		}

		return true;
	}

	/**
	* Kills a session
	*
	* This method does what it says on the tin. It will delete a pre-existing session.
	* It resets cookie information (destroying any autologin key within that cookie data)
	* and update the users information from the relevant session data. It will then
	* grab guest user information.
	*/
	function session_kill($new_session = true)
	{
		global $SID, $_SID, $db, $config, $phpbb_root_path, $phpEx;

		$sql = 'DELETE FROM ' . SESSIONS_TABLE . "
			WHERE session_id = '" . $db->sql_escape($this->session_id) . "'
				AND session_user_id = " . (int) $this->data['user_id'];
		$db->sql_query($sql);

		// Allow connecting logout with external auth method logout
		$method = basename(trim($config['auth_method']));
		include_once($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx);

		$method = 'logout_' . $method;
		if (function_exists($method))
		{
			$method($this->data, $new_session);
		}

		if ($this->data['user_id'] != ANONYMOUS)
		{
			// Delete existing session, update last visit info first!
			if (!isset($this->data['session_time']))
			{
				$this->data['session_time'] = time();
			}

			$sql = 'UPDATE ' . USERS_TABLE . '
				SET user_lastvisit = ' . (int) $this->data['session_time'] . '
				WHERE user_id = ' . (int) $this->data['user_id'];
			$db->sql_query($sql);

			if ($this->cookie_data['k'])
			{
				$sql = 'DELETE FROM ' . SESSIONS_KEYS_TABLE . '
					WHERE user_id = ' . (int) $this->data['user_id'] . "
						AND key_id = '" . $db->sql_escape(md5($this->cookie_data['k'])) . "'";
				$db->sql_query($sql);
			}

			// Reset the data array
			$this->data = array();

			$sql = 'SELECT *
				FROM ' . USERS_TABLE . '
				WHERE user_id = ' . ANONYMOUS;
			$result = $db->sql_query($sql);
			$this->data = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
		}

		$cookie_expire = $this->time_now - 31536000;
		$this->set_cookie('u', '', $cookie_expire);
		$this->set_cookie('k', '', $cookie_expire);
		$this->set_cookie('sid', '', $cookie_expire);
		unset($cookie_expire);

		$SID = '?sid=';
		$this->session_id = $_SID = '';

		// To make sure a valid session is created we create one for the anonymous user
		if ($new_session)
		{
			$this->session_create(ANONYMOUS);
		}

		return true;
	}

	/**
	* Session garbage collection
	*
	* This looks a lot more complex than it really is. Effectively we are
	* deleting any sessions older than an admin definable limit. Due to the
	* way in which we maintain session data we have to ensure we update user
	* data before those sessions are destroyed. In addition this method
	* removes autologin key information that is older than an admin defined
	* limit.
	*/
	function session_gc()
	{
		global $db, $config;

		$batch_size = 10;

		if (!$this->time_now)
		{
			$this->time_now = time();
		}

		// Firstly, delete guest sessions
		$sql = 'DELETE FROM ' . SESSIONS_TABLE . '
			WHERE session_user_id = ' . ANONYMOUS . '
				AND session_time < ' . (int) ($this->time_now - $config['session_length']);
		$db->sql_query($sql);

		// Get expired sessions, only most recent for each user
		$sql = 'SELECT session_user_id, session_page, MAX(session_time) AS recent_time
			FROM ' . SESSIONS_TABLE . '
			WHERE session_time < ' . ($this->time_now - $config['session_length']) . '
			GROUP BY session_user_id, session_page';
		$result = $db->sql_query_limit($sql, $batch_size);

		$del_user_id = array();
		$del_sessions = 0;

		while ($row = $db->sql_fetchrow($result))
		{
			$sql = 'UPDATE ' . USERS_TABLE . '
				SET user_lastvisit = ' . (int) $row['recent_time'] . ", user_lastpage = '" . $db->sql_escape($row['session_page']) . "'
				WHERE user_id = " . (int) $row['session_user_id'];
			$db->sql_query($sql);

			$del_user_id[] = (int) $row['session_user_id'];
			$del_sessions++;
		}
		$db->sql_freeresult($result);

		if (sizeof($del_user_id))
		{
			// Delete expired sessions
			$sql = 'DELETE FROM ' . SESSIONS_TABLE . '
				WHERE ' . $db->sql_in_set('session_user_id', $del_user_id) . '
					AND session_time < ' . ($this->time_now - $config['session_length']);
			$db->sql_query($sql);
		}

		if ($del_sessions < $batch_size)
		{
			// Less than 10 users, update gc timer ... else we want gc
			// called again to delete other sessions
			set_config('session_last_gc', $this->time_now, true);

			if ($config['max_autologin_time'])
			{
				$sql = 'DELETE FROM ' . SESSIONS_KEYS_TABLE . '
					WHERE last_login < ' . (time() - (86400 * (int) $config['max_autologin_time']));
				$db->sql_query($sql);
			}
			$this->confirm_gc();
		}

		return;
	}

	function confirm_gc($type = 0)
	{
		global $db, $config;

		$sql = 'SELECT DISTINCT c.session_id
				FROM ' . CONFIRM_TABLE . ' c
				LEFT JOIN ' . SESSIONS_TABLE . ' s ON (c.session_id = s.session_id)
				WHERE s.session_id IS NULL' .
					((empty($type)) ? '' : ' AND c.confirm_type = ' . (int) $type);
		$result = $db->sql_query($sql);

		if ($row = $db->sql_fetchrow($result))
		{
			$sql_in = array();
			do
			{
				$sql_in[] = (string) $row['session_id'];
			}
			while ($row = $db->sql_fetchrow($result));

			if (sizeof($sql_in))
			{
				$sql = 'DELETE FROM ' . CONFIRM_TABLE . '
					WHERE ' . $db->sql_in_set('session_id', $sql_in);
				$db->sql_query($sql);
			}
		}
		$db->sql_freeresult($result);
	}


	/**
	* Sets a cookie
	*
	* Sets a cookie of the given name with the specified data for the given length of time. If no time is specified, a session cookie will be set.
	*
	* @param string $name		Name of the cookie, will be automatically prefixed with the phpBB cookie name. track becomes [cookie_name]_track then.
	* @param string $cookiedata	The data to hold within the cookie
	* @param int $cookietime	The expiration time as UNIX timestamp. If 0 is provided, a session cookie is set.
	*/
	function set_cookie($name, $cookiedata, $cookietime)
	{
		global $config;

		$name_data = rawurlencode($config['cookie_name'] . '_' . $name) . '=' . rawurlencode($cookiedata);
		$expire = gmdate('D, d-M-Y H:i:s \\G\\M\\T', $cookietime);
		$domain = (!$config['cookie_domain'] || $config['cookie_domain'] == 'localhost' || $config['cookie_domain'] == '127.0.0.1') ? '' : '; domain=' . $config['cookie_domain'];

		header('Set-Cookie: ' . $name_data . (($cookietime) ? '; expires=' . $expire : '') . '; path=' . $config['cookie_path'] . $domain . ((!$config['cookie_secure']) ? '' : '; secure') . '; HttpOnly', false);
	}

	/**
	* Check for banned user
	*
	* Checks whether the supplied user is banned by id, ip or email. If no parameters
	* are passed to the method pre-existing session data is used. If $return is false
	* this routine does not return on finding a banned user, it outputs a relevant
	* message and stops execution.
	*
	* @param string|array	$user_ips	Can contain a string with one IP or an array of multiple IPs
	*/
	function check_ban($user_id = false, $user_ips = false, $user_email = false, $return = false)
	{
		global $config, $db;

		if (defined('IN_CHECK_BAN'))
		{
			return;
		}

		$banned = false;
		$cache_ttl = 3600;
		$where_sql = array();

		$sql = 'SELECT ban_ip, ban_userid, ban_email, ban_exclude, ban_give_reason, ban_end
			FROM ' . BANLIST_TABLE . '
			WHERE ';

		// Determine which entries to check, only return those
		if ($user_email === false)
		{
			$where_sql[] = "ban_email = ''";
		}

		if ($user_ips === false)
		{
			$where_sql[] = "(ban_ip = '' OR ban_exclude = 1)";
		}

		if ($user_id === false)
		{
			$where_sql[] = '(ban_userid = 0 OR ban_exclude = 1)';
		}
		else
		{
			$cache_ttl = ($user_id == ANONYMOUS) ? 3600 : 0;
			$_sql = '(ban_userid = ' . $user_id;

			if ($user_email !== false)
			{
				$_sql .= " OR ban_email <> ''";
			}

			if ($user_ips !== false)
			{
				$_sql .= " OR ban_ip <> ''";
			}

			$_sql .= ')';

			$where_sql[] = $_sql;
		}

		$sql .= (sizeof($where_sql)) ? implode(' AND ', $where_sql) : '';
		$result = $db->sql_query($sql, $cache_ttl);

		$ban_triggered_by = 'user';
		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['ban_end'] && $row['ban_end'] < time())
			{
				continue;
			}

			$ip_banned = false;
			if (!empty($row['ban_ip']))
			{
				if (!is_array($user_ips))
				{
					$ip_banned = preg_match('#^' . str_replace('\*', '.*?', preg_quote($row['ban_ip'], '#')) . '$#i', $user_ips);
				}
				else
				{
					foreach ($user_ips as $user_ip)
					{
						if (preg_match('#^' . str_replace('\*', '.*?', preg_quote($row['ban_ip'], '#')) . '$#i', $user_ip))
						{
							$ip_banned = true;
							break;
						}
					}
				}
			}

			if ((!empty($row['ban_userid']) && intval($row['ban_userid']) == $user_id) ||
				$ip_banned ||
				(!empty($row['ban_email']) && preg_match('#^' . str_replace('\*', '.*?', preg_quote($row['ban_email'], '#')) . '$#i', $user_email)))
			{
				if (!empty($row['ban_exclude']))
				{
					$banned = false;
					break;
				}
				else
				{
					$banned = true;
					$ban_row = $row;

					if (!empty($row['ban_userid']) && intval($row['ban_userid']) == $user_id)
					{
						$ban_triggered_by = 'user';
					}
					else if ($ip_banned)
					{
						$ban_triggered_by = 'ip';
					}
					else
					{
						$ban_triggered_by = 'email';
					}

					// Don't break. Check if there is an exclude rule for this user
				}
			}
		}
		$db->sql_freeresult($result);

		if ($banned && !$return)
		{
			global $template;

			// If the session is empty we need to create a valid one...
			if (empty($this->session_id))
			{
				// This seems to be no longer needed? - #14971
//				$this->session_create(ANONYMOUS);
			}

			// Initiate environment ... since it won't be set at this stage
			$this->setup();

			// Logout the user, banned users are unable to use the normal 'logout' link
			if ($this->data['user_id'] != ANONYMOUS)
			{
				$this->session_kill();
			}

			// We show a login box here to allow founders accessing the board if banned by IP
			if (defined('IN_LOGIN') && $this->data['user_id'] == ANONYMOUS)
			{
				global $phpEx;

				$this->setup('ucp');
				$this->data['is_registered'] = $this->data['is_bot'] = false;

				// Set as a precaution to allow login_box() handling this case correctly as well as this function not being executed again.
				define('IN_CHECK_BAN', 1);

				login_box("index.$phpEx");

				// The false here is needed, else the user is able to circumvent the ban.
				$this->session_kill(false);
			}

			// Ok, we catch the case of an empty session id for the anonymous user...
			// This can happen if the user is logging in, banned by username and the login_box() being called "again".
			if (empty($this->session_id) && defined('IN_CHECK_BAN'))
			{
				$this->session_create(ANONYMOUS);
			}


			// Determine which message to output
			$till_date = ($ban_row['ban_end']) ? $this->format_date($ban_row['ban_end']) : '';
			$message = ($ban_row['ban_end']) ? 'BOARD_BAN_TIME' : 'BOARD_BAN_PERM';

			$message = sprintf($this->lang[$message], $till_date, '<a href="mailto:' . $config['board_contact'] . '">', '</a>');
			$message .= ($ban_row['ban_give_reason']) ? '<br /><br />' . sprintf($this->lang['BOARD_BAN_REASON'], $ban_row['ban_give_reason']) : '';
			$message .= '<br /><br /><em>' . $this->lang['BAN_TRIGGERED_BY_' . strtoupper($ban_triggered_by)] . '</em>';

			// To circumvent session_begin returning a valid value and the check_ban() not called on second page view, we kill the session again
			$this->session_kill(false);

			trigger_error($message);
		}

		return ($banned) ? true : false;
	}

	/**
	* Check if ip is blacklisted
	* This should be called only where absolutly necessary
	*
	* Only IPv4 (rbldns does not support AAAA records/IPv6 lookups)
	*
	* @author satmd (from the php manual)
	* @param string $mode register/post - spamcop for example is ommitted for posting
	* @return false if ip is not blacklisted, else an array([checked server], [lookup])
	*/
	function check_dnsbl($mode, $ip = false)
	{
		if ($ip === false)
		{
			$ip = $this->ip;
		}

		$dnsbl_check = array(
			'list.dsbl.org'			=> 'http://dsbl.org/listing?',
			'sbl-xbl.spamhaus.org'	=> 'http://www.spamhaus.org/query/bl?ip=',
		);

		if ($mode == 'register')
		{
			$dnsbl_check['bl.spamcop.net'] = 'http://spamcop.net/bl.shtml?';
		}

		if ($ip)
		{
			$quads = explode('.', $ip);
			$reverse_ip = $quads[3] . '.' . $quads[2] . '.' . $quads[1] . '.' . $quads[0];

			// Need to be listed on all servers...
			$listed = true;
			$info = array();

			foreach ($dnsbl_check as $dnsbl => $lookup)
			{
				if (phpbb_checkdnsrr($reverse_ip . '.' . $dnsbl . '.', 'A') === true)
				{
					$info = array($dnsbl, $lookup . $ip);
				}
				else
				{
					$listed = false;
				}
			}

			if ($listed)
			{
				return $info;
			}
		}

		return false;
	}

	/**
	* Check if URI is blacklisted
	* This should be called only where absolutly necessary, for example on the submitted website field
	* This function is not in use at the moment and is only included for testing purposes, it may not work at all!
	* This means it is untested at the moment and therefore commented out
	*
	* @param string $uri URI to check
	* @return true if uri is on blacklist, else false. Only blacklist is checked (~zero FP), no grey lists
	function check_uribl($uri)
	{
		// Normally parse_url() is not intended to parse uris
		// We need to get the top-level domain name anyway... change.
		$uri = parse_url($uri);

		if ($uri === false || empty($uri['host']))
		{
			return false;
		}

		$uri = trim($uri['host']);

		if ($uri)
		{
			// One problem here... the return parameter for the "windows" method is different from what
			// we expect... this may render this check useless...
			if (phpbb_checkdnsrr($uri . '.multi.uribl.com.', 'A') === true)
			{
				return true;
			}
		}

		return false;
	}
	*/

	/**
	* Set/Update a persistent login key
	*
	* This method creates or updates a persistent session key. When a user makes
	* use of persistent (formerly auto-) logins a key is generated and stored in the
	* DB. When they revisit with the same key it's automatically updated in both the
	* DB and cookie. Multiple keys may exist for each user representing different
	* browsers or locations. As with _any_ non-secure-socket no passphrase login this
	* remains vulnerable to exploit.
	*/
	function set_login_key($user_id = false, $key = false, $user_ip = false)
	{
		global $config, $db;

		$user_id = ($user_id === false) ? $this->data['user_id'] : $user_id;
		$user_ip = ($user_ip === false) ? $this->ip : $user_ip;
		$key = ($key === false) ? (($this->cookie_data['k']) ? $this->cookie_data['k'] : false) : $key;

		$key_id = unique_id(hexdec(substr($this->session_id, 0, 8)));

		$sql_ary = array(
			'key_id'		=> (string) md5($key_id),
			'last_ip'		=> (string) $this->ip,
			'last_login'	=> (int) time()
		);

		if (!$key)
		{
			$sql_ary += array(
				'user_id'	=> (int) $user_id
			);
		}

		if ($key)
		{
			$sql = 'UPDATE ' . SESSIONS_KEYS_TABLE . '
				SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
				WHERE user_id = ' . (int) $user_id . "
					AND key_id = '" . $db->sql_escape(md5($key)) . "'";
		}
		else
		{
			$sql = 'INSERT INTO ' . SESSIONS_KEYS_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
		}
		$db->sql_query($sql);

		$this->cookie_data['k'] = $key_id;

		return false;
	}

	/**
	* Reset all login keys for the specified user
	*
	* This method removes all current login keys for a specified (or the current)
	* user. It will be called on password change to render old keys unusable
	*/
	function reset_login_keys($user_id = false)
	{
		global $config, $db;

		$user_id = ($user_id === false) ? $this->data['user_id'] : $user_id;

		$sql = 'DELETE FROM ' . SESSIONS_KEYS_TABLE . '
			WHERE user_id = ' . (int) $user_id;
		$db->sql_query($sql);

		// Let's also clear any current sessions for the specified user_id
		// If it's the current user then we'll leave this session intact
		$sql_where = 'session_user_id = ' . (int) $user_id;
		$sql_where .= ($user_id === $this->data['user_id']) ? " AND session_id <> '" . $db->sql_escape($this->session_id) . "'" : '';

		$sql = 'DELETE FROM ' . SESSIONS_TABLE . "
			WHERE $sql_where";
		$db->sql_query($sql);

		// We're changing the password of the current user and they have a key
		// Lets regenerate it to be safe
		if ($user_id === $this->data['user_id'] && $this->cookie_data['k'])
		{
			$this->set_login_key($user_id);
		}
	}
}


/**
* Base user class
*
* This is the overarching class which contains (through session extend)
* all methods utilised for user functionality during a session.
*
* @package phpBB3
*/
class user extends session
{
	var $lang = array();
	var $help = array();
	var $theme = array();
	var $date_format;
	var $timezone;
	var $dst;

	var $lang_name;
	var $lang_id = false;
	var $lang_path;
	var $img_lang;
	var $img_array = array();

	// Able to add new option (id 7)
	var $keyoptions = array('viewimg' => 0, 'viewflash' => 1, 'viewsmilies' => 2, 'viewsigs' => 3, 'viewavatars' => 4, 'viewcensors' => 5, 'attachsig' => 6, 'bbcode' => 8, 'smilies' => 9, 'popuppm' => 10);
	var $keyvalues = array();

	/**
	* Setup basic user-specific items (style, language, ...)
	*/
	function setup($lang_set = false, $style = false)
	{
		global $db, $template, $config, $auth, $phpEx, $phpbb_root_path, $cache;

		if ($this->data['user_id'] != ANONYMOUS)
		{
			$this->lang_name = (file_exists($phpbb_root_path . 'language/' . $this->data['user_lang'] . "/common.$phpEx")) ? $this->data['user_lang'] : basename($config['default_lang']);
			$this->lang_path = $phpbb_root_path . 'language/' . $this->lang_name . '/';

			$this->date_format = $this->data['user_dateformat'];
			$this->timezone = $this->data['user_timezone'] * 3600;
			$this->dst = $this->data['user_dst'] * 3600;
		}
		else
		{
			$this->lang_name = basename($config['default_lang']);
			$this->lang_path = $phpbb_root_path . 'language/' . $this->lang_name . '/';
			$this->date_format = $config['default_dateformat'];
			$this->timezone = $config['board_timezone'] * 3600;
			$this->dst = $config['board_dst'] * 3600;

			/**
			* If a guest user is surfing, we try to guess his/her language first by obtaining the browser language
			* If re-enabled we need to make sure only those languages installed are checked
			* Commented out so we do not loose the code.

			if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE']))
			{
				$accept_lang_ary = explode(',', $_SERVER['HTTP_ACCEPT_LANGUAGE']);

				foreach ($accept_lang_ary as $accept_lang)
				{
					// Set correct format ... guess full xx_YY form
					$accept_lang = substr($accept_lang, 0, 2) . '_' . strtoupper(substr($accept_lang, 3, 2));
					$accept_lang = basename($accept_lang);

					if (file_exists($phpbb_root_path . 'language/' . $accept_lang . "/common.$phpEx"))
					{
						$this->lang_name = $config['default_lang'] = $accept_lang;
						$this->lang_path = $phpbb_root_path . 'language/' . $accept_lang . '/';
						break;
					}
					else
					{
						// No match on xx_YY so try xx
						$accept_lang = substr($accept_lang, 0, 2);
						$accept_lang = basename($accept_lang);

						if (file_exists($phpbb_root_path . 'language/' . $accept_lang . "/common.$phpEx"))
						{
							$this->lang_name = $config['default_lang'] = $accept_lang;
							$this->lang_path = $phpbb_root_path . 'language/' . $accept_lang . '/';
							break;
						}
					}
				}
			}
			*/
		}

		// We include common language file here to not load it every time a custom language file is included
		$lang = &$this->lang;

		if ((@include $this->lang_path . "common.$phpEx") === false)
		{
			die('Language file ' . $this->lang_name . "/common.$phpEx" . " couldn't be opened.");
		}

		$this->add_lang($lang_set);
		unset($lang_set);

		if (!empty($_GET['style']) && $auth->acl_get('a_styles'))
		{
			global $SID, $_EXTRA_URL;

			$style = request_var('style', 0);
			$SID .= '&amp;style=' . $style;
			$_EXTRA_URL = array('style=' . $style);
		}
		else
		{
			// Set up style
			$style = ($style) ? $style : ((!$config['override_user_style']) ? $this->data['user_style'] : $config['default_style']);
		}

		$sql = 'SELECT s.style_id, t.template_storedb, t.template_path, t.template_id, t.bbcode_bitfield, c.theme_path, c.theme_name, c.theme_storedb, c.theme_id, i.imageset_path, i.imageset_id, i.imageset_name
			FROM ' . STYLES_TABLE . ' s, ' . STYLES_TEMPLATE_TABLE . ' t, ' . STYLES_THEME_TABLE . ' c, ' . STYLES_IMAGESET_TABLE . " i
			WHERE s.style_id = $style
				AND t.template_id = s.template_id
				AND c.theme_id = s.theme_id
				AND i.imageset_id = s.imageset_id";
		$result = $db->sql_query($sql, 3600);
		$this->theme = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		// User has wrong style
		if (!$this->theme && $style == $this->data['user_style'])
		{
			$style = $this->data['user_style'] = $config['default_style'];

			$sql = 'UPDATE ' . USERS_TABLE . "
				SET user_style = $style
				WHERE user_id = {$this->data['user_id']}";
			$db->sql_query($sql);

			$sql = 'SELECT s.style_id, t.template_storedb, t.template_path, t.template_id, t.bbcode_bitfield, c.theme_path, c.theme_name, c.theme_storedb, c.theme_id, i.imageset_path, i.imageset_id, i.imageset_name
				FROM ' . STYLES_TABLE . ' s, ' . STYLES_TEMPLATE_TABLE . ' t, ' . STYLES_THEME_TABLE . ' c, ' . STYLES_IMAGESET_TABLE . " i
				WHERE s.style_id = $style
					AND t.template_id = s.template_id
					AND c.theme_id = s.theme_id
					AND i.imageset_id = s.imageset_id";
			$result = $db->sql_query($sql, 3600);
			$this->theme = $db->sql_fetchrow($result);
			$db->sql_freeresult($result);
		}

		if (!$this->theme)
		{
			trigger_error('Could not get style data', E_USER_ERROR);
		}

		// Now parse the cfg file and cache it
		$parsed_items = $cache->obtain_cfg_items($this->theme);

		// We are only interested in the theme configuration for now
		$parsed_items = $parsed_items['theme'];

		$check_for = array(
			'parse_css_file'	=> (int) 0,
			'pagination_sep'	=> (string) ', '
		);

		foreach ($check_for as $key => $default_value)
		{
			$this->theme[$key] = (isset($parsed_items[$key])) ? $parsed_items[$key] : $default_value;
			settype($this->theme[$key], gettype($default_value));

			if (is_string($default_value))
			{
				$this->theme[$key] = htmlspecialchars($this->theme[$key]);
			}
		}

		// If the style author specified the theme needs to be cached
		// (because of the used paths and variables) than make sure it is the case.
		// For example, if the theme uses language-specific images it needs to be stored in db.
		if (!$this->theme['theme_storedb'] && $this->theme['parse_css_file'])
		{
			$this->theme['theme_storedb'] = 1;

			$stylesheet = file_get_contents("{$phpbb_root_path}styles/{$this->theme['theme_path']}/theme/stylesheet.css");
			// Match CSS imports
			$matches = array();
			preg_match_all('/@import url\(["\'](.*)["\']\);/i', $stylesheet, $matches);

			if (sizeof($matches))
			{
				$content = '';
				foreach ($matches[0] as $idx => $match)
				{
					if ($content = @file_get_contents("{$phpbb_root_path}styles/{$this->theme['theme_path']}/theme/" . $matches[1][$idx]))
					{
						$content = trim($content);
					}
					else
					{
						$content = '';
					}
					$stylesheet = str_replace($match, $content, $stylesheet);
				}
				unset($content);
			}

			$stylesheet = str_replace('./', 'styles/' . $this->theme['theme_path'] . '/theme/', $stylesheet);

			$sql_ary = array(
				'theme_data'	=> $stylesheet,
				'theme_mtime'	=> time(),
				'theme_storedb'	=> 1
			);

			$sql = 'UPDATE ' . STYLES_THEME_TABLE . '
				SET ' . $db->sql_build_array('UPDATE', $sql_ary) . '
				WHERE theme_id = ' . $this->theme['theme_id'];
			$db->sql_query($sql);

			unset($sql_ary);
		}

		$template->set_template();

		$this->img_lang = (file_exists($phpbb_root_path . 'styles/' . $this->theme['imageset_path'] . '/imageset/' . $this->lang_name)) ? $this->lang_name : $config['default_lang'];

		$sql = 'SELECT image_name, image_filename, image_lang, image_height, image_width
			FROM ' . STYLES_IMAGESET_DATA_TABLE . '
			WHERE imageset_id = ' . $this->theme['imageset_id'] . "
			AND image_filename <> ''
			AND image_lang IN ('" . $db->sql_escape($this->img_lang) . "', '')";
		$result = $db->sql_query($sql, 3600);

		$localised_images = false;
		while ($row = $db->sql_fetchrow($result))
		{
			if ($row['image_lang'])
			{
				$localised_images = true;
			}

			$this->img_array[$row['image_name']] = $row;
		}
		$db->sql_freeresult($result);

		// there were no localised images, try to refresh the localised imageset for the user's language
		if (!$localised_images)
		{
			// Attention: this code ignores the image definition list from acp_styles and just takes everything
			// that the config file contains
			$sql_ary = array();

			$db->sql_transaction('begin');

			$sql = 'DELETE FROM ' . STYLES_IMAGESET_DATA_TABLE . '
				WHERE imageset_id = ' . $this->theme['imageset_id'] . '
					AND image_lang = \'' . $db->sql_escape($this->img_lang) . '\'';
			$result = $db->sql_query($sql);

			if (@file_exists("{$phpbb_root_path}styles/{$this->theme['imageset_path']}/imageset/{$this->img_lang}/imageset.cfg"))
			{
				$cfg_data_imageset_data = parse_cfg_file("{$phpbb_root_path}styles/{$this->theme['imageset_path']}/imageset/{$this->img_lang}/imageset.cfg");
				foreach ($cfg_data_imageset_data as $image_name => $value)
				{
					if (strpos($value, '*') !== false)
					{
						if (substr($value, -1, 1) === '*')
						{
							list($image_filename, $image_height) = explode('*', $value);
							$image_width = 0;
						}
						else
						{
							list($image_filename, $image_height, $image_width) = explode('*', $value);
						}
					}
					else
					{
						$image_filename = $value;
						$image_height = $image_width = 0;
					}

					if (strpos($image_name, 'img_') === 0 && $image_filename)
					{
						$image_name = substr($image_name, 4);
						$sql_ary[] = array(
							'image_name'		=> (string) $image_name,
							'image_filename'	=> (string) $image_filename,
							'image_height'		=> (int) $image_height,
							'image_width'		=> (int) $image_width,
							'imageset_id'		=> (int) $this->theme['imageset_id'],
							'image_lang'		=> (string) $this->img_lang,
						);
					}
				}
			}

			if (sizeof($sql_ary))
			{
				$db->sql_multi_insert(STYLES_IMAGESET_DATA_TABLE, $sql_ary);
				$db->sql_transaction('commit');
				$cache->destroy('sql', STYLES_IMAGESET_DATA_TABLE);

				add_log('admin', 'LOG_IMAGESET_LANG_REFRESHED', $this->theme['imageset_name'], $this->img_lang);
			}
			else
			{
				$db->sql_transaction('commit');
				add_log('admin', 'LOG_IMAGESET_LANG_MISSING', $this->theme['imageset_name'], $this->img_lang);
			}
		}

		// Call phpbb_user_session_handler() in case external application want to "bend" some variables or replace classes...
		// After calling it we continue script execution...
		phpbb_user_session_handler();

		// If this function got called from the error handler we are finished here.
		if (defined('IN_ERROR_HANDLER'))
		{
			return;
		}

		// Disable board if the install/ directory is still present
		// For the brave development army we do not care about this, else we need to comment out this everytime we develop locally
		if (!defined('DEBUG_EXTRA') && !defined('ADMIN_START') && !defined('IN_INSTALL') && !defined('IN_LOGIN') && file_exists($phpbb_root_path . 'install'))
		{
			// Adjust the message slightly according to the permissions
			if ($auth->acl_gets('a_', 'm_') || $auth->acl_getf_global('m_'))
			{
				$message = 'REMOVE_INSTALL';
			}
			else
			{
				$message = (!empty($config['board_disable_msg'])) ? $config['board_disable_msg'] : 'BOARD_DISABLE';
			}
			trigger_error($message);
		}

		// Is board disabled and user not an admin or moderator?
		if ($config['board_disable'] && !defined('IN_LOGIN') && !$auth->acl_gets('a_', 'm_') && !$auth->acl_getf_global('m_'))
		{
			header('HTTP/1.1 503 Service Unavailable');

			$message = (!empty($config['board_disable_msg'])) ? $config['board_disable_msg'] : 'BOARD_DISABLE';
			trigger_error($message);
		}

		// Is load exceeded?
		if ($config['limit_load'] && $this->load !== false)
		{
			if ($this->load > floatval($config['limit_load']) && !defined('IN_LOGIN'))
			{
				// Set board disabled to true to let the admins/mods get the proper notification
				$config['board_disable'] = '1';

				if (!$auth->acl_gets('a_', 'm_') && !$auth->acl_getf_global('m_'))
				{
					header('HTTP/1.1 503 Service Unavailable');
					trigger_error('BOARD_UNAVAILABLE');
				}
			}
		}

		if (isset($this->data['session_viewonline']))
		{
			// Make sure the user is able to hide his session
			if (!$this->data['session_viewonline'])
			{
				// Reset online status if not allowed to hide the session...
				if (!$auth->acl_get('u_hideonline'))
				{
					$sql = 'UPDATE ' . SESSIONS_TABLE . '
						SET session_viewonline = 1
						WHERE session_user_id = ' . $this->data['user_id'];
					$db->sql_query($sql);
					$this->data['session_viewonline'] = 1;
				}
			}
			else if (!$this->data['user_allow_viewonline'])
			{
				// the user wants to hide and is allowed to  -> cloaking device on.
				if ($auth->acl_get('u_hideonline'))
				{
					$sql = 'UPDATE ' . SESSIONS_TABLE . '
						SET session_viewonline = 0
						WHERE session_user_id = ' . $this->data['user_id'];
					$db->sql_query($sql);
					$this->data['session_viewonline'] = 0;
				}
			}
		}


		// Does the user need to change their password? If so, redirect to the
		// ucp profile reg_details page ... of course do not redirect if we're already in the ucp
		if (!defined('IN_ADMIN') && !defined('ADMIN_START') && $config['chg_passforce'] && $this->data['is_registered'] && $auth->acl_get('u_chgpasswd') && $this->data['user_passchg'] < time() - ($config['chg_passforce'] * 86400))
		{
			if (strpos($this->page['query_string'], 'mode=reg_details') === false && $this->page['page_name'] != "ucp.$phpEx")
			{
				redirect(append_sid("{$phpbb_root_path}ucp.$phpEx", 'i=profile&amp;mode=reg_details'));
			}
		}

		return;
	}

	/**
	* Add Language Items - use_db and use_help are assigned where needed (only use them to force inclusion)
	*
	* @param mixed $lang_set specifies the language entries to include
	* @param bool $use_db internal variable for recursion, do not use
	* @param bool $use_help internal variable for recursion, do not use
	*
	* Examples:
	* <code>
	* $lang_set = array('posting', 'help' => 'faq');
	* $lang_set = array('posting', 'viewtopic', 'help' => array('bbcode', 'faq'))
	* $lang_set = array(array('posting', 'viewtopic'), 'help' => array('bbcode', 'faq'))
	* $lang_set = 'posting'
	* $lang_set = array('help' => 'faq', 'db' => array('help:faq', 'posting'))
	* </code>
	*/
	function add_lang($lang_set, $use_db = false, $use_help = false)
	{
		global $phpEx;

		if (is_array($lang_set))
		{
			foreach ($lang_set as $key => $lang_file)
			{
				// Please do not delete this line.
				// We have to force the type here, else [array] language inclusion will not work
				$key = (string) $key;

				if ($key == 'db')
				{
					$this->add_lang($lang_file, true, $use_help);
				}
				else if ($key == 'help')
				{
					$this->add_lang($lang_file, $use_db, true);
				}
				else if (!is_array($lang_file))
				{
					$this->set_lang($this->lang, $this->help, $lang_file, $use_db, $use_help);
				}
				else
				{
					$this->add_lang($lang_file, $use_db, $use_help);
				}
			}
			unset($lang_set);
		}
		else if ($lang_set)
		{
			$this->set_lang($this->lang, $this->help, $lang_set, $use_db, $use_help);
		}
	}

	/**
	* Set language entry (called by add_lang)
	* @access private
	*/
	function set_lang(&$lang, &$help, $lang_file, $use_db = false, $use_help = false)
	{
		global $phpEx;

		// Make sure the language path is set (if the user setup did not happen it is not set)
		if (!$this->lang_path)
		{
			global $phpbb_root_path, $config;

			$this->lang_path = $phpbb_root_path . 'language/' . basename($config['default_lang']) . '/';
		}

		// $lang == $this->lang
		// $help == $this->help
		// - add appropriate variables here, name them as they are used within the language file...
		if (!$use_db)
		{
			if ($use_help && strpos($lang_file, '/') !== false)
			{
				$language_filename = $this->lang_path . substr($lang_file, 0, stripos($lang_file, '/') + 1) . 'help_' . substr($lang_file, stripos($lang_file, '/') + 1) . '.' . $phpEx;
			}
			else
			{
				$language_filename = $this->lang_path . (($use_help) ? 'help_' : '') . $lang_file . '.' . $phpEx;
			}

			if ((@include $language_filename) === false)
			{
				trigger_error('Language file ' . basename($language_filename) . ' couldn\'t be opened.', E_USER_ERROR);
			}
		}
		else if ($use_db)
		{
			// Get Database Language Strings
			// Put them into $lang if nothing is prefixed, put them into $help if help: is prefixed
			// For example: help:faq, posting
		}
	}

	/**
	* Format user date
	*/
	function format_date($gmepoch, $format = false, $forcedate = false)
	{
		static $midnight;

		$lang_dates = $this->lang['datetime'];
		$format = (!$format) ? $this->date_format : $format;

		// Short representation of month in format
		if ((strpos($format, '\M') === false && strpos($format, 'M') !== false) || (strpos($format, '\r') === false && strpos($format, 'r') !== false))
		{
			$lang_dates['May'] = $lang_dates['May_short'];
		}

		unset($lang_dates['May_short']);

		if (!$midnight)
		{
			list($d, $m, $y) = explode(' ', gmdate('j n Y', time() + $this->timezone + $this->dst));
			$midnight = gmmktime(0, 0, 0, $m, $d, $y) - $this->timezone - $this->dst;
		}

		if (strpos($format, '|') === false || ($gmepoch < $midnight - 86400 && !$forcedate) || ($gmepoch > $midnight + 172800 && !$forcedate))
		{
			return strtr(@gmdate(str_replace('|', '', $format), $gmepoch + $this->timezone + $this->dst), $lang_dates);
		}

		if ($gmepoch > $midnight + 86400 && !$forcedate)
		{
			$format = substr($format, 0, strpos($format, '|')) . '||' . substr(strrchr($format, '|'), 1);
			return str_replace('||', $this->lang['datetime']['TOMORROW'], strtr(@gmdate($format, $gmepoch + $this->timezone + $this->dst), $lang_dates));
		}
		else if ($gmepoch > $midnight && !$forcedate)
		{
			$format = substr($format, 0, strpos($format, '|')) . '||' . substr(strrchr($format, '|'), 1);
			return str_replace('||', $this->lang['datetime']['TODAY'], strtr(@gmdate($format, $gmepoch + $this->timezone + $this->dst), $lang_dates));
		}
		else if ($gmepoch > $midnight - 86400 && !$forcedate)
		{
			$format = substr($format, 0, strpos($format, '|')) . '||' . substr(strrchr($format, '|'), 1);
			return str_replace('||', $this->lang['datetime']['YESTERDAY'], strtr(@gmdate($format, $gmepoch + $this->timezone + $this->dst), $lang_dates));
		}

		return strtr(@gmdate(str_replace('|', '', $format), $gmepoch + $this->timezone + $this->dst), $lang_dates);
	}

	/**
	* Get language id currently used by the user
	*/
	function get_iso_lang_id()
	{
		global $config, $db;

		if (!empty($this->lang_id))
		{
			return $this->lang_id;
		}

		if (!$this->lang_name)
		{
			$this->lang_name = $config['default_lang'];
		}

		$sql = 'SELECT lang_id
			FROM ' . LANG_TABLE . "
			WHERE lang_iso = '" . $db->sql_escape($this->lang_name) . "'";
		$result = $db->sql_query($sql);
		$this->lang_id = (int) $db->sql_fetchfield('lang_id');
		$db->sql_freeresult($result);

		return $this->lang_id;
	}

	/**
	* Get users profile fields
	*/
	function get_profile_fields($user_id)
	{
		global $db;

		if (isset($this->profile_fields))
		{
			return;
		}

		$sql = 'SELECT *
			FROM ' . PROFILE_FIELDS_DATA_TABLE . "
			WHERE user_id = $user_id";
		$result = $db->sql_query_limit($sql, 1);
		$this->profile_fields = (!($row = $db->sql_fetchrow($result))) ? array() : $row;
		$db->sql_freeresult($result);
	}

	/**
	* Specify/Get image
	*/
	function img($img, $alt = '', $width = false, $suffix = '', $type = 'full_tag')
	{
		static $imgs;
		global $phpbb_root_path;

		$img_data = &$imgs[$img];

		if (empty($img_data))
		{
			if (!isset($this->img_array[$img]))
			{
				// Do not fill the image to let designers decide what to do if the image is empty
				$img_data = '';
				return $img_data;
			}

			$img_data['src'] = $phpbb_root_path . 'styles/' . $this->theme['imageset_path'] . '/imageset/' . ($this->img_array[$img]['image_lang'] ? $this->img_array[$img]['image_lang'] .'/' : '') . $this->img_array[$img]['image_filename'];
			$img_data['width'] = $this->img_array[$img]['image_width'];
			$img_data['height'] = $this->img_array[$img]['image_height'];
		}

		$alt = (!empty($this->lang[$alt])) ? $this->lang[$alt] : $alt;

		switch ($type)
		{
			case 'src':
				return $img_data['src'];
			break;

			case 'width':
				return ($width === false) ? $img_data['width'] : $width;
			break;

			case 'height':
				return $img_data['height'];
			break;

			default:
				$use_width = ($width === false) ? $img_data['width'] : $width;

				return '<img src="' . $img_data['src'] . '"' . (($use_width) ? ' width="' . $use_width . '"' : '') . (($img_data['height']) ? ' height="' . $img_data['height'] . '"' : '') . ' alt="' . $alt . '" title="' . $alt . '" />';
			break;
		}
	}

	/**
	* Get option bit field from user options
	*/
	function optionget($key, $data = false)
	{
		if (!isset($this->keyvalues[$key]))
		{
			$var = ($data) ? $data : $this->data['user_options'];
			$this->keyvalues[$key] = ($var & 1 << $this->keyoptions[$key]) ? true : false;
		}

		return $this->keyvalues[$key];
	}

	/**
	* Set option bit field for user options
	*/
	function optionset($key, $value, $data = false)
	{
		$var = ($data) ? $data : $this->data['user_options'];

		if ($value && !($var & 1 << $this->keyoptions[$key]))
		{
			$var += 1 << $this->keyoptions[$key];
		}
		else if (!$value && ($var & 1 << $this->keyoptions[$key]))
		{
			$var -= 1 << $this->keyoptions[$key];
		}
		else
		{
			return ($data) ? $var : false;
		}

		if (!$data)
		{
			$this->data['user_options'] = $var;
			return true;
		}
		else
		{
			return $var;
		}
	}
}

?>