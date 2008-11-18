<?php
/* vim: set expandtab tabstop=4 shiftwidth=4 softtabstop=4: */
// +----------------------------------------------------------------------+
// | PEAR::Net_DNSBL                                                      |
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
// $Id: DNSBL.php,v 1.4 2004/12/02 14:23:51 nohn Exp $

/**
 * PEAR::Net_DNSBL
 *
 * This class acts as interface to generic Realtime Blocking Lists
 * (RBL)
 *
 * Net_RBL looks up an supplied host if it's listed in 1-n supplied
 * Blacklists
 *
 * @author  Sebastian Nohn <sebastian@nohn.net>
 * @package Net_DNSBL
 * @license http://www.php.net/license/3_0.txt
 * @version 0.5.3
 */
require_once dirname(__FILE__) . '/CheckIP.php';

class Net_DNSBL {

    /**     
     * Array of blacklists.
     *
     * Must have one or more elements.
     *
     * @var    array
     * @access protected
     */
    var $blacklists = array('sbl-xbl.spamhaus.net',
                            'bl.spamcop.net');

    /**
     * Set the blacklist to a desired blacklist.
     *
     * @param  array Array of blacklists to use. May contain only one element.
     * @access public
     * @return bool true if the operation was successful
     */
    function setBlacklists($blacklists)
    {
        if (is_array($blacklists)) {
            $this->blacklists = $blacklists;
            return true;
        } else {
            return false;
        } // if
    } // function

    /**
     * Get the blacklists.
     *
     * @access public
     * @return array Currently set blacklists.
     */
    function getBlacklists()
    {
        return $this->blacklists;
    }

    /** 
     * Checks if the supplied Host is listed in one or more of the
     * RBLs.
     *
     * @param  string Host to check for being listed.
     * @access public
     * @return boolean true if the checked host is listed in a blacklist.
     */
    function isListed($host)
    {
        
        $isListed = false;
        
        foreach ($this->blacklists as $blacklist) {
            $result = gethostbyname($this->getHostForLookup($host, $blacklist));
            if ($result != $this->getHostForLookup($host, $blacklist)) { 
                $isListed = true;
                
                //if the Host was listed we don't need to check other RBLs,
                break;
                
            } // if
        } // foreach
        
        return $isListed;
    } // function

    /** 
     * Get host to lookup. Lookup a host if neccessary and get the
     * complete FQDN to lookup.
     *
     * @param  string Host OR IP to use for building the lookup.
     * @param  string Blacklist to use for building the lookup.
     * @access protected
     * @return string Ready to use host to lookup
     */    
    function getHostForLookup($host, $blacklist) 
    {
        // Currently only works for v4 addresses.
        if (!Net_CheckIP::check_ip($host)) {
            $ip = gethostbyname($host);
        } else {
            $ip = $host;
        }

        return $this->buildLookUpHost($ip, $blacklist);
    } // function

    /**
     * Build the host to lookup from an IP.
     *
     * @param  string IP to use for building the lookup.
     * @param  string Blacklist to use for building the lookup.
     * @access protected
     * @return string Ready to use host to lookup
     */    
    function buildLookUpHost($ip, $blacklist)
    {
        return $this->reverseIp($ip).'.'.$blacklist;        
    } // function

    /**
     * Reverse the order of an IP. 127.0.0.1 -> 1.0.0.127. Currently
     * only works for v4-adresses
     *
     * @param  string IP to reverse.
     * @access protected
     * @return string Reversed IP
     */    
    function reverseIp($ip) 
    {        
        return implode('.', array_reverse(explode('.', $ip)));        
    } // function

} // class
?>