CXX=g++
CXXFLAGS=-std=c++17 -Ofast -g -Wall -Wextra

SDL2    =$(shell pkg-config sdl2 --libs --cflags \
              || sdl2-config --libs --cflags \
              || echo '-I/usr/include/SDL -I/usr/include/SDL2 -lSDL2')
GL      =$(shell pkg-config gl                       --libs --cflags \
              || echo '-lGL')
SFML    =$(shell pkg-config sfml-system   sfml-audio --libs --cflags \
              || echo '-lsfml-audio -lsfml-system')
SFML_GFX=$(shell pkg-config sfml-graphics sfml-audio --libs --cflags \
              || echo '-lsfml-audio -lsfml-graphics')
IMGUI=-Iimgui

TARGETS=engsyn finsyn float_opt \
	pcmaudio-lpc pcmaudio-lpc-small pcmaudio-lpc-wav \
	pcmaudio-lpc-browser pcmaudio-lpc-browser-sw pcmaudio-lpc-browser-sfml

all: $(TARGETS) ;

engsyn: engsyn.cc
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SFML)
finsyn: finsyn.cc
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SFML)

float_opt: float_opt.cc
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) -fopenmp

pcmaudio-lpc: pcmaudio-lpc.cc
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SDL2)
pcmaudio-lpc-small: pcmaudio-lpc-small.cc
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SDL2)
pcmaudio-lpc-wav: pcmaudio-lpc-wav.cc
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS)

pcmaudio-lpc-browser: pcmaudio-lpc-browser.cc \
		      imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp \
		      imgui/examples/imgui_impl_sdl.cpp \
		      imgui/examples/imgui_impl_opengl2.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SDL2) $(GL) $(IMGUI)

pcmaudio-lpc-browser-sw: pcmaudio-lpc-browser-sw.cc \
		      imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp \
		      imgui_software_renderer/src/imgui_sw.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SFML_GFX) $(IMGUI) -I.

pcmaudio-lpc-browser-sfml: pcmaudio-lpc-browser-sfml.cc \
		      imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) $(SFML_GFX) $(IMGUI)



clean:	;
	rm -f $(TARGETS)
