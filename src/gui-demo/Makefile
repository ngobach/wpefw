CC = zig cc -target x86_64-windows-gnu
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LIBS = -lcomctl32 -lshell32 -luser32 -lgdi32 -lkernel32 -Wl,--gc-sections -s

SRC = main.c
OUT = gui-demo.exe

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(OUT)

.PHONY: all clean
