<?php # $Id: phpbb.inc.php 50 2005-04-25 16:50:43Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *  phpbb  Importer,     by Garvin Hicking *
 * ****************************************************************/

class Serendipity_Import_phpbb extends Serendipity_Import {
    var $info        = array('software' => 'phpBB');
    var $data        = array();
    var $inputFields = array();
    var $categories  = array();

    function Serendipity_Import_phpbb($data) {
        $this->data = $data;
        $this->inputFields = array(array('text' => INSTALL_DBHOST,
                                         'type' => 'input',
                                         'name' => 'host'),

                                   array('text' => INSTALL_DBUSER,
                                         'type' => 'input',
                                         'name' => 'user'),

                                   array('text' => INSTALL_DBPASS,
                                         'type' => 'protected',
                                         'name' => 'pass'),

                                   array('text' => INSTALL_DBNAME,
                                         'type' => 'input',
                                         'name' => 'name'),

                                   array('text' => INSTALL_DBPREFIX,
                                         'type' => 'input',
                                         'name' => 'prefix',
                                         'default' => 'phpbb_'),

                                   array('text'    => CHARSET,
                                         'type'    => 'list',
                                         'name'    => 'charset',
                                         'value'   => 'native',
                                         'default' => $this->getCharsets(false)),

                                   array('text'    => CONVERT_HTMLENTITIES,
                                         'type'    => 'bool',
                                         'name'    => 'use_strtr',
                                         'default' => 'true'),

                                   array('text'    => ACTIVATE_AUTODISCOVERY,
                                         'type'    => 'bool',
                                         'name'    => 'autodiscovery',
                                         'default' => 'false')
                            );
    }

    function validateData() {
        return sizeof($this->data);
    }

    function getInputFields() {
        return $this->inputFields;
    }

    function import() {
        global $serendipity;

        // Save this so we can return it to its original value at the end of this method.
        $noautodiscovery = isset($serendipity['noautodiscovery']) ? $serendipity['noautodiscovery'] : false;

        if ($this->data['autodiscovery'] == 'false') {
            $serendipity['noautodiscovery'] = 1;
        }

        $this->getTransTable();

        $this->data['prefix'] = serendipity_db_escape_string($this->data['prefix']);
        $users = array();
        $entries = array();

        if (!extension_loaded('mysql')) {
            return MYSQL_REQUIRED;
        }

        $gdb = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if (!$gdb) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if (!@mysql_select_db($this->data['name'])) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($gdb));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT user_id       AS ID,
                                    username      AS user_login,
                                    user_password AS user_pass,
                                    user_email    AS user_email,
                                    user_website  AS user_url,
                                    user_level
                               FROM {$this->data['prefix']}users
                              WHERE user_active = 1", $gdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($gdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res); $x < $max_x ; $x++ ) {
            $users[$x] = mysql_fetch_assoc($res);

            $data = array('right_publish' => 1,
                          'realname'      => $users[$x]['user_login'],
                          'username'      => $users[$x]['user_login'],
                          'email'         => $users[$x]['user_email'],
                          'userlevel'     => ($users[$x]['user_level'] == 0 ? USERLEVEL_EDITOR : USERLEVEL_ADMIN),
                          'password'      => $users[$x]['user_pass']); // MD5 compatible

            if ($serendipity['serendipityUserlevel'] < $data['userlevel']) {
                $data['userlevel'] = $serendipity['serendipityUserlevel'];
            }

            serendipity_db_insert('authors', $this->strtrRecursive($data));
            echo mysql_error();
            $users[$x]['authorid'] = serendipity_db_insert_id('authors', 'authorid');
        }

        /* Categories */
        $res = @$this->nativeQuery("SELECT cat_id AS cat_ID, 
                                    cat_title AS cat_name 
                               FROM {$this->data['prefix']}categories", $gdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($gdb));
        }

        // Get all the info we need
        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++) {
            $parent_categories[] = mysql_fetch_assoc($res);
        }

        for ($x=0, $max_x = sizeof($parent_categories) ; $x < $max_x ; $x++ ) {
            $cat = array('category_name'        => $parent_categories[$x]['cat_name'],
                         'category_description' => '',
                         'parentid'             => 0, // <---
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $parent_categories[$x]['categoryid'] = serendipity_db_insert_id('category', 'categoryid');
        }

        /* Categories */
        $res = @$this->nativeQuery("SELECT forum_id AS cat_ID,
                                    cat_id   AS parent_cat_id, 
                                    forum_name AS cat_name, 
                                    forum_desc AS category_description 
                               FROM {$this->data['prefix']}forums ORDER BY forum_order;", $gdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($gdb));
        }

        // Get all the info we need
        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++) {
            $categories[] = mysql_fetch_assoc($res);
        }

        // Insert all categories as top level (we need to know everyone's ID before we can represent the hierarchy).
        for ($x=0, $max_x = sizeof($categories) ; $x < $max_x ; $x++ ) {
            $pcatid = 0;
            foreach($parent_categories AS $pcat) {
                if ($pcat['cat_ID'] == $categories[$x]['parent_cat_id']) {
                    $pcatid = $pcat['cat_ID'];
                    break;
                }
            }

            $cat = array('category_name'        => $categories[$x]['cat_name'],
                         'category_description' => $categories[$x]['category_description'],
                         'parentid'             => $pcatid, // <---
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $categories[$x]['categoryid'] = serendipity_db_insert_id('category', 'categoryid');
        }

        serendipity_rebuildCategoryTree();

        /* Entries */
        $res = @$this->nativeQuery("SELECT t.topic_title, 
                                    t.topic_poster,
                                    t.forum_id,
                                    p.post_time,
                                    pt.post_subject,
                                    pt.post_text,
                                    count(p.topic_id) AS ccount,
                                    p.topic_id,
                                    MIN(p.post_id) AS post_id
                               FROM {$this->data['prefix']}topics AS t
                    LEFT OUTER JOIN {$this->data['prefix']}posts  AS p
                                 ON t.topic_id = p.topic_id
                    LEFT OUTER JOIN {$this->data['prefix']}posts_text  AS pt
                                 ON pt.post_id = p.post_id
                           GROUP BY p.topic_id
                           ", $gdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($gdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['post_subject']),
                           'isdraft'        => 'false',
                           'allow_comments' => 'true',
                           'timestamp'      => $entries[$x]['post_time'],
                           'body'           => $this->strtr($entries[$x]['post_text']),
                           'extended'       => ''
                           );

            $entry['authorid'] = '';
            $entry['author']   = '';
            foreach ($users as $user) {
                if ($user['ID'] == $entries[$x]['topic_poster']) {
                    $entry['authorid'] = $user['authorid'];
                    $entry['author']   = $user['user_login'];
                    break;
                }
            }

            if (!is_int($entries[$x]['entryid'] = serendipity_updertEntry($entry))) {
                return $entries[$x]['entryid'];
            }

            /* Entry/category */
            foreach ($categories as $category) {
                if ($category['cat_ID'] == $entries[$x]['forum_id'] ) {
                    $data = array('entryid'    => $entries[$x]['entryid'],
                                  'categoryid' => $category['categoryid']);
                    serendipity_db_insert('entrycat', $this->strtrRecursive($data));
                    break;
                }
            }
            
            /* Comments */
            $topic_id = $entries[$x]['topic_id'];
            $c_res = @$this->nativeQuery("SELECT t.topic_title, 
                                        t.topic_poster,
                                        p.poster_id,
                                        t.forum_id,
                                        p.post_time,
                                        pt.post_subject,
                                        pt.post_text,
                                        pt.post_id
                                   FROM {$this->data['prefix']}topics AS t
                        LEFT OUTER JOIN {$this->data['prefix']}posts  AS p
                                     ON t.topic_id = p.topic_id
                        LEFT OUTER JOIN {$this->data['prefix']}posts_text  AS pt
                                     ON pt.post_id = p.post_id
                                  WHERE p.topic_id = {$topic_id} 
                               ", $gdb);
            if (!$c_res) {
                return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($gdb));
            }
    
            while ($a = mysql_fetch_assoc($c_res)) {
                if ($a['post_id'] == $entries[$x]['post_id']) {
                    continue;
                }
                $author   = '';
                $mail     = '';
                $url      = '';

                foreach($users AS $user) {
                    if ($user['ID'] == $a['poster_id']) {
                        $author = $user['user_login'];
                        $mail   = $user['user_email'];
                        $url    = $user['user_url'];
                        break;
                    }
                }

                $comment = array('entry_id ' => $entries[$x]['entryid'],
                                 'parent_id' => 0,
                                 'timestamp' => $a['post_time'],
                                 'author'    => $author,
                                 'email'     => $mail,
                                 'url'       => $url,
                                 'ip'        => '',
                                 'status'    => 'approved',
                                 'body'      => $a['post_text'],
                                 'subscribed'=> 'false',
                                 'type'      => 'NORMAL');

                serendipity_db_insert('comments', $this->strtrRecursive($comment));
                $cid = serendipity_db_insert_id('comments', 'id');
                serendipity_approveComment($cid, $entries[$x]['entryid'], true);
            }
        }

        $serendipity['noautodiscovery'] = $noautodiscovery;

        // That was fun.
        return true;
    }
}

return 'Serendipity_Import_phpbb';

/* vim: set sts=4 ts=4 expandtab : */
?>
