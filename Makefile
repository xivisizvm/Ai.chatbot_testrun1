CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
TARGET  = chatbot
SRC     = src/chatbot.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: all clean
