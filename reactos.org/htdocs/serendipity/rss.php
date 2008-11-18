<?php # $Id: rss.php 712 2005-11-17 19:54:06Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

header('Content-Type: text/xml; charset=utf-8');
session_cache_limiter('public');
include_once('serendipity_config.inc.php');
include_once(S9Y_INCLUDE_PATH . 'include/functions_rss.inc.php');

$version         = $_GET['version'];
$description     = $serendipity['blogDescription'];
$title           = $serendipity['blogTitle'];
$comments        = FALSE;

if (empty($version)) {
    list($version) = serendipity_discover_rss($_GET['file'], $_GET['ext']);
}

if (isset($_GET['category'])) {
    $serendipity['GET']['category'] = $_GET['category'];
}

if (isset($_GET['viewAuthor'])) {
    $serendipity['GET']['viewAuthor'] = $_GET['viewAuthor'];
}

if (!isset($_GET['type'])) {
    $_GET['type'] = 'content';
}

if (!isset($_GET['nocache'])) {
    switch ($_GET['type']) {
    case 'comments':
            $latest_entry = serendipity_fetchComments(isset($_GET['cid']) ? $_GET['cid'] : null, 1, 'desc');
        break;
    case 'content':
    default:
        $latest_entry = serendipity_fetchEntries(null, false, 1, false, false, 'last_modified DESC', '', false, true);
        break;
    }

    /*
     * Caching logic - Do not send feed if nothing has changed
     * Implementation inspired by Simon Willison [http://simon.incutio.com/archive/2003/04/23/conditionalGet], Thiemo Maettig
     */

    // See if the client has provided the required headers.
    // Always convert the provided header into GMT timezone to allow comparing to the server-side last-modified header
    $modified_since = !empty($_SERVER['HTTP_IF_MODIFIED_SINCE'])
                    ? gmdate('D, d M Y H:i:s \G\M\T', serendipity_serverOffsetHour(strtotime(stripslashes($_SERVER['HTTP_IF_MODIFIED_SINCE'])), true))
                    : false;
    $none_match     = !empty($_SERVER['HTTP_IF_NONE_MATCH'])
                    ? str_replace('"', '', stripslashes($_SERVER['HTTP_IF_NONE_MATCH']))
                    : false;

    if (is_array($latest_entry) && isset($latest_entry[0]['last_modified'])) {
        $last_modified = gmdate('D, d M Y H:i:s \G\M\T', serendipity_serverOffsetHour($latest_entry[0]['last_modified'], true));
        $etag          = '"' . $last_modified . '"';

        header('Last-Modified: ' . $last_modified);
        header('ETag: '          . $etag);

        if (($none_match == $last_modified && $modified_since == $last_modified) ||
            (!$none_match && $modified_since == $last_modified) ||
            (!$modified_since && $none_match == $last_modified)) {
            header('HTTP/1.0 304 Not Modified');
            return;
        }
    }
}

switch ($_GET['type']) {
case 'comments':
    $entries     = serendipity_fetchComments(isset($_GET['cid']) ? $_GET['cid'] : null, 15, 'desc');
    $title       = $title . ' ' . COMMENTS;
    $description = COMMENTS_FROM . ' ' . $description;
    $comments    = TRUE;
    break;
case 'content':
default:
    if (isset($_GET['all']) && $_GET['all']) {
        // Fetch all entries in reverse order for later importing. Fetch sticky entries as normal entries.
        $entries = serendipity_fetchEntries(null, true, '', false, false, 'id ASC', '', false, true);
    } else {
        $entries = serendipity_fetchEntries(null, true, 15, false, (isset($modified_since) ? $modified_since : false), 'timestamp DESC', '', false, true);
    }
    break;
}

if (!empty($serendipity['GET']['category'])) {
    $cInfo       = serendipity_fetchCategoryInfo((int)$serendipity['GET']['category']);
    $title       = serendipity_utf8_encode(htmlspecialchars($title . ' - '. $cInfo['category_name']));
} elseif (!empty($serendipity['GET']['viewAuthor'])) {
    list($aInfo) = serendipity_fetchAuthor((int)$serendipity['GET']['viewAuthor']);
    $title       = serendipity_utf8_encode(htmlspecialchars($aInfo['realname'] . ' - '. $title ));
} else {
    $title       = serendipity_utf8_encode(htmlspecialchars($title));
}

$description = serendipity_utf8_encode(htmlspecialchars($description));

$metadata = array(
    'title'             => $title,
    'description'       => $description,
    'language'          => $serendipity['lang'],
    'additional_fields' => array(),
    'link'              => $serendipity['baseURL'],
    'email'             => $serendipity['email'],
    'fullFeed'          => false,
    'showMail'          => false,
    'version'           => $version
);

include_once(S9Y_INCLUDE_PATH . 'include/plugin_api.inc.php');
$plugins = serendipity_plugin_api::enum_plugins();

if (is_array($plugins)) {
    // load each plugin to make some introspection
    foreach ($plugins as $plugin_data) {
        if (preg_match('|@serendipity_syndication_plugin|', $plugin_data['name'])) {
            $plugin =& serendipity_plugin_api::load_plugin($plugin_data['name'], $plugin_data['authorid']);

            $metadata['additional_fields'] = $plugin->generate_rss_fields($metadata['title'], $metadata['description'], $entries);
            $metadata['fullFeed']          = $plugin->get_config('fullfeed', false);
            $metadata['showMail']          = serendipity_db_bool($plugin->get_config('show_mail', $metadata['showMail']));
            break;
        }
    }
}

serendipity_plugin_api::hook_event('frontend_rss', $metadata);

$self_url = 'http://' . $_SERVER['HTTP_HOST'] . htmlspecialchars($_SERVER['REQUEST_URI']);
echo '<?xml version="1.0" encoding="utf-8" ?>';

if (strstr($version, 'atom')) {
    echo "\n" . '<?xml-stylesheet href="' . serendipity_getTemplateFile('atom.css') . '" type="text/css" ?>';
}

echo "\n";
switch ($version) {
case '0.91':
    print <<<HEAD
<rss version="0.91">
<channel>
<title>{$metadata['title']}</title>
<link>{$metadata['link']}</link>
<description>{$metadata['description']}</description>
<language>{$metadata['language']}</language>
{$metadata['additional_fields']['image']}

HEAD;
break;

case '1.0':
    $rdf_seq_li = "\n";
    if (is_array($entries)) {
        foreach($entries AS $entry) {
            $rdf_seq_li .= '        <rdf:li resource="' . serendipity_rss_getguid($entry, $comments) . '" />' . "\n";
        }

        $entries['display_dat'] = '';
        serendipity_plugin_api::hook_event('frontend_display:rss-1.0:namespace', $entries);
    } else {
        $entries = array();
    }
    
    if ($metadata['showMail']) {
        $head_mail = "<admin:errorReportsTo rdf:resource=\"mailto:{$metadata['email']}\" />";
    } else {
        $head_mail = '';
    }

    print <<<HEAD
<rdf:RDF {$entries['display_dat']}
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:admin="http://webns.net/mvcb/"
   xmlns:content="http://purl.org/rss/1.0/modules/content/"
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:slash="http://purl.org/rss/1.0/modules/slash/"
   xmlns:wfw="http://wellformedweb.org/CommentAPI/"
   xmlns="http://my.netscape.com/rdf/simple/0.9/">
<channel>
    <title>{$metadata['title']}</title>
    <link>{$metadata['link']}</link>
    <description>{$metadata['description']}</description>
    <dc:language>{$metadata['language']}</dc:language>
    $head_mail

    {$metadata['additional_fields']['image_rss1.0_channel']}

    <items>
      <rdf:Seq>{$rdf_seq_li}</rdf:Seq>
    </items>
</channel>

{$metadata['additional_fields']['image_rss1.0_rdf']}

HEAD;
break;

case '2.0':
    if (is_array($entries)) {
        $entries['display_dat'] = '';
        serendipity_plugin_api::hook_event('frontend_display:rss-2.0:namespace', $entries);
    } else {
        $entries = array();
    }

    if ($metadata['showMail']) {
        $head_mail = "<admin:errorReportsTo rdf:resource=\"mailto:{$metadata['email']}\" />";
    } else {
        $head_mail = '';
    }

    print <<<HEAD
<rss version="2.0"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:admin="http://webns.net/mvcb/"
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:slash="http://purl.org/rss/1.0/modules/slash/"
   xmlns:wfw="http://wellformedweb.org/CommentAPI/"
   xmlns:content="http://purl.org/rss/1.0/modules/content/"
   {$entries['display_dat']}>
<channel>
    <title>{$metadata['title']}</title>
    <link>{$metadata['link']}</link>
    <description>{$metadata['description']}</description>
    <dc:language>{$metadata['language']}</dc:language>
    $head_mail
    <generator>Serendipity {$serendipity['version']} - http://www.s9y.org/</generator>
    {$metadata['additional_fields']['channel']}
    {$metadata['additional_fields']['image']}

HEAD;
break;

case 'atom0.3':
    if (is_array($entries)) {
        $modified = gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entries[0]['last_modified']));
    } else {
        $modified = gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour());
    }

    if ($metadata['showMail']) {
        $head_mail = "<admin:errorReportsTo rdf:resource=\"mailto:{$metadata['email']}\" />";
    } else {
        $head_mail = '';
    }

    print <<<HEAD
<feed version="0.3"
   xmlns="http://purl.org/atom/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:admin="http://webns.net/mvcb/"
   xmlns:slash="http://purl.org/rss/1.0/modules/slash/"
   xmlns:wfw="http://wellformedweb.org/CommentAPI/">
    <link href="{$serendipity['baseURL']}rss.php?version=atom0.3" rel="service.feed" title="{$metadata['title']}" type="application/x.atom+xml" />
    <link href="{$serendipity['baseURL']}"                        rel="alternate"    title="{$metadata['title']}" type="text/html" />
    <link href="{$serendipity['baseURL']}rss.php?version=2.0"     rel="alternate"    title="{$metadata['title']}" type="application/rss+xml" />
    <title mode="escaped" type="text/html">{$metadata['title']}</title>
    <tagline mode="escaped" type="text/html">{$metadata['description']}</tagline>
    <id>{$metadata['link']}</id>
    <modified>$modified</modified>
    <generator url="http://www.s9y.org/" version="{$serendipity['version']}">Serendipity {$serendipity['version']} - http://www.s9y.org/</generator>
    <dc:language>{$metadata['language']}</dc:language>
    $head_mail
    <info mode="xml" type="text/html">
        <div xmlns="http://www.w3.org/1999/xhtml">You are viewing an ATOM formatted XML site feed. Usually this file is inteded to be viewed in an aggregator or syndication software. If you want to know more about ATOM, please visist <a href="http://atomenabled.org/">Atomenabled.org</a></div>
    </info>

HEAD;
break;

case 'atom1.0':
    if (is_array($entries)) {
        $modified = gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entries[0]['last_modified']));
    } else {
        $modified = gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour());
    }

    if ($metadata['showMail']) {
        $head_mail = "<admin:errorReportsTo rdf:resource=\"mailto:{$metadata['email']}\" />";
    } else {
        $head_mail = '';
    }

    print <<<HEAD
<feed
   xmlns="http://www.w3.org/2005/Atom"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:admin="http://webns.net/mvcb/"
   xmlns:slash="http://purl.org/rss/1.0/modules/slash/"
   xmlns:wfw="http://wellformedweb.org/CommentAPI/">
    <link href="{$self_url}" rel="self" title="{$metadata['title']}" type="application/x.atom+xml" />
    <link href="{$serendipity['baseURL']}"                        rel="alternate"    title="{$metadata['title']}" type="text/html" />
    <link href="{$serendipity['baseURL']}rss.php?version=2.0"     rel="alternate"    title="{$metadata['title']}" type="application/rss+xml" />
    <title type="html">{$metadata['title']}</title>
    <subtitle type="html">{$metadata['description']}</subtitle>
    {$metadata['additional_fields']['image_atom1.0']}
    <id>{$metadata['link']}</id>
    <updated>$modified</updated>
    <generator uri="http://www.s9y.org/" version="{$serendipity['version']}">Serendipity {$serendipity['version']} - http://www.s9y.org/</generator>
    <dc:language>{$metadata['language']}</dc:language>
    $head_mail

HEAD;
break;

case 'opml1.0':
    if (is_array($entries)) {
        $modified = gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour($entries[0]['last_modified']));
    } else {
        $modified = gmdate('Y-m-d\TH:i:s\Z', serendipity_serverOffsetHour());
    }
    print <<<HEAD
<opml version="{$version}">
<head>
    <title>{$metadata['title']}</title>
    <dateModified>$modified</dateModified>
    <ownerName>Serendipity {$serendipity['version']} - http://www.s9y.org/</ownerName>
</head>
<body>
HEAD;
break;

default:
    die("Invalid RSS version specified\n");
}

unset($entries['display_dat']); // Only needed for headers.
serendipity_printEntries_rss($entries, $version, $comments, $metadata['fullFeed'], $metadata['showMail']);

switch ($version) {
case '0.91':
case '2.0':
    print "</channel>\n";
    print "</rss>\n";
    break;
case '1.0':
    if (is_array($entries)) {
        $entries['display_dat'] = '';
        serendipity_plugin_api::hook_event('frontend_display:rss-1.0:once', $entries);
        echo $entries['display_dat'];
    }
    print '</rdf:RDF>';
    break;
case 'atom1.0':
case 'atom0.3':
    print '</feed>';
    break;
case 'opml1.0':
    print "</body>\n</opml>";
}

/* vim: set sts=4 ts=4 expandtab : */
?>
