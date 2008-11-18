<?php # $Id: pmachine.inc.php 50 2005-04-25 16:50:43Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *  pMachine  Importer,  by Garvin Hicking *
 * ****************************************************************/

class Serendipity_Import_pMachine extends Serendipity_Import {
    var $info        = array('software' => 'pMachine Pro 2.4');
    var $data        = array();
    var $inputFields = array();


    function Serendipity_Import_pMachine($data) {
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
                                         'default' => 'pm_'),

                                   array('text'    => CHARSET,
                                         'type'    => 'list',
                                         'name'    => 'charset',
                                         'value'   => 'native',
                                         'default' => $this->getCharsets()),

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
        $categories = array();
        $entries = array();

        if (!extension_loaded('mysql')) {
            return MYSQL_REQUIRED;
        }

        $pmdb = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if (!$pmdb) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if (!@mysql_select_db($this->data['name'])) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($pmdb));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT id         AS ID,
                                    username   AS user_login,
                                    `password` AS user_pass,
                                    email      AS user_email,
                                    status     AS user_level,
                                    url        AS url
                               FROM {$this->data['prefix']}members", $pmdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($pmdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res); $x < $max_x ; $x++ ) {
            $users[$x] = mysql_fetch_assoc($res);

            $data = array('right_publish' => ($users[$x]['user_level'] >= 3) ? 1 : 0,
                          'realname'      => $users[$x]['user_login'],
                          'username'      => $users[$x]['user_login'],
                          'email'         => $users[$x]['user_email'],
                          'password'      => $users[$x]['user_pass']); // pMachine uses md5, too.

            if ( $users[$x]['user_level'] < 12 ) {
                $data['userlevel'] = USERLEVEL_EDITOR;
            } else {
                $data['userlevel'] = USERLEVEL_ADMIN;
            }

            if ($serendipity['serendipityUserlevel'] < $data['userlevel']) {
                $data['userlevel'] = $serendipity['serendipityUserlevel'];
            }

            serendipity_db_insert('authors', $this->strtrRecursive($data));
            $users[$x]['authorid'] = serendipity_db_insert_id('authors', 'authorid');
        }

        /* Categories */
        $res = @$this->nativeQuery("SELECT id       AS cat_ID,
                                    category AS cat_name,
                                    category AS category_description
                               FROM {$this->data['prefix']}categories ORDER BY id", $pmdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($pmdb));
        }

        // Get all the info we need
        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++) {
            $categories[] = mysql_fetch_assoc($res);
        }

        // Insert all categories as top level (we need to know everyone's ID before we can represent the hierarchy).
        for ($x=0, $max_x = sizeof($categories) ; $x < $max_x ; $x++ ) {
            $cat = array('category_name'        => $categories[$x]['cat_name'],
                         'category_description' => $categories[$x]['category_description'],
                         'parentid'             => 0, // <---
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $categories[$x]['categoryid'] = serendipity_db_insert_id('category', 'categoryid');
        }

        serendipity_rebuildCategoryTree();

        /* Entries */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}weblog ORDER BY t_stamp;", $pmdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($pmdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['title']),
                           'isdraft'        => ($entries[$x]['status'] == 'open') ? 'false' : 'true',
                           'allow_comments' => ($entries[$x]['showcomments'] == '1' ) ? 'true' : 'false',
                           'timestamp'      => $entries[$x]['t_stamp'],
                           'extended'       => $this->strtr($entries[$x]['more']),
                           'body'           => $this->strtr($entries[$x]['body']));

            $entry['authorid'] = '';
            $entry['author']   = '';
            foreach ($users as $user) {
                if ($user['ID'] == $entries[$x]['member_id']) {
                    $entry['authorid'] = $user['authorid'];
                    $entry['author']   = $user['username'];
                    break;
                }
            }

            if (!is_int($entries[$x]['entryid'] = serendipity_updertEntry($entry))) {
                return $entries[$x]['entryid'];
            }

            /* Entry/category */
            foreach ($categories as $category) {
                if ($category['cat_ID'] == $entries[$x]['category'] ) {
                    $data = array('entryid'    => $entries[$x]['entryid'],
                                  'categoryid' => $category['categoryid']);
                    serendipity_db_insert('entrycat', $this->strtrRecursive($data));
                    break;
                }
            }
        }

        /* Comments */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}comments;", $pmdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($pmdb));
        }

        while ($a = mysql_fetch_assoc($res)) {
            foreach ($entries as $entry) {
                if ($entry['post_id'] == $a['post_id'] ) {
                    $author   = '';
                    $mail     = '';
                    $url      = '';
                    if (!empty($a['member_id'])) {
                        foreach($users AS $user) {
                            if ($user['ID'] == $a['member_id']) {
                                $author = $user['user_login'];
                                $mail     = $user['user_email'];
                                $url      = $user['url'];
                                break;
                            }
                        }
                    }

                    $comment = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => $a['t_stamp'],
                                     'author'    => $author,
                                     'email'     => $mail,
                                     'url'       => $url,
                                     'ip'        => $a['comment_ip'],
                                     'status'    => ($a['status'] == 'open' ? 'approved' : 'pending'),
                                     'body'      => $a['body'],
                                     'subscribed'=> 'false',
                                     'type'      => 'NORMAL');

                    serendipity_db_insert('comments', $this->strtrRecursive($comment));
                    if ($a['status'] == 'open') {
                        $cid = serendipity_db_insert_id('comments', 'id');
                        serendipity_approveComment($cid, $entry['entryid'], true);
                    }
                }
            }
        }

        $serendipity['noautodiscovery'] = $noautodiscovery;

        // That was fun.
        return true;
    }
}

return 'Serendipity_Import_pMachine';

/* vim: set sts=4 ts=4 expandtab : */
?>
