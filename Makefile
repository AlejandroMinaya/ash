.PHONY: run build-and-run

build/ash: src/main.c deps/SketchyBarHelper/sketchybar.h
	@echo "Building target"
	mkdir -p build/
	$(CC) $(CFLAGS) $< -o $@
	chmod u+x $<


run: build/ash
	pkill sketchybar&
	sketchybar&
	./build/ash

