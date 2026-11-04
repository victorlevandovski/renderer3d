#include <stdlib.h>
#include <stdio.h>

#include "fenster.h"
#include "lib/fenster_ext.h"

#define W 800
#define H 800

int main()
{
    uint32_t buf[W * H];
    struct fenster f = { .title = "renderer3d", .width = W, .height = H, .buf = buf };
    fenster_open(&f);
    
    int accu = 0;
    int64_t last = 0;
    int64_t prev = fenster_time();
    char text[32];

    while (fenster_loop(&f) == 0) {
        int64_t now = fenster_time();
        double delta_time = (double) (now - prev) * 0.001;

        if (f.keys[27])
            break;

        memset(f.buf, 0, f.width * f.height * sizeof(uint32_t));

        // draw here...

        if (now - last > 999) {
            last = now;
            snprintf(text, sizeof(text), "FPS: %d", accu);
            accu = 0;
        }
        fenster_text(&f, 8, 8, text, 2, 0xffffff);

        accu += 1;
        prev = now;
    }
    
    fenster_close(&f);
    
    return 0;
}
