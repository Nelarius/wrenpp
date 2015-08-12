
CC=g++

WREN_LINK = C:/dev/wren/lib
WREN_COMP = C:/dev/wren/src/include

CFLAGS = -std=gnu++14 -Wall -O2 -DDEBUG

LDFLAGS = 

EXECUTABLE = 

TEST_EXECUTABLE = 

ifeq ($(OS),Windows_NT)
	CFLAGS += -I ./src -I $(WREN_COMP)
	LDFLAGS += -l:libwren.a -L $(WREN_LINK)
	EXECUTABLE += wrenly.exe
else
	UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGS +=  -I ./src -I /home/muszynsk/dev/wren/src/include
        LDFLAGS +=  -l:libwren.a -L /home/muszynsk/dev/wren/lib
		EXECUTABLE += wrenly
    endif
endif

OBJ = src/Main.o \
	src/Wren.o \

all: $(EXECUTABLE)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXECUTABLE): $(OBJ)
	$(CC) -o $(EXECUTABLE) $(OBJ) $(LDFLAGS)


.PHONY: clean

clean:
	rm $(OBJ) $(EXECUTABLE)
