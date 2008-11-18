<?php # $Id: serendipity_event_mailer.php 400 2005-08-15 12:25:59Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_MAILER_NAME', 'Send entries via E-Mail');
@define('PLUGIN_EVENT_MAILER_DESC', 'Let you send a newly created entry via E-Mail to a specific address');
@define('PLUGIN_EVENT_MAILER_RECIPIENT', 'Mail recipient');
@define('PLUGIN_EVENT_MAILER_RECIPIENTDESC', 'E-Mail address you want to send the entries to (suggested: a mailing list)');
@define('PLUGIN_EVENT_MAILER_LINK', 'Mail link to article?');
@define('PLUGIN_EVENT_MAILER_LINKDESC', 'Include a link to the article in the mail.');
@define('PLUGIN_EVENT_MAILER_STRIPTAGS', 'Remove HTML?');
@define('PLUGIN_EVENT_MAILER_STRIPTAGSDESC', 'Remove HTML-Tags from the mail.');
@define('PLUGIN_EVENT_MAILER_CONVERTP', 'Convert HTML-paragraphs to newlines?');
@define('PLUGIN_EVENT_MAILER_CONVERTPDESC', 'Adds a newline after each HTML paragraph. This is very useful if you enable HTML removing, so that your paragraphs can be kept if not manually entered.');

class serendipity_event_mailer extends serendipity_event
{
    var $title = PLUGIN_EVENT_MAILER_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_MAILER_NAME);
        $propbag->add('description',   PLUGIN_EVENT_MAILER_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Sebastian Nohn, Kristian Köhntopp, Garvin Hicking');
        $propbag->add('version',       '1.3');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',    array(
            'backend_publish' => true
        ));
        $propbag->add('groups', array('FRONTEND_ENTRY_RELATED'));

        $propbag->add('configuration', array('mailto', 'includelink', 'striptags', 'convertp'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'mailto':
                $propbag->add('type',        'string');
                $propbag->add('name',        PLUGIN_EVENT_MAILER_RECIPIENT);
                $propbag->add('description', PLUGIN_EVENT_MAILER_RECIPIENTDESC);
                $propbag->add('default', '');
                break;

            case 'includelink':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_MAILER_LINK);
                $propbag->add('description', PLUGIN_EVENT_MAILER_LINKDESC);
                $propbag->add('default',     'false');
                break;

            case 'striptags':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_MAILER_STRIPTAGS);
                $propbag->add('description', PLUGIN_EVENT_MAILER_STRIPTAGSDESC);
                $propbag->add('default',     'false');
                break;

            case 'convertp':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_EVENT_MAILER_CONVERTP);
                $propbag->add('description', PLUGIN_EVENT_MAILER_CONVERTPDESC);
                $propbag->add('default',     'false');
                break;

            default:
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
                case 'backend_publish':
                    $mail = array(
                      'to'      => $this->get_config('mailto'),
                      'subject' => $eventData['title'],
                      'body'    => $eventData['body'] . $eventData['extended'],
//                      'from'    => $serendipity['blogTitle'] . ' - ' . $eventData['author'] . ' <' . $serendipity['serendipityEmail'] . '>'
                      'from'    => $serendipity['serendipityEmail']
                    );

                    if (serendipity_db_bool($this->get_config('convertp', false)) == true) {
                        $mail['body'] = str_replace('</p>', "</p>\n", $mail['body']);
                    }

                    if (serendipity_db_bool($this->get_config('striptags', false)) == true) {
                        $mail['body'] = preg_replace('§<a[^>]+href=["\']([^"\']*)["\'][^>]*>([^<]*)</a>§i', "$2 [$1]", $mail['body']);
                        $mail['body'] = preg_replace('§<img[^>]+src=["\']([^"\']*)["\'][^>]*>§i', "[" . IMAGE . ": $1]", $mail['body']);
                        $mail['body'] = strip_tags($mail['body']);
                    }

                    if (serendipity_db_bool($this->get_config('includelink', false)) == true) {
                       $mail['body'] = serendipity_archiveURL($eventData['id'], $eventData['title'], 'baseURL', true, array('timestamp' => $eventData['timestamp'])) . "\n\n" . $mail['body'];
                    }

                    serendipity_sendMail($mail['to'], $mail['subject'], $mail['body'], $mail['from']);
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
}

/* vim: set sts=4 ts=4 expandtab : */
?>
