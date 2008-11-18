<?php
// $Id: Center.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find lines marked for centering.
* The line must start with "= " (i.e., an equal-sign followed by a space).
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Center extends Text_Wiki_Parse {
    
    
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
        $start = $this->wiki->addToken(
            $this->rule,
            array('type' => 'start')
        );
        
        $end = $this->wiki->addToken(
            $this->rule,
            array('type' => 'end')
        );
        
        return "\n" . $start . $matches[1] . $end . "\n";
    }
}
?>