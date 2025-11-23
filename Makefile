CC = cc

DEPS = -lSDL2 -lGLESv2
INCLUDE = -Iinclude
CFLAGS = -std=c99 -O2 -Wall -Wextra $(INCLUDE)
LDFLAGS = $(DEPS)

SRC = src/quark.c src/sdl.c src/render.c src/engine.c
OBJ = build/quark.o build/sdl.o build/render.o build/engine.o

all: quark

quark: $(OBJ)
	$(CC) -o quark $(OBJ) $(LDFLAGS)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f quark $(OBJ)

run:
	./quark

compile_flags:
	rm -f compile_flags.txt
	for f in ${CFLAGS}; do echo $$f >> compile_flags.txt; done
