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
// $Id: include.php,v 1.2 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to include the results of a
* script directly into the source at parse-time; thus, the output of the
* script will be parsed by Text_Wiki.  This differs from the 'embed'
* rule, which incorporates the results at render-time, meaning that the
* 'embed' content is not parsed by Text_Wiki.
*
* This rule is inherently not secure; it allows cross-site scripting to
* occur if the embedded output has <script> or other similar tags.  Be
* careful.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_include extends Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to find source text matching this
    * rule.
    * 
    * @access public
    * 
    * @var string
    * 
    */
    
    var $regex = '/(\[\[include )(.+?)(\]\])/i';
    
    
    /**
    * 
    * Includes the results of the script directly into the source; the output
    * will subsequently be parsed by the remaining Text_Wiki rules.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return The results of the included script.
    *
    */
    
    function process(&$matches)
    {
    	$file = $this->_conf['base'] . $matches[2];
    	ob_start();
    	include($file);
    	$output = ob_get_contents();
    	ob_end_clean();
		return $output;
    }
}
?>
