#ifndef CAMERA_H
#define CAMERA_H

#include <thread>
#include <vector>
#include "hittable.h"
#include "pdf.h"
#include "material.h"
#include "external/stb_image_write.h"

class camera {
  public:
    double aspect_ratio      = 1.0;  // Ratio of image width over height
    int    image_width       = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth         = 10;   // Maximum number of ray bounces into scene
    color  background_top;           // Scene background gradient start color
    color  background_bottom;        // Scene background gradient end color

    double vfov     = 90;              // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,0);   // Point camera is looking from
    point3 lookat   = point3(0,0,-1);  // Point camera is looking at
    vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist    = 10; // Distance from camera lookfrom point to plane of perfect focus

    int max_threads = 8; // Maximum number of rendering threads

    void render(shared_ptr<const hittable> world, shared_ptr<const hittable> lights) {
        initialize();

        uint32_t* image_data = new uint32_t[image_width * image_height];

        int thread_count = static_cast<int>(std::thread::hardware_concurrency());
        thread_count = thread_count < 1 ? 1 : thread_count;
        thread_count = thread_count > max_threads ? max_threads : thread_count;

        std::clog << "Processing image with " << thread_count << " threads\n";

        // Process a number of scanlines with each thread
        const auto scanlines_per_thread = image_height / thread_count;
        const auto scanline_remainder = image_height % thread_count;
        std::vector<std::thread> threads;
        for (int i = 0; i < thread_count - 1; i++) {
            auto start_line = i * scanlines_per_thread;
            threads.push_back(std::thread(
                &camera::render_portion, this,
                    world, lights, scanlines_per_thread, start_line, image_data
            ));
        }
        auto final_thread_start_line = (thread_count - 1) * scanlines_per_thread;
        auto final_thread_line_count = scanlines_per_thread + scanline_remainder;
        threads.push_back(std::thread(
            &camera::render_portion, this,
                world, lights, final_thread_line_count, final_thread_start_line, image_data
            ));

        threads.push_back(std::thread(&camera::print_progress, this));

        for (auto& thread : threads) thread.join();

        std::clog << "\nWriting file...\n";
        stbi_write_png("image.png", image_width, image_height, 4, image_data, image_width*4);
        delete [] image_data;

        std::clog << "Done\n";
    }

  private:
    int image_height;            // Rendered image height
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    int    sqrt_spp;             // Square root of number of samples per pixel
    double recip_sqrt_spp;       // 1 / sqrt_spp
    point3 center;               // Camera center
    point3 pixel00_loc;          // Location of pixel 0, 0
    vec3   pixel_delta_u;        // Offset to pixel to the right
    vec3   pixel_delta_v;        // Offset to pixel below
    vec3   u, v, w;              // Camera frame basis vectors
    vec3   defocus_disk_u;       // Defocus disk horizontal radius
    vec3   defocus_disk_v;       // Defocus disk vertical radius
    std::atomic_int lines_done;  // Count of lines which have finished rendering

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        lines_done = 0;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        sqrt_spp = int(std::sqrt(samples_per_pixel));
        pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.0 / sqrt_spp;

        center = lookfrom;

        // Determine viewport dimensions.
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors.
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    void render_portion(shared_ptr<const hittable> world, shared_ptr<const hittable> lights,
                        int line_count, int start_line, uint32_t* image_data) {
        for (int j = start_line; j < start_line + line_count; j++) {
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0,0,0);
                for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                    for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                        ray r = get_ray(i, j, s_i, s_j);
                        pixel_color += ray_color(r, max_depth, world, lights);
                    }
                }
                image_data[j*image_width+i] = get_color(pixel_samples_scale * pixel_color);
            }
            lines_done.fetch_add(1);
        }
    }

    ray get_ray(int i, int j, int s_i, int s_j) const {
        // Construct a camera ray originating from the defocus disk and directed at a randomly
        // sampled point around the pixel location i, j for stratified sample square s_i, s_j.

        auto offset = sample_square_stratified(s_i, s_j);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        auto ray_time = random_double();

        return ray(ray_origin, ray_direction, ray_time);
    }

    vec3 sample_square_stratified(int s_i, int s_j) const {
        // Returns the vector to a random point in the square sub-pixel specified by grid
        // indices s_i and s_j, for an idealized unit square pixel [-.5,-.5] to [+.5,+.5].

        auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    color ray_color(const ray& r, int depth, shared_ptr<const hittable> world,
                    shared_ptr<const hittable> lights) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        // If the ray hits nothing, return the background gradient color.
        if (!world->hit(r, interval(0.001, infinity), rec)) {
            vec3 unit_direction = unit_vector(r.direction());
            auto a = 0.5*(unit_direction.y() + 1.0); // [-1, 1] -> [0, 1]
            return (1.0-a)*background_bottom + a*background_top; // color gradient lerp
        }

        scatter_record srec;
        color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, srec))
            return color_from_emission;

        if (srec.skip_pdf)
            return srec.attenuation * ray_color(srec.skip_pdf_ray, depth-1, world, lights);

        auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
        mixture_pdf p(light_ptr, srec.pdf_ptr);

        ray scattered = ray(rec.p, p.generate(), r.time());
        auto pdf_value = p.value(scattered.direction());

        double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);

        color sample_color = ray_color(scattered, depth-1, world, lights);
        color color_from_scatter =
            (srec.attenuation * scattering_pdf * sample_color) / pdf_value;

        return color_from_emission + color_from_scatter;

    }

    void print_progress() const {
        int current_lines_done = 0;
        while (current_lines_done < image_height) {
            current_lines_done = lines_done.load();
            int percent_complete = current_lines_done / static_cast<double>(image_height) * 100;
            std::clog << "\r" << percent_complete << "%" << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

#endif
