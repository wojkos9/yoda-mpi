CC := mpicc
CFLAGS := -pthread -g -lrt
SRC := src
BIN := proj

OUT := out

$(OUT)/$(BIN): $(SRC)/*
	$(CC) -o$@ $(wildcard $(SRC)/*.c) $(CFLAGS)

.PHONY: clean
clean:
	rm $(OUT)/$(BIN)

run-%: $(OUT)/$(BIN)
	mpirun --oversubscribe -np $(@:run-%=%) $< $(ARGS)

test/out/test-%: test/test-%.c $(SRC)/%.c
	gcc -o $@ $^

test-%: test/out/test-%
	./$<
