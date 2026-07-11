CC = zig cc -target x86_64-windows-gnu
CFLAGS = -Os -Wall -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LIBS = -lcomctl32 -lshell32 -luser32 -lgdi32 -lkernel32 -Wl,--gc-sections,--subsystem,windows -s

SRC = main.c
RES = gui-demo.res
OUT = gui-demo.exe

all: $(OUT)

gui-demo.res: gui-demo.exe.manifest gui-demo.rc
	zig rc gui-demo.rc

$(OUT): $(SRC) gui-demo.res
	$(CC) $(CFLAGS) -o $@ $(SRC) $(RES) $(LIBS)

clean:
	rm -f $(OUT) gui-demo.res

.PHONY: all clean
