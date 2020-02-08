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

template<typename frame>
void draw_loop() {
	thread_name = "draw";
	auto fb = framebuf::make_unique();
	vec2u screen_size{fb->vinfo.xres, fb->vinfo.yres};

	std::chrono::time_point start_time = std::chrono::steady_clock::now();

	while(!should_quit()) {
		sleep_until_next<frame>(start_time);

		signal_begin_draw.notify_all();

		for(unsigned x = 0; x < screen_size.x; x++) {
			LG_alias g(world_mutex, "draw_loop");
			for(unsigned y = 0; y < screen_size.y; y++) {
				vec2u world_coords = get_world_coords_stretch({x,y}, screen_size);

				pixel_ pix(x, y, (matter::colours[to_underlying(world[world_coords.y][world_coords.x].type)]));
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
		world[matter_coord.y - 1][matter_coord.x + left_or_right] = world[matter_coord.y][matter_coord.x];
		world[matter_coord.y][matter_coord.x] = {matter::type::none, false};
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

void simulate(vec2u matter_coord) {
	matter& matter = world[matter_coord.y][matter_coord.x];

#if 0
	{
		char buf[50];
		matter_coord.snprint(buf, 50);
		printf("coord: %s\n", buf);
	}
#endif
	switch(matter.type) {
		case matter::type::none:
			break;
		case matter::type::sand:
			simulate_grain(matter_coord);
			break;
		case matter::type::water:
			simulate_liquid(matter_coord);
			break;
		case matter::type::COUNT:
		default:
			printf("ERROR unknown matter type\n");
			assert(false);
	}
}

void simulate() {
	for(unsigned y = 0; y < WORLD_HEIGHT; y++) {
		for(unsigned x = 0; x < WORLD_WIDTH; x++) {
			simulate((vec2u){x,y});
		}
	}
}


template<typename tick>
void simulate(unsigned count) {
	std::chrono::time_point start_time = std::chrono::steady_clock::now();

	for(unsigned i = 0; i < count; i++) {
		sleep_until_next<tick>(start_time);

		simulate();
	}
}

int main(int argc, char **argv) {
	using namespace std::chrono;

	using frame = duration<int64_t, std::ratio<1,60>>;
	using tick = duration<int64_t, std::ratio<1,10>>;

	seconds one_second(1);
	printf("frames per sec: %ld\n", frame(one_second).count());
	printf("ticks  per sec: %ld\n", tick(one_second).count());







	thread_name = "main";
	std::thread draw_thread(draw_loop<frame>);
	//draw_loop<frames>();



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



	simulate<tick>(200);



	{
		std::unique_lock lock(signal_begin_draw_mutex);
		signal_begin_draw.wait(lock);

		LG_alias g(quit_mutex, "main to-quit");
		g_quit = true;
	}

	printf("joining draw thread\n");
	draw_thread.join();
}
