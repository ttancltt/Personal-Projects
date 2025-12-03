# @file Makefile
# @brief Build web version of game project.
# @author aurelien.esnard@u-bordeaux.fr
# @copyright University of Bordeaux. All rights reserved, 2022.

# Configuration
EMCC = emcc
SRC_DIR = src
OUT_DIR = public
RES_DIR = res
JS_DIR = demo.js

# Fichiers sources
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

# Flags de compilation
CFLAGS = -I$(SRC_DIR) -O3
LDFLAGS = -s WASM=1 \
          -s ALLOW_MEMORY_GROWTH=1 \
          -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
          -s NO_EXIT_RUNTIME=1 \
          --preload-file $(RES_DIR)@res
	
.PHONY: all clean serve

all: game.js

game.js: $(OBJS)
	
	$(EMCC) $^ -o $@ $(LDFLAGS)
	

%.o: %.c
	$(EMCC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o
	rm -f *.o
	rm -f libgame.a
	rm -f  game.js game.wasm game.data

serve:
	python3 -m http.server 8001
