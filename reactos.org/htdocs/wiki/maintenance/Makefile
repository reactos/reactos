help:
	@echo "Run 'make test' to run the parser tests."
	@echo "Run 'make doc' to run the doxygen generation."

test:
	php parserTests.php --quiet

doc:
	php mwdocgen.php --all
	@echo 'Doc generation done. Look at ./docs/html/'
