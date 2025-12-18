# tooling
CXX	= c++
CXXFLAGS = -std=c++23 -Os -Wall -Wextra
PKG_CONFIG = pkg-config

# CHANGE THIS: ladybird source tree
LADYBIRD = $(HOME)/clones/ladybird-test
#          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
BUILD = $(LADYBIRD)/Build/release
INCS = -I$(LADYBIRD) -I$(LADYBIRD)/Services -I$(LADYBIRD)/Libraries \
       -I$(BUILD)/Lagom -I$(BUILD)/Lagom/Services -I$(BUILD)/Lagom/Libraries \
       -I$(BUILD)/vcpkg_installed/x64-linux-dynamic/include \
       $$($(PKG_CONFIG) --cflags sdl3 2>/dev/null)

# ladybird libs
LAGOM_LIBS = -L$(BUILD)/lib \
			 -llagom-webview \
			 -llagom-web \
			 -llagom-js \
			 -llagom-gfx \
			 -llagom-url \
			 -llagom-core \
			 -llagom-coreminimal \
			 -llagom-ak \
			 -llagom-ipc \
			 -llagom-unicode \
			 -llagom-textcodec \
			 -llagom-gc \
			 -llagom-requests \
			 -llagom-http \
			 -llagom-crypto \
			 -llagom-tls \
			 -llagom-compress \
			 -llagom-xml \
			 -llagom-wasm \
			 -llagom-media \
			 -llagom-imagedecoderclient \
			 -llagom-devtools \
			 -llagom-database \
			 -llagom-filesystem \
			 -llagom-syntax \
			 -llagom-idl

# external libs
EXT_LIBS = $$($(PKG_CONFIG) --libs sdl3 sdl3-ttf 2>/dev/null) \
			-L$(BUILD)/vcpkg_installed/x64-linux-dynamic/lib \
			-lfontconfig

# main() wrapper (static)
MAIN_LIB = $(BUILD)/lib/liblagom-main.a
LIBS = $(MAIN_LIB) $(LAGOM_LIBS) $(EXT_LIBS) -lpthread

# rpath
LDFLAGS = -Wl,-rpath,$(BUILD)/lib -Wl,-rpath,$(BUILD)/vcpkg_installed/x64-linux-dynamic/lib

# sources
SRCDIR = quartz
SRC	= $(SRCDIR)/quartz.cpp
OBJ	= $(SRC:.cpp=.o)
BIN	= quartz/quartz

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

install: $(BIN)
	cp $(BIN) $(BUILD)/bin/

run: install
	cd $(BUILD)/bin && \
	LD_LIBRARY_PATH=$(BUILD)/lib:$(BUILD)/vcpkg_installed/x64-linux-dynamic/lib \
	./quartz

patch:
	cd $(LADYBIRD) && \
	git apply $$PWD/build_patches/cmake.patch && \
	git apply $$PWD/build_patches/lagom_options.patch && \
	git apply $$PWD/build_patches/vcpkg.patch && \
	git apply $$PWD/build_patches/ui_cmake.patch && \
	rm -f $(LADYBIRD)/UI/quartz && \
	ln -sf $$PWD/quartz $(LADYBIRD)/UI/quartz

ladybird:
	cd $(LADYBIRD) && \
	cmake --preset Release -DENABLE_QT=OFF -DENABLE_QUARTZ=ON && \
	ninja -C Build/release

clangd:
	rm -f compile_flags.txt
	for f in $(CXXFLAGS) $(INCS); do echo $$f >> compile_flags.txt; done
