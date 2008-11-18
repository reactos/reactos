<?php
/**
*
* @package phpBB3
* @version $Id: faq.php 8479 2008-03-29 00:22:48Z naderman $
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

// Start session management
$user->session_begin();
$auth->acl($user->data);
$user->setup();

$mode = request_var('mode', '');

// Load the appropriate faq file
switch ($mode)
{
	case 'bbcode':
		$l_title = $user->lang['BBCODE_GUIDE'];
		$user->add_lang('bbcode', false, true);
	break;

	default:
		$l_title = $user->lang['FAQ_EXPLAIN'];
		$user->add_lang('faq', false, true);
	break;
}

// Pull the array data from the lang pack
$help_blocks = array();
foreach ($user->help as $help_ary)
{
	if ($help_ary[0] == '--')
	{
		$template->assign_block_vars('faq_block', array(
			'BLOCK_TITLE'		=> $help_ary[1])
		);

		continue;
	}

	$template->assign_block_vars('faq_block.faq_row', array(
		'FAQ_QUESTION'		=> $help_ary[0],
		'FAQ_ANSWER'		=> $help_ary[1])
	);
}

// Lets build a page ...
$template->assign_vars(array(
	'L_FAQ_TITLE'	=> $l_title,
	'L_BACK_TO_TOP'	=> $user->lang['BACK_TO_TOP'])
);

page_header($l_title);

$template->set_filenames(array(
	'body' => 'faq_body.html')
);
make_jumpbox(append_sid("{$phpbb_root_path}viewforum.$phpEx"));

page_footer();

?>