#include "region.h"

#define DIM_X 	200
#define DIM_Y	200

#define SPLIT_HORIZONTAL	0
#define SPLIT_VERTICAL		1

class Extent {

	private:
		uint32_t x;
		uint32_t y;
		uint32_t w;
		uint32_t h;
	public:
		Extent(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
		~Extent();
		void Display();
		bool CanSplit(uint32_t sm, uint32_t minxsize, uint32_t minysize); 
		bool CanSplitAny(uint32_t minx, uint32_t miny);
		bool SplitHorizontal(uint32_t miny, float generosity);
		bool SplitVertical(uint32_t minx, float generosity);
		Region *GetRegion();

    };
