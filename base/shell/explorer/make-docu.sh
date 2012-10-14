sed 's/@GEN@/generated on '`date +%d.%m.%Y`/ <doxy-footer.htmt >doxy-footer.html
doxygen Doxyfile
