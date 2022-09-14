The Vt100ScreenPaser is a library provides parse serial data to get BIOS Screen info, such as BIOS Menu, Highlight Knob, BIOS Title, Description and efi shell info. The Vt100ScreenPaser library is developed by C++ language and provide API can be called by python language.


# How to use?
1. building to get .dll/.so
2. calling export function in python

# Export Function
1. Init(): Function for initializing Vt100ScreenParser.
2. Feed(): Function for feeding serial data to Vt100ScreenParser, parse the data into screen_info.
3. CleanScreenData(): Function for cleaning the screen data.
4. GetValueByKey(): Function for getting all value with certain key.
5. GetScreenColored(): Function for getting the info including texts and colors in the current screen.
6. GetSelectPage(): Function for parsing the screen_info, and get all the selectable formated BIOS info.
7. GetWholePage(): Function for parsing serial input, and get all texts in the page.
8. CheckVt100Draw(): Function for checking the serial data include vt100 keywords.
9. ParseEdkShell(): Function for parsing the serial data from edk shell.

# Workflow
1. Initialize the library by calling Init(), clean screen data by CleanScreenData() if needed.
2. Capture serial data and feed the data to the library to parse by calling Feed().
3. Get the selectable formated BIOS info by calling GetSelectPage().
4. Print the bios screen by calling GetWholePage()/GetScreenColored().
5. Get dedicate BIOS knob value by calling GetValueByKey().
6. Pasre EFI shell screen info by calling ParseEdkShell().


# How to build?
build .dll in windows:<br>
g++ -m32 -std=c++11 -static debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser32.dll<br>
g++ -m64 -std=c++11 -static debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser64.dll<br>

build .so in linux:<br>
g++ -m32 -std=c++11  debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser32.so<br>
g++ -m64 -std=c++11  debug_screen.cpp  vt100_screen_parse.cpp debug_screen.h  vt100_screen_parse.h -fPIC -shared -o Vt100ScreenPaser64.so<br>
