# Clouds-OpenCL-GL

Simulate clouds in real-time by raymarching using OpenCL to create noise and OpenGL to generate graphics. Written in C++, OpenCL 1.2, OpenGL 4.1. Tested on Mac OS 10.15.2, GPU: Radeon Pro 560.

# TODO

* Improve sampling of density at each point.
* Improve light effects.

INCREASE PERFORMANCE:

* Move density calculations to the OpenCL kernel to calculate density once: combine all channels (RGBA) to one channel (R) - will improve sampling at the fragment shader in OpenGL.
* Precalculate light map in a OpenCL kernel, use channel (G) of the texture to store the data. The resulting 3D texture will only have 2 channels -> improve performance at sampling and precalculate data once instead of calculating it each frame.
* Increase number of steps (after doing two previous points)/
