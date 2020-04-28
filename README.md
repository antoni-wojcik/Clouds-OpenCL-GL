# Clouds-OpenCL-GL

Simulate clouds in real-time by raymarching using OpenCL to create noise and OpenGL to generate graphics. Written in C++, OpenCL 1.2, OpenGL 4.1. Tested on Mac OS 10.15.3, GPU: AMD Radeon Pro 560.

# TODO

* Change the sampling number from a constant to a variable depending on the angle from a normal to the camera's plane to the direction of the ray.
* Improve sampling of density at each point.
* Improve light effects.

INCREASE PERFORMANCE:

* ✓✓✓ Decrease resolution of the output (on Retina displays) and scale the image back to the desired size - use a separate, smaller framebuffer (GL_NEAREST) -> write to texture -> scale texture using a fragment shader -> draw on the main framebuffer (at each frame). This will give a 4x increase in performance. 
* Move density calculations to the OpenCL kernel to calculate density once: combine all channels (RGBA) to one channel (R) - will improve sampling at the fragment shader in OpenGL.
* Precalculate light map in a OpenCL kernel, use channel (G) of the texture to store the data. The resulting 3D texture will only have 2 channels -> improve performance at sampling and precalculate data once instead of calculating it each frame.
* Increase number of steps (after doing two previous points)
