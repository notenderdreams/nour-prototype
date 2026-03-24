CC      = gcc
CFLAGS  = -Wall -Wextra -I. -I../nour-core
TARGET  = nour
BUILD   = build
OUT     = $(BUILD)/$(TARGET)

CLI_SRCS  = main.c dispatch.c cmd.c ansi.c fn.c fs.c
CORE_SRCS = ../nour-core/core.c ../nour-core/preprocess.c

SRCS = $(CLI_SRCS) $(CORE_SRCS)

all: $(OUT)

$(BUILD):
	mkdir -p $(BUILD)

$(OUT): $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) -o $(OUT) $(SRCS)

run: $(OUT)
	./$(OUT) $(filter-out $@,$(MAKECMDGOALS))

clean:
	rm -rf $(BUILD)

%:
	@: