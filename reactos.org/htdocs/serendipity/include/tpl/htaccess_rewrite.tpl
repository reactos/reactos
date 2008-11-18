# BEGIN s9y
ErrorDocument 404 {PREFIX}{indexFile}
DirectoryIndex {PREFIX}{indexFile}
php_value session.use_trans_sid 0
php_value register_globals off

RewriteEngine On
RewriteBase {PREFIX}
RewriteRule ^({PAT_PERMALINK}) {indexFile}?/$1 [NC,L,QSA]
RewriteRule ^({PAT_PERMALINK_AUTHORS}) {indexFile}?/$1 [NC,L,QSA]
RewriteRule ^({PAT_PERMALINK_FEEDCATEGORIES}) {indexFile}?/$1 [NC,L,QSA]
RewriteRule ^({PAT_PERMALINK_FEEDAUTHORS}) {indexFile}?/$1 [NC,L,QSA]
RewriteRule ^({PAT_PERMALINK_CATEGORIES}) {indexFile}?/$1 [NC,L,QSA]
RewriteRule ^{PAT_ARCHIVES} {indexFile}?url=/{PATH_ARCHIVES}/$1.html [NC,L,QSA]
RewriteRule ^([0-9]+)[_\-][0-9a-z_\-]*\.html {indexFile}?url=$1-article.html [L,NC,QSA]
RewriteRule ^{PAT_FEEDS}/(.*) {indexFile}?url=/{PATH_FEEDS}/$1 [L,QSA]
RewriteRule ^{PAT_UNSUBSCRIBE} {indexFile}?url=/{PATH_UNSUBSCRIBE}/$1/$2 [L,QSA]
RewriteRule ^{PAT_APPROVE} {indexFile}?url={PATH_APPROVE}/$1/$2/$3 [L,QSA]
RewriteRule ^{PAT_DELETE} {indexFile}?url={PATH_DELETE}/$1/$2/$3 [L,QSA]
RewriteRule ^{PAT_ADMIN} {indexFile}?url={PATH_ADMIN}/ [L,QSA]
RewriteRule ^{PAT_ARCHIVE} {indexFile}?url=/{PATH_ARCHIVE} [L,QSA]
RewriteRule ^{PAT_FEED} rss.php?file=$1&ext=$2
RewriteRule ^{PAT_PLUGIN} {indexFile}?url=$1/$2 [L,QSA]
RewriteRule ^{PAT_SEARCH} {indexFile}?url=/{PATH_SEARCH}/$1 [L,QSA]
RewriteRule ^{PAT_CSS} {indexFile}?url=/$1 [L,QSA]
RewriteRule ^index\.(html?|php.+) {indexFile}?url=index.html [L,QSA]
RewriteRule ^htmlarea/(.*) htmlarea/$1 [L,QSA]
RewriteRule (.*\.html?) {indexFile}?url=/$1 [L,QSA]

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
