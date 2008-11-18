<?php
/**
*
* acp_bots [English]
*
* @package language
* @version $Id: bots.php 8479 2008-03-29 00:22:48Z naderman $
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

// Bot settings
$lang = array_merge($lang, array(
	'BOTS'				=> 'Manage bots',
	'BOTS_EXPLAIN'		=> '“Bots”, “spiders” or “crawlers” are automated agents most commonly used by search engines to update their databases. Since they rarely make proper use of sessions they can distort visitor counts, increase load and sometimes fail to index sites correctly. Here you can define a special type of user to overcome these problems.',
	'BOT_ACTIVATE'		=> 'Activate',
	'BOT_ACTIVE'		=> 'Bot active',
	'BOT_ADD'			=> 'Add bot',
	'BOT_ADDED'			=> 'New bot successfully added.',
	'BOT_AGENT'			=> 'Agent match',
	'BOT_AGENT_EXPLAIN'	=> 'A string matching the bots browser agent, partial matches are allowed.',
	'BOT_DEACTIVATE'	=> 'Deactivate',
	'BOT_DELETED'		=> 'Bot deleted successfully.',
	'BOT_EDIT'			=> 'Edit bots',
	'BOT_EDIT_EXPLAIN'	=> 'Here you can add or edit an existing bot entry. You may define an agent string and/or one or more IP addresses (or range of addresses) to match. Be careful when defining matching agent strings or addresses. You may also specify a style and language that the bot will view the board using. This may allow you to reduce bandwidth use by setting a simple style for bots. Remember to set appropriate permissions for the special Bot usergroup.',
	'BOT_LANG'			=> 'Bot language',
	'BOT_LANG_EXPLAIN'	=> 'The language presented to the bot as it browses.',
	'BOT_LAST_VISIT'	=> 'Last visit',
	'BOT_IP'			=> 'Bot IP address',
	'BOT_IP_EXPLAIN'	=> 'Partial matches are allowed, separate addresses with a comma.',
	'BOT_NAME'			=> 'Bot name',
	'BOT_NAME_EXPLAIN'	=> 'Used only for your own information.',
	'BOT_NAME_TAKEN'	=> 'The name is already in use on your board and can’t be used for the Bot.',
	'BOT_NEVER'			=> 'Never',
	'BOT_STYLE'			=> 'Bot style',
	'BOT_STYLE_EXPLAIN'	=> 'The style used for the board by the bot.',
	'BOT_UPDATED'		=> 'Existing bot updated successfully.',

	'ERR_BOT_AGENT_MATCHES_UA'	=> 'The bot agent you supplied is similar to the one you are currently using. Please adjust the agent for this bot.',
	'ERR_BOT_NO_IP'				=> 'The IP addresses you supplied were invalid or the hostname could not be resolved.',
	'ERR_BOT_NO_MATCHES'		=> 'You must supply at least one of an agent or IP for this bot match.',

	'NO_BOT'		=> 'Found no bot with the specified ID.',
	'NO_BOT_GROUP'	=> 'Unable to find special bot group.',
));

?>