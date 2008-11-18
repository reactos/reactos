<?php
/* vim: set expandtab tabstop=4 shiftwidth=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at                              |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Paul M. Jones <pmjones@ciaweb.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: interwiki.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as
* an Interwiki link.  See the regex for a detailed explanation of the
* text matching procedure; e.g., "InterWikiName:PageName".
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_interwiki extends Text_Wiki_Rule {
    
    
    var $regex = '([A-Za-z0-9_]+):([\/=&~#A-Za-z0-9_]+)';
    
    
    /**
    * 
    * Parser.  We override the standard parser so we can
    * find both described interwiki links and standalone links.
    * 
    * @access public
    * 
    * @return void
    * 
    */
    
    function parse()
    {
    	// described interwiki links
    	$tmp_regex = '/\[' . $this->regex . ' (.+?)\]/';
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'processDescr'),
            $this->_wiki->_source
        );
        
    	// standalone interwiki links
        $tmp_regex = '/' . $this->regex . '/';
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'process'),
            $this->_wiki->_source
        );
   	
    }
    
    
    /**
    * 
	* Generates a replacement for the matched standalone interwiki text.
	* Token options are:
	* 
	* 'site' => The key name for the Text_Wiki interwiki array map,
	* usually the name of the interwiki site.
	* 
	* 'page' => The page on the target interwiki to link to.
	* 
	* 'text' => The text to display as the link.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token to be used as a placeholder in
    * the source text, plus any text priot to the match.
    *
    */
    
    function process(&$matches)
    {
        $options = array(
            'site' => $matches[1],
            'page' => $matches[2],
            'text' => $matches[0]
        );
        
        // if not in the interwiki map, don't make it an interwiki link
        if (isset($this->_conf['sites'][$options['site']])) {
	        return $this->addToken($options);
	    } else {
	        return $matches[0];
	    }
    }
    
    
    /**
    * 
	* Generates a replacement for described interwiki links. Token
	* options are:
	* 
	* 'site' => The key name for the Text_Wiki interwiki array map,
	* usually the name of the interwiki site.
	* 
	* 'page' => The page on the target interwiki to link to.
	* 
	* 'text' => The text to display as the link.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token to be used as a placeholder in
    * the source text, plus any text priot to the match.
    *
    */
    
    function processDescr(&$matches)
    {
        $options = array(
            'site' => $matches[1],
            'page' => $matches[2],
            'text' => $matches[3]
        );
        
        // if not in the interwiki map, don't make it an interwiki link
        if (isset($this->_conf['sites'][$options['site']])) {
	        return $this->addToken($options);
	    } else {
	        return $matches[0];
	    }
    }
    
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function renderXhtml($options)
    {
        $site = $options['site'];
        $page = $options['page'];
        $text = $options['text'];
        
        if (isset($this->_conf['sites'][$site])) {
            $href = $this->_conf['sites'][$site];
        } else {
            return $text;
        }
        
		// old form where page is at end,
		// or new form with %s placeholder for sprintf()?
		if (strpos($href, '%s') === false) {
			// use the old form
			$href = $href . $page;
		} else {
			// use the new form
			$href = sprintf($href, $page);
		}
		
		// allow for alternative targets
		if (isset($this->_conf['target']) &&
			trim($this->_conf['target']) != '') {
			$target = 'target="' . $this->_conf['target'] . '"';
		} else {
			$target = '';
		}
		
		
        return "<a $target href=\"$href\">$text</a>";
    }
}
?>
