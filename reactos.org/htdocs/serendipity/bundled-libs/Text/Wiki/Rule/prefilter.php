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
// $Id: prefilter.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to "pre-filter" source text so
* that line endings are consistently \n, lines ending in a backslash \
* are concatenated with the next line, and tabs are converted to spaces.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_prefilter extends Text_Wiki_Rule {
    
    
    /**
    * 
    * Simple parsing method to apply the rule directly to the source
    * text.
    *
    * @access public
    * 
    */
    
    function parse()
    {
        // convert DOS line endings
        $this->_wiki->_source = str_replace("\r\n", "\n",
            $this->_wiki->_source);
        
        // convert Macintosh line endings
        $this->_wiki->_source = str_replace("\r", "\n",
            $this->_wiki->_source);
        
        // concat lines ending in a backslash
        $this->_wiki->_source = str_replace("\\\n", "",
            $this->_wiki->_source);
        
        // convert tabs to spaces
        $this->_wiki->_source = str_replace("\t", "    ",
            $this->_wiki->_source);
           
        // add extra newlines at the top and end; this
        // seems to help many rules.
        $this->_wiki->_source = "\n" . $this->_wiki->_source . "\n\n";
        
        // finally, compress all instances of 3 or more newlines
        // down to two newlines.
    	$find = "/\n{3,}/m";
    	$replace = "\n\n";
    	$this->_wiki->_source = preg_replace($find, $replace,
    		$this->_wiki->_source);
    }

}
?>
