<?php # $Id: functions_entries.inc.php 435 2005-08-25 12:36:39Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_printEntries_rss($entries, $version, $comments = false, $fullFeed = false, $showMail = true) {
    global $serendipity;

    $options = array(
        'version'  => $version,
        'comments' => $comments,
        'fullFeed' => $fullFeed,
        'showMail' => $showMail
    );
    serendipity_plugin_api::hook_event('frontend_entries_rss', $entries, $options);

    if (is_array($entries)) {
        foreach ($entries as $entry) {
            $id   = (isset($entry['entryid']) && !empty($entry['entryid']) ? $entry['entryid'] : $entry['id']);
            $guid = serendipity_rss_getguid($entry, $options['comments']);
            $entryLink = serendipity_archiveURL($id, $entry['title'], 'baseURL', true, array('timestamp' => $entry['timestamp']));
            if ($options['comments'] == true) {
                // Display username as part of the title for easier feed-readability
                $entry['title'] = $entry['author'] . ': ' . $entry['title'];
            }

            // Embed a link to extended entry, if existing
            if ($options['fullFeed']) {
                $entry['body'] .= ' ' . $entry['extended'];
                $ext = '';
            } elseif ($entry['exflag']) {
                $ext = '<br /><a href="' . $guid . '#extended">' . sprintf(VIEW_EXTENDED_ENTRY, htmlspecialchars($entry['title'])) . '</a>';
            } else {
                $ext = '';
            }

            serendipity_plugin_api::hook_event('frontend_display', $entry);
            // Do some relative -> absolute URI replacing magic. Replaces all HREF/SRC (<a>, <img>, ...) references to only the serendipitypath with the full baseURL URI
            // garvin: Could impose some problems. Closely watch this one.
            $entry['body'] = preg_replace('@(href|src)=("|\')(' . preg_quote($serendipity['serendipityHTTPPath']) . ')(.*)("|\')(.*)>@imsU', '\1=\2' . $serendipity['baseURL'] . '\4\2\6>', $entry['body']);
            // jbalcorn: clean up body for XML compliance as best we can.
            $entry['body'] = xhtml_cleanup($entry['body']);

            // extract author information
            if ((isset($entry['no_email']) && $entry['no_email']) || $options['showMail'] === FALSE) {
                $entry['email'] = 'nospam@example.com'; // RSS Feeds need an E-Mail address!
            } elseif (empty($entry['email'])) {
                $query = "select email FROM {$serendipity['dbPrefix']}authors WHERE authorid = '". serendipity_db_escape_string($entry['authorid']) ."'";
                $results = serendipity_db_query($query);
                $entry['email'] = $results[0]['email'];
            }

            if (!is_array($entry['categories'])) {
                $entry['categories'] = array(0 => array('category_name' => $entry['category_name']));
            }

            if ($options['version'] == 'atom0.3') {
                /*********** ATOM 0.3 FEED *************/
?>
<entry>
    <link href="<?php echo $entryLink; ?>" rel="alternate" title="<?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?>" type="text/html" />
    <author>
        <name><?php echo serendipity_utf8_encode(htmlspecialchars($entry['author'])); ?></name>
        <email><?php echo serendipity_utf8_encode(htmlspecialchars($entry['email'])); ?></email>
    </author>

    <issued><?php echo gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entry['timestamp'])); ?></issued>
    <created><?php echo gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entry['timestamp'])); ?></created>
    <modified><?php echo gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entry['last_modified'])); ?></modified>
    <wfw:comment><?php echo $serendipity['baseURL']; ?>wfwcomment.php?cid=<?php echo $id; ?></wfw:comment>

<?php
                    if ($options['comments'] === false) {
?>
    <slash:comments><?php echo $entry['comments']; ?></slash:comments>
    <wfw:commentRss><?php echo $serendipity['baseURL']; ?>rss.php?version=<?php echo $options['version']; ?>&amp;type=comments&amp;cid=<?php echo $id; ?></wfw:commentRss>
<?php
                    }
?>

    <id><?php echo $guid; ?></id>
    <title mode="escaped" type="text/html"><?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?></title>
<?php               if (!empty($entry['body'])) { ?>
    <content type="application/xhtml+xml" xml:base="<?php echo $serendipity['baseURL']; ?>">
        <div xmlns="http://www.w3.org/1999/xhtml">
<?php
                    echo serendipity_utf8_encode($entry['body'].$ext);
?>
        </div>
    </content>
<?php
                    }
                    $entry['display_dat'] = '';
                    serendipity_plugin_api::hook_event('frontend_display:atom-0.3:per_entry', $entry);
                    echo $entry['display_dat'];
?>
</entry>
<?php
            } elseif ($options['version'] == 'atom1.0') {
                /*********** ATOM 1.0 FEED *************/
?>
<entry>
    <link href="<?php echo $entryLink; ?>" rel="alternate" title="<?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?>" />
    <author>
        <name><?php echo serendipity_utf8_encode(htmlspecialchars($entry['author'])); ?></name>
        <email><?php echo serendipity_utf8_encode(htmlspecialchars($entry['email'])); ?></email>
    </author>

    <published><?php echo gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entry['timestamp'])); ?></published>
    <updated><?php echo gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entry['last_modified'])); ?></updated>
    <wfw:comment><?php echo $serendipity['baseURL']; ?>wfwcomment.php?cid=<?php echo $id; ?></wfw:comment>

<?php
                    if ($options['comments'] === false) {
?>
    <slash:comments><?php echo $entry['comments']; ?></slash:comments>
    <wfw:commentRss><?php echo $serendipity['baseURL']; ?>rss.php?version=<?php echo $options['version']; ?>&amp;type=comments&amp;cid=<?php echo $id; ?></wfw:commentRss>
<?php
                    }

                    foreach ($entry['categories'] AS $idx => $cat) { 
                        $name = serendipity_utf8_encode(htmlspecialchars($cat['category_name'])); ?>
                        <category scheme="<?php echo serendipity_categoryURL($cat, 'baseURL'); ?>" label="<?php echo $name; ?>" term="<?php echo $name; ?>" />
<?php
                    }
?>
    <id><?php echo $guid; ?></id>
    <title type="html"><?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?></title>
<?php               if (!empty($entry['body'])) { ?>
    <content type="xhtml" xml:base="<?php echo $serendipity['baseURL']; ?>">
        <div xmlns="http://www.w3.org/1999/xhtml">
<?php
                    echo serendipity_utf8_encode($entry['body'].$ext);
?>
        </div>
    </content>
<?php
                    }
                    $entry['display_dat'] = '';
                    serendipity_plugin_api::hook_event('frontend_display:atom-1.0:per_entry', $entry);
                    echo $entry['display_dat'];
?>
</entry>
<?php

            } elseif ($options['version'] == '0.91' || $options['version'] == '2.0') {
                /*********** BEGIN RSS 0.91/2.0 FEED *************/
?>
<item>
    <title><?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?></title>
    <link><?php echo $entryLink; ?></link>
<?php
                /*********** END RSS 0.91/2.0 FEED *************/

                if ($options['version'] == '2.0') {
                    /*********** RSS 2.0 FEED EXTRAS *************/
                    foreach ($entry['categories'] AS $idx => $cat) {
                        ?><category><?php echo serendipity_utf8_encode(htmlspecialchars($cat['category_name'])); ?></category><?php
                    }
?>
    <comments><?php echo $entryLink; ?>#comments</comments>
    <wfw:comment><?php echo $serendipity['baseURL']; ?>wfwcomment.php?cid=<?php echo $id; ?></wfw:comment>
<?php
                    if ($options['comments'] === false) {
?>
    <slash:comments><?php echo $entry['comments']; ?></slash:comments>
    <wfw:commentRss><?php echo $serendipity['baseURL']; ?>rss.php?version=<?php echo $options['version']; ?>&amp;type=comments&amp;cid=<?php echo $id; ?></wfw:commentRss>
<?php
                    }
?>
    <author><?php echo serendipity_utf8_encode(htmlspecialchars($entry['email'])) . ' (' . serendipity_utf8_encode(htmlspecialchars($entry['author'])) . ')'; ?></author>
<?php               if (!empty($entry['body'])) { ?>
    <content:encoded>
<?php
                        echo serendipity_utf8_encode(htmlspecialchars($entry['body'].$ext));
?>
    </content:encoded>
<?php
                    }
?>                
    <pubDate><?php echo date('r', serendipity_serverOffsetHour($entry['timestamp'])); ?></pubDate>
    <guid isPermaLink="false"><?php echo $guid; ?></guid>
    <?php
      $entry['display_dat'] = '';
      serendipity_plugin_api::hook_event('frontend_display:rss-2.0:per_entry', $entry);
      echo $entry['display_dat'];
    ?>
</item>
<?php
                    /*********** END 2.0 FEED EXTRAS *************/
                } else {
                    /*********** BEGIN RSS 0.91 FEED EXTRAS *************/
                    if (!empty($entry['body'])) { ?>
?>
    <description>
        <?php echo serendipity_utf8_encode(htmlspecialchars($entry['body'] . $ext)); ?>
    </description>
<?php
                    }
?>
</item>
<?php
                    /*********** END RSS 0.91 FEED EXTRAS *************/
                }
            } else if ($options['version'] == '1.0') {
                $categories = array();
                foreach ($entry['categories'] AS $idx => $cat) {
                    $categories[] = $cat['category_name'];
                }

?>
<item rdf:about="<?php echo $guid; ?>">
    <title><?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?></title>
    <link><?php echo $entryLink; ?></link>
<?php           if (!empty($entry['body'])) { ?>
    <description>
<?php
                echo serendipity_utf8_encode(htmlspecialchars($entry['body'].$ext));
?>
    </description>
<?php
                }

                $entry['display_dat'] = '';
                serendipity_plugin_api::hook_event('frontend_display:rss-1.0:per_entry', $entry);
                echo $entry['display_dat'];
?>
    <dc:publisher><?php echo serendipity_utf8_encode(htmlspecialchars($serendipity['blogTitle'])); ?></dc:publisher>
    <dc:creator><?php echo serendipity_utf8_encode(htmlspecialchars($entry['email'])) . ' (' . serendipity_utf8_encode(htmlspecialchars($entry['author'])) . ')'; ?></dc:creator>
    <dc:subject><?php echo serendipity_utf8_encode(htmlspecialchars(implode(', ', $categories))); ?></dc:subject>
    <dc:date><?php echo date('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entry['timestamp'])); ?></dc:date>
    <wfw:comment><?php echo $serendipity['baseURL']; ?>wfwcomment.php?cid=<?php echo $id; ?></wfw:comment>
<?php
                    if ($options['comments'] === false) {
?>
    <slash:comments><?php echo $entry['comments']; ?></slash:comments>
    <wfw:commentRss><?php echo $serendipity['baseURL']; ?>rss.php?version=<?php echo $options['version']; ?>&amp;type=comments&amp;cid=<?php echo $id; ?></wfw:commentRss>
<?php
                    }
?>
</item>
<?php
            } elseif ($options['version'] == 'opml1.0') {
?>
    <outline text="<?php echo serendipity_utf8_encode(htmlspecialchars($entry['title'])); ?>" type="url" htmlUrl="<?php echo $entryLink; ?>" urlHTTP="<?php echo $entryLink; ?>" />
<?php
            }
        }
    }
}
