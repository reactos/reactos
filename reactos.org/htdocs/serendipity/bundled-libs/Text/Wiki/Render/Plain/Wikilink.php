<?php

class Text_Wiki_Render_Plain_Wikilink extends Text_Wiki_Render {
    
    
    /**
    * 
    * Renders a token into plain text.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function token($options)
    {
        return $options['text'];
    }
}
?>