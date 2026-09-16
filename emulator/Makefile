CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

TARGET = zevmobile
SOURCES = main.c phone.c ../kernel/kernel.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) -luser32 -lgdi32

clean:
	rm -f $(TARGET) $(TARGET).exe *.o

windows:
	x86_64-w64-mingw32-gcc $(CFLAGS) -mwindows -o zevmobile.exe $(SOURCES) -luser32 -lgdi32
