<?php # $Id: serendipity_event_spamblock.php 617 2005-10-27 11:02:22Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

/* BC - TODO: Remove for 0.8 final */
if (!function_exists('serendipity_serverOffsetHour')) {
    function serendipity_serverOffsetHour() {
        return time();
    }
}

@define('PLUGIN_EVENT_SPAMBLOCK_TITLE', 'Spam Protector');
@define('PLUGIN_EVENT_SPAMBLOCK_DESC', 'A variety of methods to prevent comment spam');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', 'Spam Prevention: Invalid message.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', 'Spam Prevention: You cannot post a comment so soon after submitting another one.');

@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', 'This blog is in "Emergency Comment Blockage Mode", please come back another time');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', 'Do not allow duplicate comments');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', 'Do not allow users to submit a comment which contains the same body as an already submitted comment');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', 'Emergency comment shutdown');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', 'Temporarily disable comments for all entries. Useful if your blog is under spam attack.');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'IP block interval');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', 'Only allow an IP to submit a comment every n minutes. Useful to prevent comment floods.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', 'Enable Captchas');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', 'Will force the user to input a random string displayed in a specially crafted image. This will disallow automated submits to your blog. Please remember that people with decreased vision may find it hard to read those captchas.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', 'To prevent automated Bots from commentspamming, please enter the string you see in the image below in the appropriate input box. Your comment will only be submitted if the strings match. Please ensure that your browser supports and accepts cookies, or your comment cannot be verified correctly.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', 'Enter the string you see here in the input box!');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', 'Enter the string from the spam-prevention image above: ');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', 'You did not enter the correct string displayed in the spam-prevention image box. Please look at the image and enter the values displayed there.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', 'Captchas disabled on your server. You need GDLib and freetype libraries compiled to PHP, and need the .TTF files residing in your directory.');

@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', 'Force captchas after how many days');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', 'Captchas can be enforced depending on the age of your articles. Enter the amount of days after which entering a correct captcha is necessary. If set to 0, captchas will always be used.');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', 'Force comment moderation after how many days');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', 'You can automatically set all comments for entries to be moderated. Enter the age of an entry in days, after which it should be auto-moderated. 0 means no auto-moderation.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', 'How many links before a comment gets moderated');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', 'When a comment reaches a certain amount of links, that comment can be set to be moderated. 0 means that no link-checking is done.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', 'How many links before a comment gets rejected');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', 'When a comment reaches a certain amount of links, that comment can be set to be rejected. 0 means that no link-checking is done.');

@define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', 'Because of some conditions, your comment has been marked to require moderation by the owner of this blog.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'Background color of the captcha');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', 'Enter RGB values: 0,255,255');

@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', 'Logfile location');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', 'Information about rejected/moderated posts can be written to a logfile. Set this to an empty string if you want to disable logging.');

@define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', 'Emergency Comment Blockage');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', 'Duplicate comment');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'IP-block');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', 'Invalid captcha (Entered: %s, Expected: %s)');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', 'Auto-moderation after X days');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT', 'Too many hyperlinks');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE', 'Too many hyperlinks');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', 'Hide E-Mail addresses of commenting users');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', 'Will show no E-Mail addresses of commenting users');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', 'E-Mail addresses will not be displayed and will only be used for E-Mail notifications');

@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', 'Choose logging method');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', 'Logging of rejected comments can be done in Database or to a plaintext file');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', 'File (see "logfile" option below)');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', 'Database');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', 'No Logging');

@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', 'How to treat comments made via APIs');
@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', 'This affects the moderation of comments made via API calls (Trackbacks, WFW:commentAPI comments). If set to "moderate", all those comments always need to be approved first. If set to "reject", the are completely disallowed. If set to "none", the comments will be treated as usual comments.');
@define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', 'moderate');
@define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', 'reject');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', 'No API-created comments (like trackbacks) allowed');

@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', 'Activate wordfilter');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', 'Searches comments for certain strings and marks them as spam.');

@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', 'Wordfilter for URLs');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', 'Regular Expressions allowed, separate strings by semicolons (;).');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', 'Wordfilter for author names');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', 'Regular Expressions allowed, separate strings by semicolons (;).');

@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CHECKMAIL', 'Invalid e-mail address');
@define('PLUGIN_EVENT_SPAMBLOCK_CHECKMAIL', 'Check e-mail addresses?');
@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS', 'Required comment fields');
@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS_DESC', 'Enter a list of required fields that need to be filled when a user comments. Seperate multiple fields with a ",". Available keys are: name, email, url, replyTo, comment');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_REQUIRED_FIELD', 'You did not specify the %s field!');

@define('PLUGIN_EVENT_SPAMBLOCK_CONFIG', 'Configure Anti-Spam methods');
@define('PLUGIN_EVENT_SPAMBLOCK_ADD_AUTHOR', 'Block this author via Spamblock plugin');
@define('PLUGIN_EVENT_SPAMBLOCK_ADD_URL', 'Block this URL via Spamblock plugin');
@define('PLUGIN_EVENT_SPAMBLOCK_REMOVE_AUTHOR', 'Unblock this author via Spamblock plugin');
@define('PLUGIN_EVENT_SPAMBLOCK_REMOVE_URL', 'Unblock this URL via Spamblock plugin');

@define('PLUGIN_EVENT_SPAMBLOCK_BLOGG_SPAMLIST', 'Activate URL filtering by blogg.de Blacklist');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_BLOGG_SPAMLIST', 'Filtered by blogg.de Blacklist');

class serendipity_event_spamblock extends serendipity_event
{
var $filter_defaults;

    function introspect(&$propbag)
    {
        global $serendipity;

        $this->title = PLUGIN_EVENT_SPAMBLOCK_TITLE;

        $propbag->add('name',          PLUGIN_EVENT_SPAMBLOCK_TITLE);
        $propbag->add('description',   PLUGIN_EVENT_SPAMBLOCK_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking, Sebastian Nohn');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('version',       '1.26');
        $propbag->add('event_hooks',    array(
            'frontend_saveComment' => true,
            'external_plugin'      => true,
            'frontend_comment'     => true,
            'fetchcomments'        => true,
            'backend_comments_top' => true,
            'backend_view_comment' => true
        ));
        $propbag->add('configuration', array('killswitch', 'bodyclone', 'ipflood', 'captchas', 'captchas_ttl', 'captcha_color', 'forcemoderation', 'disable_api_comments', 'links_moderate', 'links_reject', 'contentfilter_activate', 'contentfilter_urls', 'bloggdeblacklist', 'contentfilter_authors', 'hide_email', 'checkmail', 'required_fields', 'logtype', 'logfile'));
        $propbag->add('groups', array('ANTISPAM'));

        $this->filter_defaults = array(
                                   'authors' => 'casino;phentermine;credit;loans;poker',
                                   'urls'    => '8gold\.com;911easymoney\.com;canadianlabels\.net;condodream\.com;crepesuzette\.com;debt-help-bill-consolidation-elimination\.com;fidelityfunding\.net;flafeber\.com;gb\.com;houseofsevengables\.com;instant-quick-money-cash-advance-personal-loans-until-pay-day\.com;mediavisor\.com;newtruths\.com;oiline\.com;onlinegamingassociation\.com;online\-+poker\.com;popwow\.com;royalmailhotel\.com;spoodles\.com;sportsparent\.com;stmaryonline\.org;thatwhichis\.com;tmsathai\.org;uaeecommerce\.com;learnhowtoplay\.com'
        );
    }

    function introspect_config_item($name, &$propbag)
    {
        global $serendipity;

        switch($name) {
            case 'disable_api_comments':
                $propbag->add('type', 'radio');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC);
                $propbag->add('default', 'none');
                $propbag->add('radio', array(
                    'value' => array('moderate', 'reject', 'none'),
                    'desc'  => array(PLUGIN_EVENT_SPAMBLOCK_API_MODERATE, PLUGIN_EVENT_SPAMBLOCK_API_REJECT, NONE)
                ));
                $propbag->add('radio_per_row', '1');

                break;

            case 'hide_email':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC);
                $propbag->add('default', false);
                break;

            case 'checkmail':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_CHECKMAIL);
                $propbag->add('description', '');
                $propbag->add('default', false);
                break;

            case 'required_fields':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS_DESC);
                $propbag->add('default', '');
                break;

            case 'bodyclone':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_BODYCLONE);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC);
                $propbag->add('default', true);
                break;

            case 'captchas':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC);
                $propbag->add('default', true);
                break;

            case 'killswitch':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC);
                $propbag->add('default', false);
                break;

            case 'contentfilter_activate':
                $propbag->add('type', 'radio');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC);
                $propbag->add('default', 'moderate');
                $propbag->add('radio', array(
                    'value' => array('moderate', 'reject', 'none'),
                    'desc'  => array(PLUGIN_EVENT_SPAMBLOCK_API_MODERATE, PLUGIN_EVENT_SPAMBLOCK_API_REJECT, NONE)
                ));
                $propbag->add('radio_per_row', '1');

                break;

            case 'bloggdeblacklist':
                $propbag->add('type', 'radio');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_BLOGG_SPAMLIST);
                $propbag->add('description', '');
                $propbag->add('default', 'none');
                $propbag->add('radio', array(
                    'value' => array('moderate', 'reject', 'none'),
                    'desc'  => array(PLUGIN_EVENT_SPAMBLOCK_API_MODERATE, PLUGIN_EVENT_SPAMBLOCK_API_REJECT, NONE)
                ));
                $propbag->add('radio_per_row', '1');

                break;

            case 'contentfilter_urls':
                $propbag->add('type', 'text');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC);
                $propbag->add('default', $this->filter_defaults['urls']);
                break;

            case 'contentfilter_authors':
                $propbag->add('type', 'text');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC);
                $propbag->add('default', $this->filter_defaults['authors']);
                break;

            case 'logfile':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_LOGFILE);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC);
                $propbag->add('default', $serendipity['serendipityPath'] . 'spamblock.log');
                break;

            case 'logtype':
                $propbag->add('type', 'radio');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_LOGTYPE);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC);
                $propbag->add('default', 'db');
                $propbag->add('radio',         array(
                    'value' => array('file', 'db', 'none'),
                    'desc'  => array(PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE, PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB, PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE)
                ));
                $propbag->add('radio_per_row', '1');

                break;

            case 'ipflood':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_IPFLOOD);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC);
                $propbag->add('default', 0);
                break;

            case 'captchas_ttl':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC);
                $propbag->add('default', '7');
                break;

            case 'captcha_color':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC);
                $propbag->add('default', '255,255,255');
                $propbag->add('validate', '@^[0-9]{1,3},[0-9]{1,3},[0-9]{1,3}$@');
                break;

            case 'forcemoderation':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC);
                $propbag->add('default', '30');
                break;

            case 'links_moderate':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC);
                $propbag->add('default', '7');
                break;

            case 'links_reject':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT);
                $propbag->add('description', PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC);
                $propbag->add('default', '13');
                break;

            default:
                    return false;
        }

        return true;
    }

    function getBlacklist($where) {
        global $serendipity;
        
        switch($where) {
            case 'blogg.de':
                $target  = $serendipity['serendipityPath'] . PATH_SMARTY_COMPILE . '/blogg.de.blacklist.txt';
                $timeout = 3600; // One hour
  
                if (file_exists($target) && filemtime($target) > time()-$timeout) {
                    $data = file_get_contents($target);
                } else {
                    $data = '';
                    require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
                    $req    = &new HTTP_Request('http://spam.blogg.de/blacklist.txt');
    
                    if (PEAR::isError($req->sendRequest()) || $req->getResponseCode() != '200') {
                        if (file_exists($target) && filesize($target) > 0) {
                            $data = file_get_contents($target);
                        }
                    } else {
                        // Fetch file
                        $data = $req->getResponseBody();
                        $fp = @fopen($target, 'w');
        
                        if ($fp) {
                            fwrite($fp, $data);
                            fclose($fp);
                        }
                    }
                }
                
                $blacklist = explode("\n", $data);
                return $blacklist;
            
            default:
                return false;
        }
    }

    function checkScheme($maxVersion) {
        global $serendipity;

        $version = $this->get_config('version', '1.0');

        if ($version != $maxVersion) {
            $q   = "CREATE TABLE {$serendipity['dbPrefix']}spamblocklog (
                          timestamp int(10) {UNSIGNED} default null,
                          type varchar(255),
                          reason text,
                          entry_id int(10) {UNSIGNED} not null default '0',
                          author varchar(80),
                          email varchar(200),
                          url varchar(200),
                          useragent varchar(255),
                          ip varchar(15),
                          referer varchar(255),
                          body text)";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE INDEX kspamidx ON {$serendipity['dbPrefix']}spamblocklog (timestamp);";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE INDEX kspamtypeidx ON {$serendipity['dbPrefix']}spamblocklog (type);";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE INDEX kspamentryidx ON {$serendipity['dbPrefix']}spamblocklog (entry_id);";
            $sql = serendipity_db_schema_import($q);

            $this->set_config('version', $maxVersion);
        }

        return true;
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function event_hook($event, &$bag, &$eventData, $addData = null) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            $captchas_ttl = $this->get_config('captchas_ttl', 7);
            $captchas     = (serendipity_db_bool($this->get_config('captchas', true)) ? true : false);
            // Check if the entry is older than the allowed amount of time. Enforce kaptchas if that is true
            // of if kaptchas are activated for every entry
            $show_captcha = ($captchas && isset($eventData['timestamp']) && ($captchas_ttl < 1 || ($eventData['timestamp'] < (time() - ($captchas_ttl*60*60*24)))) ? true : false);

            $forcemoderation = $this->get_config('forcemoderation', 60);
            $links_moderate  = $this->get_config('links_moderate', 10);
            $links_reject    = $this->get_config('links_reject', 20);

            if (function_exists('imagettftext') && function_exists('imagejpeg')) {
                $max_char = 5;
                $min_char = 3;
                $use_gd   = true;
            } else {
                $max_char = $min_char = 5;
                $use_gd   = false;
            }

            switch($event) {
                case 'fetchcomments':
                    if (is_array($eventData) && !$_SESSION['serendipityAuthedUser'] && serendipity_db_bool($this->get_config('hide_email', false))) {
                        // Will force emails to be not displayed in comments and RSS feed for comments. Will not apply to logged in admins (so not in the backend as well)
                        @reset($eventData);
                        while(list($idx, $comment) = each($eventData)) {
                            $eventData[$idx]['no_email'] = true;
                        }
                    }
                    break;

                case 'frontend_saveComment':
                    if (!is_array($eventData) || serendipity_db_bool($eventData['allow_comments'])) {
                        if ($this->get_config('logtype', 'db') == 'db' && $this->get_config('version') != $bag->get('version')) {
                            $this->checkScheme($bag->get('version'));
                        }

                        $serendipity['csuccess'] = 'true';
                        $logfile = $this->get_config('logfile', $serendipity['serendipityPath'] . 'spamblock.log');
                        $required_fields = $this->get_config('required_fields', '');

                        // Check required fields
                        if ($addData['type'] == 'NORMAL' && !empty($required_fields)) {
                            $required_field_list = explode(',', $required_fields);
                            foreach($required_field_list as $required_field) {
                                $required_field = trim($required_field);
                                if (empty($addData[$required_field])) {
                                    $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_REQUIRED_FIELD, $addData);
                                    $eventData = array('allow_comments' => false);
                                    $serendipity['messagestack']['comments'][] = sprintf(PLUGIN_EVENT_SPAMBLOCK_REASON_REQUIRED_FIELD, $required_field);
                                    return false;
                                }
                            }
                        }

                        // Check for global emergency moderation
                        if ( $this->get_config('killswitch', false) === true ) {
                            $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH, $addData);
                            $eventData = array('allow_comments' => false);
                            $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH;
                            return false;
                        }

                        // Check for not allowing trackbacks/wfwcomments
                        if ( ($addData['type'] != 'NORMAL' || $addData['source'] == 'API') &&
                             $this->get_config('disable_api_comments', 'none') != 'none') {
                            if ($this->get_config('disable_api_comments') == 'reject') {
                                $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_API, $addData);
                                $eventData = array('allow_comments' => false);
                                $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_REASON_API;
                            } elseif ($this->get_config('disable_api_comments') == 'moderate') {
                                $this->log($logfile, $eventData['id'], 'MODERATE', PLUGIN_EVENT_SPAMBLOCK_REASON_API, $addData);
                                $eventData['moderate_comments'] = true;
                                $serendipity['csuccess']        = 'moderate';
                                $serendipity['moderate_reason'] = PLUGIN_EVENT_SPAMBLOCK_REASON_API;
                            }
                            return false;
                        }

                        // Check for word filtering
                        if ($filter_type = $this->get_config('contentfilter_activate', 'moderate')) {

                            // Filter authors names
                            $filter_authors = explode(';', $this->get_config('contentfilter_authors', $this->filter_defaults['authors']));
                            if (is_array($filter_authors)) {
                                foreach($filter_authors AS $filter_author) {
                                    if (empty($filter_author)) {
                                        continue;
                                    }
                                    if (preg_match('@' . $filter_author . '@', $addData['name'])) {
                                        if ($filter_type == 'moderate') {
                                            $this->log($logfile, $eventData['id'], 'MODERATE', PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS, $addData);
                                            $eventData['moderate_comments'] = true;
                                            $serendipity['csuccess']        = 'moderate';
                                            $serendipity['moderate_reason'] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                            return false;
                                        } else {
                                            $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS, $addData);
                                            $eventData = array('allow_comments' => false);
                                            $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                            return false;
                                        }
                                    }
                                }
                            }

                            // Filter URL
                            $filter_urls = explode(';', $this->get_config('contentfilter_urls', $this->filter_defaults['urls']));
                            if (is_array($filter_urls)) {
                                foreach($filter_urls AS $filter_url) {
                                    if (empty($filter_url)) {
                                        continue;
                                    }
                                    if (preg_match('@' . $filter_url . '@', $addData['url'])) {
                                        if ($filter_type == 'moderate') {
                                            $this->log($logfile, $eventData['id'], 'MODERATE', PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS, $addData);
                                            $eventData['moderate_comments'] = true;
                                            $serendipity['csuccess']        = 'moderate';
                                            $serendipity['moderate_reason'] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                            return false;
                                        } else {
                                            $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS, $addData);
                                            $eventData = array('allow_comments' => false);
                                            $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                            return false;
                                        }
                                    }
                                }
                            }
                            
                            // Filter Blogg.de Blacklist?
                            $bloggdeblacklist = $this->get_config('bloggdeblacklist');
                            if ($bloggdeblacklist == 'moderate' || $bloggdeblacklist == 'reject') {
                                $domains = $this->getBlacklist('blogg.de');
                                if (is_array($domains)) {
                                    foreach($domains AS $domain) {
                                        $domain = trim($domain);
                                        if (empty($domain)) {
                                            continue;
                                        }
                                        
                                        if (preg_match('@' . preg_quote($domain) . '@i', $addData['url'])) {
                                            if ($bloggdeblacklist == 'moderate') {
                                                $this->log($logfile, $eventData['id'], 'MODERATE', PLUGIN_EVENT_SPAMBLOCK_REASON_BLOGG_SPAMLIST . ': ' . $domain, $addData);
                                                $eventData['moderate_comments'] = true;
                                                $serendipity['csuccess']        = 'moderate';
                                                $serendipity['moderate_reason'] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                                return false;
                                            } else {
                                                $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_BLOGG_SPAMLIST . ': ' . $domain, $addData);
                                                $eventData = array('allow_comments' => false);
                                                $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                                return false;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Check for maximum number of links before rejecting
                        $link_count = substr_count(strtolower($addData['comment']), 'http://');
                        if ($links_reject > 0 && $link_count > $links_reject) {
                            $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT, $addData);
                            $eventData = array('allow_comments' => false);
                            $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                            return false;
                        }

                        // Captcha checking
                        if ($show_captcha && $addData['type'] == 'NORMAL') {
                            if (!isset($_SESSION['spamblock']['captcha']) || !isset($serendipity['POST']['captcha']) || strtolower($serendipity['POST']['captcha']) != strtolower($_SESSION['spamblock']['captcha'])) {
                                $this->log($logfile, $eventData['id'], 'REJECTED', sprintf(PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS, $serendipity['POST']['captcha'], $_SESSION['spamblock']['captcha']), $addData);
                                $eventData = array('allow_comments' => false);
                                $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS;
                                return false;
                            } else {
// DEBUG
//                                $this->log($logfile, $eventData['id'], 'REJECTED', 'Captcha passed: ' . $serendipity['POST']['captcha'] . ' / ' . $_SESSION['spamblock']['captcha'] . ' // Source: ' . $_SERVER['REQUEST_URI'], $addData);
                            }
                        } else {
// DEBUG
//                            $this->log($logfile, $eventData['id'], 'REJECTED', 'Captcha not needed: ' . $serendipity['POST']['captcha'] . ' / ' . $_SESSION['spamblock']['captcha'] . ' // Source: ' . $_SERVER['REQUEST_URI'], $addData);
                        }

                        // Check for forced moderation
                        if ($forcemoderation > 0 && $eventData['timestamp'] < (time() - ($forcemoderation * 60 * 60 * 24))) {
                            $this->log($logfile, $eventData['id'], 'MODERATE', PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION, $addData);
                            $eventData['moderate_comments'] = true;
                            $serendipity['csuccess']        = 'moderate';
                            $serendipity['moderate_reason'] = PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION;
                            return false;
                        }

                        // Check for maximum number of links before forcing moderation
                        if ($links_moderate > 0 && $link_count > $links_moderate) {
                            $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE, $addData);
                            $eventData['moderate_comments'] = true;
                            $serendipity['csuccess']        = 'moderate';
                            $serendipity['moderate_reason'] = PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE;
                            return false;
                        }

                        // Check for identical comments. We allow to bypass trackbacks from our server to our own blog.
                        if ( $this->get_config('bodyclone', true) === true && $_SERVER['REMOTE_ADDR'] != $_SERVER['SERVER_ADDR']) {
                            $query = "SELECT count(id) AS counter FROM {$serendipity['dbPrefix']}comments WHERE body = '" . serendipity_db_escape_string($addData['comment']) . "'";
                            $row   = serendipity_db_query($query, true);
                            if (is_array($row) && $row['counter'] > 0) {
                                $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE, $addData);
                                $eventData = array('allow_comments' => false);
                                $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY;
                                return false;
                            }
                        }

                        // Check last IP
                        if ($addData['type'] == 'NORMAL' && $this->get_config('ipflood', 2) != 0 ) {
                            $query = "SELECT max(timestamp) AS last_post FROM {$serendipity['dbPrefix']}comments WHERE ip = '" . serendipity_db_escape_string($_SERVER['REMOTE_ADDR']) . "'";
                            $row   = serendipity_db_query($query, true);
                            if (is_array($row) && $row['last_post'] > (time() - $this->get_config('ipflood', 2)*60)) {
                                $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD, $addData);
                                $eventData = array('allow_comments' => false);
                                $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_ERROR_IP;
                                return false;
                            }
                        }
                        
                        // Check invalid email
                        if ($addData['type'] == 'NORMAL' && serendipity_db_bool($this->get_config('checkmail', false))) {
                            if (!empty($addData['email']) && strstr($addData['email'], '@') === false) {
                                $this->log($logfile, $eventData['id'], 'REJECTED', PLUGIN_EVENT_SPAMBLOCK_REASON_CHECKMAIL, $addData);
                                $eventData = array('allow_comments' => false);
                                $serendipity['messagestack']['comments'][] = PLUGIN_EVENT_SPAMBLOCK_REASON_CHECKMAIL;
                            }
                        }
                    }
                        
                    return true;
                    break;

                case 'frontend_comment':
                    if (serendipity_db_bool($this->get_config('hide_email', false))) {
                        echo '<div class="serendipity_commentDirection">' . PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE . '</div>';
                    }

                    if ($show_captcha) {
                        echo '<div class="serendipity_commentDirection">';
                        if (!isset($serendipity['POST']['preview']) || strtolower($serendipity['POST']['captcha'] != strtolower($_SESSION['spamblock']['captcha']))) {
                            echo '<br />' . PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC . '<br />';
                            if ($use_gd) {
                                printf('<img src="%s" title="%s" alt="CAPTCHA" class="captcha" />',
                                    $serendipity['baseURL'] . ($serendipity['rewrite'] == 'none' ? $serendipity['indexFile'] . '?/' : '') . 'plugin/captcha_' . md5(time()),
                                    htmlspecialchars(PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2)
                                );
                            } else {
                                $bgcolors = explode(',', $this->get_config('captcha_color', '255,0,255'));
                                $hexval   = '#' . dechex(trim($bgcolors[0])) . dechex(trim($bgcolors[1])) . dechex(trim($bgcolors[2]));
                                $this->random_string($max_char, $min_char);
                                echo '<div style="background-color: ' . $hexval . '">';
                                for ($i = 1; $i <= $max_char; $i++) {
                                    printf('<img src="%s" title="%s" alt="CAPTCHA ' . $i . '" class="captcha" />',
                                        $serendipity['baseURL'] . ($serendipity['rewrite'] == 'none' ? $serendipity['indexFile'] . '?/' : '') . 'plugin/captcha_' . $i . '_' . md5(time()),
                                        htmlspecialchars(PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2)
                                    );
                                }
                                echo '</div>';
                            }
                            echo '<br />';
                            echo '<label for="captcha">'. PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3 . '</label><br /><input type="text" size="5" name="serendipity[captcha]" value="" id="captcha" />';
                        } elseif (isset($serendipity['POST']['captcha'])) {
                            echo '<input type="hidden" name="serendipity[captcha]" value="' . htmlspecialchars($serendipity['POST']['captcha']) . '" />';
                        }
                        echo '</div>';
                    }

                    return true;
                    break;


                case 'external_plugin':
                    $parts     = explode('_', $eventData);
                    if (!empty($parts[1])) {
                        $param     = (int) $parts[1];
                    } else {
                        $param     = null;
                    }

                    $methods = array('captcha');

                    if (!in_array($parts[0], $methods)) {
                        return;
                    }

                    list($musec, $msec) = explode(' ', microtime());
                    $srand = (float) $msec + ((float) $musec * 100000);
                    srand($srand);
                    mt_srand($srand);
                    $width    = 120;
                    $height   = 40;

                    $bgcolors = explode(',', $this->get_config('captcha_color', '255,255,255'));
                    $fontfiles = array('Vera.ttf', 'VeraSe.ttf', 'chumbly.ttf', '36daysago.ttf');

                    if ($use_gd) {
                        $strings  = $this->random_string($max_char, $min_char);
                        $fontname = $fontfiles[array_rand($fontfiles)];
                        $font     = $serendipity['serendipityPath'] . 'plugins/serendipity_event_spamblock/' . $fontname;

                        if (!file_exists($font)) {
                            // Search in shared plugin directory
                            $font = S9Y_INCLUDE_PATH . 'plugins/serendipity_event_spamblock/' . $fontname;
                        }

                        if (!file_exists($font)) {
                            die(PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF);
                        }

                        header('Content-Type: image/jpeg');
                        $image  = imagecreate($width, $height);
                        imagecolorallocate($image, trim($bgcolors[0]), trim($bgcolors[1]), trim($bgcolors[2]));
                        // imagettftext($image, 10, 1, 1, 15, imagecolorallocate($image, 255, 255, 255), $font, 'String: ' . $string);

                        $pos_x  = 5;
                        foreach($strings AS $idx => $charidx) {
                            $color = imagecolorallocate($image, mt_rand(50, 235), mt_rand(50, 235), mt_rand(50,235));
                            $size  = mt_rand(15, 21);
                            $angle = mt_rand(-20, 20);
                            $pos_y = ceil($height - (mt_rand($size/3, $size/2)));

                            imagettftext(
                              $image,
                              $size,
                              $angle,
                              $pos_x,
                              $pos_y,
                              $color,
                              $font,
                              $this->chars[$charidx]
                            );

                            $pos_x = $pos_x + $size + 2;

                        }
                        imagejpeg($image, '', 90);
                        imagedestroy($image);
                    } else {
                        header('Content-Type: image/png');
                        $output_char = strtolower($_SESSION['spamblock']['captcha']{$parts[1] - 1});
                        $cap = $serendipity['serendipityPath'] . 'plugins/serendipity_event_spamblock/captcha_' . $output_char . '.png';
                        if (!file_exists($cap)) {
                            $cap = S9Y_INCLUDE_PATH . 'plugins/serendipity_event_spamblock/captcha_' . $output_char . '.png';
                        }

                        if (file_exists($cap)) {
                            echo file_get_contents($cap);
                        }
                    }
                    return true;
                    break;

                case 'backend_comments_top':

                    // Add Author to blacklist. If already filtered, it will be removed from the filter. (AKA "Toggle")
                    if (isset($serendipity['GET']['spamBlockAuthor'])) {
                        $item    = $this->getComment('author', $serendipity['GET']['spamBlockAuthor']);
                        $items   = &$this->checkFilter('authors', $item, true);
                        $this->set_config('contentfilter_authors', implode(';', $items));
                    }

                    // Add URL to blacklist. If already filtered, it will be removed from the filter. (AKA "Toggle")
                    if (isset($serendipity['GET']['spamBlockURL'])) {
                        $item    = $this->getComment('url', $serendipity['GET']['spamBlockURL']);
                        $items   = &$this->checkFilter('urls', $item, true);
                        $this->set_config('contentfilter_urls', implode(';', $items));
                    }

                    echo ' - ' . WORD_OR . ' - <a class="serendipityPrettyButton" href="serendipity_admin.php?serendipity[adminModule]=plugins&amp;serendipity[plugin_to_conf]=' . $this->instance . '">' . PLUGIN_EVENT_SPAMBLOCK_CONFIG . '</a>';
                    return true;
                    break;

                case 'backend_view_comment':
                    $author_is_filtered = $this->checkFilter('authors', $eventData['author']);
                    $eventData['action_author'] .= ' <a class="serendipityIconLink" title="' . ($author_is_filtered ? PLUGIN_EVENT_SPAMBLOCK_REMOVE_AUTHOR : PLUGIN_EVENT_SPAMBLOCK_ADD_AUTHOR) . '" href="serendipity_admin.php?serendipity[adminModule]=comments&amp;serendipity[spamBlockAuthor]=' . $eventData['id'] . '"><img src="' . serendipity_getTemplateFile('admin/img/' . ($author_is_filtered ? 'un' : '') . 'configure.png') . '" /></a>';

                    if (!empty($eventData['url'])) {
                        $url_is_filtered    = $this->checkFilter('urls', $eventData['url']);
                        $eventData['action_url']    .= ' <a class="serendipityIconLink" title="' . ($url_is_filtered ? PLUGIN_EVENT_SPAMBLOCK_REMOVE_URL : PLUGIN_EVENT_SPAMBLOCK_ADD_URL) . '" href="serendipity_admin.php?serendipity[adminModule]=comments&amp;serendipity[spamBlockURL]=' . $eventData['id'] . '"><img src="' . serendipity_getTemplateFile('admin/img/' . ($url_is_filtered ? 'un' : '') . 'configure.png') . '" /></a>';
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

    function &checkFilter($what, $match, $getItems = false) {
        $items = explode(';', $this->get_config('contentfilter_' . $what, $this->filter_defaults[$what]));

        $filtered = false;
        if (is_array($items)) {
            foreach($items AS $key => $item) {
                if (empty($match)) {
                    continue;
                }

                if (empty($item)) {
                    unset($items[$key]);
                    continue;
                }

                if (preg_match('@' . $item . '@', $match)) {
                    $filtered = true;
                    unset($items[$key]);
                }
            }
        }
        
        if ($getItems) {
            if (!$filtered && !empty($match)) {
                $items[] = preg_quote($match, '@');
            }

            return $items;
        }
        
        return $filtered;
    }

    function getComment($key, $id) {
        global $serendipity;
        $c = serendipity_db_query("SELECT $key FROM {$serendipity['dbPrefix']}comments WHERE id = '" . (int)$id . "'", true, 'assoc');
        
        if (!is_array($c) || !isset($c[$key])) {
            return false;
        }
        
        return $c[$key];
    }

    function random_string($max_char, $min_char) {
        $this->chars = array(2, 3, 4, 7, 9); // 1, 5, 6 and 8 may look like characters.
        $this->chars = array_merge($this->chars, array('A','B','C','D','E','F','H','J','K','L','M','N','P','Q','R','T','U','V','W','X','Y','Z')); // I, O, S may look like numbers

        $strings   = array_rand($this->chars, mt_rand($min_char, $max_char));
        $string    = '';
        foreach($strings AS $idx => $charidx) {
            $string .= $this->chars[$charidx];
        }
        $_SESSION['spamblock'] = array('captcha' => $string);

        return $strings;
    }

    function log($logfile, $id, $switch, $reason, $comment) {
        global $serendipity;

        $method = $this->get_config('logtype');

        switch($method) {
            case 'file':
                if (empty($logfile)) {
                    return;
                }

                $fp = @fopen($logfile, 'a+');
                if (!is_resource($fp)) {
                    return;
                }

                fwrite($fp, sprintf(
                    '[%s] - [%s: %s] - [#%s, Name "%s", E-Mail "%s", URL "%s", User-Agent "%s", IP %s] - [%s]' . "\n",
                    date('Y-m-d H:i:s', serendipity_serverOffsetHour()),
                    $switch,
                    $reason,
                    $id,
                    str_replace("\n", ' ', $comment['name']),
                    str_replace("\n", ' ', $comment['email']),
                    str_replace("\n", ' ', $comment['url']),
                    str_replace("\n", ' ', $_SERVER['HTTP_USER_AGENT']),
                    $_SERVER['REMOTE_ADDR'],
                    str_replace("\n", ' ', $comment['comment'])
                ));

                fclose($fp);
                break;

            case 'none':
                return;
                break;

            case 'db':
            default:
                $q = sprintf("INSERT INTO {$serendipity['dbPrefix']}spamblocklog
                                          (timestamp, type, reason, entry_id, author, email, url,  useragent, ip,   referer, body)
                                   VALUES (%d,        '%s',  '%s',  '%s',     '%s',   '%s',  '%s', '%s',      '%s', '%s',    '%s')",

                           serendipity_serverOffsetHour(),
                           serendipity_db_escape_string($switch),
                           serendipity_db_escape_string($reason),
                           serendipity_db_escape_string($id),
                           serendipity_db_escape_string($comment['name']),
                           serendipity_db_escape_string($comment['email']),
                           serendipity_db_escape_string($comment['url']),
                           serendipity_db_escape_string($_SERVER['HTTP_USER_AGENT']),
                           serendipity_db_escape_string($_SERVER['REMOTE_ADDR']),
                           serendipity_db_escape_string(isset($_SESSION['HTTP_REFERER']) ? $_SESSION['HTTP_REFERER'] : $_SERVER['HTTP_REFERER']),
                           serendipity_db_escape_string($comment['comment'])
                );

                serendipity_db_query($q);
                break;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
