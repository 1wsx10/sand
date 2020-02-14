#include "main.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <cassert>
#include <chrono>
#include "print_mutex.hpp"


template<typename Duration, typename Clock>
void sleep_until_next(std::chrono::time_point<Clock> start_time) {
	using namespace std::chrono;

	auto time_since_start = Clock::now() - start_time;
	Duration Durations_since_start = duration_cast<Duration>(time_since_start);
	time_point alarm = start_time + Durations_since_start + Duration(1);

	std::this_thread::sleep_until(alarm);
}



std::condition_variable signal_begin_draw;
std::mutex              signal_begin_draw_mutex;

matter world[WORLD_HEIGHT][WORLD_WIDTH];
#if 1
std::mutex world_mutex;
#else
print_mutex world_mutex("world_mutex");
#endif

bool       g_quit = false;
#if 1
std::mutex quit_mutex;
#else
print_mutex quit_mutex("quit_mutex");
#endif

bool should_quit() {
	LG_alias g(quit_mutex, "should_quit");
	return g_quit;
}

// TODO
//  - version that respects aspect ratio
//  - add panning & zooming, currently we view the entire world
vec2u get_world_coords_stretch(vec2u screen_coords, vec2u screen_size) {
	unsigned inverted_y = screen_size.y - screen_coords.y - 1;
	return {
		(screen_coords.x * WORLD_WIDTH) / screen_size.x,
		(inverted_y * WORLD_HEIGHT) / screen_size.y
	};
}

template<typename frame>
void draw_loop() {
	thread_name = "draw";
	auto fb = framebuf::make_unique();
	vec2u screen_size{fb->vinfo.xres, fb->vinfo.yres};

	std::chrono::time_point start_time = std::chrono::steady_clock::now();

	while(!should_quit()) {
		sleep_until_next<frame>(start_time);

		signal_begin_draw.notify_all();

			LG_alias g(world_mutex, "draw_loop");
		for(unsigned y = 0; y < screen_size.y; y++) {
			for(unsigned x = 0; x < screen_size.x; x++) {
				vec2u world_coords = get_world_coords_stretch({x,y}, screen_size);

				RGBT pixel_colour = matter::colours[to_underlying(world[world_coords.y][world_coords.x].type)];
				pixel_colour.r *= (0.5f * world[world_coords.y][world_coords.x].is_active) + 0.5f;
				pixel_colour.g *= (0.5f * world[world_coords.y][world_coords.x].is_active) + 0.5f;
				pixel_colour.b *= (0.5f * world[world_coords.y][world_coords.x].is_active) + 0.5f;
				pixel_colour.t *= (0.5f * world[world_coords.y][world_coords.x].is_active) + 0.5f;


				pixel_ pix(x, y, pixel_colour);
				draw(fb.get(), &pix);
			}
		}

	}
};

// returns true if we moved it
bool simulate_grain(vec2u matter_coord) {
	if(matter_coord.y == 0) return false;

	// check below
	if(world[matter_coord.y - 1][matter_coord.x].type == matter::type::none) {
		// move down
		world[matter_coord.y - 1][matter_coord.x] = world[matter_coord.y][matter_coord.x];
		world[matter_coord.y][matter_coord.x] = {matter::type::none, false};
	} else {
		// check below to either side
		int left_or_right = (rand()%2) * 2 - 1;
		if(world[matter_coord.y - 1][matter_coord.x + left_or_right].type == matter::type::none) {
			world[matter_coord.y - 1][matter_coord.x + left_or_right] = world[matter_coord.y][matter_coord.x];
			world[matter_coord.y][matter_coord.x] = {matter::type::none, false};
		} else return false;
	}

	if(matter_coord.y < WORLD_HEIGHT-1) {
		// notify above grains

		world[matter_coord.y + 1][matter_coord.x].is_active = true;
		world[matter_coord.y + 1][matter_coord.x + 1].is_active = true;
		world[matter_coord.y + 1][matter_coord.x - 1].is_active = true;
	}

	return true;
}

bool simulate_liquid(vec2u matter_coord) {
	return false;
}

bool simulate_pixel(vec2u matter_coord) {
	matter& matter = world[matter_coord.y][matter_coord.x];
	if(!matter.is_active) return false;

#if 0
	{
		char buf[50];
		matter_coord.snprint(buf, 50);
		printf("coord: %s\n", buf);
	}
#endif
	switch(matter.type) {
		case matter::type::none:
			return false;
			break;

		case matter::type::sand:
			return simulate_grain(matter_coord);
			break;

		case matter::type::water:
			return simulate_liquid(matter_coord);
			break;

		case matter::type::COUNT:
		default:
			printf("ERROR unknown matter type %d\n", to_underlying(matter.type));
			assert(false);
	}
}

void simulate_screen() {

	for(unsigned y = 0; y < WORLD_HEIGHT; y++) {
		std::unique_lock lock(world_mutex);


		for(unsigned x = 0; x < WORLD_WIDTH; x++) {
			bool still_active = simulate_pixel((vec2u){x,y});
			if(!still_active)
				world[y][x].is_active = false;
		}
	}
}


template<typename tick>
void simulate_for(unsigned count) {
	std::chrono::time_point start_time = std::chrono::steady_clock::now();

	for(unsigned i = 0; i < count; i++) {
		sleep_until_next<tick>(start_time);

		simulate_screen();
	}
}

int main(int argc, char **argv) {
	using frame = std::chrono::duration<int64_t, std::ratio<1,60>>;
	using tick = std::chrono::duration<int64_t, std::ratio<1,10>>;

	std::chrono::seconds one_second(1);
	printf("frames per sec: %ld\n", frame(one_second).count());
	printf("ticks  per sec: %ld\n", tick(one_second).count());

	srand(std::chrono::steady_clock::now().time_since_epoch().count());



	thread_name = "main";
	std::thread draw_thread(draw_loop<frame>);



	// populate the world with some matter
	vec2u top_left = {10, 10};
	vec2u bottom_right = {70, 40};
	{
		LG_alias g(world_mutex, "main_loop");
		for(unsigned y = top_left.y; y < bottom_right.y; y++) {
			for(unsigned x = top_left.x; x < bottom_right.x; x++) {
				if(x%2)
					world[y][x].type = matter::type::sand;
				else
					world[y][x].type = matter::type::water;
			}
		}
	}



	simulate_for<tick>(200);



	{
		std::unique_lock lock(signal_begin_draw_mutex);
		signal_begin_draw.wait(lock);

		LG_alias g(quit_mutex, "main to-quit");
		g_quit = true;
	}

	printf("joining draw thread\n");
	draw_thread.join();
}
