#include "main.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <cassert>
#include "print_mutex.hpp"




std::condition_variable signal_begin_draw;
std::mutex              signal_begin_draw_mutex;

grain world[WORLD_HEIGHT][WORLD_WIDTH];
std::mutex world_mutex;
//print_mutex world_mutex("world_mutex");

bool       g_quit = false;
std::mutex quit_mutex;
//print_mutex quit_mutex("quit_mutex");

bool should_quit() {
	LG_alias g(quit_mutex, "should_quit");
	return g_quit;
}

// TODO
//  - version that respects aspect ratio
//  - add panning & zooming, currently we view the entire world
vec2u get_world_coords_stretch(vec2u screen_coords, vec2u screen_size) {
	return {
		(screen_coords.x * WORLD_WIDTH) / screen_size.x,
		(screen_coords.y * WORLD_HEIGHT) / screen_size.y
	};
}

void draw_loop() {
	thread_name = "draw";
	auto fb = framebuf::make_unique();
	vec2u screen_size{fb->vinfo.xres, fb->vinfo.yres};

	while(!should_quit()) {
		signal_begin_draw.notify_all();

		for(unsigned x = 0; x < screen_size.x; x++) {
			LG_alias g(world_mutex, "draw_loop");
			for(unsigned y = 0; y < screen_size.y; y++) {
				vec2u world_coords = get_world_coords_stretch({x,y}, screen_size);

				pixel_ pix(x, y, (grain::colours[to_underlying(world[world_coords.y][world_coords.x].type)]));
				draw(fb.get(), &pix);
			}
		}

	}
};

int main(int argc, char **argv) {
	thread_name = "main";
	std::thread draw_thread(draw_loop);



	vec2u top_left = {10, 10};
	vec2u bottom_right = {70, 40};
	{
		LG_alias g(world_mutex, "main_loop");
		for(unsigned y = top_left.y; y < bottom_right.y; y++) {
			for(unsigned x = top_left.x; x < bottom_right.x; x++) {
				if(x%2)
					world[y][x].type = grain::type::sand;
				else
					world[y][x].type = grain::type::water;
			}
		}
	}



	{
		std::unique_lock lock(signal_begin_draw_mutex);
		signal_begin_draw.wait(lock);

		LG_alias g(quit_mutex, "main to-quit");
		g_quit = true;
	}

	printf("joining draw thread\n");
	draw_thread.join();
}
