CC=gcc
CFLAGS=-Wall
RAYLIB_LD_FLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

LD_FLAGS=$(RAYLIB_LD_FLAGS)

FOLDER=src
GAME_OUT=game.out
OBJECTS=$(FOLDER)/game.o

default: build

debug: CFLAGS += -g -DDEBUG=1
debug: LD_FLAGS += -g -DDEBUG=1
debug: build

build: $(OBJECTS)
	$(CC) $(CFLAGS) $(EXTRAFLAGS) -o $(GAME_OUT) $(OBJECTS) $(LD_FLAGS)

clean:
	rm -f $(FOLDER)/*.o $(GAME_OUT)

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRAFLAGS) -c -o $@ $<
