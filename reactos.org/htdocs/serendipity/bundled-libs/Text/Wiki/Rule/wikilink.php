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
// $Id: wikilink.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as a
* wiki page name and automatically create a link to that page.
*
* Wiki page names are typically in StudlyCapsStyle made of
* WordsSmashedTogether.
*
* You can also create described links to pages in this style:
* [WikiPageName nice text link to use for display]
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_wikilink extends Text_Wiki_Rule {
    
    
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
    
    function Text_Wiki_Rule_wikilink(&$obj, $name)
    {
        parent::Text_Wiki_Rule($obj, $name);
        
        $this->regex =
            "(!?" .              // START WikiPage pattern (1)
            "[A-Z]" .            // 1 upper
            "[A-Za-z]*" .        // 0+ alpha
            "[a-z]+" .           // 1+ lower
            "[A-Z]" .            // 1 upper
            "[A-Za-z]*" .        // 0+ or more alpha
            ")" .                // END WikiPage pattern (/1)
            "((\#" .             // START Anchor pattern (2)(3)
            "[A-Za-z]" .         // 1 alpha
            "(" .                // start sub pattern (4)
            "[-A-Za-z0-9_:.]*" . // 0+ dash, alpha, digit, underscore, colon, dot
            "[-A-Za-z0-9_]" .    // 1 dash, alpha, digit, or underscore
            ")?)?)";             // end subpatterns (/4)(/3)(/2)
    }
    
    
    /**
    * 
    * First parses for described links, then for standalone links.
    * 
    * @access public
    * 
    * @return void
    * 
    */
    
    function parse()
    {
    	// described wiki links
    	$tmp_regex = '/\[' . $this->regex . ' (.+?)\]/';
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'processDescr'),
            $this->_wiki->_source
        );
    	
        // standalone wiki links
        $tmp_regex = '/(^|[^A-Za-z0-9\-_])' . $this->regex . '/';
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'process'),
            $this->_wiki->_source
        );
    }
    
    
    /**
    * 
    * Generates a replacement for described links.  Token options are:
    * 
    * 'page' => the wiki page name.
    * 
    * 'text' => the displayed link text.
    * 
    * 'anchor' => a named anchor on the target wiki page.
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
        // set the options
        $options = array(
            'page' => $matches[1],
            'text' => $matches[5],
            'anchor' => $matches[3]
        );
        
        // create and return the replacement token and preceding text
        return $this->addToken($options); // . $matches[7];
    }
    
    
    /**
    * 
    * Generates a replacement for standalone links.  Token options are:
    * 
    * 'page' => the wiki page name.
    * 
    * 'text' => the displayed link text.
    * 
    * 'anchor' => a named anchor on the target wiki page.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token to be used as a placeholder in
    * the source text, plus any text prior to the match.
    *
    */
    
    function process(&$matches)
    {
        // when prefixed with !, it's explicitly not a wiki link.
        // return everything as it was.
        if ($matches[2]{0} == '!') {
            return $matches[1] . substr($matches[2], 1) . $matches[3];
        }
        
        // set the options
        $options = array(
            'page' => $matches[2],
            'text' => $matches[2] . $matches[3],
            'anchor' => $matches[3]
        );
        
        // create and return the replacement token and preceding text
        return $matches[1] . $this->addToken($options);
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
        // make nice variable names (page, anchor, text)
        extract($options);
        
        // does the page exist?
        if (in_array($page, $this->_conf['pages'])) {
        
            // yes, link to the page view, but we have to build
            // the HREF.  we support both the old form where
            // the page always comes at the end, and the new
            // form that uses %s for sprintf()
            $href = $this->_conf['view_url'];
            
            if (strpos($href, '%s') === false) {
            	// use the old form
	            $href = $href . $page . $anchor;
	        } else {
	        	// use the new form
	        	$href = sprintf($href, $page . $anchor);
	        }
	        
            return "<a href=\"$href\">$text</a>";
            
        }
        
		// no, link to a create-page url, but only if new_url is set
		if (! isset($this->_conf['new_url']) ||
			trim($this->_conf['new_url']) == '') {
			return $text;
		} else {
		
            // yes, link to the page view, but we have to build
            // the HREF.  we support both the old form where
            // the page always comes at the end, and the new
            // form that uses sprintf()
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
