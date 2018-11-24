all: 
	g++ -I/usr/include/X11R5 -L/usr/lib/X11R5 -I/usr/include/freetype2/ -o x x.cpp -lX11 -lXft -lstdc++fs -std=c++17