# 
# VERSION CHANGES
#

#BV=$(shell (git rev-list HEAD --count))
#BD=$(shell (date))
BV=1234
BD=today
SDLFLAGS=$(shell (sdl2-config --static-libs --cflags))
CFLAGS= -ggdb -O -DBUILD_VER="$(BV)" -DBUILD_DATE=\""$(BD)"\" -DFAKE_SERIAL=$(FAKE_SERIAL)
LIBS=-lSDL2_ttf
CC=gcc
GCC=g++

OBJ=vc8145-sdl2

default: $(OBJ)
	@echo
	@echo

vc8145-sdl2: vc8145-sdl2.cpp
	@echo Build Release $(BV)
	@echo Build Date $(BD)
	${GCC} ${CFLAGS} $(COMPONENTS) vc8145-sdl2.cpp $(SDLFLAGS) $(LIBS) ${OFILES} -o ${OBJ} 

clean:
	del /s ${OBJ} ${WINOBJ}
