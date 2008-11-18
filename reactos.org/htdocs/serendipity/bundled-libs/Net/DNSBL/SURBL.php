<?php
/* vim: set expandtab tabstop=4 shiftwidth=4 softtabstop=4: */
// +----------------------------------------------------------------------+
// | PEAR::Net_DNSBL_SURBL                                                |
// +----------------------------------------------------------------------+
// | Copyright (c) 2004 Sebastian Nohn <sebastian@nohn.net>               |
// +----------------------------------------------------------------------+
// | This source file is subject to version 3.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at the following url:           |
// | http://www.php.net/license/3_0.txt.                                  |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Sebastian Nohn <sebastian@nohn.net>                         |
// +----------------------------------------------------------------------+
//
// $Id: SURBL.php,v 1.4 2004/12/02 14:23:51 nohn Exp $

/**
 * PEAR::Net_DNSBL_SURBL
 *
 * This class acts as interface to the SURBL - Spam URI Realtime Blocklists.
 *
 * Services_SURBL looks up an supplied URI if it's listed in a
 * Spam URI Realtime Blocklists.
 *
 * @author  Sebastian Nohn <sebastian@nohn.net>
 * @package Net_DNSBL
 * @license http://www.php.net/license/3_0.txt
 * @version 0.5.4
 */
require_once dirname(__FILE__) . '/../../Cache/Lite.php';
require_once dirname(__FILE__) . '/../../HTTP/Request.php';
require_once dirname(__FILE__) . '/../CheckIP.php';
require_once dirname(__FILE__) . '/../DNSBL.php';

class Net_DNSBL_SURBL extends Net_DNSBL {

    /**     
     * Array of blacklists.
     *
     * Must have one or more elements.
     *
     * @var    string[]
     * @access protected
     */
    var $blacklists = array('multi.surbl.org');

    /**
     * File containing whitelisted hosts.
     *
     * There are some whitelisted hosts (co.uk for example). This
     * requires the package to not ask the domain name but the host
     * name (spammer.co.uk instead of co.uk).
     * 
     * @var    string
     * @see    $twoLevelCcTld
     * @access protected
     */
    var $doubleCcTldFile = 'http://spamcheck.freeapp.net/two-level-tlds';

    /**
     * Array of whitelisted hosts.
     *
     * @var    array
     * @see    $twoLevelCcTldFile
     * @access private
     */
    var $twoLevelCcTld = array();

    /**
     * Check if the last two parts of the FQDN are whitelisted.
     *
     * @param  string Host to check if it is whitelisted
     * @access protected
     * @return boolean True if the host is whitelisted
     */
    function isDoubleCcTld($fqdn)
    {
        // 30 Days should be way enough
        $options = array(
                         'lifeTime' => '2592000',
                         'automaticSerialization' => true
                         );
        $id = md5($this->doubleCcTldFile);

        $cache = new Cache_Lite($options);
        if ($data = $cache->get($id)) {
            // Cache hit
        } else {
            // Cache miss
            $http = &new HTTP_Request($this->doubleCcTldFile);
            if (!PEAR::isError($http->sendRequest())) {
                $data = $http->getResponseBody();
            }
            $data = explode("\n", $data);
            $data = array_flip($data);
            $cache->save($data, $id);
        } // if 
        if (array_key_exists($fqdn, $data)) {
            return true;
        } else {
            return false;
        } // if
    } // function

    /**
     * Get Hostname to ask for.
     *
     * Performs the following steps:
     *
     * (1) Extract the hostname from the given URI
     * (2) Check if the "hostname" is an ip
     * (3a) IS_IP Reverse the IP (1.2.3.4 -> 4.3.2.1)
     * (3b) IS_FQDN Check if is in "CC-2-level-TLD"
     * (3b1) IS_IN_2LEVEL: we want the last three names
     * (3b2) IS_NOT_2LEVEL: we want the last two names
     * (4) return the FQDN to query.
     *
     * @param  string URL to check. 
     * @access protected
     * @return string Host to lookup
     */
    function getHostForLookup($uri, $blacklist) 
    {
        $host       = '';
        // (1) Extract the hostname from the given URI
        $parsed_uri = parse_url($uri);
        $host       = $parsed_uri['host'];
        // (2) Check if the "hostname" is an ip
        if (Net_CheckIP::check_ip($host)) {
            // (3a) IS_IP Reverse the IP (1.2.3.4 -> 4.3.2.1)
            $host = $this->reverseIp($host);
        } else {
            $host_elements = explode('.', $host);
            while (count($host_elements) > 3) {
                array_shift($host_elements);
            } // while
            $host_3_elements = implode('.', $host_elements);

            $host_elements = explode('.', $host);
            while (count($host_elements) > 2) {
                array_shift($host_elements);
            } // while
            $host_2_elements = implode('.', $host_elements);

            // (3b) IS_FQDN Check if is in "CC-2-level-TLD"
            if ($this->isDoubleCcTld($host_2_elements)) {
                // (3b1) IS_IN_2LEVEL: we want the last three names
                $host = $host_3_elements;
            } else {
                // (3b2) IS_NOT_2LEVEL: we want the last two names
                $host = $host_2_elements;
            } // if
        } // if
        // (4) return the FQDN to query
        $host      .= '.'.$blacklist;
        return $host;
    } // function

} // class
?>