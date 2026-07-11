CC = zig cc -target x86_64-windows-gnu
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LIBS = -lgdi32 -luser32 -lshell32 -lkernel32 -Wl,--gc-sections,--subsystem,windows -s

SRC = main.c
OUT = build/pestart.exe
RES = build/pestart.res
RC = pestart.rc

all: $(OUT)

$(RES): $(RC)
	mkdir -p build
	zig rc $(RC) $(RES)

$(OUT): $(SRC) $(RES)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC) $(RES) $(LIBS)

clean:
	rm -rf build

.PHONY: all clean
