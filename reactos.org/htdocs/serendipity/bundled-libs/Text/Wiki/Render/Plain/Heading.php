<?php

class Text_Wiki_Render_Plain_Heading extends Text_Wiki_Render {
    
    function token($options)
    {
        if ($options['type'] == 'end') {
            return "\n\n";
        } else {
            return "\n";
        }
    }
}
?>