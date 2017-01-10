

all: main

%.o:	%.cpp
	g++ -c $< -o $@ -g -ggdb

main: main.o Extent.o
	g++ -o main main.o Extent.o /usr/lib/libpng16.dll.a -g -ggdb

upscale:
	convert -geometry 600x600 extents.png extents.png

clean:
	rm -f *.exe main.exe *.stackdump *.o *.png
