<?php # $Id: wfwcomment.php 7 2005-04-16 06:39:31Z s_bergmann $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

include_once('serendipity_config.inc.php');

if ($_REQUEST['cid'] != '' && $HTTP_RAW_POST_DATA != '') {
    $comment = array();

    if (!preg_match('@<author[^>]*>(.*)</author[^>]*>@i', $HTTP_RAW_POST_DATA, $name)) {
        preg_match('@<dc:creator[^>]*>(.*)</dc:creator[^>]*>@i', $HTTP_RAW_POST_DATA, $name);
    }

    if (isset($name[1]) && !empty($name[1])) {
        if (preg_match('@^(.*)\((.*)\)@i', $name[1], $names)) {
            $comment['name'] = utf8_decode($names[2]);
            $comment['email'] = utf8_decode($names[1]);
        } else {
            $comment['name'] = utf8_decode($name[1]);
        }
    }

    if (preg_match('@<link[^>]*>(.*)</link[^>]*>@i', $HTTP_RAW_POST_DATA, $link)) {
        $comment['url'] = utf8_decode($link[1]);
    }

    if (preg_match('@<description[^>]*>(.*)</description[^>]*>@ims', $HTTP_RAW_POST_DATA, $description)) {
        if (preg_match('@^<!\[CDATA\[(.*)\]\]>@ims', $description[1], $cdata)) {
            $comment['comment'] = utf8_decode($cdata[1]);
        } else {
            $comment['comment'] = utf8_decode($description[1]);
        }

        if (!empty($comment['comment'])) {
            serendipity_saveComment($_REQUEST['cid'], $comment, 'NORMAL', 'API');
        }
    }
}

?>
