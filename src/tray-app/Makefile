CC = zig cc -target x86_64-windows-gnu
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LIBS = -lshell32 -lgdi32 -luser32 -Wl,--subsystem,windows -Wl,--gc-sections -s

SRC = main.c
RC = tray-app.rc
RES = build/tray-app.res
OUT = build/tray.exe

all: $(OUT)

$(RES): $(RC)
	mkdir -p build
	zig rc $(RC) $(RES)

$(OUT): $(SRC) $(RES)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(RES) $(LIBS)

clean:
	rm -rf build

.PHONY: all clean
