#include <stdio.h>
#include <stdlib.h>

#include "../deps/SketchyBarHelper/sketchybar.h"

void handler(env env) {
    printf("Hello World!\n");
}
int main() {
    event_server_begin(handler, "ash");
}
