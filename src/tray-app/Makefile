CC = zig cc -target x86_64-windows-gnu
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LIBS = -lshell32 -lgdi32 -luser32 -Wl,--gc-sections -s

SRC = main.c
OUT = build/tray.exe

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf build

.PHONY: all clean
