CC     = gcc
CFLAGS = -Wall -Wextra
TARGET = nour
SRCS   = dispatch.c main.c cmd.c ansi.c fn.c fs.c
BUILD  = build
OUT    = $(BUILD)/$(TARGET)

all: $(OUT)

$(BUILD):
	mkdir -p $(BUILD)

$(OUT): $(SRCS) dispatch.h | $(BUILD)
	$(CC) $(CFLAGS) -o $(OUT) $(SRCS)

run: $(OUT)
	./$(OUT) $(filter-out $@,$(MAKECMDGOALS))

clean:
	rm -rf $(BUILD)