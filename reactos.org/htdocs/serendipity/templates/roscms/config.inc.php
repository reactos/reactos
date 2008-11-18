<?php # $Id: config.inc.php 108 2005-05-19 08:40:00Z garvinhicking $

$probelang = dirname(__FILE__) . '/lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
} else {
    include dirname(__FILE__) . '/lang_en.inc.php';
}

@define('LOGIN_TO_LEAVE_COMMENT', 'Please <a href="/roscms/?page=login&target=%s#comments">login</a> to leave a comment');

$serendipity['smarty']->register_function('roscms_sidebar_transform',
                                          'roscms_smarty_sidebar_transform');
$serendipity['smarty']->register_function('roscms_can_add_comment',
                                          'roscms_smarty_can_add_comment');

function roscms_smarty_sidebar_transform($params, &$smarty)
{
    $old_content = $params['content'];

    if (empty($old_content)) {
        $new_content = '';
    } else {
        $new_content = '<li>';
        $new_content .= preg_replace('=<br />\n?=i', "</li>\n<li>", $old_content);
        $new_content .= "</li>\n";
        $new_content = preg_replace('=<li></li>=i', '', $new_content);
    }

    return $new_content;
}

function roscms_smarty_can_add_comment($params, &$smarty)
{
    if (0 == roscms_subsys_login('roscms', ROSCMS_LOGIN_OPTIONAL, '')) {
        $smarty->assign('can_add_comment', 'false');
    } else {
        $smarty->assign('can_add_comment', 'true');
    }
}

?>
