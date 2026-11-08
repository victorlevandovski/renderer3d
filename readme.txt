In case anyone accidentally bumps into this project here...

This is a software renderer for a narrow use case: the camera
and every object are attached to the world plane, so the height
(Z axis) is always zero for all of them. The camera viewport is
a circle, not a rectangle (like a telescope/periscope).

This simplifies quite a lot of calculations that happen before
rasterizing the triangles. No shadows, or maybe some simplified
ones. Only a single direct light.

Also, I just started learning C, so the code quality and my use
of the language will improve as I spend more time on it, but
don't expect the code itself to be a good example of programming
in C.
