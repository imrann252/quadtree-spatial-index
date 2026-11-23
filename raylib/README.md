# raylib 5.5 (trimmed)

This folder holds the parts of raylib 5.5 (https://github.com/raysan5/raylib,
zlib/libpng license, see LICENSE) that the boids window needs: the core,
shapes, textures and text modules, and the bundled GLFW. Audio and 3D models
were left out.

You do not need to install raylib: `make` compiles this folder once into
build/libraylib.a (this takes about a minute the first time) and reuses it
after that. You never need to edit anything in here.
