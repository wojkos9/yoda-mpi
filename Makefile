CC := mpicc
CFLAGS := -pthread -g -lrt
SRC := src
BIN := proj

CONFFLAGS := ALL

OUT := out

$(OUT)/$(BIN): $(SRC)/* $(OUT)/ src
	$(CC) -o$@ $(wildcard $(SRC)/*.c) $(CFLAGS)

%/:
	mkdir -p $@

.PHONY: clean yoda-mpi
clean:
	rm -r $(OUT)
	rm -r test/out

run-%: $(OUT)/$(BIN)
	mpirun --oversubscribe -np $(@:run-%=%) $< $(ARGS)

test/out/test-%: test/test-%.c $(SRC)/%.c
	gcc -o $@ $^

test-%: test/out/ test/out/test-%
	./$<$@

src: pre/* configure.sh
	./configure.sh $(CONFFLAGS)

yoda-mpi:
	rm -rf $@
	mkdir $@
	cp -r configure.sh run.sh pre Makefile $@