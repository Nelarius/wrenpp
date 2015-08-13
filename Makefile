
CC=g++

WREN_COMP = C:/dev/wren/src/include
WREN_LINK = C:/dev/wren/lib

CFLAGS = -std=gnu++14 -Wall -O2 -DDEBUG

LDFLAGS = 

EXECUTABLE = 

ifeq ($(OS),Windows_NT)
	CFLAGS += -I./src -I $(WREN_COMP)
	LDFLAGS += -L $(WREN_LINK) -l:libwren.a
	EXECUTABLE += wrenly.exe
else
	UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGS +=  -I ./src -I /home/muszynsk/dev/wren/src/include
        LDFLAGS +=  -L /home/muszynsk/dev/wren/lib -l:libwren.a
		EXECUTABLE += wrenly
    endif
endif

OBJ = src/Main.o \
	src/Wrenly.o \

all: $(EXECUTABLE)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXECUTABLE): $(OBJ)
	$(CC) -o $(EXECUTABLE) $(OBJ) $(LDFLAGS)


.PHONY: clean

clean:
	rm $(OBJ) $(EXECUTABLE)
