

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

	./main.exe 50 50 0 200 200 0 666
	cp extents.png region50-nodeviation-no-clusters-666.png

	./main.exe 50 50 25 200 200 0 666
	cp extents.png region50-devation10-no-clusters-666.png

	./main.exe 50 50 10 200 200 0 666
	cp extents.png region50-devation20-no-clusters-666.png

	./main.exe 50 50 20 200 200 0 666
	cp extents.png region50-deviation25-no-clusters-666.png

	./main.exe 25 25 25 200 200 0 666
	cp extents.png region25-deviation25-no-clusters-666.png

	./main.exe 25 25 50 200 200 0 666
	cp extents.png region25-deviation25-no-clusters-666.png

	./main.exe 25 25 50 4 4 0 666
	cp extents.png region25-deviation50-regular-clusters-666.png

	./main.exe 25 25 50 4 4 25 666
	cp extents.png multiregion-irregular-clusters-666.png

	./main.exe 25 25 50 4 4 25 667
	cp extents.png multiregion-irregular-clusters-667.png

	./main.exe 200 200 0 4 4 0 667
	cp extents.png legacy-map-recreation.png

	./main.exe 4 4 0 200 200 0 667
	cp extents.png legacy-map-recreation-lotsaregions.png

	rm extents.png
	mv *.png examples/

	
clean-examples:
	rm -f examples/*.png

clean:
	rm -f *.exe main.exe *.stackdump *.o *.png examples/*.png
