// I18N constants

// LANG: "pt_br", ENCODING: UTF-8 | ISO-8859-1
// Author: Agner Olson, agner@agner.net for Portuguese brasilian
// João P Matos, jmatos@math.ist.utl.pt for European Portuguese

// FOR TRANSLATORS:
//
//   1. PLEASE PUT YOUR CONTACT INFO IN THE ABOVE LINE
//      (at least a valid email address)
//
//   2. PLEASE TRY TO USE UTF-8 FOR ENCODING;
//      (if this is not possible, please include a comment
//       that states what encoding is necessary.)


HTMLArea.I18N = {

	// the following should be the filename without .js extension
	// it will be used for automatically load plugin language.
	lang: "pt_br",

	tooltips: {
		bold:           "Negrito",
		italic:         "Itálico",
		underline:      "Sublinhado",
		strikethrough:  "Riscado",
		subscript:      "Subscrito",
		superscript:    "Sobrescrito",
		justifyleft:    "Alinhado à esquerda",
		justifycenter:  "Centralizado",
		justifyright:   "Alinhado à direita",
		justifyfull:    "Justificado",
		orderedlist:    "Lista ordenada",
		unorderedlist:  "Lista não ordenada",
		outdent:        "Diminuir a indentação",
		indent:         "Aumentar a indentação",
		forecolor:      "Cor do texto",
		hilitecolor:    "Cor de ênfase",
		horizontalrule: "Linha horizontal",
		createlink:     "Inserir uma hiperligação",
		insertimage:    "Inserir/Modificar uma imagem",
		inserttable:    "Inserir uma tabela",
		htmlmode:       "Mostrar código fonte",
		popupeditor:    "Maximizar o editor",
		about:          "Sobre o editor",
		showhelp:       "Ajuda",
		textindicator:  "Estilo corrente",
		undo:           "Anular a última ação",
		redo:           "Repetir a última ação",
		cut:            "Cortar",
		copy:           "Copiar",
		paste:          "Colar",
		lefttoright:    "Da esquerda para a direita",
		righttoleft:    "Da direita para a esquerda"
	},

	buttons: {
		"ok":           "OK",
		"cancel":       "Cancelar"
	},

	msg: {
		"Path":         "Caminho",
		"TEXT_MODE":    "Está em MODO TEXTO.  Pressione o botão [<>] para voltar ao modo gráfico.",

		"IE-sucks-full-screen" :
		// translate here
		"O modo área de trabalho completo pode causar problemas no IE, " +
		"devido aos seus problemas que não foi possível de evitar.  " +
		"Os sintomas podem ser erros na área de trabalho ou falta de algumas" +
		"funções no editor ou falhas catastróficas do sistema operacional.  Se o seu  " +
		"sistema é Windows 9x, é possível o erro " +
		"'General Protection Fault' aconteça e que você tenha de reiniciar o computador." +
		"\n\nConsidere-se avisado.  Pressione OK se deseja continuar mesmo assim " +
		"testando o modo de tela cheia."
	},

	dialogs: {
		"OK"                                                : "OK",
		"Cancel"                                            : "Cancelar",
		"Alignment:"                                        : "Alinhamento:",
		"Not set"                                           : "Não definido",
		"Left"                                              : "Esquerda",
		"Right"                                             : "Direita",
		"Texttop"                                           : "Topo do texto",
		"Absmiddle"                                         : "Meio absoluto",
		"Baseline"                                          : "Linha base",
		"Absbottom"                                         : "Abaixo absoluto",
		"Bottom"                                            : "Fundo",
		"Middle"                                            : "Meio",
		"Top"                                               : "Topo",

		"Layout"                                            : "Formatação",
		"Spacing"                                           : "Espaçamento",
		"Horizontal:"                                       : "Horizontal:",
		"Horizontal padding"                                : "Enchimento horizontal",
		"Vertical:"                                         : "Vertical:",
		"Vertical padding"                                  : "Preenchimento vertical",
		"Border thickness:"                                 : "Espessura da borda:",
		"Leave empty for no border"                         : "Deixar vazio para ficar sem borda",

		// Insert Link
		"Insert/Modify Link"                                : "Inserir/Modificar link",
		"None (use implicit)"                               : "Nenhum (por omissão)",
		"New window (_blank)"                               : "Nova janela (_blank)",
		"Same frame (_self)"                                : "Mesmo frame (_self)",
		"Top frame (_top)"                                  : "Frame de cima (_top)",
		"Other"                                             : "Outro",
		"Target:"                                           : "Alvo:",
		"Title (tooltip):"                                  : "Título (tooltip):",

		"URL:"                                              : "URL:",
		"You must enter the URL where this link points to"  : "Você deve digitar o link para o qual aponta",

		// Insert Table
		"Insert Table"                                      : "Inserir Tabela",
		"Rows:"                                             : "Linhas:",
		"Number of rows"                                    : "Número de linhas",
		"Cols:"                                             : "Colunas:",
		"Number of columns"                                 : "Número de colunas",
		"Width:"                                            : "Largura:",
		"Width of the table"                                : "Largura da tabela",
		"Percent"                                           : "Porcentagem",
		"Pixels"                                            : "Pixel",
		"Em"                                                : "Em",
		"Width unit"                                        : "Unidade de largura",
		"Positioning of this table"                         : "Posicionamento da tabela",
		"Cell spacing:"                                     : "Espaçamento da célula:",
		"Space between adjacent cells"                      : "Espaço entre células adjacentes",
		"Cell padding:"                                     : "Preenchimento da célula:",
		"Space between content and border in cell"          : "Espaço entre o conteúdo e a borda da célula",
		// Insert Image
		"Insert Image"                                      : "Inserir imagem",
		"Image URL:"                                        : "Endereço da imagem:",
		"Enter the image URL here"                          : "Informe o endereço da imagem aqui",
		"Preview"                                           : "Previsão",
		"Preview the image in a new window"                 : "Previsão da imagem numa nova janela",
		"Alternate text:"                                   : "Texto alternativo:",
		"For browsers that don't support images"            : "Para navegadores que não suportam imagens",
		"Positioning of this image"                         : "Posicionamento desta imagem",
		"Image Preview:"                                    : "Previsão da imagem:"
	}
};
