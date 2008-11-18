<?php # $Id: serendipity_event_karma.php 555 2005-10-14 10:09:59Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_KARMA_VERSION', '1.3');
@define('PLUGIN_KARMA_NAME', 'Karma');
@define('PLUGIN_KARMA_BLAHBLAH', 'Let visitors rate the quality of your entries');
@define('PLUGIN_KARMA_VOTETEXT', 'Karma for this article: ');
@define('PLUGIN_KARMA_RATE', 'Rate this article: %s');
@define('PLUGIN_KARMA_VOTEPOINT_1', 'Very good!');
@define('PLUGIN_KARMA_VOTEPOINT_2', 'Good');
@define('PLUGIN_KARMA_VOTEPOINT_3', 'Neutral');
@define('PLUGIN_KARMA_VOTEPOINT_4', 'Not interesting');
@define('PLUGIN_KARMA_VOTEPOINT_5', 'Bad');
@define('PLUGIN_KARMA_VOTED', 'Your rating "%s" was stored.');
@define('PLUGIN_KARMA_INVALID', 'Your voting was invalid.');
@define('PLUGIN_KARMA_ALREADYVOTED', 'Your rating was already stored.');
@define('PLUGIN_KARMA_NOCOOKIE', 'Your browser must support cookies to be able to vote.');
@define('PLUGIN_KARMA_CLOSED', 'Vote for articles fresher than %s days!');
@define('PLUGIN_KARMA_ENTRYTIME', 'Voting time after publishing');
@define('PLUGIN_KARMA_VOTINGTIME', 'Voting time');
@define('PLUGIN_KARMA_ENTRYTIME_BLAHBLAH', 'How long (in minutes) after your article has been published an unrestricted voting is allowed? Default: 1440 (one day)');
@define('PLUGIN_KARMA_VOTINGTIME_BLAHBLAH', 'Amount of time (in minutes) that needs to be passed from one vote to the other. Is only applied after the time above is expired. Default: 5');
@define('PLUGIN_KARMA_TIMEOUT', 'Flood protection: Another visitor has just recently voted. Please wait %s minutes.');
@define('PLUGIN_KARMA_CURRENT', 'Current karma: %2$s, %3$s vote(s)');
@define('PLUGIN_KARMA_EXTENDEDONLY', 'Only extended article');
@define('PLUGIN_KARMA_EXTENDEDONLY_BLAHBLAH', 'Only show karmavoting on extended article view');
@define('PLUGIN_KARMA_MAXKARMA', 'Karmavoting period');
@define('PLUGIN_KARMA_MAXKARMA_BLAHBLAH', 'Only allow karmavoting until the article is X days old (Default: 7)');
@define('PLUGIN_KARMA_LOGGING', 'Log votes?');
@define('PLUGIN_KARMA_LOGGING_BLAHBLAH', 'Should karma votes be logged?');
@define('PLUGIN_KARMA_ACTIVE', 'Enable karma voting?');
@define('PLUGIN_KARMA_ACTIVE_BLAHBLAH', 'Is karma voting turned on?');
@define('PLUGIN_KARMA_VISITS', 'Enable visit tracking?');
@define('PLUGIN_KARMA_VISITS_BLAHBLAH', 'Should every click to an extended article be counted and displayed?');
@define('PLUGIN_KARMA_VISITSCOUNT', ' %4$s hits');
@define('PLUGIN_KARMA_STATISTICS_VISITS_TOP', 'Top visited articles');
@define('PLUGIN_KARMA_STATISTICS_VISITS_BOTTOM', 'Worst visited articles');
@define('PLUGIN_KARMA_STATISTICS_VOTES_TOP', 'Top karma-voted articles');
@define('PLUGIN_KARMA_STATISTICS_VOTES_BOTTOM', 'Least karma-voted articles');
@define('PLUGIN_KARMA_STATISTICS_POINTS_TOP', 'Best karma-voted articles');
@define('PLUGIN_KARMA_STATISTICS_POINTS_BOTTOM', 'Worst karma-voted articles');
@define('PLUGIN_KARMA_STATISTICS_VISITS_NO', 'visits');
@define('PLUGIN_KARMA_STATISTICS_VOTES_NO', 'votes');
@define('PLUGIN_KARMA_STATISTICS_POINTS_NO', 'points');


class serendipity_event_karma extends serendipity_event
{
    var $karmaVote    = '';
    var $karmaId      = '';
    var $karmaTimeOut = '';
    var $karmaVoting  = '';
    var $title        = PLUGIN_KARMA_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_KARMA_NAME);
        $propbag->add('description',   PLUGIN_KARMA_BLAHBLAH);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '1.6');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',   array('frontend_configure' => true, 'entry_display' => true, 'css' => true, 'event_additional_statistics' => true));
        $propbag->add('scrambles_true_content', true);
        $propbag->add('groups', array('STATISTICS'));
        $propbag->add('configuration', array('karma_active', 'visits_active', 'max_entrytime', 'max_votetime', 'extended_only', 'max_karmatime', 'logging'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'max_entrytime':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_KARMA_ENTRYTIME);
                $propbag->add('description', PLUGIN_KARMA_ENTRYTIME_BLAHBLAH);
                $propbag->add('default', 1440);
                break;

            case 'max_votetime':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_KARMA_VOTINGTIME);
                $propbag->add('description', PLUGIN_KARMA_VOTINGTIME_BLAHBLAH);
                $propbag->add('default', 5);
                break;

            case 'max_karmatime':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_KARMA_MAXKARMA);
                $propbag->add('description', PLUGIN_KARMA_MAXKARMA_BLAHBLAH);
                $propbag->add('default', 7);
                break;

            case 'karma_active':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_KARMA_ACTIVE);
                $propbag->add('description', PLUGIN_KARMA_ACTIVE_BLAHBLAH);
                $propbag->add('default', 'true');
                break;

            case 'visits_active':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_KARMA_VISITS);
                $propbag->add('description', PLUGIN_KARMA_VISITS_BLAHBLAH);
                $propbag->add('default', 'true');
                break;

            case 'extended_only':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_KARMA_EXTENDEDONLY);
                $propbag->add('description', PLUGIN_KARMA_EXTENDEDONLY_BLAHBLAH);
                $propbag->add('default', 'false');
                break;

            case 'logging':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_KARMA_LOGGING);
                $propbag->add('description', PLUGIN_KARMA_LOGGING_BLAHBLAH);
                $propbag->add('default', 'false');
                break;

            default:
                    return false;
        }
        return true;
    }

    function checkScheme() {
        global $serendipity;

        $version = $this->get_config('version', '0.9');

        if ($version == '1.1') {
            $q   = "ALTER TABLE {$serendipity['dbPrefix']}karma ADD visits INT(11) default 0";
            $sql = serendipity_db_schema_import($q);
            $this->set_config('version', PLUGIN_KARMA_VERSION);
        } elseif ($version == '1.0') {
            $q   = "ALTER TABLE {$serendipity['dbPrefix']}karma ADD visits INT(11) default 0";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE TABLE {$serendipity['dbPrefix']}karmalog (
                        entryid int(11) default null,
                        points int(4) default null,
                        ip varchar(15),
                        user_agent varchar(255),
                        votetime int(11) default null
                    )";
            $sql = serendipity_db_schema_import($q);
            $this->set_config('version', PLUGIN_KARMA_VERSION);
        } elseif ($version != PLUGIN_KARMA_VERSION) {
            $q   = "CREATE TABLE {$serendipity['dbPrefix']}karma (
                        entryid int(11) default null,
                        points int(4) default null,
                        votes int(4) default null,
                        lastvote int(10) {UNSIGNED} NULL,
                        visits int(11) default null
                    )";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE TABLE {$serendipity['dbPrefix']}karmalog (
                        entryid int(11) default null,
                        points int(4) default null,
                        ip varchar(15),
                        user_agent varchar(255)
                    )";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE INDEX kfetch ON {$serendipity['dbPrefix']}karma (entryid, lastvote);";
            $sql = serendipity_db_schema_import($q);

            $q   = "CREATE INDEX kentryid ON {$serendipity['dbPrefix']}karma (entryid);";
            $sql = serendipity_db_schema_import($q);
            $this->set_config('version', PLUGIN_KARMA_VERSION);
        }

        return true;
    }

    function generate_content(&$title)
    {
        $title       = $this->title;
    }

    function event_hook($event, &$bag, &$eventData, $addData = null) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_configure':
                    if (!isset($serendipity['COOKIE']['karmaVote'])) {
                        serendipity_setCookie('karmaVote', serialize(array()));
                    }

                    if (isset($serendipity['GET']['id'])) {
                        $entryid = (int)serendipity_db_escape_string($serendipity['GET']['id']);
                    } elseif (preg_match(PAT_COMMENTSUB, $_SERVER['REQUEST_URI'], $matches)) {
                        $entryid = (int)$matches[1];
                    } else {
                        $entryid = false;
                    }

                    if ($entryid && empty($serendipity['GET']['adminAction'])) {
                        $track_clicks  = serendipity_db_bool($this->get_config('visits_active', true));
                        if ($track_clicks) {
                            $sql = serendipity_db_query('UPDATE ' . $serendipity['dbPrefix'] . 'karma SET visits = visits + 1 WHERE entryid = ' . $entryid, true);
                            if (serendipity_db_affected_rows() < 1) {
                                serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}karma (entryid, points, votes, lastvote, visits) VALUES ('$entryid', 0, 0, 0, 1)");
                            }
                        }
                    }

                    if (!isset($serendipity['GET']['karmaId']) || !isset($serendipity['GET']['karmaVote'])) {
                        return;
                    }

                    $this->karmaId     = (int)$serendipity['GET']['karmaId'];
                    $this->karmaVoting = (int)$serendipity['GET']['karmaVote'];

                    if (!isset($serendipity['COOKIE']['karmaVote'])) {
                        $this->karmaVote = 'nocookie';
                        return;
                    }

                    $karma   = unserialize($serendipity['COOKIE']['karmaVote']);

                    if (!is_array($karma) || !is_numeric($this->karmaVoting) || !is_numeric($this->karmaId) || $this->karmaVoting > 2 || $this->karmaVoting < -2) {
                        $this->karmaVote = 'invalid1';
                        return;
                    }

                    if (!empty($karma[$this->karmaId])) {
                        $this->karmaVote = 'alreadyvoted';
                        return ;
                    }

                    if (stristr($_SERVER['HTTP_USER_AGENT'], 'google')) {
                        // We don't want googlebots hitting the karma-voting
                        $this->karmaVote = 'invalid1';
                        return ;
                    }

                    // Voting takes place here.
                    $q = 'SELECT *
                            FROM ' . $serendipity['dbPrefix'] . 'entries AS e
                 LEFT OUTER JOIN ' . $serendipity['dbPrefix'] . 'karma   AS k
                              ON e.id = k.entryid
                           WHERE e.id = ' . serendipity_db_escape_string($this->karmaId) . ' LIMIT 1';
                    $row = serendipity_db_query($q, true);

                    if (!isset($row) || !is_array($row)) {
                        $this->karmaVote = 'invalid2';
                        return;
                    }

                    $now = time();
                    if ($row['votes'] === '0' || $row['votes'] > 0) {
                        // Votes for this entry already exist. Do some checking.
                        $max_entrytime = $this->get_config('max_entrytime', 1440) * 60;
                        $max_votetime  = $this->get_config('max_votetime', 5)     * 60;
                        $max_karmatime = $this->get_config('max_karmatime', 7)    * 24 * 60 * 60;

                        if ($row['timestamp'] < ($now - $max_karmatime)) {
                            $this->karmaVote = 'timeout2';
                            return;
                        }

                        if (($row['timestamp'] > ($now - $max_entrytime)) || ($row['lastvote'] + $max_votetime < $now) || $row['lastvote'] == 0) {
                            // Update votes
                            $q = sprintf(
                              "UPDATE {$serendipity['dbPrefix']}karma
                                  SET points   = %s,
                                      votes    = %s,
                                      lastvote = %s
                                WHERE entryid  = %s",
                              $row['points'] + $this->karmaVoting,
                              $row['votes']  + 1,
                              $now,
                              $this->karmaId
                            );

                            serendipity_db_query($q);
                        } else {
                            $this->karmaVote    = 'timeout';
                            $this->karmaTimeOut = abs(round(($now - ($row['lastvote'] + $max_votetime)) / 60, 1));
                            return;
                        }
                    } else {
                        // No Votes. Just insert it.
                        $q = sprintf(
                          "INSERT INTO {$serendipity['dbPrefix']}karma
                                       (entryid, points, votes, lastvote)
                                VALUES (%s,      %s,     %s,    %s)",
                          $this->karmaId,
                          $this->karmaVoting,
                          1,
                          $now
                        );

                        $sql = serendipity_db_query($q);
                    }

                    if (serendipity_db_bool($this->get_config('logging', false))) {
                        $q = sprintf(
                          "INSERT INTO {$serendipity['dbPrefix']}karmalog
                                       (entryid, points, ip, user_agent, votetime)
                                VALUES (%s, %s, '%s', '%s', %s)",
                          $this->karmaId,
                          $this->karmaVoting,
                          serendipity_db_escape_string($_SERVER['REMOTE_ADDR']),
                          serendipity_db_escape_string($_SERVER['HTTP_USER_AGENT']),
                          $now
                        );
                        $sql = serendipity_db_query($q);
                        if (is_string($sql)) {
                            mail($serendipity['serendipityEmail'] , 'KARMA ERROR', $q . '<br />' . $sql . '<br />');
                        }
                    }

                    $karma[$this->karmaId] = $this->karmaVoting;
                    $this->karmaVote       = 'voted';
                    serendipity_setCookie('karmaVote', serialize($karma));

                    return true;
                    break;

                case 'css':
                    if (strpos($eventData, '.serendipity_karmaVoting')) {
                        // class exists in CSS, so a user has customized it and we don't need default
                        return true;
                    }
?>

.serendipity_karmaVoting {
    margin-left: auto;
    margin-right: 0px;
    text-align: right;
    font-size: 7pt;
    display: block;
    margin-top: 5px;
    margin-bottom: 0px;
}

.serendipity_karmaVoting a {
    font-size: 7pt;
    text-decoration: none;
}

.serendipity_karmaVoting a:hover {
    color: green;
}

.serendipity_karmaError {
    color: #FF8000;
}

.serendipity_karmaSuccess {
    color: green;
}
<?php
                    return true;
                    break;

                case 'event_additional_statistics':
                    $sql = array();
                    $sql['visits_top']    = array('visits', 'DESC');
                    $sql['visits_bottom'] = array('visits', 'ASC');
                    $sql['votes_top']     = array('votes', 'DESC');
                    $sql['votes_bottom']  = array('votes', 'ASC');
                    $sql['points_top']    = array('points', 'DESC');
                    $sql['points_bottom'] = array('points', 'ASC');

                    foreach($sql AS $key => $rows) {
                        $q = "SELECT e.id,
                                     e.title,
                                     e.timestamp,
                                     SUM(k.{$rows[0]}) AS no
                                FROM {$serendipity['dbPrefix']}karma
                                     AS k
                                JOIN {$serendipity['dbPrefix']}entries
                                     AS e
                                  ON k.entryid = e.id
                            WHERE k.{$rows[0]} IS NOT NULL AND k.{$rows[0]} != 0
                            GROUP BY e.id, e.title, e.timestamp ORDER BY no {$rows[1]} LIMIT {$addData['maxitems']}";
                        $sql_rows = serendipity_db_query($q);
?>
    <dt><strong><?php echo constant('PLUGIN_KARMA_STATISTICS_' . strtoupper($key)); ?></strong></dt>
    <dl>
<?php
                        if (is_array($sql_rows)) {
                            foreach($sql_rows AS $id => $row) {
    ?>
        <dt><strong><a href="<?php echo serendipity_archiveURL($row['id'], $row['title'], 'serendipityHTTPPath', true, array('timestamp' => $row['timestamp'])); ?>"><?php echo htmlspecialchars($row['title']); ?></a></strong></dt>
        <dd><?php echo $row['no']; ?> <?php echo constant('PLUGIN_KARMA_STATISTICS_' . strtoupper($rows[0]) . '_NO'); ?></dd>
    <?php
                            }
                        }
?>
    </dl>
<?php
                    }

                    return true;
                    break;

                case 'entry_display':
                    if ($bag->get('scrambles_true_content') && is_array($addData) && isset($addData['no_scramble'])) {
                        return true;
                    }

                    if ($this->get_config('version') != PLUGIN_KARMA_VERSION) {
                        $this->checkScheme();
                    }

                    // Check whether the cache plugin is used. If so, we need to append our karma-voting output
                    // to the cached version, since that one is used instead of the 'extended' key later on.
                    $extended_key = &$this->getFieldReference('add_footer', $eventData);

                    switch($this->karmaVote) {
                        case 'nocookie':
                            // Users with no cookies won't be able to vote.
                            $msg = '<div class="serendipity_karmaVoting serendipity_karmaError"><a id="karma_vote' . $this->karmaId . '"></a>' . PLUGIN_KARMA_NOCOOKIE . '</div>';

                        case 'timeout2':
                            if (!isset($msg)) {
                                $msg = '<div class="serendipity_karmaVoting serendipity_karmaError"><a id="karma_vote' . $this->karmaId . '"></a>' . PLUGIN_KARMA_CLOSED . '</div>';
                            }

                        case 'timeout':
                            if (!isset($msg)) {
                                $msg = '<div class="serendipity_karmaVoting serendipity_karmaError"><a id="karma_vote' . $this->karmaId . '"></a>' . sprintf(PLUGIN_KARMA_TIMEOUT, $this->karmaTimeOut) . '</div>';
                            }

                        case 'alreadyvoted':
                            if (!isset($msg)) {
                                $msg = '<div class="serendipity_karmaVoting serendipity_karmaError"><a id="karma_vote' . $this->karmaId . '"></a>' . PLUGIN_KARMA_ALREADYVOTED . '</div>';
                            }

                        case 'invalid1':
                        case 'invalid2':
                        case 'invalid':
                            if (!isset($msg)) {
                                $msg = '<div class="serendipity_karmaVoting serendipity_karmaError"><a id="karma_vote' . $this->karmaId . '"></a>' . PLUGIN_KARMA_INVALID . '</div>';
                            }

                            /* OUTPUT MESSAGE */
                            if ($addData['extended']) {
                                $eventData[0]['exflag'] = 1;
                                $eventData[0]['add_footer'] .= $msg;
                            } else {
                                $elements = count($eventData);
                                // Find the right container to store our message in.
                                for ($i = 0; $i < $elements; $i++) {
                                    if ($eventData[$i]['id'] == $this->karmaId) {
                                        $eventData[$i]['add_footer'] .= $msg;
                                    }
                                }
                            }
                            break;

                        case 'voted':
                        default:
                            $track_clicks  = serendipity_db_bool($this->get_config('visits_active', true));
                            $track_karma   = serendipity_db_bool($this->get_config('karma_active', true));
                            $karma_active  = $track_karma;

                            if (!is_array($eventData)) return;

                            $karmatime     = $this->get_config('max_karmatime', 7);
                            $max_karmatime = $karmatime    * 24 * 60 * 60;
                            $now           = time();

                            $url = serendipity_currentURL() . '&amp;';

                            $karma = (isset($serendipity['COOKIE']['karmaVote']) ? unserialize($serendipity['COOKIE']['karmaVote']) : array());

                            $link_1 = '<a class="serendipity_karmaVoting_link1" rel="nofollow" href="#" onclick="javascript:location.href=\'%5$sserendipity[karmaVote]=2&amp;serendipity[karmaId]=%1$s#karma_vote%1$s\';" title="' . sprintf(PLUGIN_KARMA_RATE, PLUGIN_KARMA_VOTEPOINT_1) . '">++</a>';
                            $link_2 = '<a class="serendipity_karmaVoting_link2" rel="nofollow" href="#" onclick="javascript:location.href=\'%5$sserendipity[karmaVote]=1&amp;serendipity[karmaId]=%1$s#karma_vote%1$s\';" title="' . sprintf(PLUGIN_KARMA_RATE, PLUGIN_KARMA_VOTEPOINT_2) . '">+</a>';
                            $link_3 = '<a class="serendipity_karmaVoting_link3" rel="nofollow" href="#" onclick="javascript:location.href=\'%5$sserendipity[karmaVote]=0&amp;serendipity[karmaId]=%1$s#karma_vote%1$s\';" title="' . sprintf(PLUGIN_KARMA_RATE, PLUGIN_KARMA_VOTEPOINT_3) . '">0</a>';
                            $link_4 = '<a class="serendipity_karmaVoting_link4" rel="nofollow" href="#" onclick="javascript:location.href=\'%5$sserendipity[karmaVote]=-1&amp;serendipity[karmaId]=%1$s#karma_vote%1$s\';" title="' . sprintf(PLUGIN_KARMA_RATE, PLUGIN_KARMA_VOTEPOINT_4) . '">-</a>';
                            $link_5 = '<a class="serendipity_karmaVoting_link5" rel="nofollow" href="#" onclick="javascript:location.href=\'%5$sserendipity[karmaVote]=-2&amp;serendipity[karmaId]=%1$s#karma_vote%1$s\';" title="' . sprintf(PLUGIN_KARMA_RATE, PLUGIN_KARMA_VOTEPOINT_5) . '">--</a>';

                            if ($addData['extended'] && $eventData[0]['timestamp'] < ($now - $max_karmatime)) {
                                $karma_active = false;
                            }

                            $karma_voting = '<br /><div class="serendipity_karmaVoting"><a id="karma_vote%1$s"></a>'
                                  . ($karma_active ? PLUGIN_KARMA_VOTETEXT . ' ' . $link_1 . ' | ' . $link_2 . ' | ' . $link_3 . ' | ' . $link_4 . ' | ' . $link_5 . '<br />'
                                  . PLUGIN_KARMA_CURRENT : '') . ($track_clicks ? PLUGIN_KARMA_VISITSCOUNT : '') . '</div>';

                            $karma_current = '<br /><div class="serendipity_karmaVoting"><a id="karma_vote%1$s"></a>'
                                . ($karma_active ? '<div class="serendipity_karmaSuccess">' . PLUGIN_KARMA_VOTED . '</div>'
                                . PLUGIN_KARMA_CURRENT : '') . ($track_clicks ? PLUGIN_KARMA_VISITSCOUNT : '') . '</div>';

                            $karma_timeout = '<br /><div class="serendipity_karmaVoting"><a id="karma_vote%1$s"></a>'
                                . ($track_karma ? '<div>' . sprintf(PLUGIN_KARMA_CLOSED, $karmatime) . '</div>'
                                . PLUGIN_KARMA_CURRENT : '') . ($track_clicks ? PLUGIN_KARMA_VISITSCOUNT : '') . '</div>';

                            if ($addData['extended'] || $addData['preview']) {
                                $entryid = (int)serendipity_db_escape_string($eventData[0]['id']);
                                $q = 'SELECT SUM(votes) AS votes, SUM(points) AS points, SUM(visits) AS visits
                                        FROM ' . $serendipity['dbPrefix'] . 'karma   AS k
                                       WHERE k.entryid = ' . $entryid . ' GROUP BY k.entryid LIMIT 1';
                                $row = serendipity_db_query($q, true);

                                if (empty($row['votes'])) {
                                    $row['votes'] = 0;
                                }

                                if (empty($row['points'])) {
                                    $row['points'] = 0;
                                }

                                if (empty($row['visits'])) {
                                    $row['visits'] = 0;
                                }

                                $eventData[0]['exflag'] = 1;
                                if (isset($karma[$entryid])) {
                                    $extended_key .= sprintf($karma_current, $karma[$entryid], $row['points'], $row['votes'], $row['visits'], $url);
                                } elseif ($eventData[0]['timestamp'] < ($now - $max_karmatime)) {
                                    $extended_key .= sprintf($karma_timeout, $entryid, $row['points'], $row['votes'], $row['visits'], $url);
                                } else {
                                    $extended_key .= sprintf($karma_voting, $entryid, $row['points'], $row['votes'], $row['visits'], $url);
                                }
                            } elseif (!serendipity_db_bool($this->get_config('extended_only', false))) {
                                $elements = count($eventData);

                                // Get all existing entry IDs
                                $entries  = array();
                                for ($i = 0; $i < $elements; $i++) {
                                    $entries[] = (int)$eventData[$i]['id'];
                                }

                                // Fetch votes for all entry IDs. Store them in an array for later usage.
                                $q = 'SELECT k.entryid, SUM(votes) AS votes, SUM(points) AS points, SUM(visits) AS visits
                                        FROM ' . $serendipity['dbPrefix'] . 'karma   AS k
                                       WHERE k.entryid IN (' . implode(', ', $entries) . ') GROUP BY k.entryid';

                                $sql = serendipity_db_query($q);

                                $rows = array();
                                if ($sql && is_array($sql)) {
                                    foreach($sql AS $idx => $row) {
                                        $rows[$row['entryid']] = array('votes' => $row['votes'], 'points' => $row['points'], 'visits' => $row['visits']);
                                    }
                                }

                                // Walk entry array and insert karma voting line.
                                for ($i = 0; $i < $elements; $i++) {
                                    $entryid = $eventData[$i]['id'];
                                    $votes   = (!empty($rows[$entryid]['votes']) ? $rows[$entryid]['votes'] : 0);
                                    $points  = (!empty($rows[$entryid]['points']) ? $rows[$entryid]['points'] : 0);
                                    $visits  = (!empty($rows[$entryid]['visits']) ? $rows[$entryid]['visits'] : 0);

                                    if (!isset($eventData[$i]['add_footer'])) {
                                        $eventData[$i]['add_footer'] = '';
                                    }

                                    if (isset($karma[$entryid])) {
                                        $eventData[$i]['add_footer'] .= sprintf($karma_current, $karma[$entryid], $points, $votes, $visits, $url);
                                    } elseif ($eventData[$i]['timestamp'] < ($now - $max_karmatime)) {
                                        $eventData[$i]['add_footer'] .= sprintf($karma_timeout, $entryid, $points, $votes, $visits, $url);
                                    } else {
                                        $eventData[$i]['add_footer'] .= sprintf($karma_voting, $entryid, $points, $votes, $visits, $url);
                                    }
                                }
                            }
                    }
                    return true;
                    break;

                default:
                    return false;
            }
        } else {
            return false;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
