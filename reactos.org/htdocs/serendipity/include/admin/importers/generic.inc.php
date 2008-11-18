<?php # $Id: generic.inc.php 703 2005-11-15 13:55:04Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

require_once S9Y_PEAR_PATH . 'Onyx/RSS.php';

class Serendipity_Import_Generic extends Serendipity_Import {
    var $info        = array('software' => IMPORT_GENERIC_RSS);
    var $data        = array();
    var $inputFields = array();

    function Serendipity_Import_Generic($data) {
        $this->data = $data;
        $this->inputFields = array(array('text'    => RSS . ' ' . URL,
                                         'type'    => 'input',
                                         'name'    => 'url'),

                                   array('text'    => STATUS,
                                         'type'    => 'list',
                                         'name'    => 'type',
                                         'value'   => 'publish',
                                         'default' => array('draft' => DRAFT, 'publish' => PUBLISH)),

                                   array('text'    => RSS_IMPORT_CATEGORY,
                                         'type'    => 'list',
                                         'name'    => 'category',
                                         'value'   => 0,
                                         'default' => $this->_getCategoryList()),

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

    function _getCategoryList() {
        $res = serendipity_fetchCategories('all');
        $ret = array(0 => NO_CATEGORY);
        if (is_array($res)) {
            foreach ($res as $v) {
                $ret[$v['categoryid']] = $v['category_name'];
            }
        }
        return $ret;
    }

    function buildEntry($item, &$entry) {
        global $serendipity;

        $entry    = array();
        $bodyonly = serendipity_get_bool($this->data['bodyonly']);

        if ($item['description']) {
            $entry['body'] = $this->decode($item['description']);
        }

        if ($item['content:encoded']) {
            if (!isset($entry['body']) || $bodyonly) {
                $data = &$entry['body'];
            } else {
                $data = &$entry['extended'];
            }

            // See if the 'description' element is a substring of the 'content:encoded' part. If it is,
            // we will only fetch the full 'content:encoded' part. If it's not a substring, we append
            // the 'content:encoded' part to either body or extended entry (respecting the 'bodyonly'
            // switch). We substract 4 letters because of possible '...' additions to an entry.
            $testbody = substr(trim(strip_tags($entry['body'])), 0, -4);
            if ($testbody != substr(trim(strip_tags($item['content:encoded'])), 0, strlen($testbody))) {
                $data .= $this->decode($item['content:encoded']);
            } else {
                $data = $this->decode($item['content:encoded']);
            }
        }

        $entry['title'] = $this->decode($item['title']);
        $entry['timestamp'] = $this->decode(strtotime(isset($item['pubdate']) ? $item['pubdate'] : $item['dc:date']));
        if ($entry['timestamp'] == -1) {
            // strtotime does not seem to parse ISO 8601 dates
            if (preg_match('@^([0-9]{4})\-([0-9]{2})\-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})[\-\+]([0-9]{2}):([0-9]{2})$@', isset($item['pubdate']) ? $item['pubdate'] : $item['dc:date'], $timematch)) {
                $entry['timestamp'] = mktime($timematch[4] - $timematch[7], $timematch[5] - $timematch[8], $timematch[6], $timematch[2], $timematch[3], $timematch[1]);
            } else {
                $entry['timestamp'] = time();
            }
        }

        if ($this->data['type'] == 'draft') {
            $entry['isdraft'] = 'true';
        } else {
            $entry['isdraft'] = 'false';
        }

        if (!empty($item['category'])) {
            $cat = serendipity_fetchCategoryInfo(0, trim($this->decode($item['category'])));
            if (is_array($cat) && isset($cat['categoryid'])) {
                $entry['categories'][] = $cat['categoryid'];
            }
        }

        if (!is_array($entry['categories'])) {
            $entry['categories'][] = $this->data['category'];
        }

        if (!isset($entry['extended'])) {
            $entry['extended'] = '';
        }

        $entry['allow_comments'] = true;

        return true;
    }

    function import() {
        global $serendipity;

        $c = &new Onyx_RSS($this->data['charset']);
        $c->parse($this->data['url']);
        $this->data['encoding'] = $c->rss['encoding'];

        $serendipity['noautodiscovery'] = 1;
        while ($item = $c->getNextItem()) {
            if ($this->buildEntry($item, $entry)) {
                serendipity_updertEntry($entry);
            }
        }

        return true;
    }
}

return 'Serendipity_Import_Generic';

/* vim: set sts=4 ts=4 expandtab : */
?>
