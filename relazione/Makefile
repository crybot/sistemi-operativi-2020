default: all

all:
	latexmk -pdf -shell-escape main.tex

continuous:
	latexmk -pdf -pvc -interaction=batchmode -shell-escape main.tex 

clean:
	# clean all aux files (including pdf, dvi, etc.)
	latexmk -C
	rm -f main.run.xml main.bbl main-figure*

