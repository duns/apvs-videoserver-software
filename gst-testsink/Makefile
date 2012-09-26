# Setup compiler and compiler flags

CC         = g++
CFLAGS     = `pkg-config --cflags --libs gstreamermm-0.10` -std=gnu++0x
OPTFLAG    = -O3
CPPPATH    = -I/usr/include/gstreamer-0.10 \
 	     -I/usr/include/libxml2 \
	     -I/usr/include/boost \
	     -I/usr/include/glib-2.0 \
	     -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBS       = -lgstreamer-0.10 \
	     -lboost_program_options \
	     -lboost_thread \
	     -lglib-2.0

EXTRA_DIST = doxygen.conf

# Setup implicit pattern rules

src/%.o : src/%.cpp
	$(CC) -c $(OFLAGS) $(CFLAGS) $(OPTFLAG) $(CPPPATH) $< -o $@ 

# Setup static rules

all: gst-test_sink 

binary_dir: 
	mkdir -p bin

gst-test_sink: src/gst-test_sink.o binary_dir
	$(CC) $(OFLAGS) src/gst-test_sink.o $(LIBS) -o bin/gst-test_sink

# Build documentation

doc: doxygen.conf src/*.cpp src/*.hpp
	doxygen doxygen.conf

# Clean stuff

clean:
	rm -f src/*.o
