

all: main

%.o:	%.cpp
	g++ -c $< -o $@ -g -ggdb

main: main.o Extent.o
	g++ -o main main.o Extent.o /usr/lib/libpng16.dll.a -g -ggdb

clean:
	rm -f *.exe main.exe *.stackdump *.o *.png
