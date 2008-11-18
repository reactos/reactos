// I18N constants

// LANG: "it", ENCODING: UTF-8 | ISO-8859-1
// Author: Fabio Rotondo <fabio@rotondo.it>
// Update for 3.0 rc1: Giovanni Premuda <gpremuda@softwerk.it>

HTMLArea.I18N = {

        // the following should be the filename without .js extension
        // it will be used for automatically load plugin language.
        lang: "it",

        tooltips: {
                bold:           "Grassetto",
                italic:         "Corsivo",
                underline:      "Sottolineato",
                strikethrough:  "Barrato",
                subscript:      "Pedice",
                superscript:    "Apice",
                justifyleft:    "Allinea a sinistra",
                justifycenter:  "Allinea in centro",
                justifyright:   "Allinea a destra",
                justifyfull:    "Giustifica",
                insertorderedlist:    "Lista ordinata",
                insertunorderedlist:  "Lista puntata",
                outdent:        "Decrementa indentazione",
                indent:         "Incrementa indentazione",
                forecolor:      "Colore del carattere",
                hilitecolor:    "Colore di sfondo",
                inserthorizontalrule: "Linea orizzontale",
                createlink:     "Inserisci un link",
                insertimage:    "Inserisci un'immagine",
                inserttable:    "Inserisci una tabella",
                htmlmode:       "Visualizzazione HTML",
                popupeditor:    "Editor a pieno schermo",
                about:          "Info sull'editor",
                showhelp:       "Aiuto sull'editor",
                textindicator:  "Stile corrente",
                undo:           "Annulla",
                redo:           "Ripristina",
                cut:            "Taglia",
                copy:           "Copia",
                paste:          "Incolla",
                lefttoright:    "Scrivi da sinistra a destra",
                righttoleft:    "Scrivi da destra a sinistra"
        },

        buttons: {
                "ok":           "OK",
                "cancel":       "Annulla"
        },

        msg: {
                "Path":         "Percorso",
                "TEXT_MODE":    "Sei in MODALITA' TESTO. Usa il bottone [<>] per tornare alla modalità WYSIWYG.",
                "IE-sucks-full-screen" :
                // translate here
                "The full screen mode is known to cause problems with Internet Explorer, " +
                "due to browser bugs that we weren't able to workaround.  You might experience garbage " +
                "display, lack of editor functions and/or random browser crashes.  If your system is Windows 9x " +
                "it's very likely that you'll get a 'General Protection Fault' and need to reboot.\n\n" +
                "You have been warned.  Please press OK if you still want to try the full screen editor."
        },

        dialogs: {
                "Annulla"                                            : "Cancel",
                "Inserisci/modifica Link"                                : "Insert/Modify Link",
                "Nuova finestra (_blank)"                               : "New window (_blank)",
                "Nessuno (usa predefinito)"                               : "None (use implicit)",
                "OK"                                                : "OK",
                "Altro"                                             : "Other",
                "Stessa finestra (_self)"                                : "Same frame (_self)",
                "Target:"                                           : "Target:",
                "Title (suggerimento):"                                  : "Title (tooltip):",
                "Frame principale (_top)"                                  : "Top frame (_top)",
                "URL:"                                              : "URL:",
                "You must enter the URL where this link points to"  : "Devi inserire un indirizzo per questo link"
        }
};
