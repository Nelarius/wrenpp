
CC=g++

WREN_COMP = wren/src/include
WREN_LINK = wren/lib

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
        CFLAGS +=  -I ./src -I $(WREN_COMP)
        LDFLAGS +=  -L $(WREN_LINK) -l:libwren.a
		EXECUTABLE += wrenly
    endif
endif

EXECUTABLE_OBJ = src/Main.o \
	src/Wrenly.o \
	src/detail/Type.o \
	
LIB_OBJ = src/Wrenly.o \
	src/detail/Type.o \
	
all: post-build
	
pre-build:
	mkdir lib
	mkdir include
	mkdir include/detail

lib-build: pre-build
	@$(MAKE) lib/libtemporary.a

post-build: lib-build
	cp src/Wrenly.h include/
	cp src/File.h include/
	cp src/Assert.h include/
	cp src/detail/ForeignMethod.h include/detail
	cp src/detail/ForeignClass.h include/detail
	cp src/detail/Type.h include/detail
	cp wren/src/include/wren.h include/
	ar -M <libwrenly.mri
	mv libwrenly.a lib/
	rm lib/libtemporary.a

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
lib/libtemporary.a: $(LIB_OBJ)
	ar rcs $@ $^

$(EXECUTABLE): $(EXECUTABLE_OBJ)
	$(CC) -o $(EXECUTABLE) $(EXECUTABLE_OBJ) $(LDFLAGS)


.PHONY: all clean

clean:
	rm -rf lib
	rm -rf include
	rm -rf $(EXECUTABLE_OBJ) $(EXECUTABLE)
