CC = gcc
CFLAGS = -Wall -O2 -D_GNU_SOURCE
TARGET = myzip
OBJS = main.o huffman.o

all: $(TARGET)

$(TARGET): $(OBJS)
$(CC) $(OBJS) -o $(TARGET)

main.o: main.c huffman.h
$(CC) $(CFLAGS) -c main.c

huffman.o: huffman.c huffman.h
$(CC) $(CFLAGS) -c huffman.c

clean:
rm -f $(OBJS) $(TARGET)

test: $(TARGET)
./$(TARGET) -c test.hzp test.txt
./$(TARGET) -l test.hzp
./$(TARGET) -d test.hzp -o out/
