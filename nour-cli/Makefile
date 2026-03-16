CC     = gcc
CFLAGS = -Wall -Wextra
TARGET = cli
SRCS   = dispatch.c main.c

$(TARGET): $(SRCS) dispatch.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET)

