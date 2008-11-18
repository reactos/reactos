<?php # $Id: b2evolution.inc.php 558 2005-10-15 16:02:02Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *  b2evolution  Importer,   by Garvin Hicking *
 * ****************************************************************/

class Serendipity_Import_b2evolution extends Serendipity_Import {
    var $info        = array('software' => 'b2Evolution 0.9.0.11 Paris');
    var $data        = array();
    var $inputFields = array();
    var $categories  = array();

    function getImportNotes() {
        return '';
    }

    function Serendipity_Import_b2evolution($data) {
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
                                         'value'   => 'UTF-8',
                                         'default' => $this->getCharsets(true)),

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

        $b2db = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if (!$b2db) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if (!@mysql_select_db($this->data['name'])) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($b2db));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT ID         AS ID,
                                    user_login AS user_login,
                                    user_pass  AS user_pass,
                                    user_email AS user_email,
                                    user_level AS user_level,
                                    user_url   AS user_url
                               FROM evo_users", $b2db);
        if (!$res) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($b2db));
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
            } elseif ($users[$x]['user_level'] <= 9) {
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
        if (!$this->importCategories(null, 0, $b2db)) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($b2db));
        }
        serendipity_rebuildCategoryTree();

        /* Entries */
        $res = @$this->nativeQuery("SELECT * FROM evo_posts ORDER BY ID;", $b2db);
        if (!$res) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($b2db));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['post_title']),
                           'isdraft'        => ($entries[$x]['post_status'] == 'published') ? 'false' : 'true',
                           'allow_comments' => ($entries[$x]['post_comments'] == 'open' ) ? 'true' : 'false',
                           'timestamp'      => strtotime($entries[$x]['post_issue_date']),
                           'body'           => $this->strtr($entries[$x]['post_content']));

            $entry['authorid'] = '';
            $entry['author']   = '';
            foreach ($users as $user) {
                if ($user['ID'] == $entries[$x]['post_author']) {
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
                if ($category['cat_ID'] == $entries[$x]['post_category'] ) {
                    $data = array('entryid'    => $entries[$x]['entryid'],
                                  'categoryid' => $category['categoryid']);
                    serendipity_db_insert('entrycat', $this->strtrRecursive($data));
                    break;
                }
            }
        }

        /* Even more category stuff */
        $res = @$this->nativeQuery("SELECT * FROM evo_postcats;", $b2db);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($b2db));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entrycat = mysql_fetch_assoc($res);

            $entryid = 0;
            $categoryid = 0;
            foreach($entries AS $entry) {
                if ($entry['ID'] == $entrycat['postcat_post_ID']) {
                    $entryid = $entry['entryid'];
                    break;
                }
            }

            foreach($this->categories AS $category) {
                if ($category['cat_ID'] == $entrycat['postcat_cat_ID']) {
                    $categoryid = $category['categoryid'];
                }
            }

            if ($entryid > 0 && $categoryid > 0) {
                $data = array('entryid'    => $entryid,
                              'categoryid' => $categoryid);
                serendipity_db_insert('entrycat', $this->strtrRecursive($data));
            }
        }

        /* Comments */
        $res = @$this->nativeQuery("SELECT * FROM evo_comments;", $b2db);
        if (!$res) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($b2db));
        }

        while ($a = mysql_fetch_assoc($res)) {
            foreach ($entries as $entry) {
                if ($entry['ID'] == $a['comment_post_ID'] ) {
                    $author = '';
                    $mail     = '';
                    $url      = '';
                    if (!empty($a['comment_author_ID'])) {
                        foreach($users AS $user) {
                            if ($user['ID'] == $a['comment_author_ID']) {
                                $author = $user['user_login'];
                                $mail = $user['user_email'];
                                $url  = $user['user_url'];
                                break;
                            }
                        }
                    }

                    if (empty($author) && empty($mail)) {
                        $author = $a['comment_author'];
                        $mail = $a['comment_author_email'];
                        $url = $a['comment_author_url'];
                    }

                    $comment = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => strtotime($a['comment_date']),
                                     'author'    => $author,
                                     'email'     => $mail,
                                     'url'       => $url,
                                     'ip'        => $a['comment_author_IP'],
                                     'status'    => ($a['comment_status'] == 'published' ? 'approved' : 'pending'),
                                     'body'      => $a['comment_content'],
                                     'subscribed'=> 'false',
                                     'type'      => 'NORMAL');

                    serendipity_db_insert('comments', $this->strtrRecursive($comment));
                    if ($a['comment_status'] == 'published') {
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

    function importCategories($parentid = 0, $new_parentid = 0, $b2db) {
        if (is_null($parentid)) {
            $where = 'WHERE ISNULL(cat_parent_ID)';
        } else {
            $where = "WHERE cat_parent_ID = '" . mysql_escape_string($parentid) . "'";
        }

        $res = $this->nativeQuery("SELECT * FROM evo_categories
                                     " . $where, $b2db);
        if (!$res) {
            echo mysql_error();
            return false;
        }

        // Get all the info we need
        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++) {
            $row = mysql_fetch_assoc($res);
            $cat = array('category_name'        => $row['cat_name'],
                         'category_description' => $row['cat_description'],
                         'parentid'             => (int)$new_parentid,
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $row['categoryid']  = serendipity_db_insert_id('category', 'categoryid');
            $this->categories[] = $row;
            $this->importCategories($row['cat_ID'], $row['categoryid'], $b2db);
        }

        return true;
    }
}

return 'Serendipity_Import_b2evolution';

/* vim: set sts=4 ts=4 expandtab : */
?>
