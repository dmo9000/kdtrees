#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include "Extent.h"
#include "string.h"

extern std::vector <Extent*> extentsH;
extern std::vector <Extent*> extentsV;

int clip(int n, int lower, int upper);

Extent::Extent(uint32_t ix, uint32_t iy, uint32_t iw, uint32_t ih)
{
    printf("-> Create_Extent(%d, %d, %d, %d)\n", ix, iy, iw, ih);
    x = ix;
    y = iy;
    h = ih;
    w = iw;
  
}

Extent::~Extent()
{
    printf(" -> Delete_Extent(%d, %d, %d, %d)\n", x, y, w, h);

}

void Extent::Display()
{

    printf("(%u, %u, %u, %u)\n", x, y, w, h);

}

/*
uint32_t Extent::GetSplitMode()
{
    return s;
}
*/

bool Extent::CanSplit(uint32_t splitmode, uint32_t min_size)
{

    switch (splitmode) {
    case SPLIT_HORIZONTAL:
        if (h > min_size) {
            return true;
        }
        break;
    case SPLIT_VERTICAL:
        if (w > min_size) {
            return true;
        }
        break;
    default:
        printf("UNKNOWN SPLIT MODE = %u\n", splitmode);
        exit(1);
    }

    return false;

}

bool Extent::CanSplitAny(uint32_t minx, uint32_t miny)
{
    if (minx < w) return true;
    if (miny < h) return true;

    return false;
}

bool Extent::SplitHorizontal(uint32_t miny)
{
    /* cannot be split */

    uint32_t h1 = 0;
    uint32_t h2 = 0;

    /* block is already too small */

    if (miny >= h) return false;

    int budget = h / 5;
    int segment_adjustment = (rand() % budget) - (h / 10);
    h1 = (h / 2) + segment_adjustment;
    h2 = h - h1;

    if (miny >= (h1)) return false;
    if (miny >= (h2)) return false;

    extentsV.push_back(new Extent(x, y, w, h1));
    extentsV.push_back(new Extent(x, y + h1, w, h2));

    return true; 

}

bool Extent::SplitVertical(uint32_t minx)
{
    /* cannot be split */

    uint32_t w1 = 0;
    uint32_t w2 = 0;

    /* block is already too small */    

    if (minx >= w) return false;

    int budget = w / 5;
    int segment_adjustment = (rand() % budget) - (w / 10);

    w1 = (w / 2) + segment_adjustment;
    w2 = w - w1;

    if (minx >= (w1)) return false;
    if (minx >= (w2)) return false;  
    
    extentsH.push_back(new Extent(x, y, w1, h));
    extentsH.push_back(new Extent(x + w1, y, w2, h));

    return true; 

}

int clip(int n, int lower, int upper) {
  return std::max(lower, std::min(n, upper));
}

Region* Extent::GetRegion()
{
    Region *r = (Region*) malloc(sizeof(Region));
    memset(r, 0, sizeof(Region));
    r->x1 = clip(x, 0, DIM_X - 1);
    r->y1 = clip(y, 0, DIM_Y - 1);
    r->x2 = clip(x + w, 0,  DIM_X - 1);
    r->y2 = clip(y + h, 0, DIM_Y - 1);
    return r;
}