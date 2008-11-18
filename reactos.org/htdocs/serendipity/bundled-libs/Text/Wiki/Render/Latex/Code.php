<?php

class Text_Wiki_Render_Latex_Code extends Text_Wiki_Render {
    
    
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
        $text = $options['text'];

        return "\\begin{verbatim}\n$text\n\\end{verbatim}\n\n";
    }
}
?>