// I18N constants

// LANG: "fr", ENCODING: UTF-8 | ISO-8859-1
// Author: Simon Richard, s.rich@sympatico.ca

// FOR TRANSLATORS:
//
//   1. PLEASE PUT YOUR CONTACT INFO IN THE ABOVE LINE
//      (at least a valid email address)
//
//   2. PLEASE TRY TO USE UTF-8 FOR ENCODING;
//      (if this is not possible, please include a comment
//       that states what encoding is necessary.)

// All technical terms used in this document are the ones approved
// by the Office québécois de la langue française.
// Tous les termes techniques utilisés dans ce document sont ceux
// approuvés par l'Office québécois de la langue française.
// http://www.oqlf.gouv.qc.ca/

HTMLArea.I18N = {

	// the following should be the filename without .js extension
	// it will be used for automatically load plugin language.
	lang: "fr",

	tooltips: {
		bold:           "Gras",
		italic:         "Italique",
		underline:      "Souligné",
		strikethrough:  "Barré",
		subscript:      "Indice",
		superscript:    "Exposant",
		justifyleft:    "Aligné à gauche",
		justifycenter:  "Centré",
		justifyright:   "Aligné à droite",
		justifyfull:    "Justifier",
		orderedlist:    "Numérotation",
		unorderedlist:  "Puces",
		outdent:        "Diminuer le retrait",
		indent:         "Augmenter le retrait",
		forecolor:      "Couleur de police",
		hilitecolor:    "Surlignage",
		horizontalrule: "Ligne horizontale",
		createlink:     "Insérer un hyperlien",
		insertimage:    "Insérer/Modifier une image",
		inserttable:    "Insérer un tableau",
		htmlmode:       "Passer au code source",
		popupeditor:    "Agrandir l'éditeur",
		about:          "À propos de cet éditeur",
		showhelp:       "Aide sur l'éditeur",
		textindicator:  "Style courant",
		undo:           "Annuler la dernière action",
		redo:           "Répéter la dernière action",
		cut:            "Couper la sélection",
		copy:           "Copier la sélection",
		paste:          "Coller depuis le presse-papier",
		lefttoright:    "Direction de gauche à droite",
		righttoleft:    "Direction de droite à gauche"
	},

	buttons: {
		"ok":           "OK",
		"cancel":       "Annuler"
	},

	msg: {
		"Path":         "Chemin",
		"TEXT_MODE":    "Vous êtes en MODE TEXTE.  Appuyez sur le bouton [<>] pour retourner au mode tel-tel.",

		"IE-sucks-full-screen" :
		// translate here
		"Le mode plein écran peut causer des problèmes sous Internet Explorer, " +
		"ceci dû à des bogues du navigateur qui ont été impossible à contourner.  " +
		"Les différents symptômes peuvent être un affichage déficient, le manque de " +
		"fonctions dans l'éditeur et/ou pannes aléatoires du navigateur.  Si votre " +
		"système est Windows 9x, il est possible que vous subissiez une erreur de type " +
		"«General Protection Fault» et que vous ayez à redémarrer votre ordinateur." +
		"\n\nConsidérez-vous comme ayant été avisé.  Appuyez sur OK si vous désirez tout " +
		"de même essayer le mode plein écran de l'éditeur."
	},

	dialogs: {
		"Cancel"                                            : "Annuler",
		"Insert/Modify Link"                                : "Insérer/Modifier Lien",
		"New window (_blank)"                               : "Nouvelle fenêtre (_blank)",
		"None (use implicit)"                               : "Aucun (par défaut)",
		"OK"                                                : "OK",
		"Other"                                             : "Autre",
		"Same frame (_self)"                                : "Même cadre (_self)",
		"Target:"                                           : "Cible:",
		"Title (tooltip):"                                  : "Titre (infobulle):",
		"Top frame (_top)"                                  : "Cadre du haut (_top)",
		"URL:"                                              : "Adresse Web:",
		"You must enter the URL where this link points to"  : "Vous devez entrer l'adresse Web du lien"
	}
};
