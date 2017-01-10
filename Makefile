

all: main

%.o:	%.cpp
	g++ -c $< -o $@ -g -ggdb

main: main.o Extent.o
	g++ -o main main.o Extent.o /usr/lib/libpng16.dll.a -g -ggdb

all-examples:
	./main.exe 200 200 0 5 5 0 666
	cp extents.png legacy-666.png
	./main.exe 200 200 0 200 200 0 666
	cp extents.png singleregion-no-clusters-666.png
	./main.exe 25 25 50 200 200 0 666
	cp extents.png multiregion-no-clusters-666.png
	./main.exe 50 50 0 200 200 0 666
	cp extents.png region50-nodeviation-no-clusters-666.png
	./main.exe 50 50 25 200 200 0 666
	cp extents.png region50-deviation25-no-clusters-666.png

	./main.exe 25 25 50 4 4 0 666
	cp extents.png multiregion-square-clusters-666.png
	./main.exe 25 25 50 4 4 25 666
	cp extents.png multiregion-rectangle-clusters-666.png
	./main.exe 25 25 50 4 4 25 667
	cp extents.png multiregion-rectangle-clusters-667.png
	rm extents.png
	mv *.png examples/

	


clean:
	rm -f *.exe main.exe *.stackdump *.o *.png examples/*.png
