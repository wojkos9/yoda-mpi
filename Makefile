CC := mpicc
CFLAGS := 
SRC := src
BIN := proj

OUT := out

$(OUT)/$(BIN): $(SRC)/*.c
	$(CC) -o$@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm $(BIN)

run-%: $(OUT)/$(BIN)
	mpirun -np $(@:run-%=%) $< $(ARGS)

test/out/test-%: test/test-%.c $(SRC)/%.c
	gcc -o $@ $^

test-%: test/out/test-%
	./$<
