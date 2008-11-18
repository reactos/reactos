<?php
/**
*
* viewforum [English]
*
* @package language
* @version $Id: viewforum.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* DO NOT CHANGE
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

if (empty($lang) || !is_array($lang))
{
	$lang = array();
}

// DEVELOPERS PLEASE NOTE
//
// All language files should use UTF-8 as their encoding and the files must not contain a BOM.
//
// Placeholders can now contain order information, e.g. instead of
// 'Page %s of %s' you can (and should) write 'Page %1$s of %2$s', this allows
// translators to re-order the output of data while ensuring it remains correct
//
// You do not need this where single placeholders are used, e.g. 'Message %d' is fine
// equally where a string contains only two placeholders which are used to wrap text
// in a url you again do not need to specify an order e.g., 'Click %sHERE%s' is fine

$lang = array_merge($lang, array(
	'ACTIVE_TOPICS'			=> 'Active topics',
	'ANNOUNCEMENTS'			=> 'Announcements',

	'FORUM_PERMISSIONS'		=> 'Forum permissions',

	'ICON_ANNOUNCEMENT'		=> 'Announcement',
	'ICON_STICKY'			=> 'Sticky',

	'LOGIN_NOTIFY_FORUM'	=> 'You have been notified about this forum, please login to view it.',

	'MARK_TOPICS_READ'		=> 'Mark topics read',

	'NEW_POSTS_HOT'			=> 'New posts [ Popular ]',
	'NEW_POSTS_LOCKED'		=> 'New posts [ Locked ]',
	'NO_NEW_POSTS_HOT'		=> 'No new posts [ Popular ]',
	'NO_NEW_POSTS_LOCKED'	=> 'No new posts [ Locked ]',
	'NO_READ_ACCESS'		=> 'You do not have the required permissions to read topics within this forum.',

	'POST_FORUM_LOCKED'		=> 'Forum is locked',

	'TOPICS_MARKED'			=> 'The topics for this forum have now been marked read.',

	'VIEW_FORUM'			=> 'View forum',
	'VIEW_FORUM_TOPIC'		=> '1 topic',
	'VIEW_FORUM_TOPICS'		=> '%d topics',
));

?>