CC = clang
LD = clang
CC_FLAGS = -O2 -Wall -Wextra -c
LD_FLAGS = -Wall -Wextra

SRC = src
OBJ = obj
BIN = bin

.PHONY: all
all: $(OBJ) $(BIN) $(BIN)/caddy65

$(OBJ):
	mkdir $(OBJ)

$(BIN):
	mkdir $(BIN)

$(BIN)/caddy65: $(OBJ)/caddy65.o
	$(CC) $(LD_FLAGS) $(OBJ)/caddy65.o -o $(BIN)/caddy65

$(OBJ)/caddy65.o: $(SRC)/caddy65.c
	$(CC) $(CC_FLAGS) $(SRC)/caddy65.c -o $(OBJ)/caddy65.o

.PHONY: test
test: all
	cd test &&\
	cp input.s test.s &&\
	touch CHR-ROM.bin &&\
	ca65 test.s -o test.o &&\
	../$(BIN)/caddy65 test.s &&\
	diff test.s expected.s &&\
	rm -f test.s test.o CHR-ROM.bin

.PHONY: clean
clean:
	rm -f $(BIN)/caddy65
	rm -rf $(OBJ)