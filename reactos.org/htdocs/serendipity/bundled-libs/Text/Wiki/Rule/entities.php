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
// $Id: entities.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to convert HTML entities in the
* source text.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_entities extends Text_Wiki_Rule {
    
    
    /**
    * 
    * Simple parsing method to apply the rule.
    *
    * @access public
    * 
    */
    
    function parse()
    {
        // first, decode any entities already in the text so that they
        // don't get double-encoded
        $trans_table = get_html_translation_table(HTML_SPECIALCHARS);
        $this->_wiki->_source = strtr($this->_wiki->_source,
            array_flip($trans_table));
            
        // now encode all html special characters
        $this->_wiki->_source = htmlspecialchars($this->_wiki->_source);
    }

}
?>
