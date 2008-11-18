<?php # $Id: wordpress.inc.php 50 2005-04-25 16:50:43Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *                WordPress Importer, by Evan Nemerson           *
 *****************************************************************/

class Serendipity_Import_WordPress extends Serendipity_Import {
    var $info        = array('software' => 'WordPress');
    var $data        = array();
    var $inputFields = array();


    function Serendipity_Import_WordPress($data) {
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
                                         'name' => 'prefix'),

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

        $this->data['prefix'] = serendipity_db_escape_string($this->data['prefix']);
        $users = array();
        $categories = array();
        $entries = array();

        if ( !extension_loaded('mysql') ) {
            return MYSQL_REQUIRED;;
        }

        $wpdb = @mysql_connect($this->data['host'], $this->data['user'], $this->data['pass']);
        if ( !$wpdb ) {
            return sprintf(COULDNT_CONNECT, $this->data['host']);
        }

        if ( !@mysql_select_db($this->data['name']) ) {
            return sprintf(COULDNT_SELECT_DB, mysql_error($wpdb));
        }

        /* Users */
        $res = @$this->nativeQuery("SELECT ID, user_login, user_pass, user_email, user_level FROM {$this->data['prefix']}users;", $wpdb);
        if ( !$res ) {
            return sprintf(COULDNT_SELECT_USER_INFO, mysql_error($wpdb));
        }

        for ( $x=0 ; $x<mysql_num_rows($res) ; $x++ ) {
            $users[$x] = mysql_fetch_assoc($res);

            $data = array('right_publish' => ($users[$x]['user_level'] >= 1) ? 1 : 0,
                          'realname'      => $users[$x]['user_login'],
                          'username'      => $users[$x]['user_login'],
                          'password'      => $users[$x]['user_pass']); // WP uses md5, too.

            if ( $users[$x]['user_level'] <= 1 ) {
                $data['userlevel'] = USERLEVEL_EDITOR;
            } elseif ( $users[$x]['user_level'] < 5 ) {
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
        $res = @$this->nativeQuery("SELECT cat_ID, cat_name, category_description, category_parent FROM {$this->data['prefix']}categories ORDER BY category_parent, cat_ID;", $wpdb);
        if ( !$res ) {
            return sprintf(COULDNT_SELECT_CATEGORY_INFO, mysql_error($wpdb));
        }

        // Get all the info we need
        for ( $x=0 ; $x<mysql_num_rows($res) ; $x++ )
            $categories[] = mysql_fetch_assoc($res);

        // Insert all categories as top level (we need to know everyone's ID before we can represent the hierarchy).
        for ( $x=0 ; $x<sizeof($categories) ; $x++ ) {
            $cat = array('category_name'        => $categories[$x]['cat_name'],
                         'category_description' => $categories[$x]['category_description'],
                         'parentid'             => 0, // <---
                         'category_left'        => 0,
                         'category_right'       => 0);

            serendipity_db_insert('category', $this->strtrRecursive($cat));
            $categories[$x]['categoryid'] = serendipity_db_insert_id('category', 'categoryid');
        }

        // There has to be a more efficient way of doing this...
        foreach ( $categories as $cat ) {
            if ( $cat['category_parent'] != 0 ) {
                // Find the parent
                $par_id = 0;
                foreach ( $categories as $possible_par ) {
                    if ( $possible_par['cat_ID'] == $cat['category_parent'] ) {
                        $par_id = $possible_par['categoryid'];
                        break;
                    }
                }

                if ( $par_id != 0 ) {
                  serendipity_db_query("UPDATE {$serendipity['dbPrefix']}category SET parentid={$par_id} WHERE categoryid={$cat['categoryid']};");
                } // else { echo "D'oh! " . random_string_of_profanity(); }
            }
        }

        serendipity_rebuildCategoryTree();

        /* Entries */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}posts ORDER BY post_date;", $wpdb);
        if ( !$res ) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($wpdb));
        }

        for ( $x=0 ; $x<mysql_num_rows($res) ; $x++ ) {
            $entries[$x] = mysql_fetch_assoc($res);

            $entry = array('title'          => $this->decode($entries[$x]['post_title']), // htmlentities() is called later, so we can leave this.
                           'isdraft'        => ($entries[$x]['post_status'] == 'publish') ? 'false' : 'true',
                           'allow_comments' => ($entries[$x]['comment_status'] == 'open' ) ? 'true' : 'false',
                           'timestamp'      => strtotime($entries[$x]['post_date']),
                           'body'           => $this->strtr($entries[$x]['post_content']));

            foreach ( $users as $user ) {
                if ( $user['ID'] == $entries[$x]['post_author'] ) {
                    $entry['authorid'] = $user['authorid'];
                    break;
                }
            }

            if ( !is_int($entries[$x]['entryid'] = serendipity_updertEntry($entry)) ) {
                return $entries[$x]['entryid'];
            }
        }

        /* Entry/category */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}post2cat;", $wpdb);
        if ( !$res ) {
            return sprintf(COULDNT_SELECT_ENTRY_INFO, mysql_error($wpdb));
        }

        while ( $a = mysql_fetch_assoc($res) ) {
            foreach ( $categories as $category ) {
                if ( $category['cat_ID'] == $a['category_id'] ) {
                    foreach ( $entries as $entry ) {
                        if ( $a['post_id'] == $entry['ID'] ) {
                            $data = array('entryid' => $entry['entryid'],
                                          'categoryid' => $category['categoryid']);
                            serendipity_db_insert('entrycat', $this->strtrRecursive($data));
                            break;
                        }
                    }
                    break;
                }
            }
        }

        /* Comments */
        $res = @$this->nativeQuery("SELECT * FROM {$this->data['prefix']}comments;", $wpdb);
        if ( !$res ) {
            return sprintf(COULDNT_SELECT_COMMENT_INFO, mysql_error($wpdb));
        }

        while ( $a = mysql_fetch_assoc($res) ) {
            foreach ( $entries as $entry ) {
                if ( $entry['ID'] == $a['comment_post_ID'] ) {
                    $comment = array('entry_id ' => $entry['entryid'],
                                     'parent_id' => 0,
                                     'timestamp' => strtotime($a['comment_date']),
                                     'author'    => $a['comment_author'],
                                     'email'     => $a['comment_author_email'],
                                     'url'       => $a['comment_author_url'],
                                     'ip'        => $a['comment_author_IP'],
                                     'status'    => (empty($a['comment_approved']) || $a['comment_approved'] == '1') ? 'approved' : 'pending',
                                     'subscribed'=> 'false',
                                     'body'      => $a['comment_content'],
                                     'type'      => 'NORMAL');

                    serendipity_db_insert('comments', $this->strtrRecursive($comment));
                    if ($comment['status'] == 'approved') {
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

return 'Serendipity_Import_WordPress';

/* vim: set sts=4 ts=4 expandtab : */
?>
