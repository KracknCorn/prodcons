TARGET=prodcons
CC=gcc
CFLAGS=-m32 -o
INCLUDED=/u/OSLab/alm336/linux-2.6.23.1/include

default:
	$(CC) $(CFLAGS) $(TARGET) -I $(INCLUDED) $(TARGET).c

clean:
	$(RM) $(TARGET)
