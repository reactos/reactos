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
// $Id: freelink.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as a
* wiki freelink, and automatically create a link to that page.
* 
* A freelink is any page name not conforming to the standard
* StudlyCapsStyle for a wiki page name.  For example, a page normally
* named MyHomePage can be renamed and referred to as ((My Home Page)) --
* note the spaces in the page name.  You can also make a "nice-looking"
* link without renaming the target page; e.g., ((MyHomePage|My Home
* Page)).  Finally, you can use named anchors on the target page:
* ((MyHomePage|My Home Page#Section1)).
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_freelink extends Text_Wiki_Rule {
    
    
    /**
    * 
    * Constructor.  We override the Text_Wiki_Rule constructor so we can
    * explicitly comment each part of the $regex property.
    * 
    * @access public
    * 
    * @param object &$obj The calling "parent" Text_Wiki object.
    * 
    * @param string $name The token name to use for this rule.
    * 
    */
    
    function Text_Wiki_Rule_freelink(&$obj, $name)
    {
        parent::Text_Wiki_Rule($obj, $name);
        
        $this->regex =
            '/' .                                                  // START regex
            "\\(\\(" .                                             // double open-parens
            "(" .                                                  // START freelink page patter
            "[-A-Za-z0-9 _+\\/.,;:!?'\"\\[\\]\\{\\}&\xc0-\xff]+" . // 1 or more of just about any character
            ")" .                                                  // END  freelink page pattern
            "(" .                                                  // START display-name
            "\|" .                                                 // a pipe to start the display name
            "[-A-Za-z0-9 _+\\/.,;:!?'\"\\[\\]\\{\\}&\xc0-\xff]+" . // 1 or more of just about any character
            ")?" .                                                 // END display-name pattern 0 or 1
            "(" .                                                  // START pattern for named anchors
            "\#" .                                                 // a hash mark
            "[A-Za-z]" .                                           // 1 alpha
            "[-A-Za-z0-9_:.]*" .                                   // 0 or more alpha, digit, underscore
            ")?" .                                                 // END named anchors pattern 0 or 1
            "()\\)\\)" .                                           // double close-parens
            '/';                                                   // END regex
    }
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'page' => the wiki page name (e.g., HomePage).
    * 
    * 'text' => alternative text to be displayed in place of the wiki
    * page name.
    * 
    * 'anchor' => a named anchor on the target wiki page
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
        // use nice variable names
        $page = $matches[1];
        $text = $matches[2];
        
        // get rid of the leading # from the anchor, if any
        $anchor = substr($matches[3], 1);
        
        // is the page given a new text appearance?
        if (trim($text) == '') {
            // no
            $text = $page;
        } else {
            // yes, strip the leading | character
            $text = substr($text, 1);
        }
        
        // set the options
        $options = array(
            'page'   => $page,
            'text'   => $text,
            'anchor' => $anchor
        );
        
        // return a token placeholder
        return $this->addToken($options);
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
        // get nice variable names (page, text, anchor)
        extract($options);
        
        if (in_array($page, $this->_conf['pages'])) {
        
            // the page exists, show a link to the page
            $href = $this->_conf['view_url'];
            if (strpos($href, '%s') === false) {
            	// use the old form
	            $href = $href . $page . '#' . $anchor;
	        } else {
	        	// use the new form
	        	$href = sprintf($href, $page . '#' . $anchor);
	        }
            return "<a href=\"$href\">$text</a>";
            
        } else {
        
            // the page does not exist, show the page name and
            // the "new page" text
            $href = $this->_conf['new_url'];
            if (strpos($href, '%s') === false) {
            	// use the old form
	            $href = $href . $page;
	        } else {
	        	// use the new form
	        	$href = sprintf($href, $page);
	        }
            return $text . "<a href=\"$href\">{$this->_conf['new_text']}</a>";
            
        }
    }
}
?>
