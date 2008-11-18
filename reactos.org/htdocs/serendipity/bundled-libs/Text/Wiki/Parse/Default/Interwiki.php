<?php
// $Id: Interwiki.php,v 1.2 2005/02/01 03:47:37 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find source text marked as
* an Interwiki link.  See the regex for a detailed explanation of the
* text matching procedure; e.g., "InterWikiName:PageName".
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Interwiki extends Text_Wiki_Parse {
    
    // double-colons wont trip up now
    var $regex = '([A-Za-z0-9_]+):((?!:)[A-Za-z0-9_\/=&~#.:;-]+)';
    
    
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
        $this->wiki->source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'processDescr'),
            $this->wiki->source
        );
        
        // standalone interwiki links
        $tmp_regex = '/' . $this->regex . '/';
        $this->wiki->source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'process'),
            $this->wiki->source
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
        
        return $this->wiki->addToken($this->rule, $options);
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
        
        return $this->wiki->addToken($this->rule, $options);
    }
}
?>