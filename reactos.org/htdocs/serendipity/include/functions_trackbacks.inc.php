<?php # $Id: functions_trackbacks.inc.php 712 2005-11-17 19:54:06Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

/**
 * validate trackback response
 */
function serendipity_trackback_is_success($resp) {
    if (preg_match('@<error>(\d+)</error>@', $resp, $matches)) {
        if ((int) $matches[1] === 0) {
            return true;
        } else {
            if (preg_match('@<message>([^<]+)</message>@', $resp, $matches)) {
                return $matches[1];
            }
            else {
                return 'unknown error';
            }
        }
    }
    return true;
}

function serendipity_pingback_autodiscover($loc, $body) {
global $serendipity;
    if (!empty($_SERVER['X-PINGBACK'])) {
        $pingback = $_SERVER['X-PINGBACK'];
    } elseif (preg_match('@<link rel="pingback" href="([^"]+)" ?/?>@i', $body, $matches)) {
        $pingback = $matches[1];
    } else {
        return;
    }

    // xml-rpc hack
    $query = "
<?xml version=\"1.0\"?>
<methodCall>
  <methodName>pingback.ping</methodName>
  <params>
    <param>
      <name>sourceURI</name>
      <value><string>{$serendipity['baseURL']}</string></value>
    </param>
      <name>targetURI</name>
      <value><string>$loc</string></value>
    </param>
  </params>
</methodCall>";
        _serendipity_send($pingback, $query);
        return;
}

/**
 * Send a trackback ping
 */
function _serendipity_send($loc, $data) {
    global $serendipity;

    $target = parse_url($loc);
    if ($target['query'] != '') {
        $target['query'] = '?' . str_replace('&amp;', '&', $target['query']);
    }

    if (!is_numeric($target['port'])) {
       $target['port'] = 80;
    }

    $sock = @fsockopen($target['host'], $target['port']);
    if (!is_resource($sock)) {
        return "Couldn't connect to $loc";
    }

    $headers = "POST {$target['path']}{$target['query']} HTTP/1.1\r\n";
    $headers .= "Host: {$target['host']}\r\n";
    $headers .= "User-Agent: Serendipity/{$serendipity['version']}\r\n";
    $headers .= "Content-Type: application/x-www-form-urlencoded; charset=" . LANG_CHARSET . "\r\n";
    $headers .= "Content-Length: " . strlen($data) . "\r\n";
    $headers .= "Connection: close\r\n";
    $headers .= "\r\n";
    $headers .= $data;

    fwrite($sock, $headers);

    $res = '';
    while (!feof($sock)) {
        $res .= fgets($sock, 1024);
    }

    fclose($sock);
    return $res;
}

function serendipity_trackback_autodiscover($res, $loc, $url, $author, $title, $text, $loc2 = '') {
    if (!preg_match('@trackback:ping(\s*rdf:resource)?\s*=\s*["\'](https?:[^"\']+)["\']@i', $res, $matches)) {
        $matches = array();
        serendipity_plugin_api::hook_event('backend_trackback_check', $matches, $loc);
        
        // Plugins may say that a URI is valid, in situations where a blog has no valid RDF metadata
        if (empty($matches[2])) {
            echo '<div>&#8226; ' . sprintf(TRACKBACK_FAILED, TRACKBACK_NOT_FOUND) . '</div>';
            return false;
        }
    }

    $trackURI = trim($matches[2]);

    if (preg_match('@dc:identifier\s*=\s*["\'](https?:[^\'"]+)["\']@i', $res, $test)) {
        if ($loc != $test[1] && $loc2 != $test[1]) {
            echo '<div>&#8226; ' . sprintf(TRACKBACK_FAILED, TRACKBACK_URI_MISMATCH) . '</div>';
            return false;
        }
    }

    $data = 'url='        . rawurlencode($url)
          . '&title='     . rawurlencode($title)
          . '&blog_name=' . rawurlencode($author)
          . '&excerpt='   . rawurlencode(strip_tags($text));

    printf(TRACKBACK_SENDING, htmlspecialchars($trackURI));
    flush();

    $response = serendipity_trackback_is_success(_serendipity_send($trackURI, $data));

    if ($response === true) {
        echo '<div>&#8226; ' . TRACKBACK_SENT .'</div>';
    } else {
        echo '<div>&#8226; ' . sprintf(TRACKBACK_FAILED, $response) . '</div>';
    }

    return $response;
}

function serendipity_reference_autodiscover($loc, $url, $author, $title, $text) {
global $serendipity;
    $timeout   = 30;

    $u = parse_url($loc);

    if ($u['scheme'] != 'http' && $u['scheme'] != 'https') {
        return;
    } elseif ($u['scheme'] == 'https' && !extension_loaded('openssl')) {
        return; // Trackbacks to HTTPS URLs can only be performed with openssl activated
    }

    echo '<div>&#8226; '. sprintf(TRACKBACK_CHECKING, $loc) .'</div>';
    flush();

    if (empty($u['port'])) {
        $u['port'] = 80;
        $port      = '';
    } else {
        $port      = ':' . $u['port'];
    }

    if (!empty($u['query'])) {
        $u['path'] .= '?' . $u['query'];
    }

    $parsed_loc = $u['scheme'] . '://' . $u['host'] . $port . $u['path'];

    $res = '';

    $fp  = @fsockopen($u['host'], $u['port'], $err, $timeout);
    if (!$fp) {
        echo '<div>&#8226; ' . sprintf(TRACKBACK_COULD_NOT_CONNECT, $u['host'], $u['port']) .'</div>';
        return;
    } else {
        fputs($fp, "GET {$u['path']} HTTP/1.0\r\n");
        fputs($fp, "Host: {$u['host']}\r\n");
        fputs($fp, "User-Agent: Serendipity/{$serendipity['version']}\r\n");
        fputs($fp, "Connection: close\r\n\r\n");

        while ($fp && !feof($fp) && strlen($res) < $serendipity['trackback_filelimit']) {
            $res .= fgets($fp, 4096);
        }
        fclose($fp);

        if (strlen($res) >= $serendipity['trackback_filelimit']) {
            echo '<div>&#8226; ' . sprintf(TRACKBACK_SIZE, $serendipity['trackback_filelimit']) .'</div>';
            return;
        }
    }

    if (strlen($res) != 0) {
        serendipity_trackback_autodiscover($res, $parsed_loc, $url, $author, $title, $text, $loc);
        serendipity_pingback_autodiscover($loc, $res);
    } else {
        echo '<div>&#8226; ' . TRACKBACK_NO_DATA . '</div>';
    }
    echo '<hr noshade="noshade" />';
}

/**
 *
 */
function add_trackback ($id, $title, $url, $name, $excerpt) {
    global $serendipity;

    // We can't accept a trackback if we don't get any URL
    // This is a protocol rule.
    if ( empty($url) ) {
        return 0;
    }

    // If title is not provided, the value for url will be set as the title
    // This is a protocol rule.
    if ( empty($title) ) {
        $title = $url;
    }
    $comment['title']   = $title;
    $comment['url']     = $url;
    $comment['name']    = $name;
    $comment['comment'] = $excerpt;

    serendipity_saveComment($id, $comment, 'TRACKBACK');

    return 1;
}

function add_pingback ($id, $postdata) {
    global $serendipity;

    if(preg_match('@<methodcall>\s*<methodName>\s*pingback.ping\s*</methodName>\s*<params>\s*<param>\s*<value>\s*<string>([^<])*</string>\s*</value>\s*</param>\s*<param>\s*<value>\s*<string>([^<])*</string>\s*</value>\s*</param>\s*</params>\s*</methodCall>@i', $postdata, $matches)) {
        $remote             = $matches[1];
        $local              = $matches[2];
        $comment['title']   = '';
        $comment['url']     = $remote;
        $comment['comment'] = '';
        $comment['name']    = '';

        serendipity_saveComment($id, $comment, 'PINGBACK');
        return 1;
    }
    return 0;
}

/**
 * Cut text
 */
function serendipity_trackback_excerpt($text) {
    return substr(strip_tags($text), 0, 255);
}

function report_trackback_success () {
print '<?xml version="1.0" encoding="iso-8859-1"?>' . "\n";
print <<<SUCCESS
<response>
    <error>0</error>
</response>
SUCCESS;
}

function report_trackback_failure () {
print '<?xml version="1.0" encoding="iso-8859-1"?>' . "\n";
print <<<FAILURE
<response>
    <error>1</error>
    <message>Danger Will Robinson, trackback failed.</message>
</response>
FAILURE;
}

function report_pingback_success () {
print '<?xml version="1.0"?>' . "\n";
print <<<SUCCESS
<methodResponse>
   <params>
      <param>
         <value><string>success</string></value>
         </param>
      </params>
   </methodResponse>
SUCCESS;
}

function report_pingback_failure () {
print '<?xml version="1.0"?>' . "\n";
print <<<FAILURE
<methodResponse>
    <fault>
    <value><i4>0</i4></value>
    </fault>
</methodResponse>
FAILURE;
}

/**
 * search through link body, and automatically send a trackback ping.
 */
function serendipity_handle_references($id, $author, $title, $text) {
    global $serendipity;

    if (!preg_match_all('@<a[^>]+?href\s*=\s*["\']?([^\'" >]+?)[ \'"][^>]*>(.+?)</a>@i', $text, $matches)) {
        $matches = array(0 => array(), 1 => array());
    } else {
        // remove full matches
        array_shift($matches);
    }

    // Make trackback URL
    $url = serendipity_archiveURL($id, $title, 'baseURL');

    // Add URL references
    $locations = $matches[0];
    $names     = $matches[1];

    $tmpid = serendipity_db_escape_string($id);

    $checked_locations = array();
    serendipity_plugin_api::hook_event('backend_trackbacks', $locations);
    for ($i = 0, $j = count($locations); $i < $j; ++$i) {
        if ($locations[$i][0] == '/') {
            $locations[$i] = 'http' . (!empty($_SERVER['HTTPS']) && strtolower($_SERVER['HTTPS']) != 'off' ? 's' : '') . '://' . $_SERVER['HTTP_HOST'] . $locations[$i];
        }

        if (isset($checked_locations[$locations[$i]])) {
            continue;
        }

        if (preg_match_all('@<img[^>]+?alt=["\']?([^\'">]+?)[\'"][^>]+?>@i', $names[$i], $img_alt)) {
            if (is_array($img_alt) && is_array($img_alt[0])) {
                foreach($img_alt[0] as $alt_idx => $alt_img) {
                    // Replace all <img>s within a link with their respective ALT tag, so that references
                    // can be stored with a title.
                    $names[$i] = str_replace($alt_img, $img_alt[1][$alt_idx], $names[$i]);
                }
            }
        }

        $query = "SELECT COUNT(id) FROM {$serendipity['dbPrefix']}references
                                  WHERE entry_id = '". (int)$tmpid ."'
                                    AND link = '" . serendipity_db_escape_string($locations[$i]) . "'";

        $row = serendipity_db_query($query, true, 'num');
        if ($row[0] > 0) {
            continue;
        }

        if (!isset($serendipity['noautodiscovery']) || !$serendipity['noautodiscovery']) {
            serendipity_reference_autodiscover($locations[$i], $url, $author, $title, serendipity_trackback_excerpt($text));
            $checked_locations[$locations[$i]] = true; // Store trackbacked link so that no further trackbacks will be sent to the same link
        }
    }
    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}references WHERE entry_id='" . (int)$tmpid . "'");

    for ($i = 0; $i < $j; ++$i) {
        $query = "INSERT INTO {$serendipity['dbPrefix']}references (entry_id, name, link) VALUES(";
        $query .= "'" . (int)$tmpid . "', '" . serendipity_db_escape_string(strip_tags($names[$i])) . "', '";
        $query .= serendipity_db_escape_string($locations[$i]) . "')";

        serendipity_db_query($query);
    }

    // Add citations
    preg_match_all('@<cite[^>]*>([^<]+)</cite>@i', $text, $matches);

    foreach ($matches[1] as $citation) {
        $query = "INSERT INTO {$serendipity['dbPrefix']}references (entry_id, name) VALUES(";
        $query .= "'" . (int)$tmpid . "', '" . serendipity_db_escape_string($citation) . "')";

        serendipity_db_query($query);
    }
}
