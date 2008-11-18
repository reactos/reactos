<?php # $Id: serendipity_plugin_comments.php 527 2005-10-04 12:50:29Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_COMMENTS_BLAHBLAH', 'Displays the last comments to your entries');
@define('PLUGIN_COMMENTS_WORDWRAP', 'Wordwrap');
@define('PLUGIN_COMMENTS_WORDWRAP_BLAHBLAH', 'How many words until a wordwrap will occur? (Default: 30)');
@define('PLUGIN_COMMENTS_MAXCHARS', 'Maximum chars per comment');
@define('PLUGIN_COMMENTS_MAXCHARS_BLAHBLAH', 'How many chars will be displayed for each comment? (Default: 120)');
@define('PLUGIN_COMMENTS_MAXENTRIES', 'Maximum number of comments');
@define('PLUGIN_COMMENTS_MAXENTRIES_BLAHBLAH', 'How many comments will be shown? (Default: 15)');
@define('PLUGIN_COMMENTS_ABOUT', '%s about%s');

class serendipity_plugin_comments extends serendipity_plugin
{
    var $title = COMMENTS;

    function introspect(&$propbag)
    {
        global $serendipity;

        $this->title = $this->get_config('title', $this->title);

        $propbag->add('name',          COMMENTS);
        $propbag->add('description',   PLUGIN_COMMENTS_BLAHBLAH);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '1.3');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('groups', array('FRONTEND_VIEWS'));
        $propbag->add('configuration', array(
                                             'title',
                                             'wordwrap',
                                             'max_chars',
                                             'max_entries',
                                             'dateformat',
                                             'viewmode'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'viewmode':
                $types = array(
                    'comments'   => COMMENTS,
                    'trackbacks' => TRACKBACKS,
                    'all'        => COMMENTS . ' + ' . TRACKBACKS
                );
                $propbag->add('type',        'select');
                $propbag->add('name',        TYPE);
                $propbag->add('description', '');
                $propbag->add('select_values', $types);
                $propbag->add('default',     'all');

                $propbag->add('type',        'string');
                $propbag->add('name',        TITLE);
                $propbag->add('description', '');
                $propbag->add('default',     COMMENTS);
                break;

            case 'title':
                $propbag->add('type',        'string');
                $propbag->add('name',        TITLE);
                $propbag->add('description', '');
                $propbag->add('default',     COMMENTS);
                break;

            case 'wordwrap':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_COMMENTS_WORDWRAP);
                $propbag->add('description', PLUGIN_COMMENTS_WORDWRAP_BLAHBLAH);
                $propbag->add('default', 30);
                break;

            case 'max_chars':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_COMMENTS_MAXCHARS);
                $propbag->add('description', PLUGIN_COMMENTS_MAXCHARS_BLAHBLAH);
                $propbag->add('default', 120);
                break;

            case 'max_entries':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_COMMENTS_MAXENTRIES);
                $propbag->add('description', PLUGIN_COMMENTS_MAXENTRIES_BLAHBLAH);
                $propbag->add('default', 15);
                break;

            case 'dateformat':
                $propbag->add('type', 'string');
                $propbag->add('name', GENERAL_PLUGIN_DATEFORMAT);
                $propbag->add('description', sprintf(GENERAL_PLUGIN_DATEFORMAT_BLAHBLAH, '%a, %d.%m.%Y %H:%M'));
                $propbag->add('default', '%a, %d.%m.%Y %H:%M');
                break;

            default:
                    return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;
        $title       = $this->get_config('title', $this->title);
        $max_entries = $this->get_config('max_entries');
        $max_chars   = $this->get_config('max_chars');
        $wordwrap    = $this->get_config('wordwrap');
        $dateformat  = $this->get_config('dateformat');

        if (!$max_entries || !is_numeric($max_entries) || $max_entries < 1) {
            $max_entries = 15;
        }

        if (!$max_chars || !is_numeric($max_chars) || $max_chars < 1) {
            $max_chars = 120;
        }

        if (!$wordwrap || !is_numeric($wordwrap) || $wordwrap < 1) {
            $wordwrap = 30;
        }

        if (!$dateformat || strlen($dateformat) < 1) {
            $dateformat = '%a, %d.%m.%Y %H:%M';
        }

        $viewtype = '';
        if ($this->get_config('viewtype') == 'comments') {
            $viewtype .= ' AND c.type = \'NORMAL\'';
        } elseif ($this->get_config('viewtype') == 'trackbacks') {
            $viewtype .= ' AND c.type = \'TRACKBACK\'';
        }

        $q = 'SELECT    c.body              AS comment,
                        c.timestamp         AS stamp,
                        c.author            AS user,
                        e.title             AS subject,
                        e.timestamp         AS entrystamp,
                        e.id                AS entry_id,
                        c.id                AS comment_id,
                        c.type              AS comment_type,
                        c.url               AS comment_url,
                        c.title             AS comment_title,
                        c.email             AS comment_email
                FROM    '.$serendipity['dbPrefix'].'comments AS c,
                        '.$serendipity['dbPrefix'].'entries  AS e
               WHERE    e.id = c.entry_id
                 AND    NOT (c.type = \'TRACKBACK\' AND c.author = \'' . serendipity_db_escape_string($serendipity['blogTitle']) . '\' AND c.title != \'\')
                 AND    c.status = \'approved\'
                        ' . $viewtype . '
            ORDER BY    c.timestamp DESC
            LIMIT ' . $max_entries;
?>
<div style="margin: 0px; padding: 0px; text-align: left;">
<?php
        $sql = serendipity_db_query($q);

        if ($sql && is_array($sql)) {
            foreach($sql AS $key => $row) {
                $comments = wordwrap(strip_tags($row['comment']), $max_chars, '@@@', 1);
                $aComment = explode('@@@', $comments);
                $comment  = $aComment[0];
                if (count($aComment) > 1) {
                    $comment .= ' [...]';
                }

                if ($row['comment_type'] == 'TRACKBACK' && $row['comment_url'] != '') {
                    $user = '<a class="highlight" href="' . htmlspecialchars(strip_tags($row['comment_url'])) . '" title="' . htmlspecialchars(strip_tags($row['comment_title'])) . '">' . htmlspecialchars(strip_tags($row['user'])) . '</a>';
                } else {
                    $user = htmlspecialchars(strip_tags($row['user']));
                }

                $entry = array('comment' => wordwrap($comment, $wordwrap, "\n", 1),
                               'email'   => $row['comment_email']);
                serendipity_plugin_api::hook_event('frontend_display', $entry);
                printf(
                  PLUGIN_COMMENTS_ABOUT,

                  $user,
                  ' <a class="highlight" href="' . serendipity_archiveURL($row['entry_id'], $row['subject'], 'baseURL', true, array('timestamp' => $row['entrystamp'])) .'#c' . $row['comment_id'] . '" title="' . htmlspecialchars($row['subject']) . '">'
                      . htmlspecialchars($row['subject'])
                      . '</a><br />' . "\n"
                      . htmlspecialchars(serendipity_strftime($dateformat, $row['stamp'])) . '<br />' . "\n"
                      . strip_tags($entry['comment'], '<br><img>')
                      . '<br /><br /><br />' . "\n\n"
                );
            }
        }
?>
</div>
<?php
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
