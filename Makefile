CC = gcc
#truncate nao funciona com -std=c99
CFLAGS = -Wall 
SRC= ./src
NAMES = $(wildcard $(SRC)/*.c)
OBJS = ${NAMES:%.c=%.o}

all: vina++

debug: CFLAGS += -g -DDEBUG
debug: vina++

%.o: %.c %.h $(SRC)/macros.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC)/main.o: $(SRC)/main.c $(SRC)/macros.h
	$(CC) $(CFLAGS) -c $< -o $@

vina++: $(OBJS) 
	$(CC) $^ -o $@

purge:
	rm -rf $(SRC)/*.o vina++

clean:
	rm -rf $(SRC)/*.o

