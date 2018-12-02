L_LANG = -std=c++17 -pthread # language options
L_SEARCH_DIRS = -I/usr/include/X11R5  -I/usr/include/freetype2/ # extra directory to look for #includes
L_LIB_DIRS = -L/usr/lib/X11R5 # extra directories to look for -l flags
L_LIBS = -lX11 -lXft -lstdc++fs # 3rd party libraries
L_OPTIMIZATION = -O3 -fno-unroll-loops -fmerge-all-constants -fno-ident -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-stack-protector -fomit-frame-pointer -fno-math-errno -Wl,--gc-sections -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none -s # improve speed and reduce binary size

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

proto-launcher : launcher.cpp
	g++ -o proto-launcher launcher.cpp $(L_LANG) $(L_SEARCH_DIRS) $(L_LIB_DIRS) $(L_LIBS) $(L_OPTIMIZATION)

.PHONY         : clean
clean          :
	rm -f proto-launcher

.PHONY         : install
install        : clean proto-launcher
	install -m 0755 proto-launcher $(PREFIX)/bin

.PHONY         : uninstall
uninstall      :
	rm $(PREFIX)/bin/proto-launcher

.PHONY         : run
run            : clean proto-launcher
	./proto-launcher
