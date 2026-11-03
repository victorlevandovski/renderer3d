#include <stdlib.h>
#include <stdio.h>
#include "fenster.h"
#include "lib/fenster_ext.h"

#define W 1280
#define H 800

int main() {
    uint32_t buf[W * H];
    struct fenster f = { .title = "hello", .width = W, .height = H, .buf = buf };
    fenster_open(&f);
    
    int64_t last = fenster_time();
    int frames = 0;
    double fps = 0;
    
    while (fenster_loop(&f) == 0) {
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                fenster_pixel(&f, i, j) = rand();
            }
        }
        
        int64_t now = fenster_time();
        int64_t dt = now - last;
        frames++;
        
        if (dt >= 1000) {
            fps = frames * 1000.0 / dt;
            frames = 0;
            last = now;
        }
        
        char text[32];
        snprintf(text, sizeof(text), "FPS: %.0f", fps);
        fenster_text(&f, 8, 8, text, 2, 0xffffff);
    }
    
    fenster_close(&f);
    
    return 0;
}
