# Boids Part III - Quadtree
#
#   make          build the window program (raylib is built automatically the
#                 first time; that takes about a minute)
#   make test     build and run the automatic tests (they also find memory leaks)
#   make bench    build the timing program
#   make asan     extra memory checking with AddressSanitizer (Linux and macOS only)
#   make clean    delete everything that was built
#
# raylib is included in the raylib/ folder. You do not need to install it.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -Isrc

CORE    = src/boids.c src/quadtree.c
HEADERS = src/boids.h src/boid.h src/quadtree.h src/dynarray.h src/timer.h

# ---------- raylib (built once from the bundled source) ----------
RAYLIB_SRC    = raylib/src
RAYLIB_LIB    = build/libraylib.a
RAYLIB_MODS   = rcore rshapes rtextures rtext utils rglfw
RAYLIB_OBJS   = $(addprefix build/raylib/,$(addsuffix .o,$(RAYLIB_MODS)))
RAYLIB_CFLAGS = -O2 -std=gnu99 -w -DPLATFORM_DESKTOP_GLFW -DPLATFORM_DESKTOP \
                -DGRAPHICS_API_OPENGL_33 -I$(RAYLIB_SRC) -I$(RAYLIB_SRC)/external/glfw/include
GLFW_LANG     =

ifeq ($(OS),Windows_NT)
    EXE            = .exe
    RAYLIB_CFLAGS += -I$(RAYLIB_SRC)/external/glfw/deps/mingw
    WINDOW_LIBS    = -lopengl32 -lgdi32 -lwinmm
else
    EXE            =
    CFLAGS        += -D_POSIX_C_SOURCE=199309L
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        GLFW_LANG   = -x objective-c
        WINDOW_LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    else
        RAYLIB_CFLAGS += -D_GNU_SOURCE -D_GLFW_X11
        WINDOW_LIBS    = -lX11 -lm -lpthread -ldl
    endif
endif

# ---------- programs ----------
all: boids$(EXE)

boids$(EXE): src/main.c $(CORE) $(HEADERS) $(RAYLIB_LIB)
	$(CC) $(CFLAGS) -I$(RAYLIB_SRC) src/main.c $(CORE) $(RAYLIB_LIB) -o $@ $(WINDOW_LIBS)

bench$(EXE): src/bench.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) src/bench.c $(CORE) -o $@ -lm

run_tests$(EXE): tests/tests.c tests/memcheck.c tests/memcheck.h $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -Itests -include tests/memcheck.h tests/tests.c tests/memcheck.c $(CORE) -o $@ -lm

ifneq ($(EXE),)
bench: bench$(EXE)
.PHONY: bench
endif

test: run_tests$(EXE)
	./run_tests$(EXE)

asan: tests/tests.c src/bench.c $(CORE) $(HEADERS)
	$(CC) -std=c11 -Wall -Wextra -g -O1 -D_POSIX_C_SOURCE=199309L -fsanitize=address,undefined -Isrc -Itests -include tests/memcheck.h tests/tests.c tests/memcheck.c $(CORE) -o tests_asan -lm
	$(CC) -std=c11 -Wall -Wextra -g -O1 -D_POSIX_C_SOURCE=199309L -fsanitize=address,undefined -Isrc src/bench.c $(CORE) -o bench_asan -lm
	./tests_asan
	./bench_asan --method quadtree --n 500 --frames 100
	./bench_asan --method update --n 500 --frames 100

# ---------- building raylib ----------
$(RAYLIB_LIB): $(RAYLIB_OBJS)
	ar rcs $@ $(RAYLIB_OBJS)

build/raylib/rglfw.o: $(RAYLIB_SRC)/rglfw.c
	@mkdir -p build/raylib
	$(CC) -c $(GLFW_LANG) $(RAYLIB_CFLAGS) $< -o $@

build/raylib/%.o: $(RAYLIB_SRC)/%.c
	@mkdir -p build/raylib
	$(CC) -c $(RAYLIB_CFLAGS) $< -o $@

clean:
	rm -rf build boids boids.exe bench bench.exe run_tests run_tests.exe tests_asan bench_asan test_output.txt

.PHONY: all test asan clean
