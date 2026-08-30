# ls *.cpp
#
#3dobject-ase.cpp  construction.cpp  maps.cpp         netherdebug.cpp    radar.cpp
#3dobject.cpp      enemy_ai.cpp      menu.cpp         nethersave.cpp     robot_ai.cpp
#bitmap.cpp        glprintf.cpp      myglutaux.cpp    particles.cpp      robots.cpp
#bullet.cpp        main.cpp          nether.cpp       piece3dobject.cpp  shadow3dobject.cpp
#cmc.cpp           mainmenu.cpp      nethercycle.cpp  quaternion.cpp     vector.cpp

UNAME_S := $(shell uname -s)

SOURCES = 3dobject-ase.cpp 3dobject.cpp cmc.cpp nether.cpp piece3dobject.cpp vector.cpp bitmap.cpp bullet.cpp glprintf.cpp main.cpp mainmenu.cpp maps.cpp menu.cpp myglutaux.cpp nethercycle.cpp netherdebug.cpp nethersave.cpp particles.cpp construction.cpp quaternion.cpp radar.cpp enemy_ai.cpp robot_ai.cpp robots.cpp shadow3dobject.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = nether_earth
CXX ?= g++
SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_mixer)
SDL2_LIBS   := $(shell pkg-config --libs sdl2 SDL2_mixer)

ifeq ($(UNAME_S),Darwin)
  LIBRARIES = -framework Cocoa -framework OpenGL -framework GLUT $(SDL2_LIBS) -lpthread
else
  LIBRARIES = -lGL -lGLU -lglut $(SDL2_LIBS) -lpthread
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBRARIES)

%.o: %.cpp
	$(CXX) $(SDL2_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
