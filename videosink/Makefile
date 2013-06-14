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
	     -lboost_filesystem \
	     -lboost_system \
	     -lgobject-2.0 \
	     -lglib-2.0

EXTRA_DIST = doxygen.conf

VIDEOSINKOBJ = src/videosink.o
VIDEOSINKBIN = bin/videosink

# Setup implicit pattern rules

src/%.o : src/%.cpp src/%.hpp
	$(CC) -c $(OFLAGS) $(CFLAGS) $(OPTFLAG) $(CPPPATH) $< -o $@ 

# Setup static rules

all: videosink 

binary_dir: 
	mkdir -p bin

videosink: $(VIDEOSINKOBJ) binary_dir
	$(CC) $(OFLAGS) $(VIDEOSINKOBJ) $(LIBS) -o $(VIDEOSINKBIN) 

# Build documentation

doc: doxygen.conf src/*.cpp src/*.hpp
	doxygen doxygen.conf

# Clean stuff

clean:
	rm -f src/*.o
