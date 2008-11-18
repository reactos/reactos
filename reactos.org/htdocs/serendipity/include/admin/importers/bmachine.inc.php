<?php # $Id: bmachine.inc.php 50 2005-04-25 16:50:43Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *  bmachine  Importer,   by Garvin Hicking *
 * ****************************************************************/

class Serendipity_Import_bmachine extends Serendipity_Import {
    var $info        = array('software' => 'boastMachine 3.0');
    var $data        = array();
    var $inputFields = array();
    var $categories  = array();

    function getImportNotes() {
        return '';
    }

    function Serendipity_Import_bmachine($data) {
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

        $users = array();
        $entries = array();

        if (!extension_loaded('mysql')) {
            return MYSQL_REQUIRED;
        }

        $txpdb = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if (!$txpdb) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if (!@mysql_select_db($this->data['name'])) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($txpdb));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT id         AS ID,
                                    user_login AS user_login,
                                    user_pass  AS user_pass,
                                    user_email AS user_email,
                                    level      AS user_level,
                                    user_url   AS user_url
                               FROM bmc_users", $txpdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($txpdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res); $x < $max_x ; $x++ ) {
            $users[$x] = mysql_fetch_assoc($res);

            $data = array('right_publish' => ($users[$x]['user_level'] >= 2) ? 1 : 0,
                          'realname'      => $users[$x]['user_login'],
                          'username'      => $users[$x]['user_login'],
                          'email'         => $users[$x]['user_email'],
                          'password'      => $users[$x]['user_pass']); // MD5 compatible

            if ( $users[$x]['user_level'] <= 2 ) {
                $data['userlevel'] = USERLEVEL_EDITOR;
            } elseif ($users[$x]['user_level'] == 3) {
                $data['userlevel'] = USERLEVEL_CHIEF;
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
                                    cat_name AS cat_name,
                                    cat_info AS category_description
                               FROM bmc_cats ORDER BY id;", $txpdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($txpdb));
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
        $res = @$this->nativeQuery("SELECT * FROM bmc_posts ORDER BY id;", $txpdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($txpdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['title']),
                           'isdraft'        => ($entries[$x]['status'] == '1') ? 'false' : 'true',
                           'allow_comments' => ($entries[$x]['m_cmt'] == '1' ) ? 'true' : 'false',
                           'timestamp'      => $entries[$x]['date'],
                           'extended'       => $this->strtr($entries[$x]['data']),
                           'body'           => $this->strtr($entries[$x]['summary']));

            $entry['authorid'] = '';
            $entry['author']   = '';
            foreach ($users as $user) {
                if ($user['ID'] == $entries[$x]['author']) {
                    $entry['authorid'] = $user['authorid'];
                    $entry['author']   = $user['user_login'];
                    break;
                }
            }

            if (!is_int($entries[$x]['entryid'] = serendipity_updertEntry($entry))) {
                return $entries[$x]['entryid'];
            }

            /* Entry/category */
            foreach ($this->categories as $category) {
                if ($category['cat_ID'] == $entries[$x]['cat'] ) {
                    $data = array('entryid'    => $entries[$x]['entryid'],
                                  'categoryid' => $category['categoryid']);
                    serendipity_db_insert('entrycat', $this->strtrRecursive($data));
                    break;
                }
            }
        }

        /* Comments */
        $res = @$this->nativeQuery("SELECT * FROM bmc_comments;", $txpdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($txpdb));
        }

        while ($a = mysql_fetch_assoc($res)) {
            foreach ($entries as $entry) {
                if ($entry['id'] == $a['post'] ) {
                    $author   = '';
                    $mail     = '';
                    $url      = '';
                    if (!empty($a['author'])) {
                        foreach($users AS $user) {
                            if ($user['ID'] == $a['author']) {
                                $author = $user['user_login'];
                                $mail = $user['user_email'];
                                $url  = $user['user_url'];
                                break;
                            }
                        }
                    }

                    if (empty($author) && empty($mail)) {
                        $author = $a['auth_name'];
                        $mail = $a['auth_email'];
                        $url = $a['auth_url'];
                    }

                    $comment = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => $a['date'],
                                     'author'    => $author,
                                     'email'     => $mail,
                                     'url'       => $url,
                                     'ip'        => $a['auth_ip'],
                                     'status'    => 'approved',
                                     'body'      => $a['data'],
                                     'subscribed'=> 'false',
                                     'type'      => 'NORMAL');

                    serendipity_db_insert('comments', $this->strtrRecursive($comment));
                    $cid = serendipity_db_insert_id('comments', 'id');
                    serendipity_approveComment($cid, $entry['entryid'], true);
                }
            }
        }

        $serendipity['noautodiscovery'] = $noautodiscovery;

        // That was fun.
        return true;
    }
}

return 'Serendipity_Import_bmachine';

/* vim: set sts=4 ts=4 expandtab : */
?>
