# BEGIN s9y
DirectoryIndex {PREFIX}{indexFile}
php_value session.use_trans_sid 0
php_value register_globals off

<Files *.tpl.php>
    deny from all
</Files>

<Files *.tpl>
    deny from all
</Files>

<Files *.sql>
    deny from all
</Files>

<Files *.inc.php>
    deny from all
</Files>

<Files *.db>
    deny from all
</Files>

# END s9y
