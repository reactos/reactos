<?php # $Id: comment.php 449 2005-09-06 18:00:05Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

include_once('serendipity_config.inc.php');
include_once S9Y_INCLUDE_PATH . 'include/functions_entries_admin.inc.php';

header('Content-Type: text/html; charset=' . LANG_CHARSET);

if (isset($serendipity['GET']['delete'], $serendipity['GET']['entry'], $serendipity['GET']['type'])) {
    serendipity_deleteComment($serendipity['GET']['delete'], $serendipity['GET']['entry'], $serendipity['GET']['type']);
    if (serendipity_isResponseClean($_SERVER['HTTP_REFERER'])) {
        header('Location: '. $_SERVER['HTTP_REFERER']);
    }
}

if (isset($serendipity['GET']['switch'], $serendipity['GET']['entry'])) {
    serendipity_allowCommentsToggle($serendipity['GET']['entry'], $serendipity['GET']['switch']);
}

serendipity_rememberComment();

if (!($type = @$_REQUEST['type'])) {
    $type = 'normal';
}

$tb_logging = false; // for developers: can be switched to true!

if ($type == 'trackback') {
    if ($tb_logging) {
        # PHP 4.2.2 way of doing things
        ob_start();
        print_r($_REQUEST);
        $tmp = ob_get_contents();
        ob_end_clean();

        $fp = fopen('trackback2.log', 'a');
        fwrite($fp, '[' . date('d.m.Y H:i') . '] RECEIVED TRACKBACK' . "\n");
        fwrite($fp, '[' . date('d.m.Y H:i') . '] ' . $tmp . "\n");
    }

    $uri = $_SERVER['REQUEST_URI'];
    if (isset($_REQUEST['entry_id'])) {
        $id = (int)$_REQUEST['entry_id'];
    } else if ($_REQUEST['amp;entry_id']) {
        // For possible buggy variable transmission caused by an intermediate CVS-release of s9y
        $id = (int)$_REQUEST['amp;entry_id'];
    } else if (preg_match('@/(\d+)_[^/]*$@', $uri, $matches)) {
        $id = (int)$matches[1];
    }

    if ($tb_logging) {
        fwrite($fp, '[' . date('d.m.Y H:i') . '] Match on ' . $uri . "\n");
        fwrite($fp, '[' . date('d.m.Y H:i') . '] ID: ' . $id . "\n");
        fclose($fp);
    }

    if (add_trackback($id, $_REQUEST['title'], $_REQUEST['url'], $_REQUEST['blog_name'], $_REQUEST['excerpt'])) {
        if ($tb_logging) {
            $fp = fopen('trackback2.log', 'a');
            fwrite($fp, '[' . date('d.m.Y H:i') . '] TRACKBACK SUCCESS' . "\n");
        }
        report_trackback_success();
    } else {
        if ($tb_logging) {
            $fp = fopen('trackback2.log', 'a');
            fwrite($fp, '[' . date('d.m.Y H:i') . '] TRACKBACK FAILURE' . "\n");
        }
        report_trackback_failure();
    }

    if ($tb_logging) {
        fclose($fp);
    }
} else if ($type == 'pingback') {
    if (add_pingback($_REQUEST['entry_id'], $HTTP_RAW_POST_DATA)) {
        report_pingback_success();
    } else {
        report_pingback_failure();
    }
} else {
    $id = (int)(!empty($serendipity['POST']['entry_id']) ? $serendipity['POST']['entry_id'] : $serendipity['GET']['entry_id']);
    $serendipity['head_subtitle'] = COMMENTS;
    $serendipity['smarty_file'] = 'commentpopup.tpl';
    serendipity_smarty_init();

    if ($id == 0) {
        return false;
    } else {
        $serendipity['smarty']->assign('entry_id', $id);
    }

    if (isset($_GET['success']) && $_GET['success'] == 'true') {
        $serendipity['smarty']->assign(
            array(
                'is_comment_added'   => true,
                'comment_url'        => htmlspecialchars($_GET['url']) . '&amp;serendipity[entry_id]=' . $id,
                'comment_string'     => explode('%s', COMMENT_ADDED_CLICK)
            )
        );
    } else if (!isset($serendipity['POST']['submit'])) {
        if ($serendipity['GET']['type'] == 'trackbacks') {
            $query = "SELECT title, timestamp FROM {$serendipity['dbPrefix']}entries WHERE id = '". $id ."'";
            $entry = serendipity_db_query($query);
            $entry = serendipity_archiveURL($id, $entry[0]['title'], 'baseURL', true, array('timestamp' => $entry[0]['timestamp']));

            $serendipity['smarty']->assign(
                array(
                    'is_showtrackbacks' => true,
                    'comment_url'       => $serendipity['baseURL'] . 'comment.php?type=trackback&amp;entry_id=' . $id,
                    'comment_entryurl'  => $entry
                )
            );
        } else {
            $query = "SELECT id, last_modified, timestamp, allow_comments, moderate_comments FROM {$serendipity['dbPrefix']}entries WHERE id = '" . $id . "'";
            $ca    = serendipity_db_query($query, true);
            $comment_allowed = serendipity_db_bool($ca['allow_comments']) || !is_array($ca) ? true : false;
            $serendipity['smarty']->assign(
                array(
                    'is_showcomments'    => true,
                    'is_comment_allowed' => $comment_allowed
                )
            );

            if ($comment_allowed) {
                serendipity_displayCommentForm($id, '?', NULL, $serendipity['POST'], true, serendipity_db_bool($ca['moderate_comments']), $ca);
            }
        }
    } else {
        $comment['url']       = $serendipity['POST']['url'];
        $comment['comment']   = trim($serendipity['POST']['comment']);
        $comment['name']      = $serendipity['POST']['name'];
        $comment['email']     = $serendipity['POST']['email'];
        $comment['subscribe'] = $serendipity['POST']['subscribe'];
        $comment['parent_id'] = $serendipity['POST']['replyTo'];
        if (!empty($comment['comment'])) {
            if (serendipity_saveComment($id, $comment, 'NORMAL')) {
                $sc_url = $serendipity['baseURL'] . 'comment.php?serendipity[entry_id]=' . $id . '&success=true&url=' . urlencode($_SERVER['HTTP_REFERER']);
                if (serendipity_isResponseClean($sc_url)) {
                    header('Location: ' . $sc_url);
                }
                exit;
            } else {
                $serendipity['smarty']->assign(
                    array(
                        'is_comment_notadded' => true,
                        'comment_url'         => htmlspecialchars($_SERVER['HTTP_REFERER']),
                        'comment_string'      => explode('%s', COMMENT_NOT_ADDED_CLICK)
                    )
                );
            }
        } else {
            $serendipity['smarty']->assign(
                array(
                    'is_comment_empty' => true,
                    'comment_url'      => htmlspecialchars($_SERVER['HTTP_REFERER']),
                    'comment_string'   => explode('%s', EMPTY_COMMENT)
                )
            );
        }
    }

    $serendipity['smarty']->display(serendipity_getTemplateFile($serendipity['smarty_file'], 'serendipityPath'));
}
/* vim: set sts=4 ts=4 expandtab : */
?>
