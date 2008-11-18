<?php # $Id: lang_ja.inc.php,v 1.4 2005/05/17 11:37:42 garvinhicking Exp $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# Sebastian Mordziol <argh@php-tools.net>                                #
# http://sebastian.mordziol.de                                           #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_SPAMBLOCK_TITLE', 'Protection contre le Spam');
@define('PLUGIN_EVENT_SPAMBLOCK_DESC', 'Offre une multitude de possibilités pour protéger votre blog contre le Spam dans les commentaires.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', 'Protection contre le Spam: message non valide.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', 'Protection contre le Spam: Vous ne pouvez pas ajouter de commentaire supplémentaire dans un intervalle si court.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_RBL', 'Protection contre le Spam: L\'adresse IP de l\'ordinateur duquel vous écrivez votre commentaire est listé comme relais ouvert.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_SURBL', 'Protection contre le Spam: Votre commentaire contient une adresse listée dans SURBL.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', 'Ce blog est en mode "Bloquage d\'urgence des commentaires", merci de réessayer un peu plus tard.');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', 'Ne pas autoriser les doublons de commentaires');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', 'Ne pas autoriser les utilisateurs d\'ajouter de commentaires qui ont le même contenu qu\'un commentaire existant.');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', 'Bloquage d\'urgence des commentaires');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', 'Vous permet de désactiver temporairement les commentaires pour tous les billets. Pratique si votre blog est sous attaque de Spam.');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'Intervalle de bloquage IP');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', 'N\'autoriser une adresse IP de commenter que toutes les n minutes. Pratique pour éviter l\'inondation de commentaires.');
@define('PLUGIN_EVENT_SPAMBLOCK_RBL', 'Refuser les commentaires par liste noire');
@define('PLUGIN_EVENT_SPAMBLOCK_RBL_DESC', 'Si active, cette option permet de refuser les commentaires ventant d\'hôtes listés dans les RBLs (listes noires). Notez que cela peut avoir un effet sur les utilisateurs derrière un proxy ou dont le fournisseur internet est sur liste noire.');
@define('PLUGIN_EVENT_SPAMBLOCK_SURBL', 'Refuser les commentaires par SURBL');
@define('PLUGIN_EVENT_SPAMBLOCK_SURBL_DESC', 'Refuse les commentaires contenant des liens vers des hôtes listés dans la base de données <a href="http://www.surbl.org">SURBL</a>');
@define('PLUGIN_EVENT_SPAMBLOCK_RBLLIST', 'RBLs à contacter');
@define('PLUGIN_EVENT_SPAMBLOCK_RBLLIST_DESC', 'Bloque les commentaires en se basant sur les listes RBL définies ici.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', 'Activer les captchas');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', 'Force les utilisateurs à entrer un texte affiché dans une image générée automatiquement pour éviter que des systèmes automatisés puissent ajouter des commentaires. Notez cependant que cela peut poser des problèmes aux personnes malvoyantes.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', 'Pour éviter le spam par des robits automatisés (spambots), merci d\'entrer les caractères que vous voyez dans l\'image ci-dessous dans le champ de fomulaire prévu à cet effet. Assurez-vous que votre navigateur gère et accepte les cookies, sinon votre commentaire ne pourra pas être enregistré.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', 'Entrez le texte que vous voyez ici dans le champs!');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', 'Entrez le texte de l\'image anti-spam ci-dessus ici: ');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', 'Vous n\'avez pas entré correctement le texte de l\'image anti-spam. Merci de corriger votre code en revérifiant avec l\'image.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', 'Les captchas ne sont pas disponibles sur votre serveur. Il faut que la GDLib et les librairies freetype soient compilées dans votre installation de PHP.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', 'Captchas automatiques après X jours');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', 'Les captchaes peuvent être activés automatiquement après un nombre défini de jours pour chaque billet. Pour toujours activer les captchas, entrez un 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', 'Modération automatique après X jours');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', 'La modération des commentaires peut être activée automatiquement après un nombre défini de jours après la publication d\'un billet. Pour ne pas utiliser la modération automatique, entrez un 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', 'Modération autmatique après X liens');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', 'La modération des commentaires peut être activée automatiquement si le nombre des liens contenus dans un commentaire dépasse un nombre défini. Pour ne pas utiliser cette fonction, entrez un 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', 'Refus automatique après X liens');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', 'Un commentaire peut être refusé automatiquement si le nombre de liens qu\'il contient dépasse le nombre défini. Pour ne pas utiliser cette fonction, entrez un 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', 'À cause de certaines conditions, votre commentaire est sujet à modération par l\'auteur du blog avant d\'être affiché.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'Couleur d\'arrière-plan du captcha');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', 'Entrez des valeurs RVB: 0,255,255');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', 'Fichier log');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', 'L\'information sur les commentaires refusés/modérés peut être enregistrée dans un fichier log, précisez un emplacement pour ce fichier ici si vous voulez utiliser cette fonction.');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', 'Bloquage d\'urgence des commentaires');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', 'Doublon de commentaire');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'Bloquage IP');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_RBL', 'Bloquage RBL');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_SURBL', 'Bloquage SURBL');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', 'Captcha invalide (Entré: %s, Valide: %s)');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', 'Modération automatique après X jours');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT', 'Top de liens');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE', 'trop de liens');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', 'Masquer les adresses Email');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', 'Masque les adresses Emqil des utilisateurs qui ont écrit des commentaires');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', 'Les adresses Email ne sont pas affichées, et sont seulement utilisées pour la communication.');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', 'Choisissez une méthode de logage');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', 'Le logage des commentaires refusés peut se faire dans un fichier texte, ou dans la base de données.');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', 'Fichier (voir l\'option \'Fichier log\')');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', 'Base de données');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', 'Pas de logage');
@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', 'Gestion des commentaires par interface');
@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', 'Définit comment Serendipity gère les commentaires faits par l\'interface (rétroliens, commentaires WFW:commentAPI). Si vous sélectionnez "modération", ce commentaires seront toujours sujets à modération. Avec "refus", ils ne sont pas autorisés. Avec "aucune", ces commentaires seront gérés comme des commentaires traditionnels.');
@define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', 'modération');
@define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', 'refus');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', 'aucune');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', 'Filtrage par mots clés');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', 'Marque tous les commentaires contenant les mots clés définis comme Spam.');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', 'Filtrage par mots clés pour les liens');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', 'Marque tous les commentaires dont les liens contiennent les mots clés définis comme Spam. Les expressions régulières sont autorisées, séparez les mots clés par des points virgule (;).');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', 'Filtrage par nom d\'auteur');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', 'Marque tous les commentaires dont le nom d\'auteur contient les mots clés définis comme Spam. Les expressions régulières sont autorisées, séparez les mots clés par des points virgule (;).');

/* vim: set sts=4 ts=4 expandtab : */
?>