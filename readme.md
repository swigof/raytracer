Work from 
[raytracing in one weekend](https://raytracing.github.io/)

Uses the stb library 
([stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h)) 
to make pngs 

Uses xoshiro 
([xoshiro.hpp](https://gist.github.com/imneme/3eb1bcc5418c4ae83c4c6a86d9cbb1cd)) 
for random number generation

### Multithreading

This project adds multithreaded rendering by chunking image scanlines into separate threads.
This significantly reduces render times but isn't an ideal implementation.
Different parts of the image can be busier than others so threads will arbitrarily take
varying amounts of time to finish.
Ideally, the pixel sampling process would be multithreaded instead.
I have attempted to do this a couple different ways to varying levels of success.

#### Futures

On the `multithread-sampling-futures` branch is an implementation using `std::future` where
each thread accumulates its portion of the sample color independently, after which they are all 
added together.

This unfortunately produces an image which is darker scaling with the number of threads used.
Below is the same scene on one thread and six threads respectively.

![futures-1-thread](images/futures-1-thread.png "1 Thread")
![futures-6-threads](images/futures-6-threads.png "6 Threads")

This might most obviously be caused by samples being missed.
After verifying however, this isn't the case and the total sample count is as expected.

My only remaining suspicion is some kind of floating point error caused by summing separate 
accumulators rather than always summing a single color value per sample.

#### Shared Accumulator

TODO
