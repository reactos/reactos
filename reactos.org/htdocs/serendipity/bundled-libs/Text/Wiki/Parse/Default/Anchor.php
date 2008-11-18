<?php

/**
* 
* This class implements a Text_Wiki_Parse to add an anchor target name
* in the wiki page.
*
* @author Manuel Holtgrewe <purestorm at ggnore dot net>
*
* @author Paul M. Jones <pmjones at ciaweb dot net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Anchor extends Text_Wiki_Parse {
    
    
    /**
    * 
    * The regular expression used to find source text matching this
    * rule.  Looks like a macro: [[# anchor_name]]
    * 
    * @access public
    * 
    * @var string
    * 
    */
    
    var $regex = '/(\[\[# )([-_A-Za-z0-9.]+?)( .+)?(\]\])/i';
    
    
    /**
    * 
    * Generates a token entry for the matched text.  Token options are:
    * 
    * 'text' => The full matched text, not including the <code></code> tags.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token number to be used as a placeholder in
    * the source text.
    *
    */
    
    function process(&$matches) {
    
        $name = $matches[2];
        $text = $matches[3];
        
        $start = $this->wiki->addToken(
            $this->rule,
            array('type' => 'start', 'name' => $name)
        );
        
        $end = $this->wiki->addToken(
            $this->rule,
            array('type' => 'end', 'name' => $name)
        );
        
        // done, place the script output directly in the source
        return $start . trim($text) . $end;
    }
}
?>
