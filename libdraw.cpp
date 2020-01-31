#include "libdraw.hpp"


std::unique_ptr<framebuf, framebuf::deleter> framebuf::make_unique() {
	framebuf* ptr = static_cast<framebuf*>(init());

	//this is calling the ctor, which init already does
	//*static_cast<FBINFO*>(ptr) = FBINFO();

	std::unique_ptr<framebuf, deleter> uniq = std::unique_ptr<framebuf, deleter>(ptr, deleter());


	return uniq;
}

void framebuf::deleter::operator()(framebuf* __ptr) const {
	end(__ptr);
}
