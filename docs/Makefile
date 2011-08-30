all:
	hg rm manual/* || true
	rm -f manual/*.pdf manual/*.html
	docbook2html -o manual manual.docbook.sgml
	docbook2html -u manual.docbook.sgml && mv manual.docbook.html manual/manual.html
	docbook2pdf -u manual.docbook.sgml && mv manual.docbook.pdf manual/manual.pdf
	hg add manual/*
