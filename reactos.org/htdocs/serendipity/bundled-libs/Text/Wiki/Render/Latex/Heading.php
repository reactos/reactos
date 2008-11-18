<?php

class Text_Wiki_Render_Latex_Heading extends Text_Wiki_Render {

    function token($options)
    {
        // get nice variable names (type, level)
        extract($options);

        if ($type == 'start') {
            switch ($level)
                {
                case '1':
                    return '\part{';
                case '2':
                    return '\section{';
                case '3':
                    return '\subsection{';
                case '4':
                    return '\subsubsection{';
                case '5':
                    return '\paragraph{';
                case '6':
                    return '\subparagraph{';
                }
        }
        
        if ($type == 'end') {
            return "}\n";
        }
    }
}
?>