#CFLAGS = -O0 -Wall -g -Wno-unused-variable
CFLAGS = -O3 -Wall -g -Wno-unused-variable

tp_src := $(wildcard taquin.c misc.c)
tp_obj := $(patsubst %.c,%.o,$(tp_src))

taquin: $(tp_obj) misc.o

.PHONY: clean
clean:
	rm -f taquin  taquin.o taquin.cor.o
	rm -fr *.dSYM/





# Pour les enseignants
tp_src_cor := $(wildcard taquin.cor.c misc.c)
tp_obj_cor := $(patsubst %.c,%.o,$(tp_src_cor))

cor: taquin.cor

taquin.cor: $(tp_obj_cor)

archive:
	rm -f tp-note-TAP-2025.tgz
	pdflatex sujet && pdflatex sujet &&	pdflatex sujet_uk && pdflatex sujet_uk
	rm -f *.aux *.log *.out
	make clean
	mkdir -p tp-note-TAP-2025
	cp -p misc.c misc.h taquin.c taquin.h Makefile translation_taquin_dot_c sujet.pdf sujet_uk.pdf tp-note-TAP-2025
	scp -rp ./tp-note-TAP-2025 "trelawney:"
	rm -rf ./tp-note-TAP-2025
	ssh trelawney "(cd ./tp-note-TAP-2025 && gcc -O3 -Wall -g -Wno-unused-variable -c -o misc.o misc.c && rm -f misc.c)"
	ssh trelawney tar cfvz tp-note-TAP-2025.tgz tp-note-TAP-2025
	scp "trelawney:tp-note-TAP-2025.tgz" .
	ssh trelawney rm -rf tp-note-TAP-2025.tgz tp-note-TAP-2025
