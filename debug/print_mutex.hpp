#pragma once

#include <mutex>
#include <cstdio>

extern thread_local const char* thread_name;


// printing mutex
//   prints inside locked code section
struct print_mutex {
	std::mutex mutex;
	char const* const name;

	print_mutex(const char* name) : name(name) {};

	void lock() {
		struct defer_print {
			const char* name;
			~defer_print() {
				printf("locking            %8s %s\n", name, thread_name);
			}
		};
		defer_print defer = {name};
		mutex.lock();
	}

	void unlock() {
		printf("unlocking          %8s %s\n", name, thread_name);
		return mutex.unlock();
	}

	bool try_lock() {
		bool is_locked = mutex.try_lock();
		if(is_locked) {
			printf("try_lock success   %-8s %s\n", name, thread_name);
		} else {
			printf("try_lock failure   %-8s %s\n", name, thread_name);
		}
		return is_locked;
	}
};

// printing lock_guard
//   prints outside locked code section
template<typename MUTEX>
struct print_LG {
	const char* context_name;
	MUTEX& mutex;

	print_LG(MUTEX& m, const char* n) : context_name(n), mutex(m) {
		printf("getting mutex   %s\n", context_name);
		mutex.lock();
	};
	~print_LG() {
		mutex.unlock();
		printf("releasing mutex %s\n", context_name);
	}
};

// printing lock_guard specialization:
//   if we're given a printing mutex, also print its name
template<>
struct print_LG<print_mutex> {
	const char* context_name;
	print_mutex& mutex;

	print_LG(print_mutex& m, const char* n) : context_name(n), mutex(m) {
		printf("getting mutex   %15s %s\n", m.name, context_name);
		mutex.lock();
	};
	~print_LG() {
		mutex.unlock();
		printf("releasing mutex %15s %s\n", mutex.name, context_name);
	}
};

// lock_guard alias
//   use a normal lock_guard, but have aconstructor that accepts a name.. do nothing with it
template<typename MUTEX>
struct LG_alias : public std::lock_guard<MUTEX> {
	LG_alias(MUTEX& mutex, const char* name) : std::lock_guard<MUTEX>(mutex) {}
};

// if we have a print_mutex, become a print_LG
template<>
struct LG_alias<print_mutex> : public print_LG<print_mutex> {
	LG_alias(print_mutex& mutex, const char* name) : print_LG<print_mutex>(mutex, name) {}
};
