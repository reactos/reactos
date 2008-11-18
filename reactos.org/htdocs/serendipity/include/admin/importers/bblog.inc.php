<?php # $Id: bblog.inc.php 144 2005-06-05 17:53:31Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *  bblog  Importer,    by Garvin Hicking *
 * ****************************************************************/

class Serendipity_Import_bblog extends Serendipity_Import {
    var $info        = array('software' => 'bBlog 0.7.4');
    var $data        = array();
    var $inputFields = array();
    var $categories  = array();

    function Serendipity_Import_bblog($data) {
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
                                         'default' => 'bB_'),

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

        $bblogdb = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if (!$bblogdb) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if (!@mysql_select_db($this->data['name'])) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($bblogdb));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT id         AS ID,
                                    password   AS pw,
                                    nickname   AS user_login,
                                    email      AS user_email,
                                    url        AS user_url
                               FROM {$this->data['prefix']}authors", $bblogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($bblogdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res); $x < $max_x ; $x++ ) {
            $users[$x] = mysql_fetch_assoc($res);

            $data = array('right_publish' => 1,
                          'username'      => $users[$x]['user_login'],
                          'email'         => $users[$x]['user_email'],
                          'userlevel'     => USERLEVEL_ADMIN,
                          'password'      => md5($users[$x]['pw'])); // Wicked. This is the first blog I've seen storing cleartext passwords :-D

            if ($serendipity['serendipityUserlevel'] < $data['userlevel']) {
                $data['userlevel'] = $serendipity['serendipityUserlevel'];
            }

            serendipity_db_insert('authors', $this->strtrRecursive($data));
            echo mysql_error();
            $users[$x]['authorid'] = serendipity_db_insert_id('authors', 'authorid');
        }

        /* Categories */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}sections", $bblogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($bblogdb));
        }

        // Get all the info we need
        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++) {
            $row = mysql_fetch_assoc($res);
            $cat = array('category_name'        => $row['nicename'],
                         'category_description' => $row['nicename'],
                         'parentid'             => 0,
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $row['categoryid']  = serendipity_db_insert_id('category', 'categoryid');
            $this->categories[] = $row;
        }

        serendipity_rebuildCategoryTree();

        /* Entries */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}posts ORDER BY postid;", $bblogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($bblogdb));
        }

        for ($x=0, $max_x = mysql_num_rows($res) ; $x < $max_x ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['title']),
                           'isdraft'        => ($entries[$x]['status'] == 'live') ? 'false' : 'true',
                           'allow_comments' => ($entries[$x]['allowcomments'] == 'allow' ) ? 'true' : 'false',
                           'timestamp'      => $entries[$x]['posttime'],
                           'body'           => $this->strtr($entries[$x]['body']),
                           'extended'       => '',
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

            $sections = explode(':', $entries[$x]['sections']);
            foreach($sections AS $section) {
                if (empty($section)) {
                    continue;
                }

                foreach($this->categories AS $category) {
                    if ($category['sectionid'] == $section) {
                        $categoryid = $category['categoryid'];
                    }
                }

                if ($categoryid > 0) {
                    $data = array('entryid'    => $entries[$x]['entryid'],
                                  'categoryid' => $categoryid);
                    serendipity_db_insert('entrycat', $this->strtrRecursive($data));
                }
            }
        }

        /* Comments */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}comments WHERE type = 'comment';", $bblogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($bblogdb));
        }

        while ($a = mysql_fetch_assoc($res)) {
            foreach ($entries as $entry) {
                if ($entry['postid'] == $a['postid'] ) {
                    $comment = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => $a['posttime'],
                                     'author'    => $a['postername'],
                                     'email'     => $a['posteremail'],
                                     'url'       => $a['posterwebsite'],
                                     'ip'        => $a['ip'],
                                     'status'    => 'approved',
                                     'body'      => $a['commenttext'],
                                     'subscribed'=> 'false',
                                     'type'      => 'NORMAL');

                    serendipity_db_insert('comments', $this->strtrRecursive($comment));
                    $cid = serendipity_db_insert_id('comments', 'id');
                    serendipity_approveComment($cid, $entry['entryid'], true);
                }
            }
        }

        /* Trackbacks */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}comments WHERE type = 'trackback';", $bblogdb);
        if (!$res) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($bblogdb));
        }

        while ($a = mysql_fetch_assoc($res)) {
            foreach ($entries as $entry) {
                if ($entry['postid'] == $a['postid'] ) {
                    $trackback = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => $a['posttime'],
                                     'title'     => $a['title'],
                                     'author'    => $a['postername'],
                                     'email'     => $a['posteremail'],
                                     'url'       => $a['posterwebsite'],
                                     'ip'        => $a['ip'],
                                     'status'    => 'approved',
                                     'body'      => $a['commenttext'],
                                     'subscribed'=> 'false',
                                     'type'      => 'TRACKBACK');

                    serendipity_db_insert('comments', $this->strtrRecursive($trackback));
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

return 'Serendipity_Import_bblog';

/* vim: set sts=4 ts=4 expandtab : */
?>
