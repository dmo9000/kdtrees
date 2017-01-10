#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "Extent.h"
#include "string.h"
#include "math.h"
#include "libpng16/png.h"



#define MIN_X	25
#define MIN_Y	25

std::vector <Extent*> extentsH;
std::vector <Extent*> extentsV;
std::vector <Extent*> extentsOut;

void dump_stack();
void process_stack();
void write_png_file(char *);
int png_setpixel(int, int, unsigned char, unsigned char, unsigned int);
int png_drawline(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);
int png_drawbox(int x1, int y1,int x2, int y2, int r, int g, int b);
int png_fillbox(int x1, int y1,int x2, int y2, int r, int g, int b);

png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;

int main(int argc, char *argv[])
{

    /* we use a vector here with push_back()/pop_back(), as std::stack doesn't have
     * an iterator */

    printf("Seeding random number generator ...");
    srand(time(NULL));

    printf("Creating initial map extent ...\n");
    extentsH.push_back(new Extent(0,0, DIM_X, DIM_Y));

    while (extentsH.size() || extentsV.size()) {
        process_stack();
    }
    printf("Queues have drained.\n");
    dump_stack();
    write_png_file("extents.png");
}


void process_stack()
{

    bool cansplit = false;

    while (extentsH.size()) {
        Extent *current = extentsH.back();
        extentsH.pop_back();
        printf("process_stack(): processing horizontal split:\n");
        current->Display();
        cansplit = current->CanSplitAny(MIN_X, MIN_Y);
        if (!cansplit) {
            printf("Encountered unsplittable block - pushing to output queue.\n");
            exit (1);
        }
        cansplit = current->CanSplit(SPLIT_HORIZONTAL, MIN_Y);
        if (cansplit) {
            printf("Block can be split horizontally\n");
            if (!current->SplitHorizontal(MIN_Y)) {
                printf("Block refused to split. Putting on output queue.\n");
                extentsOut.push_back(current);
            } else {
                /* block was split into two new ones, so delete it */
                delete current;
            }
        } else {
            printf("Cannot be split horizontally. Pushing to vertical queue.\n");
            /* push to the vertical split queue */
            extentsV.push_back(current);
        }
    }
    printf("Horizontal split queue is now empty.\n");

    while (extentsV.size()) {
        Extent *current = extentsV.back();
        extentsV.pop_back();
        printf("process_stack(): processing vertical split:\n");
        current->Display();
        cansplit = current->CanSplitAny(MIN_X, MIN_Y);
        if (!cansplit) {
            printf("Encountered unsplittable block. Pushing to output queue.\n");
            exit (1);
        }
        cansplit = current->CanSplit(SPLIT_VERTICAL, MIN_X);
        if (cansplit) {
            printf("Block can be split vertically.\n");
            if (!current->SplitVertical(MIN_X)) {
                printf("Block refused to split. Putting on output queue.\n");
                extentsOut.push_back(current);
            } else {
                /* block was split into two new ones, so delete it */
                delete current;
            }
        } else {
            printf("Cannot be split vertically.\n");
            /* push to the horizontal split queue */
            extentsH.push_back(current);
            /* block no longer exists, two new ones have been created on other list, so delete it */
            delete current;
        }
    }

    printf("Vertical split queue is now empty.\n");

    return;
}


void dump_stack()
{

    printf("---------------------------------------------------------------\n");
    printf("Output Stack now contains %d items\n", extentsOut.size());
    printf("Extents remaining on the stack at completion:\n");
    printf("---------------------------------------------------------------\n");

    std::vector<Extent*>::iterator it;

    for(it=extentsOut.begin() ; it < extentsOut.end(); it++ ) {
        Extent *myExtent = *it;
        myExtent->Display();
    }
    printf("---------------------------------------------------------------\n");
}

void write_png_file(char *filename) {
    int y;
    int width = DIM_X;
    int height = DIM_Y;


    FILE *fp = fopen(filename, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(width*4);
        memset(row_pointers[y], 0xff, width*4);

    }

    int region_count = extentsOut.size();
    float shaderstep = 200 / region_count;
    int region_index = 0;
    std::vector<Extent*>::iterator it;  
    for(it=extentsOut.begin() ; it < extentsOut.end(); it++ ) {
        Extent *myExtent = *it;
        Region *r = myExtent->GetRegion();

        int blueindex = (int) 50 + (int) (region_index * shaderstep);

        if (r) { 
            png_fillbox(r->x1, r->y1, r->x2, r->y2, 0, 0, blueindex);
            png_drawbox(r->x1, r->y1, r->x2, r->y2, 0, 0, 0);
            free(r);
        } else {
            printf("Did not get region data during output.\n");
            exit(1);
        }
        region_index++;
    }

    if (setjmp(png_jmpbuf(png))) abort();
    png_init_io(png, fp);
    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    for(int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
}


int png_drawline(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b)
{
    {
        int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;
        dx=x2-x1;
        dy=y2-y1;
        dx1=fabs(dx);
        dy1=fabs(dy);
        px=2*dy1-dx1;
        py=2*dx1-dy1;
        if(dy1<=dx1)
        {
            if(dx>=0)
            {
                x=x1;
                y=y1;
                xe=x2;
            }
            else
            {
                x=x2;
                y=y2;
                xe=x1;
            }
            png_setpixel(x,y,r,g,b);
            for(i=0; x<xe; i++)
            {
                x=x+1;
                if(px<0)
                {
                    px=px+2*dy1;
                }
                else
                {
                    if((dx<0 && dy<0) || (dx>0 && dy>0))
                    {
                        y=y+1;
                    }
                    else
                    {
                        y=y-1;
                    }
                    px=px+2*(dy1-dx1);
                }

                png_setpixel(x,y,r,g,b);
            }
        }
        else
        {
            if(dy>=0)
            {
                x=x1;
                y=y1;
                ye=y2;
            }
            else
            {
                x=x2;
                y=y2;
                ye=y1;
            }
            png_setpixel(x,y,r, g, b);
            for(i=0; y<ye; i++)
            {
                y=y+1;
                if(py<=0)
                {
                    py=py+2*dx1;
                }
                else
                {
                    if((dx<0 && dy<0) || (dx>0 && dy>0))
                    {
                        x=x+1;
                    }
                    else
                    {
                        x=x-1;
                    }
                    py=py+2*(dx1-dy1);
                }

                png_setpixel(x,y,r,g,b);
            }
        }
    }


}

int png_setpixel(int x, int y, unsigned char r, unsigned char g, unsigned int b)
{

    if (x < 0 || x >= DIM_X) return 0;
    if (y < 0 || y >= DIM_Y) return 0;

    unsigned char* pixoffset = row_pointers[y];
    pixoffset += x * 4;
    *pixoffset = r;
    pixoffset++;
    *pixoffset = g;
    pixoffset++;
    *pixoffset = b;
    return 0;
}

int png_drawbox(int x1, int y1,int x2, int y2, int r, int g, int b)
{

    png_drawline(x1, y1, x2, y1, r, g, b);
    png_drawline(x2, y1, x2, y2, r, g, b);
    png_drawline(x2, y2, x1, y2, r, g, b);
    png_drawline(x1, y2, x1, y1, r, g, b);
    return 0;
}

int png_fillbox(int x1, int y1,int x2, int y2, int r, int g, int b)
{

    printf("png_fillbox(%d, %d, %d, %d, %d, %d, %d)\n", x1, y1, x2, y2, r, g, b);
    for (int i = y1; i < y2; i++) {
        png_drawline(x1, i, x2, i, r, g, b);
    }
}



