build .dll in windows:<br>
g++ -m32 -std=c++11 -static debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser32.dll<br>
g++ -m64 -std=c++11 -static debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser64.dll<br>

build .so in linux:<br>
g++ -m32 -std=c++11  debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser32.so<br>
g++ -m64 -std=c++11  debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser64.so<br>
