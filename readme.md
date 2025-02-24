Work from following the raytracer book from https://raytracing.github.io/

Modifications
+ Use the stb library ([stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h)) to make images
+ Use xoshiro PRNG ([xoshiro.hpp](https://gist.github.com/imneme/3eb1bcc5418c4ae83c4c6a86d9cbb1cd)) for random number generation
+ Use multithreading to split scanline rendering load
+ Configurable two color gradient backgrounds
+ https://raytracing.github.io/books/RayTracingTheRestOfYourLife.html#generatingrandomdirections