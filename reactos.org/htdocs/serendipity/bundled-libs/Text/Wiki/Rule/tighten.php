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
// $Id: tighten.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* The rule removes all remaining newlines.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_tighten extends Text_Wiki_Rule {
    
    
    /**
    * 
    * Simple parsing method to apply tightening directly to the tokens
    * array.
    *
    * @access public
    * 
    */
    
    function parse()
    {
    	$this->_wiki->_source = str_replace("\n", '',
    		$this->_wiki->_source);
    }
}
?>
