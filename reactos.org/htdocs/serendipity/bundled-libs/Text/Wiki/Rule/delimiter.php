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
// $Id: delimiter.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find instances of the delimiter
* character already embedded in the source text; it extracts them and replaces
* them with a delimited token, then renders them as the delimiter itself
* when the target format is XHTML.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_delimiter extends Text_Wiki_Rule {
    
    /**
    * 
    * Constructor.  Overrides the Text_Wiki_Rule constructor so that we
    * can set the $regex property dynamically (we need to include the
    * Text_Wiki $delim character.
    * 
    * @param object &$obj The calling "parent" Text_Wiki object.
    * 
    * @param string $name The token name to use for this rule.
    * 
    */
    
    function Text_Wiki_Rule_delimiter(&$obj, $name)
    {
        parent::Text_Wiki_Rule($obj, $name);
        $this->regex = '/' . $this->_wiki->delim . '/';
    }
    
    
    /**
    * 
    * Generates a token entry for the matched text.  Token options are:
    * 
    * 'text' => The full matched text.
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
        return $this->addToken();
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
        return $this->_wiki->delim;
    }
}
?>
