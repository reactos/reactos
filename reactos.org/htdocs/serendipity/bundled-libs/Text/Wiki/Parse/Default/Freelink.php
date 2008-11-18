<?php
// $Id: Freelink.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find source text marked as a
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

class Text_Wiki_Parse_Freelink extends Text_Wiki_Parse {
    
    
    /**
    * 
    * Constructor.  We override the Text_Wiki_Parse constructor so we can
    * explicitly comment each part of the $regex property.
    * 
    * @access public
    * 
    * @param object &$obj The calling "parent" Text_Wiki object.
    * 
    */
    
    function Text_Wiki_Parse_Freelink(&$obj)
    {
        parent::Text_Wiki_Parse($obj);
        
        $this->regex =
            '/' .                                                   // START regex
            "\\(\\(" .                                               // double open-parens
            "(" .                                                   // START freelink page patter
            "[-A-Za-z0-9 _+\\/.,;:!?'\"\\[\\]\\{\\}&\xc0-\xff]+" . // 1 or more of just about any character
            ")" .                                                   // END  freelink page pattern
            "(" .                                                   // START display-name
            "\|" .                                                   // a pipe to start the display name
            "[-A-Za-z0-9 _+\\/.,;:!?'\"\\[\\]\\{\\}&\xc0-\xff]+" . // 1 or more of just about any character
            ")?" .                                                   // END display-name pattern 0 or 1
            "(" .                                                   // START pattern for named anchors
            "\#" .                                                   // a hash mark
            "[A-Za-z]" .                                           // 1 alpha
            "[-A-Za-z0-9_:.]*" .                                   // 0 or more alpha, digit, underscore
            ")?" .                                                   // END named anchors pattern 0 or 1
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
        return $this->wiki->addToken($this->rule, $options);
    }
}
?>