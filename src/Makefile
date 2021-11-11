CFLAGS = -g -Wall -pedantic
BUILD = emulate assemble unit_test

all: $(BUILD)

emulate: utils.o emulate.o cycle.o instructions.o
	gcc $(CFLAGS) utils.o emulate.o cycle.o instructions.o -o emulate

assemble: utils.o assemble.o symbol_table.o encode.o instructions.o
	gcc $(CFLAGS) utils.o assemble.o symbol_table.o encode.o instructions.o -o assemble

unit_test: utils.o instructions.o unit_test.o symbol_table.o encode.o
	gcc $(CFLAGS) utils.o instructions.o unit_test.o symbol_table.o encode.o -o unit_test

utils.o: utils.c utils.h
	gcc $(CFLAGS) -c utils.c

emulate.o: emulate.c utils.h cycle.h
	gcc $(CFLAGS) -c emulate.c

cycle.o: cycle.c cycle.h
	gcc $(CFLAGS) -c cycle.c

instructions.o: instructions.c instructions.h utils.h
	gcc $(CFLAGS) -c instructions.c

unit_test.o: unit_test.c utils.h instructions.h symbol_table.h encode.h
	gcc $(CFLAGS) -c unit_test.c

symbol_table.o: symbol_table.c symbol_table.h
	gcc $(CFLAGS) -c symbol_table.c

assemble.o: assemble.c utils.h symbol_table.h encode.h
	gcc $(CFLAGS) -c assemble.c

encode.o: encode.c encode.h utils.h symbol_table.h instructions.h
	gcc $(CFLAGS) -c encode.c

clean:
	rm -f $(BUILD) *.o *.out core
