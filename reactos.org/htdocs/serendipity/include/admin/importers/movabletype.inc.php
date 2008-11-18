<?php # $Id: movabletype.inc.php 706 2005-11-17 12:22:01Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/*****************************************************************
 *                MovableType Importer, by Evan Nemerson         *
 *****************************************************************/

switch ($serendipity['lang']) {
    case 'de':
        @define('IMPORTER_MT_WARN_PLUGIN',     'Bitte installieren Sie das Plugin "%s"');
        @define('IMPORTER_MT_NOTE', 'Falls Sie weiter machen, ohne die Plugins zu installieren, werden möglicherweise Zeilenumbrüche falsch importiert (verdoppelt oder entfernt)');
        break;

    case 'en':
    default:
        @define('IMPORTER_MT_WARN_PLUGIN',     'Please install the plugin "%s"');
        @define('IMPORTER_MT_NOTE', 'If you continue without installing those plugins, line breaks may be incorrectly imported (doubled or removed)');
        break;
}

class Serendipity_Import_MovableType extends Serendipity_Import {
    var $info        = array('software' => 'MovableType');
    var $data        = array();
    var $inputFields = array();

    function Serendipity_Import_MovableType($data) {
        $this->data = $data;
        $this->inputFields = array(array('text'    => MT_DATA_FILE,
                                         'type'    => 'file',
                                         'name'    => 'mt_dat'),

                                   array('text'    => FORCE,
                                         'type'    => 'bool',
                                         'name'    => 'mt_force',
                                         'default' => 'true'),

                                   array('text'    => CHARSET,
                                         'type'    => 'list',
                                         'name'    => 'charset',
                                         'value'   => 'UTF-8',
                                         'default' => $this->getCharsets(true)),

                                   array('text'    => ACTIVATE_AUTODISCOVERY,
                                         'type'    => 'bool',
                                         'name'    => 'autodiscovery',
                                         'default' => 'false'),

                                   array('text'    => 'Debugging',
                                         'type'    => 'bool',
                                         'name'    => 'debug',
                                         'default' => 'false')
                            );
    }

    function debug($string) {
        static $debug = null;
        static $c = 0;
        
        if ($debug === null) {
            if ($this->data['debug'] == 'true') {
                $debug = true;
            } else {
                $debug = false;
            }
        }
        
        if ($debug) {
            $c++;
            echo '#' . $c . ' [' . date('d.m.Y H:i.s') . '] ' . $string . "<br />\n";
        }
    }
    
    function getImportNotes(){
        $notes = array();
        if (!class_exists('serendipity_event_nl2br')){
            $notes[] = sprintf(IMPORTER_MT_WARN_PLUGIN, 'serendipity_event_nl2br');
        }
        if (!class_exists('serendipity_event_entryproperties')){
            $notes[] = sprintf(IMPORTER_MT_WARN_PLUGIN, 'serendipity_event_entryproperties');
        }
        if (count($notes) > 0){
            return '<ul><li>'.implode('</li><li>', $notes).'</li></ul>'.IMPORTER_MT_NOTE;
        }
    }
    function validateData() {
        return sizeof($this->data);
    }

    function getInputFields() {
        return $this->inputFields;
    }

    function doEntryWork(&$mt_entry, &$tasks){
        global $serendipity;

        $entry = array();
        $entry['categories'] = array();
        $entryprops = array();
        
        $this->debug("doEntryWork: " . print_r($mt_entry, true));

        foreach($mt_entry as $name => $data) {
            $name = trim($name);
            if (is_string($data)) {
                $data = trim($data);
            }
            $this->debug($name . ': "' . print_r($data, true) . '"');
            switch($name) {
                case 's9y_comments':
                    $entry['s9y_comments'] = $data;
                    break;
                case 'AUTHOR':
                    if ( !isset($authors[$data]) ) {
                        $au_inf = serendipity_fetchAuthor($data);
                        if ( !is_array($au_inf) ) {
                            $tasks[] = sprintf(CREATE_AUTHOR, htmlspecialchars($data));
                            $tasks[] = 'Input array is: ' . print_r($data, true) . '<br />Return is: ' . print_r($au_inf, true) . '<br />';
                            $au_inf = serendipity_fetchAuthor($serendipity['authorid']);
                        }
                        $authors[$data] = $au_inf[0];
                    }
                    $entry['authorid'] = $authors[$data]['authorid'];
                    $entry['author'] = $authors[$data]['username'];
                    break;
                case 'TITLE':
                    $entry['title'] = $data;
                    break;
                case 'STATUS':
                    $entry['isdraft'] = ($data == 'Publish') ? 'false' : 'true';
                    break;
                case 'ALLOW COMMENTS':
                    $entry['allow_comments'] = ($data == '1') ? 'true' : 'false';
                    break;
                case 'DATE':
                    $entry['timestamp'] = strtotime($data);
                    if ($entry['timestamp'] == 0) {
                        $entry['timestamp'] = time();
                    }
                    break;

                case 's9y_body':
                case 'BODY':
                    $entry['body'] = $data;
                    break;

                case 's9y_extended':
                case 'EXTENDED BODY':
                    $entry['extended'] = $data;
                    break;

                case 'CONVERT BREAKS':
                    $entryprops['nl2br'] = ($data == '1') ? true : false;
                    break;

                case 'PRIMARY CATEGORY':
                case 'CATEGORY':
                      $cat_found = false;
                      if ( is_array($this->categories) ) {
                          for ( $y=0 ; $y<sizeof($this->categories) ; $y++ ) {
                              if ( $this->categories[$y]['category_name'] == $data ) {
                                  $cat_found = true;
                                  break;
                              }
                          }
                          if ( $cat_found){
                              if (!in_array($this->categories[$y]['categoryid'], $entry['categories']) ) {
                                  //$entries[$n]['categories'][] = $categories[$y]['categoryid'];
                                  $entry['categories'][] = $this->categories[$y]['categoryid'];
                              }
                          }else {
                              $tasks[] = sprintf(CREATE_CATEGORY, htmlspecialchars($data));
                          }
                      }
                      break;
            }
        }
        $entry['props'] = $entryprops;
        return $entry;
    }

    function doCommentWork(&$mt_entry, &$tasks, $type = 'NORMAL'){
        $comment = array(
            'parent_id'  => 0,
            'status'     => 'approved',
            'subscribed' => 'false',
            'type'       => $type,
            'body'       => ''
        );

        $this->debug("MT_ENTRY: " . print_r($mt_entry, true));
        $parsed_entry   = array();
        $unparsed_entry = explode("\n", $mt_entry[$type == 'NORMAL' ? 'COMMENT' : 'PING']);
        foreach($unparsed_entry AS $line) {
            if (preg_match('/^([A-Z\s]+):\s+(.*)$/', $line, $match)) {
                $parsed_entry[$match[1]] = $match[2];
            } else {
                $parsed_entry['s9y_body'] .= $line . "\n";
            }
        }
        
        foreach($parsed_entry as $name => $data){
            $data = trim($data);
            $name = trim($name);

            switch($name) {
                case 'EMAIL':
                    $comment['email'] = $data;
                    break;

                case 'URL':
                    $comment['url'] = $data;
                    break;

                case 'IP':
                    $comment['ip'] = $data;
                    break;

                case 'AUTHOR':
                case 'BLOG NAME':
                    $comment['author'] = $data;
                    break;

                case 'DATE':
                    $comment['timestamp'] = strtotime($data);
                    if ($comment['timestamp'] == 0) {
                        $comment['timestamp'] = time();
                    }
                    break;
                    
                case 'TITLE':
                    break;

                default:
                    $comment['body'] .= $data;
            }
        }

        $this->debug("S9Y_ENTRY: " . print_r($comment, true));
        return $comment;
    }

    function import() {
        global $serendipity;

        $force    = ($this->data['mt_force'] == 'true');

        if ($this->data['autodiscovery'] == 'false') {
            $serendipity['noautodiscovery'] = 1;
        }

        // Rewritten to parse the file line by line. Can save quite some
        // memory on large blogs
        //$contents   = file_get_contents($_FILES['serendipity']['tmp_name']['import']['mt_dat']);

        $authors    = array();
        $this->categories = serendipity_fetchCategories();
        $tasks      = array();

        $entries = array();

        if (empty($_FILES['serendipity']['tmp_name']['import']['mt_dat'])) {
            $fh = fopen('/tmp/mt.dat', 'r');
        } else {
            $fh = fopen($_FILES['serendipity']['tmp_name']['import']['mt_dat'], 'r');
        }

        $n = 0;

        $entry        = array();
        $el           = "";
        $c_el         = "";
        $skip         = false;
        $is_comment   = false;
        $is_trackback = false;
        $nofetch      = false;
        while (!feof($fh)) {
            if ($nofetch === false) {
                $line = $this->decode(fgets($fh, 8192));
            } else {
                // Keep line from previous run.
                $nofetch = false;
            }

            if ($is_comment || $is_trackback) {
                $this->debug("COMMENT/TRACKBACK mode is active.");
                if (preg_match('/^--------/', $line)) {
                    $this->debug("Next full section requested.");
                    $is_comment = $is_trackback = false;
                } elseif (preg_match('/^-----/', $line)) {
                    $this->debug("Next partial section requested.");
                    if ($is_trackback) {
                        $this->debug("Parsing trackback.");
                        $entry['s9y_comments'][] = $this->doCommentWork($comment, $tasks, 'TRACKBACK');
                    } elseif ($is_comment) {
                        $this->debug("Parsing comment.");
                        $entry['s9y_comments'][] = $this->doCommentWork($comment, $tasks, 'NORMAL');
                    }
                    $el = $c_el = "";
                }
            }
            
            if ($skip && (!preg_match('/^--------/', $line))) {
                $this->debug("No next section match, and skip is activated. Skipping '$line'");
                continue;
            }

            if (preg_match('/^--------/', $line)) {
                // We found the end marker of the current entry. Add to entries-Array
                $this->debug("End marker found. Parsing full entry.");
                $entries[]    = $this->doEntryWork($entry, $tasks);
                $entry        = array();
                $el           = "";
                $c_el         = "";
                $skip         = false;
                $is_comment   = false;
                $is_trackback = false;
            } elseif (preg_match('/^-----/', $line)) {
                $this->debug("New section match. Current EL: $el");
                if (empty($el)) {
                    $line = $this->decode(fgets($fh, 8192));
                    $this->debug("Inspecting next line: $line");
                    $tline = trim($line);
                    while (($is_comment || $is_trackback) && empty($tline)) {
                        $line = $this->decode(fgets($fh, 8192));
                        $tline = trim($line);
                        $this->debug("Continuing inspecting next line: $line");
                    }
                    if (preg_match('/^([A-Z\s]+):/', $line, $matches)) {
                        $this->debug("Match result: $matches[1]");
                        if ($matches[1] == 'COMMENT') {
                            $this->debug("Marking COMMENT.");
                            $is_comment   = true;
                            $is_trackback = false;
                            $comment      = array();
                            $skip         = false;
                        } elseif ($matches[1] == 'PING') {
                            $this->debug("Marking TRACKBACK");
                            $is_comment   = false;
                            $is_trackback = true;
                            $comment      = array();
                            $skip         = false;
                        }

                        $this->debug("Setting EL to {$matches[1]}");
                        $el   = $matches[1];
                        $c_el = "";
                    } else {
                        $this->debug("Could not parse next line. Keeping it for next cycle.");
                        $nofetch = true;
                    }
                } else {
                    $this->debug("Resetting EL to an empty string");
                    $el = $c_el = "";
                }
            } elseif (empty($el)) {
                $this->debug("EL is empty. Line is '$line'");
                $content = "";
                if (preg_match('/^([A-Z\s]+):\s+(.*)$/s', $line, $matches)) {
                    $this->debug("Section match {$matches[1]} found, input: {$matches[2]}");
                    $c_el    = $matches[1];
                    $content = $matches[2];
                } elseif (!empty($c_el)) {
                    $this->debug("Still in subsection of previous run: $c_el.");
                    $content = trim($line);
                }
                
                if (!empty($content)) {
                    if ($is_comment || $is_trackback) {
                        $this->debug("Appending to comments: $line");
                        $comment[$c_el] = $content;
                    } else {
                        $this->debug("Appending to entry: $line");
                        $entry[$c_el]   = $content;
                    }
                }
            } elseif ($is_comment || $is_trackback) {
                $this->debug("Appending Line in current Element $el to comments: $line");
                $comment[$el] .= $line;
            } else {
                $this->debug("Appending Line in current Element $el to entries: $line");
                $entry[$el] .= $line;
            }
        }
        fclose($fh);

        if ( !sizeof($tasks) || $force == true ) {
            serendipity_db_begin_transaction();
            foreach ($entries as $entry) {
                if (empty($entry['authorid'])) {
                    $entry['authorid'] = $serendipity['authorid'];
                    $entry['author']   = $serendipity['realname'];
                }
                $comments   = $entry['s9y_comments'];
                $entryprops = $entry['props'];
                unset($entry['props']);
                unset($entry['s9y_comments']);
                
                if ( !is_int($r = serendipity_updertEntry($entry)) ) {
                    echo '<div class="serendipityAdminMsgError">' . $r . '</div>';
                } else {
                    $entry['id'] = $r;
                    foreach((array)$comments AS $comment) {
                        $comment['entry_id'] = $r;
                        if ($rc = serendipity_db_insert('comments', $comment)) {
                            $cid = serendipity_db_insert_id('comments', 'id');
                            serendipity_approveComment($cid, $entry['id'], true);
                        } else {
                            echo '<div class="serendipityAdminMsgError">' . $rc . '</div>';
                        }
                    }
                    // Let the plugins do some additional stuff. Here it's used with
                    // event_entryproperties in mind to setup the nl2br-stuff
                    serendipity_plugin_api::hook_event('backend_import_entry', $entry, $entryprops);
                }
            }
            serendipity_db_end_transaction(true);
            return true;
        } else {
            return '<ul><li>'.implode('</li><li>', array_unique($tasks)).'</li></ul>';
        }
    }
}
return 'Serendipity_Import_MovableType';
?>