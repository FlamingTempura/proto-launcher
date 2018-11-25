LANG = -std=c++17 # language options
SEARCH_DIRS = -I/usr/include/X11R5  -I/usr/include/freetype2/ # extra directory to look for #includes
LIB_DIRS = -L/usr/lib/X11R5 # extra directories to look for -l flags
LIBS = -lX11 -lXft -lstdc++fs # 3rd party libraries
OPTIMIZATION = -O3 -fno-unroll-loops -fmerge-all-constants -fno-ident -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-stack-protector -fomit-frame-pointer -fno-math-errno -Wl,--gc-sections -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none -s # improve speed and reduce binary size

all: 
	g++ -o x x.cpp $(LANG) $(SEARCH_DIRS) $(LIB_DIRS) $(LIBS) $(OPTIMIZATION)