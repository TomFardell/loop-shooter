CC=gcc
CFLAGS=-Wall
RAYLIB_LD_FLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

LDFLAGS=$(RAYLIB_LD_FLAGS)

FOLDER=src
EXE=game
OBJECTS=$(FOLDER)/game.o

debug: CFLAGS += -g -DDEBUG=1
debug: LDFLAGS += -g -DDEBUG=1
debug: $(EXE)

release: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) $(EXTRAFLAGS) -o $(EXE) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(EXE)

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRAFLAGS) -c -o $@ $<
