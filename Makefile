CC=gcc
CFLAGS=-Wall -O3

PREFIX=/usr
BIN_DIR = $(PREFIX)/bin

SRC_DIR = src
CFILES = $(wildcard $(SRC_DIR)/*.c)
OFILES = $(CFILES:%.c=%.o)


all : simward

simward: $(OFILES)
	$(CC) $(OFILES) -o simward $(CFLAGS)


%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $<


install: simward
	@echo "installing simward to '$(BIN_DIR)'"
	@install -d -m 755 $(BIN_DIR)
	@install -m 755 simward $(BIN_DIR)


clean:
	@echo "cleaning up"
	@rm -f simward
	@rm -f $(SRC_DIR)/*.o

uninstall:
	@echo "uninstalling"
	@rm $(BIN_DIR)/simward
