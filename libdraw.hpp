#pragma once

#include "drawlib.h"
#include <memory>

/*  a C++ wrapper for drawlib
 */

class framebuf : public FBINFO {
	public:
		struct deleter { void operator()(framebuf* __ptr) const; };

		static std::unique_ptr<framebuf, framebuf::deleter> make_unique();

	protected:
		// don't want anything to call this
		framebuf() {};
};



class pixel_ : public PIXEL {
	public:
		int x;
		int y;
		RGBT colour;

//complaints about compound literals
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
		pixel_(int tx, int ty, RGBT tcolour): PIXEL((PIXEL){.x=&x, .y=&y, .colour=&colour}), x(tx), y(ty), colour(tcolour) {};
#pragma GCC diagnostic pop

		double luma() const {
			return colour.r*1.0/255*0.299 + colour.g*01.0/255*.587 
				+ colour.b*01.0/255*.114;
		}
		void clamp() {
			struct col {
				int v[4];
			};
			col *v = (col*)&colour;
			for(int i = 0; i < 3; i++) {
				if(v->v[i] < 0) v->v[i] = 0;
				if(v->v[i] > 255) v->v[i] = 255;
			}
		}
		void clamp_with_desaturation() {
			struct col {
				int v[4];
			};
			col *v = (col*)&colour;
			/* if the colour represented by this triplet is too bright or too
			 * dim, decrease the saturation as much as required, while keeping
			 * the luma enmodified.
			 */
			double l = luma();
			double sat = 1.0;
			if(l > 255) {v->v[0] = v->v[1] = v->v[2] = 255; return;}
			if(l < 0) {v->v[0] = v->v[1] = v->v[2] = 0; return;}
			/* if any component is over the bounds, calculate how much the
			 * saturation must be reduced to achive an in-bounds value. Since
			 * the luma was verified to be in 0..255 range, a maximum
			 * reduction of saturation to 0% will always produce an in-bounds
			 * value, but usually such a drastic reduction is not necessary.
			 * Because we're only doing relative modifications, we don't need
			 * to determine the original saturation of the pixel.
			 */

			for(int i = 0; i < 3; i++) {
				     if(v->v[i] > 255) sat = fmin(sat, (l-1.0) / (l-v->v[i]));
				else if(v->v[i] <   0) sat = fmin(sat, l       / (l-v->v[i]));

				if(sat != 1.0) {
					for(int i = 0; i < 3; i++) {
						v->v[i] = (v->v[i] - l) * sat + l;
					}
					clamp();
				}
			}

		}

};
