#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "Extent.h"
#include "string.h"
#include "math.h"
#include "libpng16/png.h"



#define MIN_X	25
#define MIN_Y	25

std::vector <Extent*> extentsH;
std::vector <Extent*> extentsV;
std::vector <Extent*> extentsOut;
std::vector <Extent*> RegionList;

void dump_stack();
void process_stack(int reg_xmin, int reg_ymin, int reg_dev);
int png_setpixel(int, int, unsigned char, unsigned char, unsigned int, int scale);
int png_drawline(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);
int png_drawbox(int x1, int y1,int x2, int y2, int r, int g, int b);
int png_fillbox(int x1, int y1,int x2, int y2, int r, int g, int b);

int scale_setpixel(int, int, unsigned char, unsigned char, unsigned int, int scale);
int scale_drawline(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b, int scale);
int scale_drawbox(int x1, int y1,int x2, int y2, int r, int g, int b, int scale);
int scale_fillbox(int x1, int y1,int x2, int y2, int r, int g, int b, int scale);

bool write_png_file(char *filename, int scale);
void generate_png_data(bool fill, int scale);
png_bytep *row_pointers;


int main(int argc, char *argv[])
{

    if (argc < 7) {
        printf("usage: ./main.cpp <reg_xmin> <reg_ymin> <reg_dev> <clu_xmin> <clu_ymin> <clu_dev>\n");
        exit(1);
    }

    int reg_xmin = atoi(argv[1]);
    int reg_ymin = atoi(argv[2]);
    int reg_dev  = atoi(argv[3]);
		int clu_xmin = atoi(argv[4]);
		int clu_ymin = atoi(argv[5]);
		int clu_dev  = atoi(argv[6]);
		int seed = 0;
		int scale = 3;
		/* optional, otherwise we get it from srand(time(NULL)) which seeds it from the current offset from epoch */;
		if (argc == 8) {
			seed = atoi(argv[7]);			
			}

		if (argc == 9) {
			scale = atoi(argv[8]);			
			}

    if ((reg_xmin < 20 || reg_xmin > DIM_X) ||
            (reg_xmin < 20 || reg_xmin > DIM_X)) {
        printf("reg_xmin/reg_ymin out of bounds\n");
        exit (1);
    }

    if (reg_dev < 0 || reg_dev > 50) {
        printf("reg_dev must be between 0 and 50\n");
        exit(1);
    }

    if ((clu_xmin < 3 || clu_xmin > DIM_X) ||
            (clu_xmin < 3 || clu_xmin > DIM_X)) {
        printf("clu_xmin/clu_ymin out of bounds\n");
        exit (1);
    }

    if (clu_dev < 0 || clu_dev > 50) {
        printf("clu_dev must be between 0 and 50\n");
        exit(1);
    }

    /* we use a vector here with push_back()/pop_back(), as std::stack doesn't have
     * an iterator */

    printf("Seeding random number generator ...\n");
		if (seed == 0) {
	    srand(time(NULL));
			} else {
			srand(seed);
			}

    /* initialize PNG output buffer */

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * (DIM_Y*scale));
    for(int y = 0; y < (DIM_Y*scale); y++) {
        row_pointers[y] = (png_byte*)malloc((DIM_X*scale)*4);
        memset(row_pointers[y], 0xff, (DIM_X*scale)*4);
    }

    /* create the master block and push it */

    printf("Creating initial map extent ...\n");
    extentsH.push_back(new Extent(0,0, DIM_X, DIM_Y));

    while (extentsH.size() || extentsV.size()) {
        process_stack(reg_xmin, reg_ymin, reg_dev);
    }
    printf("Queues have drained.\n");
    dump_stack();

 

    generate_png_data(true, scale);

    RegionList = extentsOut;

    extentsH.clear();
    extentsV.clear();
    extentsOut.clear();

    printf("extentsH contains: %d regions\n", extentsH.size());
    printf("extentsV contains: %d regions\n", extentsV.size());
    printf("extentsOut contains: %d regions\n", extentsOut.size());
    printf("RegionList contains: %d regions\n", RegionList.size());

    std::vector<Extent*>::iterator it;

    int ri = 0;

    for(it=RegionList.begin() ; it < RegionList.end(); it++ ) {
        Extent *myExtent = *it;
        printf("Subdividing region #%d ...\n", ri);
        Region *r = myExtent->GetRegion();
        extentsH.push_back(new Extent(r->x1, r->y1, r->w, r->h));

        while (extentsH.size() || extentsV.size()) {
            process_stack(clu_xmin, clu_ymin, clu_dev);
        }
        printf("Queues have drained.\n");
        dump_stack();
        generate_png_data(false, scale);

        /* increment region index */
        ri++;
    }

    write_png_file((char *) "extents.png", scale);


}


void process_stack(int reg_xmin, int reg_ymin, int reg_dev)
{

    bool cansplit = false;

    while (extentsH.size()) {
        Extent *current = extentsH.back();
        extentsH.pop_back();
//        printf("process_stack(): processing horizontal split:\n");
        current->Display();
        cansplit = current->CanSplitAny(reg_xmin, reg_ymin);
        if (!cansplit) {
            //    printf("Encountered unsplittable block - pushing to output queue.\n");
            extentsOut.push_back(current);
            return;
        }
        cansplit = current->CanSplit(SPLIT_HORIZONTAL, reg_xmin, reg_ymin);
        if (cansplit) {
            //     printf("Block can be split horizontally\n");
            if (!current->SplitHorizontal(reg_ymin, reg_dev)) {
                //          printf("Block refused to split. Putting on output queue.\n");
                extentsOut.push_back(current);
            } else {
                /* block was split into two new ones, so delete it */
                delete current;
            }
        } else {
//           printf("Cannot be split horizontally. Pushing to vertical queue.\n");
            /* push to the vertical split queue */
            extentsV.push_back(current);
        }
    }
//    printf("Horizontal split queue is now empty.\n");

    while (extentsV.size()) {
        Extent *current = extentsV.back();
        extentsV.pop_back();
        //      printf("process_stack(): processing vertical split:\n");
        current->Display();
        cansplit = current->CanSplitAny(reg_xmin, reg_ymin);
        if (!cansplit) {
//            printf("Encountered unsplittable block. Pushing to output queue.\n");
            extentsOut.push_back(current);
            return;
        }
        cansplit = current->CanSplit(SPLIT_VERTICAL, reg_xmin, reg_ymin);
        if (cansplit) {
//            printf("Block can be split vertically.\n");
            if (!current->SplitVertical(reg_xmin, reg_dev)) {
//                printf("Block refused to split. Putting on output queue.\n");
                extentsOut.push_back(current);
            } else {
                /* block was split into two new ones, so delete it */
                delete current;
            }
        } else {
//            printf("Cannot be split vertically.\n");
            /* push to the horizontal split queue */
            extentsH.push_back(current);
            /* block no longer exists, two new ones have been created on other list, so delete it */
            delete current;
        }
    }

//    printf("Vertical split queue is now empty.\n");

    return;
}


void dump_stack()
{

    printf("---------------------------------------------------------------\n");
    printf("Output Stack contains %d items\n", extentsOut.size());
    printf("---------------------------------------------------------------\n");

    std::vector<Extent*>::iterator it;

    for(it=extentsOut.begin() ; it < extentsOut.end(); it++ ) {
        Extent *myExtent = *it;
        myExtent->Display();
    }
    printf("---------------------------------------------------------------\n");
}


void generate_png_data(bool fillboxes, int scale)
{
    printf("generate_png_data()\n");
    int cr, cg, cb;
    int region_count = extentsOut.size();
    int region_index = 0;
    std::vector<Extent*>::iterator it;
    for(it=extentsOut.begin() ; it < extentsOut.end(); it++ ) {
        Extent *myExtent = *it;
        Region *r = myExtent->GetRegion();

        if (r) {
            if (fillboxes) {
                if (extentsOut.size() ==1) {
                    cr = 255;
                    cg = 255;
                    cb = 255;
                } else {
                    cr = (rand() % 150)+50;
                    cg = (rand() % 150)+50;
                    cb = (rand() % 150)+50;
                }

                scale_fillbox(r->x1, r->y1, r->x2, r->y2, cr, cg, cb, scale);
            }
            scale_drawbox(r->x1, r->y1, r->x2, r->y2, 0, 0, 0, scale);
            //free(r);
        } else {
            printf("Did not get region data during output.\n");
            exit(1);
        }
        region_index++;
    }
    printf("generate_png_data() ok\n");
    return;
}


bool write_png_file(char *filename, int scale)
{
    png_byte color_type;
    png_byte bit_depth;
    png_structp png;
    png_infop info;

    printf("open_png_file(%s)\n", filename);

    FILE *fp = fopen(filename, "wb");
    if(!fp) abort();

    printf("open_png_file() ok\n");


    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();
    info = png_create_info_struct(png);
    if (!info) abort();
    printf("open_png_file() ok\n");


    if (setjmp(png_jmpbuf(png))) abort();
    png_init_io(png, fp);
    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        scale * DIM_X, scale * DIM_Y,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );


    png_write_info(png, info);
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    for(int y = 0; y < (scale * DIM_Y); y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
}


int png_drawline(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b, int scale)
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
            png_setpixel(x,y,r,g,b, scale);
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

                png_setpixel(x,y,r,g,b, scale);
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
            png_setpixel(x,y,r, g, b, scale);
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
                png_setpixel(x,y,r,g,b, scale);
            }
        }
    }
}


int png_setpixel(int x, int y, unsigned char r, unsigned char g, unsigned int b, int scale)
{

    if (x < 0 || x >= (scale * DIM_X)) return 0;
    if (y < 0 || y >= (scale * DIM_Y)) return 0;
    unsigned char* pixoffset = row_pointers[y];
    pixoffset += x * 4;
    *pixoffset = r;
    pixoffset++;
    *pixoffset = g;
    pixoffset++;
    *pixoffset = b;
    return 0;
}

int png_drawbox(int x1, int y1,int x2, int y2, int r, int g, int b, int scale)
{
    png_drawline(x1, y1, x2, y1, r, g, b, scale);
    png_drawline(x2, y1, x2, y2, r, g, b, scale);
    png_drawline(x2, y2, x1, y2, r, g, b, scale);
    png_drawline(x1, y2, x1, y1, r, g, b, scale);
    return 0;
}

int png_fillbox(int x1, int y1,int x2, int y2, int r, int g, int b, int scale)
{

    // printf("png_fillbox(%d, %d, %d, %d, %d, %d, %d)\n", x1, y1, x2, y2, r, g, b);
    for (int i = y1; i < y2; i++) {
        png_drawline(x1, i, x2, i, r, g, b, scale);
    }
}


int scale_fillbox(int x1, int y1,int x2, int y2, int r, int g, int b, int scale)
{

    for (int i = (y1); i < (y2) ; i++) {
        scale_drawline(x1, i, x2, i, r, g, b, scale);
    }

}


int scale_drawbox(int x1, int y1,int x2, int y2, int r, int g, int b, int scale)
{

    scale_drawline(x1, y1, x2, y1, r, g, b, scale);
    scale_drawline(x2, y1, x2, y2, r, g, b, scale);
    scale_drawline(x2, y2, x1, y2, r, g, b, scale);
    scale_drawline(x1, y2, x1, y1, r, g, b, scale);
}

int scale_setpixel(int x, int y, unsigned char r, unsigned char g, unsigned int b, int scale)
{

    if (x < 0 || x >= (DIM_X)) return 0;
    if (y < 0 || y >= (DIM_Y)) return 0;

		png_fillbox(x * scale, y * scale, (x+1) * scale, (y+1) * scale, r, g, b, scale);

    return 0;
}


int scale_drawline(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b, int scale)
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
            scale_setpixel(x,y,r,g,b, scale);
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

                scale_setpixel(x,y,r,g,b, scale);
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
            scale_setpixel(x,y,r, g, b, scale);
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
                scale_setpixel(x,y,r,g,b, scale);
            }
        }
    }
}
