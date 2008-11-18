<?php
// $Id: Delimiter.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find instances of the delimiter
* character already embedded in the source text; it extracts them and replaces
* them with a delimited token, then renders them as the delimiter itself
* when the target format is XHTML.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Delimiter extends Text_Wiki_Parse {
    
    /**
    * 
    * Constructor.  Overrides the Text_Wiki_Parse constructor so that we
    * can set the $regex property dynamically (we need to include the
    * Text_Wiki $delim character.
    * 
    * @param object &$obj The calling "parent" Text_Wiki object.
    * 
    * @param string $name The token name to use for this rule.
    * 
    */
    
    function Text_Wiki_Parse_delimiter(&$obj)
    {
        parent::Text_Wiki_Parse($obj);
        $this->regex = '/' . $this->wiki->delim . '/';
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
        return $this->wiki->addToken(
            $this->rule,
            array('text' => $this->wiki->delim)
        );
    }
}
?>