CC := mpicc
CFLAGS := -pthread -g -lrt
SRC := src
BIN := proj

OUT := out

$(OUT)/$(BIN): $(SRC)/* $(OUT)/
	$(CC) -o$@ $(wildcard $(SRC)/*.c) $(CFLAGS)

%/:
	mkdir -p $@

.PHONY: clean
clean:
	rm -r $(OUT)
	rm -r test/out

run-%: $(OUT)/$(BIN)
	mpirun --oversubscribe -np $(@:run-%=%) $< $(ARGS)

test/out/test-%: test/test-%.c $(SRC)/%.c
	gcc -o $@ $^

test-%: test/out/ test/out/test-%
	./$<$@
