<?php # $Id$
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

class Serendipity_Import_Blogger extends Serendipity_Import {
    var $info        = array('software' => 'Blogger.com');
    var $data        = array();
    var $inputFields = array();

    function Serendipity_Import_Blogger($data) {
        global $serendipity;

        $this->data = $data;
        $this->inputFields = array(array('text'    => 'Path to your Blogger export file',
                                         'type'    => 'input',
                                         'name'    => 'bloggerfile',
                                         'value'   => $serendipity['serendipityPath']),

                                   array('text'    => 'New author default password (used for non-existing authors on the serendipity backend, as author passwords from Blogger are not migrated)',
                                         'type'    => 'input',
                                         'name'    => 'defaultpass',
                                         'value'   => ''),

                                   array('text'    => CHARSET,
                                         'type'    => 'list',
                                         'name'    => 'charset',
                                         'value'   => 'UTF-8',
                                         'default' => $this->getCharsets()),

                                    array('text'   => RSS_IMPORT_BODYONLY,
                                         'type'    => 'bool',
                                         'name'    => 'bodyonly',
                                         'value'   => 'false'));
    }

    function validateData() {
        return sizeof($this->data);
    }

    function getInputFields() {
        return $this->inputFields;
    }

    function getImportNotes() {
        $out = '
<style type="text/css">
<!--
.style1 {
    font-size: large;
    font-weight: bold;
    font-family: Arial, Helvetica, sans-serif;
}
.style2 {
    font-family: Arial, Helvetica, sans-serif;
    font-size: x-small;
}
-->
</style>

<p class="style1">BLOGGER.COM to SERENDIPITY IMPORT</p>
<p class="style2">Version 0.1,( 29/10/2005 )</p>
<p class="style2"> <br />
  1. First go to Blogger.com, login.</p>
<p class="style2">2. Go to the templates section. Set the following as your template. You should backup the current template if you want to reset it back after this operation. Click &quot;Save template changes&quot; button to save this new template.</p>
<p class="style2">
  <label>
  <textarea name="textarea" cols="60" rows="20"><Blogger>
STARTPOST

TITLE: <PostSubject><$BlogItemSubject$></PostSubject>
AUTHOR: <$BlogItemAuthor$>
DATE: <$BlogItemDateTime$>
-----
BODY:
<$BlogItemBody$>
-----
<BlogItemCommentsEnabled>
<BlogItemComments>
COMMENT:
AUTHOR: <$BlogCommentAuthor$>
DATE: <$BlogCommentDateTime$>
BODY: <$BlogCommentBody$>
-----
</BlogItemComments>
</BlogItemCommentsEnabled>
ENDPOST
</Blogger>
  </textarea>
  </label>
</p>
<p class="style2">3. Go to the &quot;Settings&quot; section of blogger. </p>
<p class="style2">4. Click the &quot;Formatting&quot; link. From the formatting options, find the &quot;Timestamp Format&quot; option and set it to the top most option. i.e the one with the date and time showing. Find the &quot;Show&quot; option, and set it to 999. Save changes</p>
<p class="style2">5. Now click the &quot;Comments&quot; link. Find the &quot;Comments Timestamp Format&quot; option and set it to the 2nd option from the list. i.e the one with the date and time showing. Save changes. </p>
<p class="style2">6. On the server with your Serendipity installation, create a directory called &quot;blogger&quot;. </p>
<p class="style2">7. Next, back on Blogger.com, go to the &quot;Publishing&quot; section. Set it to publish to an FTP server. Enter the details of the server with your Serendipity installation. Set the FTP path as the path to the &quot;blogger&quot; directory you created in the previous step. </p>
<p class="style2">8. Go back to Blogger.com and find &quot;Publish Entire Blog&quot; under &quot;Posting&quot; -&gt; &quot;Status&quot;. Click the Publish entire blog button to let blogger publish the blog to your ftp server.</p>
<p class="style2">9. Now in the box below type in the path to the &quot;index.html&quot; file blogger created under your &quot;blogger&quot; directory.  File path should then look something like &quot;/httpdocs/blogger/index.html&quot;.</p>
<p class="style2">10. This script will create the users as from the blogger blog being imported. However if a user already exists, then that user will be used instead of creating a new user with similar name. For the new users that this script will create, you need to provide a default password. Type it in the box below.</p>
<p class="style2">11. Click &quot;Submit&quot;. Your posts and comments should be imported to serendipity!</p>
<p class="style2"> If you have questions or problems, feel free to drop me a mail at jaa at technova dot com dot mv.<br />
  <br />
Jaa<br />
http://jaa.technova.com.mv</p>';
        return $out;
    }

    function import() {
        global $serendipity;

        if (empty($this->data['bloggerfile']) || !file_exists($this->data['bloggerfile'])) {
            echo "Path to blogger file empty or path to file wrong! Go back and correct.";
            return false;
        }

        # get default pass from request
        $defaultpass = $this->data['defaultpass'];
        
        # get blogger uploaded file path from request and load file
        $html = file_get_contents($this->data['bloggerfile']);
        
        # find posts using pattern matching
        preg_match_all("/STARTPOST(.*)ENDPOST/sU", $html, $posts);
        
        # iterate through all posts
        foreach($posts[1] as $post) {
        
            # locate the post title
            if (preg_match("/TITLE:(.*)/", $post, $title)) {
                $title = trim($title[1]);
                echo "<b>" . htmlspecialchars($title) . "</b><br />";
            } else {
                $title = "";
                echo "<b>Empty title</b><br />";
            }
                    
            # locate the post author
            if (preg_match("/AUTHOR:(.*)/", $post, $author)) {
                $author = trim($author[1]);
                echo "<em>" . htmlspecialchars($author[1]) . "</em><br />";
            } else {
                $author = "";
                echo "<em>Unknown author</em><br />";
            }
        
            # locate the post date
            if (preg_match("/DATE:(.*)/", $post, $date)) {
                $date = strtotime(trim($date[1]));
                echo "Posted on " . htmlspecialchars($date[1]) . ".<br />";
            } else {
                $date = time();
                echo "Unknown posting time.<br />";
            }
        
            # locate the post body
            if (preg_match("/BODY:(.*)-----/sU", $post, $body)) {
                $body = trim($body[1]);
                echo strlen($body) . " Bytes of text.<br />";
            } else {
                $body = "";
                echo "<strong>Empty Body!</strong><br />";
            }
        
            # find all comments for the post using pattern matching
            if (preg_match_all( "/COMMENT:(.*)----/sU", $post, $commentlist)) {
                echo count($commentlist[1]) . " comments found.<br />";
            } else {
                $commentlist = array();
                echo "No comments found.<br />";
            }
        
            $result = serendipity_db_query("SELECT authorid FROM ". $serendipity['dbPrefix'] ."authors WHERE username = '". serendipity_db_escape_string($author) ."' LIMIT 1", true, 'assoc');
            if (!is_array($result)) {
                $data = array('right_publish' => 1,
                              'realname'      => $author,
                              'username'      => $author,
                              'userlevel'     => 0,
                              'password'      => md5($defaultpass)); // MD5 compatible
                serendipity_db_insert('authors', $data);
                $authorid = serendipity_db_insert_id('authors', 'authorid');
            } else {
                $authorid = $result['authorid'];
            }
        

            $entry = array('title'          => $title,
                           'isdraft'        => 'false',
                           'allow_comments' => 'true',
                           'timestamp'      => $date,
                           'body'           => $body,
                           'extended'       => '',
                           'author'            => $author,
                           'authorid'       => $authorid
                           );
            
            echo "Entry $id inserted.<br />";
            if (!is_int($id = serendipity_updertEntry($entry))) {
                echo "Inserting entry failed.<br />";
                return $id;
            }
            
            # iterate through all comments
            $c = 0;
            foreach($commentlist[1] as $comment) {
                $c++;

                # locate the author and author url
                $curl    = '';
                $cauthor = '';
                $cdate   = time();
                $cbody   = '';

                if (preg_match("/AUTHOR:(.*)/", $comment, $cauthor) && preg_match("/href=\"(.*)\"/", $cauthor[1], $curl)) {
                    $curl    = (isset($curl[1]) ? trim($curl[1]) : '');
                    $cauthor = trim(strip_tags($cauthor[1]));
                }
                
                # locate the date
                if (preg_match("/DATE:(.*)/", $comment, $cdate)) {
                    $cdate = strtotime($cdate[1]);
                }
                
                # locate the comment body
                if (preg_match("/BODY:(.*)/s", $comment, $cbody)) {
                    $cbody = trim($cbody[1]);
                }
                
                $icomment = array('entry_id ' => $id,
                                 'parent_id' => 0,
                                 'timestamp' => $cdate,
                                 'author'    => $cauthor,
                                 'email'     => '',
                                 'url'       => $curl,
                                 'ip'        => '',
                                 'status'    => 'approved',
                                 'body'      => $cbody,
                                 'subscribed'=> 'false',
                                 'type'      => 'NORMAL');

                serendipity_db_insert('comments', $icomment);
            }
        
            serendipity_db_query("UPDATE ". $serendipity['dbPrefix'] ."entries SET comments = ". $c ." WHERE id = ". $id);
            echo "Comment count set to: ". $c ."<br />";
        }
        
        echo "Import finished.<br />";

        return true;
    }
}

return 'Serendipity_Import_Blogger';

/* vim: set sts=4 ts=4 expandtab : */
?>