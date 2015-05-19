# Terrain Generation using a GP-GPU Architecture.

This program takes a General Purpose GPU Computing approach to Terrain Generation, which it achieves by adapting the algorithms used in terrain generation so that they can be executed on the graphics card.

By using OpenCL to perform the calculations and DirectX to render the result to the screen, the program first creates a flat grid of points. It then produces a height map using multiple octaves of Perlin Noise which is them used to alter the height value of each individual point in the grid.

As a result the program is able to complete the Perlin noise calculations, as well as the other various  necessary calculations, considerably faster than what the CPU alone is capable of.

**Note**: This program requires OpenCL compliant hardware. This program was developed on an AMD 7850 and unfortunately I can't guarantee that it will work on your hardware, althgouh it should probably work on other AMD cards.