cd\tools
ssync -r
cd\cexpr
ssync -r
rm /k /r *.*
exp /r
nmake all
nmake PROD= all
