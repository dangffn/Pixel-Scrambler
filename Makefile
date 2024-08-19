CC=gcc
CFLAGS=-I. -I/usr/include -I/usr/include/opencv4 -lssl -lcrypto -lstdc++

OPENCV_DEPS = `pkg-config --libs opencv4`

.PHONY = scrambler

scrambler: main.cpp
	$(CC) -g main.cpp $(CFLAGS) $(OPENCV_DEPS) -o scrambler
	chmod +x ./scrambler

clean:
	rm -rf ./scrambler