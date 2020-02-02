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

void draw_loop() {
	thread_name = "draw";
	auto fb = framebuf::make_unique();

	while(!should_quit()) {

		signal_begin_draw.notify_all();

		for(unsigned i = 0; i < WORLD_HEIGHT; i++) {
			{
				LG_alias g(world_mutex, "draw_loop");
				for(unsigned j = 0; j < WORLD_WIDTH; j++) {
					
					pixel_ pix(i, j, (grain::colours[to_underlying(world[i][j].type)]));
					draw(fb.get(), &pix);
				}
			}
		}

	}
};

int main(int argc, char **argv) {
	thread_name = "main";
	std::thread draw_thread(draw_loop);

	struct vec2u {
		unsigned x;
		unsigned y;
	};


	vec2u top_left = {10, 10};
	vec2u bottom_right = {70, 40};
	{
		LG_alias g(world_mutex, "main_loop");
		for(unsigned i = top_left.y; i < bottom_right.y; i++) {
			for(unsigned j = top_left.x; j < bottom_right.x; j++) {
				if(i%2)
					world[i][j].type = grain::type::sand;
				else
					world[i][j].type = grain::type::water;
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
