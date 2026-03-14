CC = gcc
#CC = gcc-13
CFLAGS = -O3 -Wall -g -Wno-unused-function -Wno-deprecated-declarations -Wno-uninitialized -Wno-unused-result -Wno-implicit-function-declaration -Wno-unknown-pragmas
LDLIBS = -lm

ifeq ($(shell uname -s), Darwin)
   LDLIBS += -framework OpenGL -framework GLUT
   CPPFLAGS += -I/opt/homebrew/include
else
   LDLIBS += -lglut -lGLU -lGL
   # parfois -lglut n'est pas nécessaire et peut-être supprimé
endif

LDLIBS += $(shell sdl2-config --libs)
LDFLAGS += $(shell sdl2-config --cflags)

tsp_src := $(wildcard tsp_*.c pppp*.c htable*.c)
tsp_obj := $(patsubst %.c,%.o,$(tsp_src))

tsp_main:  tools.o $(tsp_obj)
test_heap: test_heap.o heap.o
a_star:    tools.o a_star.o heap.o


.PHONY: clean
clean:
	rm -f *.o
	rm -f tsp_main
	rm -f test_heap
	rm -f a_star
	rm -f pppp
	rm -fr *.dSYM/
