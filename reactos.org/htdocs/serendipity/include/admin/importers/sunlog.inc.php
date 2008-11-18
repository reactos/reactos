<?php # $Id: sunlog.inc.php 558 2005-10-15 16:02:02Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *  sunlog  Importer,    by Garvin Hicking *
 * ****************************************************************/

class Serendipity_Import_sunlog extends Serendipity_Import {
    var $info        = array('software' => 'Sunlog 0.4.4');
    var $data        = array();
    var $inputFields = array();
    var $categories  = array();

    function getImportNotes() {
        return 'Sunlog uses a crypted string to represent stored passwords. Thus, those passwords are incompatible with the MD5 hashing of Serendipity and can not be reconstructed. The passwords for all users have been set to "sunlog". <strong>You need to modify the passwords manually for each user</strong>, we are sorry for that inconvenience.<br />'
             . '<br />'
             . 'Sunlog has a granular control over access privileges which cannot be migrated to Serendipity. All Users will be migrated as Superusers, you may need to set them to editor or chief users manually after import.';
    }

    function Serendipity_Import_sunlog($data) {
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
                                         'default' => 'sunlog_'),

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
        $entries = array();

        if (!extension_loaded('mysql')) {
            return MYSQL_REQUIRED;
        }

        $sunlogdb = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if (!$sunlogdb) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if (!@mysql_select_db($this->data['name'])) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($sunlogdb));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT id         AS ID,
                                    name       AS user_login,
                                    email      AS user_email,
                                    homepage   AS user_url
                               FROM {$this->data['prefix']}users", $sunlogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($sunlogdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res); $x < $max_x ; $x++ ) {
            $users[$x] = mysql_fetch_assoc($res);

            $data = array('right_publish' => 1,
                          'realname'      => $users[$x]['user_login'],
                          'username'      => $users[$x]['user_login'],
                          'email'         => $users[$x]['user_email'],
                          'userlevel'     => USERLEVEL_ADMIN,
                          'password'      => md5('sunlog'));

            if ($serendipity['serendipityUserlevel'] < $data['userlevel']) {
                $data['userlevel'] = $serendipity['serendipityUserlevel'];
            }

            serendipity_db_insert('authors', $this->strtrRecursive($data));
            echo mysql_error();
            $users[$x]['authorid'] = serendipity_db_insert_id('authors', 'authorid');
        }

        /* Categories */
        if (!$this->importCategories(null, 0, $sunlogdb)) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($sunlogdb));
        }
        serendipity_rebuildCategoryTree();

        /* Entries */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}articles ORDER BY id;", $sunlogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($sunlogdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['title']),
                           'isdraft'        => ($entries[$x]['draft'] == '0') ? 'false' : 'true',
                           'allow_comments' => ($entries[$x]['c_comments'] == '1' ) ? 'true' : 'false',
                           'timestamp'      => strtotime($entries[$x]['timestamp']),
                           'body'           => $this->strtr($entries[$x]['lead_converted']),
                           'extended'       => $this->strtr($entries[$x]['article_converted']),
                           );

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
        }

        /* Even more category stuff */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}transfer_c;", $sunlogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($sunlogdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entrycat = mysql_fetch_assoc($res);

            $entryid = 0;
            $categoryid = 0;
            foreach($entries AS $entry) {
                if ($entry['id'] == $entrycat['article']) {
                    $entryid = $entry['entryid'];
                    break;
                }
            }

            foreach($this->categories AS $category) {
                if ($category['id'] == $entrycat['category']) {
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
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}c_comments;", $sunlogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($sunlogdb));
        }

        while ($a = mysql_fetch_assoc($res)) {
            foreach ($entries as $entry) {
                if ($entry['id'] == $a['for_entry'] ) {
                    $author   = '';
                    $mail     = '';
                    $url      = '';

                    foreach($users AS $user) {
                        if ($user['ID'] == $a['user']) {
                            $author = $user['user_login'];
                            $mail = $user['user_email'];
                            $url  = $user['user_url'];
                            break;
                        }
                    }

                    $comment = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => strtotime($a['insertdate']),
                                     'author'    => $author,
                                     'email'     => $mail,
                                     'url'       => $url,
                                     'ip'        => '',
                                     'status'    => 'approved',
                                     'body'      => $a['comment'],
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

    function importCategories($parentid = 0, $new_parentid = 0, $sunlogdb) {
        $where = "WHERE parent = '" . mysql_escape_string($parentid) . "'";

        $res = $this->nativeQuery("SELECT * FROM {$this->data['prefix']}categories
                                     " . $where, $sunlogdb);
        if (!$res) {
            echo mysql_error();
            return false;
        }

        // Get all the info we need
        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++) {
            $row = mysql_fetch_assoc($res);
            $cat = array('category_name'        => $row['title'],
                         'category_description' => $row['optional_1'] . ' ' . $row['optional_2'] . ' ' . $row['optional_3'],
                         'parentid'             => (int)$new_parentid,
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $row['categoryid']  = serendipity_db_insert_id('category', 'categoryid');
            $this->categories[] = $row;
            $this->importCategories($row['id'], $row['categoryid'], $sunlogdb);
        }

        return true;
    }
}

return 'Serendipity_Import_sunlog';

/* vim: set sts=4 ts=4 expandtab : */
?>
