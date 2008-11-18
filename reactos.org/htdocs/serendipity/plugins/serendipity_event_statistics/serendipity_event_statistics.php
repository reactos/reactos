<?php # $Id: serendipity_event_statistics.php 659 2005-11-07 16:22:21Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_STATISTICS_NAME', 'Statistics');
@define('PLUGIN_EVENT_STATISTICS_DESC', 'Adds a link to interesting statistics in your entries panel, including a visitors counter');
@define('PLUGIN_EVENT_STATISTICS_OUT_STATISTICS', 'Statistics');
@define('PLUGIN_EVENT_STATISTICS_OUT_FIRST_ENTRY', 'First entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_LAST_ENTRY', 'Last entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_ENTRIES', 'Total entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_ENTRIES', 'entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_PUBLIC', ' ... public');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_DRAFTS', ' ... drafts');
@define('PLUGIN_EVENT_STATISTICS_OUT_PER_AUTHOR', 'Entries per user');
@define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES', 'Categories');
@define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES2', 'categories');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES', 'Distribution of entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES2', 'entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES', 'Uploaded images');
@define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES2', 'image(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES', 'Distribution of image types');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES2', 'file(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS', 'Received comments');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2', 'comment(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS3', 'Most frequently commented entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPCOMMENTS', 'Most frequently commenting people');
@define('PLUGIN_EVENT_STATISTICS_OUT_LINK', 'link');
@define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS', 'Subscribers');
@define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS2', 'subscriber(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS', 'Most frequently subscribed entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS2', 'subscriber(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS', 'Received trackbacks');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS2', 'trackback(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK', 'Most frequently trackbacked entires');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK2', 'trackback(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACKS3', 'Most frequently trackbacking people');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE', 'estimated comments per entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE', 'estimated trackbacks per entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY', 'estimated entries per day');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK', 'estimated entries per week');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH', 'estimated entries per month');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE2', 'comments/entries');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE2', 'trackbacks/entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY2', 'entries/day');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK2', 'entries/week');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH2', 'entries/month');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS', 'Total amount of characters');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS2', 'characters');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE', 'Characters per entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE2', 'chars/entry');
@define('PLUGIN_EVENT_STATISTICS_OUT_LONGEST_ARTICLES', 'The %s longest entries');
@define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS', 'Maximum items');
@define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS_DESC', 'How many items to display per statistical value? (Default: 20)');

//Language constants for the Extended Visitors feature
@define('PLUGIN_EVENT_STATISTICS_EXT_ADD', 'Extended Visitors Statistics');
@define('PLUGIN_EVENT_STATISTICS_EXT_ADD_DESC', 'Add Extended Visitors Statistics feature? (default: no)');
@define('PLUGIN_EVENT_STATISTICS_EXT_OPT1', 'No!');
@define('PLUGIN_EVENT_STATISTICS_EXT_OPT2', 'Yes, at the bottom of the page');
@define('PLUGIN_EVENT_STATISTICS_EXT_OPT3', 'Yes, at the top of the page');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISITORS', 'Nr of vistors');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISTODAY', 'Nr of vistors today');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISTOTAL', 'Total nr of vistors');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISSINCE', 'The extended visitor´s statistic feature has collected data since');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISLATEST', 'Latest Visitors');
@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS', 'Top Referrers');
@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS_NONE', 'No referrers has yet been registered.');
@define('PLUGIN_EVENT_STATISTICS_OUT_EXT_STATISTICS', 'Extended Visitor Statistics');
@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS', 'Ban browsers from beeing counted');
@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS_DESC', 'Insert browsers that should be excluded from counting, separated by "|"');

        
class serendipity_event_statistics extends serendipity_event
{
    var $title = PLUGIN_EVENT_STATISTICS_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_STATISTICS_NAME);
        $propbag->add('description',   PLUGIN_EVENT_STATISTICS_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking, Fredrik Sandberg');
        $propbag->add('version',       '1.23');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('groups', array('STATISTICS'));
        $propbag->add('event_hooks',    array(
            'backend_sidebar_entries' => true,
            'backend_sidebar_entries_event_display_statistics' => true,
            'frontend_configure' => true
        ));

        $propbag->add('configuration', array('max_items', 'ext_vis_stat','banned_bots'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'max_items':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_STATISTICS_MAX_ITEMS);
                $propbag->add('description', PLUGIN_EVENT_STATISTICS_MAX_ITEMS_DESC);
                $propbag->add('default', 20);
                break;
                
             
            case 'ext_vis_stat':
                $select = array('no' => PLUGIN_EVENT_STATISTICS_EXT_OPT1, 'yesBot' => PLUGIN_EVENT_STATISTICS_EXT_OPT2, 'yesTop' => PLUGIN_EVENT_STATISTICS_EXT_OPT3);
                $propbag->add('type',        'select');
                $propbag->add('name',        PLUGIN_EVENT_STATISTICS_EXT_ADD);
                $propbag->add('description', PLUGIN_EVENT_STATISTICS_EXT_ADD_DESC);
                $propbag->add('select_values', $select);
                $propbag->add('default', 'no');

                break;
                
           case 'banned_bots':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_STATISTICS_BANNED_HOSTS);
                $propbag->add('description', PLUGIN_EVENT_STATISTICS_BANNED_HOSTS_DESC);
                $propbag->add('default', 'msnbot.msn.com');
                break;
        }

        return true;
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
            
                case 'frontend_configure':
                    if ($this->get_config('ext_vis_stat') == 'no') {
                        return;
                    }

                    //checking if db tables exists, otherwise install them
                    $tableChecker = serendipity_db_query("SELECT counter_id FROM {$serendipity['dbPrefix']}visitors LIMIT 1", true);
                    if (!is_array($tableChecker)){
                        $this->createTables();
                    }
                    
                    //Unique visitors are beeing registered and counted here. Calling function below.
                    $sessionChecker = serendipity_db_query("SELECT count(sessID) FROM {$serendipity['dbPrefix']}visitors WHERE '".serendipity_db_escape_string(session_id())."' = sessID GROUP BY sessID", true);
                    if (!is_array($sessionChecker) || (is_array($sessionChecker)) && ($sessionChecker[0] == 0)) {
                        
                        // avoiding banned browsers
                        $banned_bots = $this->get_config('banned_bots');
                        $tmp_array = explode('|', $banned_bots);
                        $found = 'no';
                        for ($i=0; $i<count($tmp_array); $i++) {
                            if (trim($tmp_array[$i]) == trim($_SERVER['HTTP_USER_AGENT'])){
                                $found = 'yes';
                            }
                        } 
                        if ($found == 'no'){ 
                            $this->countVisitor();
                        }
                    }
                    
                break;
                case 'backend_sidebar_entries':
?>
                        <li><a href="?serendipity[adminModule]=event_display&amp;serendipity[adminAction]=statistics"><?php echo PLUGIN_EVENT_STATISTICS_NAME; ?></a></li>
<?php

                    break;

                case 'backend_sidebar_entries_event_display_statistics':
                    $max_items = $this->get_config('max_items');
                    $ext_vis_stat = $this->get_config('ext_vis_stat');

                    if (!$max_items || !is_numeric($max_items) || $max_items < 1) {
                        $max_items = 20;
                    }
                    
                    if ($ext_vis_stat == 'yesTop') {
                        $this->extendedVisitorStatistics($max_items);
                    }

                    
                    $first_entry    = serendipity_db_query("SELECT timestamp FROM {$serendipity['dbPrefix']}entries ORDER BY timestamp ASC limit 1", true);
                    $last_entry     = serendipity_db_query("SELECT timestamp FROM {$serendipity['dbPrefix']}entries ORDER BY timestamp DESC limit 1", true);
                    $total_count    = serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}entries", true);
                    $draft_count    = serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}entries WHERE isdraft = 'true'", true);
                    $publish_count  = serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}entries WHERE isdraft = 'false'", true);
                    $author_rows    = serendipity_db_query("SELECT author, count(author) as entries FROM {$serendipity['dbPrefix']}entries GROUP BY author ORDER BY author");
                    $category_count = serendipity_db_query("SELECT count(categoryid) FROM {$serendipity['dbPrefix']}category", true);
                    $cat_sql        = "SELECT c.category_name, count(e.id) as postings
                                                    FROM {$serendipity['dbPrefix']}entrycat ec,
                                                         {$serendipity['dbPrefix']}category c,
                                                         {$serendipity['dbPrefix']}entries e
                                                    WHERE ec.categoryid = c.categoryid AND ec.entryid = e.id
                                                    GROUP BY ec.categoryid, c.category_name
                                                    ORDER BY postings DESC";
                    $category_rows  = serendipity_db_query($cat_sql);

                    $image_count = serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}images", true);
                    $image_rows = serendipity_db_query("SELECT extension, count(id) AS images FROM {$serendipity['dbPrefix']}images GROUP BY extension ORDER BY images DESC");

                    $subscriber_count = count(serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}comments WHERE type = 'NORMAL' AND subscribed = 'true' GROUP BY email"));
                    $subscriber_rows = serendipity_db_query("SELECT e.timestamp, e.id, e.title, count(c.id) as postings
                                                    FROM {$serendipity['dbPrefix']}comments c,
                                                         {$serendipity['dbPrefix']}entries e
                                                    WHERE e.id = c.entry_id AND type = 'NORMAL' AND subscribed = 'true'
                                                    GROUP BY e.id, c.email, e.title, e.timestamp
                                                    ORDER BY postings DESC
                                                    LIMIT $max_items");

                    $comment_count = serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}comments WHERE type = 'NORMAL'", true);
                    $comment_rows = serendipity_db_query("SELECT e.timestamp, e.id, e.title, count(c.id) as postings
                                                    FROM {$serendipity['dbPrefix']}comments c,
                                                         {$serendipity['dbPrefix']}entries e
                                                    WHERE e.id = c.entry_id AND type = 'NORMAL'
                                                    GROUP BY e.id, e.title, e.timestamp
                                                    ORDER BY postings DESC
                                                    LIMIT $max_items");

                    $commentor_rows = serendipity_db_query("SELECT author, max(email) as email, max(url) as url, count(id) as postings
                                                    FROM {$serendipity['dbPrefix']}comments c
                                                    WHERE type = 'NORMAL'
                                                    GROUP BY author
                                                    ORDER BY postings DESC
                                                    LIMIT $max_items");

                    $tb_count = serendipity_db_query("SELECT count(id) FROM {$serendipity['dbPrefix']}comments WHERE type = 'TRACKBACK'", true);
                    $tb_rows = serendipity_db_query("SELECT e.timestamp, e.id, e.title, count(c.id) as postings
                                                    FROM {$serendipity['dbPrefix']}comments c,
                                                         {$serendipity['dbPrefix']}entries e
                                                    WHERE e.id = c.entry_id AND type = 'TRACKBACK'
                                                    GROUP BY e.timestamp, e.id, e.title
                                                    ORDER BY postings DESC
                                                    LIMIT $max_items");

                    $tbr_rows = serendipity_db_query("SELECT author, max(email) as email, max(url) as url, count(id) as postings
                                                    FROM {$serendipity['dbPrefix']}comments c
                                                    WHERE type = 'TRACKBACK'
                                                    GROUP BY author
                                                    ORDER BY postings DESC
                                                    LIMIT $max_items");

                    $length = serendipity_db_query("SELECT SUM(LENGTH(body) + LENGTH(extended)) FROM {$serendipity['dbPrefix']}entries", true);
                    $length_rows = serendipity_db_query("SELECT id, title, (LENGTH(body) + LENGTH(extended)) as full_length FROM {$serendipity['dbPrefix']}entries ORDER BY full_length DESC LIMIT $max_items");

?>
    <h3><?php echo PLUGIN_EVENT_STATISTICS_OUT_STATISTICS; ?></h3>

    <div style="margin: 5px; padding: 5px">
    <dl>
        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_FIRST_ENTRY; ?></strong></dt>
        <dd><?php echo serendipity_formatTime(DATE_FORMAT_ENTRY . ' %H:%m', $first_entry[0]); ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_LAST_ENTRY; ?></strong></dt>
        <dd><?php echo serendipity_formatTime(DATE_FORMAT_ENTRY . ' %H:%m', $last_entry[0]); ?></dd>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOTAL_ENTRIES; ?></strong></dt>
        <dd><?php echo $total_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ENTRIES; ?></dd>
        <br />
        <dl>
            <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOTAL_PUBLIC; ?></strong></dt>
            <dd><?php echo $publish_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ENTRIES; ?></dd>
            <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOTAL_DRAFTS; ?></strong></dt>
            <dd><?php echo $draft_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ENTRIES; ?></dd>
        </dl>
        <br />
        <hr />
        <br />
        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_PER_AUTHOR; ?></strong></dt>
        <br />
        <dl>
<?php
                    if (is_array($author_rows)) {
                        foreach($author_rows AS $author => $author_stat) {
?>
            <dt><strong><?php echo $author_stat['author']; ?></strong></dt>
            <dd><?php echo $author_stat['entries']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ENTRIES; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES; ?></strong></dt>
        <dd><?php echo $category_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES; ?></strong></dt>
        <dl>
<?php
                    if (is_array($category_rows)) {
                        foreach($category_rows AS $category => $cat_stat) {
?>
            <dt><strong><?php echo $cat_stat['category_name']; ?></strong></dt>
            <dd><?php echo $cat_stat['postings']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES; ?></strong></dt>
        <dd><?php echo $image_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES; ?></strong></dt>
        <dl>
<?php
                    if (is_array($image_rows)) {
                        foreach($image_rows AS $image => $image_stat) {
?>
            <dt><strong><?php echo $image_stat['extension']; ?></strong></dt>
            <dd><?php echo $image_stat['images']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS; ?></strong></dt>
        <dd><?php echo $comment_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS3; ?></strong></dt>
        <dl>
<?php
                    if (is_array($comment_rows)) {
                        foreach($comment_rows AS $comment => $com_stat) {
?>
            <dt><strong><a href="<?php echo serendipity_archiveURL($com_stat['id'], $com_stat['title'], 'serendipityHTTPPath', true, array('timestamp' => $com_stat['timestamp'])); ?>"><?php echo $com_stat['title']; ?></a></strong></dt>
            <dd><?php echo $com_stat['postings']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPCOMMENTS; ?></strong></dt>
        <dl>
<?php
                    if (is_array($commentor_rows)) {
                        foreach($commentor_rows AS $comment => $com_stat) {
                            $link_start = '';
                            $link_end   = '';
                            $link_url   = '';

                            if (!empty($com_stat['email'])) {
                                $link_start = '<a href="mailto:' . htmlspecialchars($com_stat['email']) . '">';
                                $link_end   = '</a>';
                            }

                            if (!empty($com_stat['url'])) {
                                $link_url = ' (<a href="' . htmlspecialchars($com_stat['url']) . '">' . PLUGIN_EVENT_STATISTICS_OUT_LINK . '</a>)';
                            }

                            if (empty($com_stat['author'])) {
                                $com_stat['author'] = ANONYMOUS;
                            }
?>
            <dt><strong><?php echo $link_start . $com_stat['author'] . $link_end . $link_url; ?> </strong></dt>
            <dd><?php echo $com_stat['postings']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS; ?></strong></dt>
        <dd><?php echo $subscriber_count; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS; ?></strong></dt>
        <dl>
<?php
                    if (is_array($subscriber_rows)) {
                        foreach($subscriber_rows AS $subscriber => $subscriber_stat) {
?>
            <dt><strong><a href="<?php echo serendipity_archiveURL($subscriber_stat['id'], $subscriber_stat['title'], 'serendipityHTTPPath', true, array('timestamp' => $subscriber_stat['timestamp'])); ?>"><?php echo $subscriber_stat['title']; ?></a></strong></dt>
            <dd><?php echo $subscriber_stat['postings']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS; ?></strong></dt>
        <dd><?php echo $tb_count[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK; ?></strong></dt>
        <dl>
<?php
                    if (is_array($tb_rows)) {
                        foreach($tb_rows AS $tb => $tb_stat) {
?>
            <dt><strong><a href="<?php echo serendipity_archiveURL($tb_stat['id'], $tb_stat['title'], 'serendipityHTTPPath', true, array('timestamp' => $tb_stat['timestamp'])); ?>"><?php echo $tb_stat['title']; ?></a></strong></dt>
            <dd><?php echo $tb_stat['postings']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACKS3; ?></strong></dt>
        <dl>
<?php
                    if (is_array($tbr_rows)) {
                        foreach($tbr_rows AS $tb => $tb_stat) {
                            if (empty($tb_stat['author'])) {
                                $tb_stat['author'] = ANONYMOUS;
                            }
?>
            <dt><strong><a href="<?php echo htmlspecialchars($tb_stat['url']); ?>"><?php echo htmlspecialchars($tb_stat['author']); ?></a></strong></dt>
            <dd><?php echo $tb_stat['postings']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE; ?></strong></dt>
        <dd><?php echo round($comment_count[0] / max($publish_count[0], 1), 2); ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE; ?></strong></dt>
        <dd><?php echo round($tb_count[0] / max($publish_count[0], 1), 2); ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY; ?></strong></dt>
        <dd><?php echo round($publish_count[0] / ((time() - $first_entry[0]) / (60*60*24)), 2);?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK; ?></strong></dt>
        <dd><?php echo round($publish_count[0] / ((time() - $first_entry[0]) / (60*60*24*7)), 2);?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH; ?></strong></dt>
        <dd><?php echo round($publish_count[0] / ((time() - $first_entry[0]) / (60*60*24*31)), 2);?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH2; ?></dd>
        <br />
        <hr />
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_CHARS; ?></strong></dt>
        <dd><?php echo $length[0]; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_CHARS2; ?></dd>
        <br />

        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE; ?></strong></dt>
        <dd><?php echo round($length[0] / max($publish_count[0], 1), 2); ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE2; ?></dd>
        <br />

        <dt><strong><?php printf(PLUGIN_EVENT_STATISTICS_OUT_LONGEST_ARTICLES, $max_items); ?></strong></dt>
        <br />
        <dl>
<?php
                    if (is_array($length_rows)) {
                        foreach($length_rows AS $tb => $length_stat) {
?>
            <dt><strong><a href="<?php echo serendipity_archiveURL($length_stat['id'], $length_stat['title'], 'serendipityHTTPPath', true, array('timestamp' => $length_stat['timestamp'])); ?>"><?php echo $length_stat['title']; ?></a></strong></dt>
            <dd><?php echo $length_stat['full_length']; ?> <?php echo PLUGIN_EVENT_STATISTICS_OUT_CHARS2; ?></dd>
<?php
                        }
                    }
?>
        </dl>
        <br />

        <dt><strong><?php echo TOP_REFERRER; ?></strong></dt>
        <dd><?php echo serendipity_displayTopReferrers($max_items, true); ?></dd>
        <br />

        <dt><strong><?php echo TOP_EXITS; ?></strong></dt>
        <dd><?php echo serendipity_displayTopExits($max_items, true); ?></dd>
    </dl>
    <hr />
    <?php serendipity_plugin_api::hook_event('event_additional_statistics', $eventData, array('maxitems' => $max_items)); ?>
    </div>
    
    <?php

                    if ($ext_vis_stat == 'yesBot') {
                        $this->extendedVisitorStatistics($max_items);
                    }

                    return true;
                    break;

                default:
                    return false;
                    break;
            }
        } else {
            return false;
        }
    }
    
    function countVisitor(){
        
        global $serendipity;

        $referer = $_SERVER['HTTP_REFERER'];
        $values = array(
            'sessID' => strip_tags(session_id()),
            'day'    => date('Y-m-d'),
            'time'   => date('H:i'),
            'ref'    => strip_tags($referer),
            'browser'=> strip_tags($_SERVER['HTTP_USER_AGENT']),
            'ip'     => strip_tags($_SERVER['REMOTE_ADDR'])
        );
                
        serendipity_db_insert('visitors', $values);
                   
        // updating the referrer-table
        if (strlen($referer) >= 1) {

            //retrieving the referrer base URL
            $temp_array = explode('?', $referer); 
            $urlA = $temp_array[0]; 
    
            //removing "http://" & trailing subdirectories
            $temp_array3 = explode('//', $urlA);
            $urlB = $temp_array3[1];
            $temp_array4 = explode('/', $urlB);
            $urlB = $temp_array4[0];
    
            //removing www
            $urlC = serendipity_db_escape_string(str_replace('www.', '', $urlB));
            
            //updating db
            $q = serendipity_db_query("SELECT count(refs) FROM {$serendipity['dbPrefix']}refs WHERE refs = '$urlC' GROUP BY refs", true);
            if (!is_array($q) || $q[0] >= 1){
                serendipity_db_query("UPDATE {$serendipity['dbPrefix']}refs SET count=count+1 WHERE (refs = '$urlC')");
            } else {
                serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}refs (refs, count) VALUES ('$urlC', 1)");
            }
        }
            
    } //end of function countVisitor
    
    
    function extendedVisitorStatistics($max_items){
        
        global $serendipity;
        
        // ---------------QUERIES for Viewing statistics ----------------------------------------------
        $day = date('Y-m-d');
        $visitors_count_today = serendipity_db_query("SELECT count(counter_id) FROM {$serendipity['dbPrefix']}visitors WHERE day = '".$day."'", true);
        $visitors_count_firstday = serendipity_db_query("SELECT day FROM {$serendipity['dbPrefix']}visitors ORDER BY counter_id ASC LIMIT 1", true);
        $visitors_count = serendipity_db_query("SELECT count(counter_id) FROM {$serendipity['dbPrefix']}visitors", true);
        $visitors_latest = serendipity_db_query("SELECT counter_id, day, time, ref, browser, ip FROM {$serendipity['dbPrefix']}visitors ORDER BY counter_id DESC LIMIT ".$max_items."");
        $top_refs = serendipity_db_query("SELECT refs, count FROM {$serendipity['dbPrefix']}refs ORDER BY count DESC LIMIT ".$max_items."");
                            
        // ---------------STYLES for Viewing statistics ----------------------------------------------
        echo "<style type='text/css'>";
        echo ".colVis {text-align: center; width:600px; font-size: 10px; background-color:#dddddd;} ";
        echo ".col1 {text-align: center; width:150px; font-size: 10px; background-color:#dddddd;} ";
        echo ".col2 {text-align: center; width:150px; font-size: 10px;} ";
        echo ".col4 {text-align: center; width:600px; font-size: 10px; background-color:#dddddd;} ";
        echo ".col5 {text-align: center; width:600px; font-size: 10px;} ";
        echo "</style>";

        ?>
        <h3><?php echo PLUGIN_EVENT_STATISTICS_OUT_EXT_STATISTICS; ?></h3>
        <div style="margin: 5px; padding: 5px">
        <dl>
            <dt><?php echo PLUGIN_EVENT_STATISTICS_EXT_VISSINCE." ".$visitors_count_firstday[0].".<br /><br /><br />\n"?></dt>
        
            <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_EXT_VISITORS; ?></strong></dt>
                <dd>
                    <div class="colVis">
                    <?php 
                    echo PLUGIN_EVENT_STATISTICS_EXT_VISTODAY.": <strong>".$visitors_count_today[0]."</strong><br />\n";
                    echo PLUGIN_EVENT_STATISTICS_EXT_VISTOTAL.": <strong>".$visitors_count[0]."</strong><br />\n";
                    ?>
                    </div>
                </dd>
        </dl>
                    
        <dl>
        
        <dt><strong><?php echo PLUGIN_EVENT_STATISTICS_EXT_VISLATEST;?></strong></dt>
            <dd>
                <table>
                    <?php
                    $i=1;
                    $color = "col2";
                    if (is_array($visitors_latest)) {
                        foreach($visitors_latest AS $key => $row) {
                            if ($color == "col1"){$color ="col2";}else{$color ="col1";}
                                echo "<tr>";
                                echo "<td class = \"".$color."\">".$row['day']." ".$row['time']."</td>\n";
                                echo "<td class = \"".$color."\">".wordwrap($row['ref'], 25, "\n", 1)."</td>\n";
                                echo "<td class = \"".$color."\">".wordwrap($row['browser'], 25, "\n", 1)."</td>\n";
                                echo "<td class = \"".$color."\">";
                                if ($row['ip']){
                                    echo wordwrap(gethostbyaddr($row['ip']), 25, "\n", 1);
                                } else {
                                    echo "--";
                                }
                                echo "</td>\n";
                                echo "</tr>\n";
                            }
                        }
                        ?>
                </table><br />
            </dd>
        </dl>
                        
        <dl>
        <dt><strong>
        <?php 
        echo PLUGIN_EVENT_STATISTICS_EXT_TOPREFS;
        ?>
        </strong></dt>
            <dd>
                <table>
                    <?php
                        $i=1;
                        if (is_array($top_refs)) {
                            foreach($top_refs AS $key => $row) {
                                if ($color == "col4"){$color ="col5";}else{$color ="col4";}
                                    echo "<tr><td class=\"".$color."\">".$i++.".  ".$row['refs']." (<strong>".$row['count']."</strong>)</td></tr>";
                            }
                        } else {
                            echo PLUGIN_EVENT_STATISTICS_EXT_TOPREFS_NONE;
                        }
                        ?>
                </table>
            </dd>
        </dl>
        <hr />
    <?php
    } //end of function extendedVisitorStatistics()
    
    
    function createTables() {
        global $serendipity;
        
        //create table xxxx_visitors
        $q   = "CREATE TABLE {$serendipity['dbPrefix']}visitors (
            counter_id {AUTOINCREMENT} {PRIMARY},
            sessID varchar(35) not null default '',
            day varchar(10) not null default '',
            time varchar(5) not null default '',
            ref varchar(255) default null,
            browser varchar(255) default null,
            ip varchar(15) default null
        )";

       serendipity_db_schema_import($q);

        //create table xxxx_refs
        $q   = "CREATE TABLE {$serendipity['dbPrefix']}refs (
            id {AUTOINCREMENT} {PRIMARY},
            refs varchar(255) not null default '',
            count int(11) not null default '0'
        )";
        serendipity_db_schema_import($q);
        
    } //end of function createTables()


    function dropTables() {
        
        global $serendipity;
        
        // Drop tables
        $q   = "DROP TABLE ".$serendipity['dbPrefix']."visitors";
        $sql = serendipity_db_schema_import($q);
        $q   = "DROP TABLE ".$serendipity['dbPrefix']."refs";
        $sql = serendipity_db_schema_import($q);
        
    } //end of function dropTables
    
    function install(){
        
        $this->createTables();
        
    }
    
    function uninstall(){
        
        $this->dropTables();
        
    }
    
}

/* vim: set sts=4 ts=4 expandtab : */
