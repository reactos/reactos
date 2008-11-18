<?php
/**
*
* @package phpBB3
* @version $Id: cron.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
*/
define('IN_PHPBB', true);
define('IN_CRON', true);
$phpbb_root_path = (defined('PHPBB_ROOT_PATH')) ? PHPBB_ROOT_PATH : './';
$phpEx = substr(strrchr(__FILE__, '.'), 1);
include($phpbb_root_path . 'common.' . $phpEx);

// Do not update users last page entry
$user->session_begin(false);
$auth->acl($user->data);

$cron_type = request_var('cron_type', '');
$use_shutdown_function = (@function_exists('register_shutdown_function')) ? true : false;

// Output transparent gif
header('Cache-Control: no-cache');
header('Content-type: image/gif');
header('Content-length: 43');

echo base64_decode('R0lGODlhAQABAIAAAP///wAAACH5BAEAAAAALAAAAAABAAEAAAICRAEAOw==');

// test without flush ;)
// flush();

//
if (!isset($config['cron_lock']))
{
	set_config('cron_lock', '0', true);
}

// make sure cron doesn't run multiple times in parallel
if ($config['cron_lock'])
{
	// if the other process is running more than an hour already we have to assume it
	// aborted without cleaning the lock
	$time = explode(' ', $config['cron_lock']);
	$time = $time[0];

	if ($time + 3600 >= time())
	{
		exit;
	}
}

define('CRON_ID', time() . ' ' . unique_id());

$sql = 'UPDATE ' . CONFIG_TABLE . "
	SET config_value = '" . $db->sql_escape(CRON_ID) . "'
	WHERE config_name = 'cron_lock' AND config_value = '" . $db->sql_escape($config['cron_lock']) . "'";
$db->sql_query($sql);

// another cron process altered the table between script start and UPDATE query so exit
if ($db->sql_affectedrows() != 1)
{
	exit;
}

/**
* Run cron-like action
* Real cron-based layer will be introduced in 3.2
*/
switch ($cron_type)
{
	case 'queue':

		if (time() - $config['queue_interval'] <= $config['last_queue_run'] || !file_exists($phpbb_root_path . 'cache/queue.' . $phpEx))
		{
			break;
		}

		// A user reported using the mail() function while using shutdown does not work. We do not want to risk that.
		if ($use_shutdown_function && !$config['smtp_delivery'])
		{
			$use_shutdown_function = false;
		}

		include_once($phpbb_root_path . 'includes/functions_messenger.' . $phpEx);
		$queue = new queue();

		if ($use_shutdown_function)
		{
			register_shutdown_function(array(&$queue, 'process'));
		}
		else
		{
			$queue->process();
		}

	break;

	case 'tidy_cache':

		if (time() - $config['cache_gc'] <= $config['cache_last_gc'] || !method_exists($cache, 'tidy'))
		{
			break;
		}

		if ($use_shutdown_function)
		{
			register_shutdown_function(array(&$cache, 'tidy'));
		}
		else
		{
			$cache->tidy();
		}

	break;

	case 'tidy_search':
		
		// Select the search method
		$search_type = basename($config['search_type']);

		if (time() - $config['search_gc'] <= $config['search_last_gc'] || !file_exists($phpbb_root_path . 'includes/search/' . $search_type . '.' . $phpEx))
		{
			break;
		}

		include_once("{$phpbb_root_path}includes/search/$search_type.$phpEx");

		// We do some additional checks in the module to ensure it can actually be utilised
		$error = false;
		$search = new $search_type($error);

		if ($error)
		{
			break;
		}

		if ($use_shutdown_function)
		{
			register_shutdown_function(array(&$search, 'tidy'));
		}
		else
		{
			$search->tidy();
		}

	break;

	case 'tidy_warnings':

		if (time() - $config['warnings_gc'] <= $config['warnings_last_gc'])
		{
			break;
		}

		include_once($phpbb_root_path . 'includes/functions_admin.' . $phpEx);

		if ($use_shutdown_function)
		{
			register_shutdown_function('tidy_warnings');
		}
		else
		{
			tidy_warnings();
		}

	break;

	case 'tidy_database':

		if (time() - $config['database_gc'] <= $config['database_last_gc'])
		{
			break;
		}

		include_once($phpbb_root_path . 'includes/functions_admin.' . $phpEx);

		if ($use_shutdown_function)
		{
			register_shutdown_function('tidy_database');
		}
		else
		{
			tidy_database();
		}

	break;

	case 'tidy_sessions':

		if (time() - $config['session_gc'] <= $config['session_last_gc'])
		{
			break;
		}

		if ($use_shutdown_function)
		{
			register_shutdown_function(array(&$user, 'session_gc'));
		}
		else
		{
			$user->session_gc();
		}

	break;

	case 'prune_forum':

		$forum_id = request_var('f', 0);

		$sql = 'SELECT forum_id, prune_next, enable_prune, prune_days, prune_viewed, forum_flags, prune_freq
			FROM ' . FORUMS_TABLE . "
			WHERE forum_id = $forum_id";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);

		if (!$row)
		{
			break;
		}

		// Do the forum Prune thang
		if ($row['prune_next'] < time() && $row['enable_prune'])
		{
			include_once($phpbb_root_path . 'includes/functions_admin.' . $phpEx);

			if ($row['prune_days'])
			{
				if ($use_shutdown_function)
				{
					register_shutdown_function('auto_prune', $row['forum_id'], 'posted', $row['forum_flags'], $row['prune_days'], $row['prune_freq']);
				}
				else
				{
					auto_prune($row['forum_id'], 'posted', $row['forum_flags'], $row['prune_days'], $row['prune_freq']);
				}
			}

			if ($row['prune_viewed'])
			{
				if ($use_shutdown_function)
				{
					register_shutdown_function('auto_prune', $row['forum_id'], 'viewed', $row['forum_flags'], $row['prune_viewed'], $row['prune_freq']);
				}
				else
				{
					auto_prune($row['forum_id'], 'viewed', $row['forum_flags'], $row['prune_viewed'], $row['prune_freq']);
				}
			}
		}

	break;
}

// Unloading cache and closing db after having done the dirty work.
if ($use_shutdown_function)
{
	register_shutdown_function('unlock_cron');
	register_shutdown_function('garbage_collection');
}
else
{
	unlock_cron();
	garbage_collection();
}

exit;


/**
* Unlock cron script
*/
function unlock_cron()
{
	global $db;

	$sql = 'UPDATE ' . CONFIG_TABLE . "
		SET config_value = '0'
		WHERE config_name = 'cron_lock' AND config_value = '" . $db->sql_escape(CRON_ID) . "'";
	$db->sql_query($sql);
}

?>