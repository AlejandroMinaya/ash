.PHONY: run

build/ash: src/main.c deps/SketchyBarHelper/sketchybar.h
	@echo "Building target"
	$(CC) $(CFLAGS) $< -o $@
	chmod u+x $<


run:
	pkill sketchybar
	sketchybar &
	./build/ash

