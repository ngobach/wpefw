CC = zig cc -target x86_64-windows-gnu
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LIBS = -lcomctl32 -lshell32 -luser32 -lgdi32 -lkernel32 -Wl,--gc-sections,--subsystem,windows -s

SRC = main.c
OUT = build/gui-demo.exe

all: $(OUT)

build/gui-demo.res: gui-demo.exe.manifest gui-demo.rc
	mkdir -p build
	zig rc gui-demo.rc build/gui-demo.res

$(OUT): $(SRC) build/gui-demo.res
	$(CC) $(CFLAGS) -o $@ $(SRC) build/gui-demo.res $(LIBS)

clean:
	rm -rf build

.PHONY: all clean
