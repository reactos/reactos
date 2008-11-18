<?php

class Text_Wiki_Render_Latex_Paragraph extends Text_Wiki_Render {
    
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
    
    function token($options)
    {
        extract($options); //type
        
        if ($type == 'start') {
            return '';
        }
        
        if ($type == 'end') {
            return "\n\n";
        }
    }
}
?>