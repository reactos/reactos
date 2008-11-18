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
// $Id: center.php,v 1.2 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find lines marked for centering.
* The line must start with "= " (i.e., an equal-sign followed by a space).
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_center extends Text_Wiki_Rule {
    
    
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
    
    //var $regex = '/\n(\<center\>)\n(.+)\n(\<\/center\>)\n/Umsi';
    var $regex = '/\n\= (.*?)\n/';
    
    /**
    * 
    * Generates a token entry for the matched text.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token number to be used as a placeholder in
    * the source text.
    *
    */
    
    function process(&$matches)
    {
        $start = $this->addToken(array('type' => 'start'));
        $end = $this->addToken(array('type' => 'end'));
        //return "\n" . $start . "\n" . $matches[2] . "\n\n" . $end . "\n";
        return "\n" . $start . $matches[1] . $end . "\n";
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
    	if ($options['type'] == 'start') {
    		//return "\n<center>\n";
    		return '<div style="text-align: center;">';
    	}
    	
    	if ($options['type'] == 'end') {
    		//return "</center>\n";
    		return '</div>';
    	}
    }
}
?>
