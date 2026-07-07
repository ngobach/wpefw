SRC_WIM       := assets/seed/boot.wim
SRC_OVERLAY   := src/wim-overlay

BUILD_DIR     := build
BUILD_OVERLAY := build/overlay
BUILD_WIM     := build/winpe.wim

REGISTRY      := zot.ngobach.com
NTLOADER_OCI  := $(REGISTRY)/bootie/ntloader:v1
ULUA_OCI      := $(REGISTRY)/bootie/ulua:v1
WINXSHELL_OCI := $(REGISTRY)/bootie/winxshell:v1
EXPLORERPP_OCI := $(REGISTRY)/bootie/explorerpp:v1
PECMD_OCI      := $(REGISTRY)/bootie/pecmd:v1
IMDISK_OCI     := $(REGISTRY)/bootie/imdisk:v1
PENETWORK_OCI  := $(REGISTRY)/bootie/penetwork:v1
E7ZIP_OCI      := $(REGISTRY)/bootie/e7zip:v1
WIN_DLLS_OCI   := $(REGISTRY)/bootie/windlls:v1

APPS_SRC   := src/apps
APPS_BUILD := build/apps
APPS_RAW   := build/apps.raw
APPS_SIZE  := 64
APPS      := $(patsubst $(APPS_SRC)/%/,%,$(wildcard $(APPS_SRC)/*/))
APP_WIMS  := $(addprefix $(APPS_BUILD)/pkgs/,$(addsuffix .wim,$(APPS)))

PROGRAMS_SRC   := src
PROGRAMS_BUILD := build/programs

WIM_BINS     := wimappend wiminfo
WIM_PATHS    := $(foreach bin,$(WIM_BINS),$(shell command -v $(bin) 2>/dev/null))

.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        Build the WinPE WIM (default)"
	@echo "  build      Build the WinPE WIM"
	@echo "  programs   Build Windows C programs"
	@echo "  run        Boot the WinPE WIM in QEMU"
	@echo "  clean      Remove build artifacts"
	@echo "  verify     List contents of app WIM files"
	@echo "  check-deps Check required dependencies"
	@echo "  apps        Build app WIM files"
	@echo "  apps-attach  Hot-plug apps drive into QEMU"
	@echo "  apps-detach  Remove apps drive from QEMU"
	@echo "  apps-replug  Rebuild and replug apps drive"

.PHONY: all
all: check-deps build

.PHONY: $(BUILD_OVERLAY)

.PHONY: check-deps
check-deps:
	@missing=""; \
	for bin in $(WIM_BINS); do \
		path=$$(command -v $$bin 2>/dev/null); \
		if [ -z "$$path" ]; then \
			missing="$$missing $$bin"; \
		else \
			echo "  found: $$path"; \
		fi \
	done; \
	if [ -n "$$missing" ]; then \
		echo "ERROR: missing wimlib binaries:$$missing"; \
		echo "Install wimlib (e.g. brew install wimlib)"; \
		exit 1; \
	fi; \
	wimlib-imagex --version; \
	missing_apps=""; \
	for bin in qemu-img mformat mcopy; do \
		path=$$(command -v $$bin 2>/dev/null); \
		if [ -z "$$path" ]; then \
			missing_apps="$$missing_apps $$bin"; \
		else \
			echo "  found: $$path"; \
		fi \
	done; \
	if [ -n "$$missing_apps" ]; then \
		echo "WARNING: missing apps image tools:$$missing_apps"; \
		echo "Install mtools (e.g. brew install mtools)"; \
		echo "The apps drive will not be built."; \
	fi

$(BUILD_OVERLAY): $(SRC_OVERLAY) build/.ulua.stamp build/.winxshell.stamp build/.explorerpp.stamp build/.pecmd.stamp build/.imdisk.stamp build/.penetwork.stamp build/.e7zip.stamp build/.windlls.stamp
	mkdir -p $(BUILD_OVERLAY)
	cp -r build/ulua $(BUILD_OVERLAY)/
	cp -r build/winxshell/WinXShell/. $(BUILD_OVERLAY)/WinXShell/
	mkdir -p "$(BUILD_OVERLAY)/Program Files/Explorer++"
	cp -r build/explorerpp/. "$(BUILD_OVERLAY)/Program Files/Explorer++/"
	mkdir -p $(BUILD_OVERLAY)/Windows/System32
	cp build/pecmd/PECMD.EXE $(BUILD_OVERLAY)/Windows/System32/PECMD.EXE
	cp "$(BUILD_OVERLAY)/Program Files/Explorer++/Explorer++.exe" $(BUILD_OVERLAY)/Windows/explorer.exe
	mkdir -p $(BUILD_OVERLAY)/Windows/System32/drivers
	cp -r build/imdisk/driver $(BUILD_OVERLAY)/Windows/System32/imdisk
	unzip -oj build/windlls/oledlg.zip 'oledlg.dll' -d $(BUILD_OVERLAY)/Windows/System32/
	mkdir -p $(BUILD_OVERLAY)/PENetwork
	cp -r build/penetwork/. $(BUILD_OVERLAY)/PENetwork/
	mkdir -p "$(BUILD_OVERLAY)/Program Files/easy7zip"
	cp -r build/e7zip/x64/. "$(BUILD_OVERLAY)/Program Files/easy7zip/"
	cp -r $(SRC_OVERLAY)/. $(BUILD_OVERLAY)/

$(BUILD_WIM): check-deps $(BUILD_OVERLAY)
	@if [ ! -f "$(SRC_WIM)" ]; then \
		echo "ERROR: source WIM not found: $(SRC_WIM)"; \
		echo "Place a boot.wim in the wpefw directory and try again."; \
		exit 1; \
	fi
	mkdir -p $(dir $(BUILD_WIM))
	rm -f $(BUILD_WIM)
	wimexport $(SRC_WIM) 1 $(BUILD_WIM) --boot
	wimupdate $(BUILD_WIM) 1 --command="add $(BUILD_OVERLAY) /"

.PHONY: build
build: $(BUILD_WIM)

.PHONY: run
run: build/boot.raw build/.apps.stamp
	rm -f /tmp/qmp.sock
	qemu-system-x86_64 \
		-M q35 \
		-cpu Skylake-Client-v3 \
		-m 2G \
		-smp cores=4,threads=1 \
		-accel tcg,thread=multi,tb-size=1024 \
		-bios resources/OVMF.fd \
		-s \
		-debugcon file:/tmp/debugcon.log \
		-device usb-ehci,id=ehci \
		-drive file=build/boot.raw,format=raw \
		-drive file=fat:rw:assets/grub4dos,format=raw \
		-qmp unix:/tmp/qmp.sock,server,nowait

build/boot.raw: $(BUILD_WIM) build/.ntloader.stamp
	rm -f $@
	dd if=/dev/zero of=$@ bs=1M count=560
	printf "y\nedit 1\n0C\nn\n2048\n1144832\nflag 1\nedit 2\n0\nedit 3\n0\nedit 4\n0\nwrite\nquit\n" | fdisk -e $@
	mformat -i $@@@1048576 -H 2048 -F ::
	mcopy -i $@@@1048576 $(BUILD_WIM) ::/winpe.wim
	mcopy -i $@@@1048576 build/ntloader/ntloader ::/ntloader
	mcopy -i $@@@1048576 build/ntloader/initrd.cpio ::/initrd.cpio

build/.ntloader.stamp:
	rm -rf build/ntloader
	mkdir -p build/ntloader
	oras pull $(NTLOADER_OCI) -o build/ntloader
	unzip -o build/ntloader/ntloader.zip ntloader initrd.cpio -d build/ntloader/
	rm -f build/ntloader/ntloader.zip
	touch $@

build/.ulua.stamp:
	rm -rf build/ulua
	mkdir -p build/ulua
	oras pull $(ULUA_OCI) -o build/ulua
	unzip -o build/ulua/ulua.zip -d build/ulua/
	rm -f build/ulua/ulua.zip
	mv build/ulua/ulua/* build/ulua/ 2>/dev/null || true
	rm -rf build/ulua/ulua 2>/dev/null || true
	touch $@

build/.winxshell.stamp:
	rm -rf build/winxshell
	mkdir -p build/winxshell
	oras pull $(WINXSHELL_OCI) -o build/winxshell
	unzip -o build/winxshell/WinXShell_RC5.1.4_beta4.zip -d build/winxshell/
	rm -f build/winxshell/WinXShell_RC5.1.4_beta4.zip
	rm -f build/winxshell/WinXShell/WinXShell_x86.exe
	rm -f build/winxshell/WinXShell/WinXShellC_x86.exe
	rm -f build/winxshell/WinXShell/wxsStub32.dll
	rm -f build/winxshell/WinXShell/wallpaper.jpg
	mv build/winxshell/WinXShell/WinXShell_x64.exe build/winxshell/WinXShell/WinXShell.exe
	oras pull $(REGISTRY)/bootie/wallpaper:v3 -o build/winxshell/WinXShell/
	for f in build/winxshell/WinXShell/wxsUI/*.zip; do \
		[ -f "$$f" ] || continue; \
		dir="$${f%.zip}"; \
		unzip -o "$$f" -d "$$dir" 2>/dev/null; \
		rm -f "$$f"; \
	done
	touch $@

build/.explorerpp.stamp:
	rm -rf build/explorerpp
	mkdir -p build/explorerpp
	oras pull $(EXPLORERPP_OCI) -o build/explorerpp
	unzip -o build/explorerpp/explorerpp.zip -d build/explorerpp/
	rm -f build/explorerpp/explorerpp.zip
	touch $@

build/.imdisk.stamp:
	rm -rf build/imdisk
	mkdir -p build/imdisk
	oras pull $(IMDISK_OCI) -o build/imdisk
	unzip -o build/imdisk/ImDiskTk-x64.zip -d build/imdisk/
	rm -f build/imdisk/ImDiskTk-x64.zip
	7z x -y build/imdisk/ImDiskTk*/files.cab -obuild/imdisk/
	rm -rf build/imdisk/ImDiskTk*
	touch $@

build/.pecmd.stamp:
	rm -rf build/pecmd
	mkdir -p build/pecmd
	oras pull $(PECMD_OCI) -o build/pecmd
	7z x -y build/pecmd/PECMD88.05.62-26-05-15x64-EN2ba2.7z -obuild/pecmd/ PECMD88.05.62-26-05-15x64-EN/X64/PECMD.EXE
	rm -f build/pecmd/PECMD88.05.62-26-05-15x64-EN2ba2.7z
	mv build/pecmd/PECMD88.05.62-26-05-15x64-EN/X64/PECMD.EXE build/pecmd/PECMD.EXE
	rm -rf build/pecmd/PECMD88.05.62-26-05-15x64-EN
	touch $@

build/.penetwork.stamp:
	rm -rf build/penetwork
	mkdir -p build/penetwork
	oras pull $(PENETWORK_OCI) -o build/penetwork
	7z x -y build/penetwork/penetwork.7z -obuild/penetwork/
	rm -f build/penetwork/penetwork.7z
	touch $@

build/.e7zip.stamp:
	rm -rf build/e7zip
	mkdir -p build/e7zip
	oras pull $(E7ZIP_OCI) -o build/e7zip
	unzip -o build/e7zip/e7zip.zip -d build/e7zip/
	rm -f build/e7zip/e7zip.zip
	touch $@

build/.windlls.stamp:
	rm -rf build/windlls
	mkdir -p build/windlls
	oras pull $(WIN_DLLS_OCI) -o build/windlls
	touch $@

PROGRAMS := gui-demo tray-app

.PHONY: programs
programs: $(PROGRAMS_BUILD)/gui-demo.exe $(PROGRAMS_BUILD)/tray.exe

$(PROGRAMS_BUILD)/gui-demo.exe: $(PROGRAMS_SRC)/gui-demo/main.c
	mkdir -p $(PROGRAMS_BUILD)
	$(MAKE) -C $(PROGRAMS_SRC)/gui-demo all
	cp $(PROGRAMS_SRC)/gui-demo/gui-demo.exe $@

$(PROGRAMS_BUILD)/tray.exe: $(PROGRAMS_SRC)/tray-app/main.c
	mkdir -p $(PROGRAMS_BUILD)
	$(MAKE) -C $(PROGRAMS_SRC)/tray-app all
	cp $(PROGRAMS_SRC)/tray-app/tray.exe $@

.PHONY: push-programs
push-programs: programs
	@for exe in $(PROGRAMS_BUILD)/*.exe; do \
		name=$$(basename "$$exe"); \
		echo "Pushing $$name..."; \
		python3 scripts/exec.py write_file "X:\\$$name" "$$exe"; \
	done

FORCE:
$(APPS_BUILD)/pkgs/%.wim: FORCE
	mkdir -p $(APPS_BUILD)/pkgs
	$(MAKE) -C $(APPS_SRC)/$* REGISTRY=$(REGISTRY) WIM_OUT=$(abspath $@)

.PHONY: apps
apps: $(APP_WIMS)

build/.apps.stamp: $(APPS_SRC) $(APP_WIMS)
	rm -rf $(APPS_RAW)
	mkdir -p $(APPS_BUILD)
	cp $(APPS_SRC)/* $(APPS_BUILD)/ 2>/dev/null || true
	rm -f $(APPS_BUILD)/Makefile 2>/dev/null || true
	for w in $(APPS_BUILD)/pkgs/*.wim; do \
		[ -f "$$w" ] || continue; \
		name=$$(basename "$$w" .wim); \
		[ -d "$(APPS_SRC)/$$name" ] || rm -f "$$w"; \
	done
	dd if=/dev/zero of=$(APPS_RAW) bs=1M count=$(APPS_SIZE)
	mformat -i $(APPS_RAW) -F ::
	mcopy -i $(APPS_RAW) -s $(APPS_BUILD)/* ::
	touch $@

.PHONY: apps-attach
apps-attach:
	scripts/qmp.py '{"execute":"blockdev-add","arguments":{"node-name":"apps_node","driver":"raw","file":{"driver":"file","filename":"$(APPS_RAW)"}}}'
	scripts/qmp.py '{"execute":"device_add","arguments":{"driver":"usb-storage","drive":"apps_node","id":"apps_usb","removable":true}}'

.PHONY: apps-detach
apps-detach:
	scripts/qmp.py '{"execute":"device_del","arguments":{"id":"apps_usb"}}'
	scripts/qmp.py '{"execute":"blockdev-del","arguments":{"node-name":"apps_node"}}'

.PHONY: apps-replug
apps-replug:
	scripts/qmp.py '{"execute":"device_del","arguments":{"id":"apps_usb"}}' || true
	scripts/qmp.py '{"execute":"blockdev-del","arguments":{"node-name":"apps_node"}}' || true
	$(RM) build/.apps.stamp
	$(MAKE) build/.apps.stamp
	scripts/qmp.py '{"execute":"blockdev-add","arguments":{"node-name":"apps_node","driver":"raw","file":{"driver":"file","filename":"$(APPS_RAW)"}}}'
	scripts/qmp.py '{"execute":"device_add","arguments":{"driver":"usb-storage","drive":"apps_node","id":"apps_usb","removable":true}}'

VERIFY_WIMS := $(if $(APP),$(APPS_BUILD)/pkgs/$(APP).wim,$(APP_WIMS))

.PHONY: verify
verify: $(VERIFY_WIMS)
	@for w in $(VERIFY_WIMS); do \
		echo "=== $$(basename $$w) ==="; \
		wiminfo "$$w" 1; \
		echo; \
		wimdir "$$w" 1; \
		echo; \
	done

.PHONY: rpc-broker
rpc-broker:
	python3 scripts/rpc_broker.py

.PHONY: sandbox-build
sandbox-build:
	$(MAKE) -C sandbox build

.PHONY: deploy
deploy: sandbox-build
	@curl -s -X POST http://localhost:8080/deploy | grep -q '"status": "ok"' && echo "Deployment successful!" || (echo "Deployment failed!" && exit 1)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(PROGRAMS_SRC)/gui-demo clean 2>/dev/null || true
	$(MAKE) -C $(PROGRAMS_SRC)/tray-app clean 2>/dev/null || true
