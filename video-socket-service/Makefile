# Setup compiler and compiler flags

CC         = g++
CFLAGS     = -std=gnu++0x
OPTFLAG    = -O3
CPPPATH    = -I/usr/include/boost 
LIBS       = -lboost_program_options \
	     -lboost_thread \
	     -lboost_system \
	     -lpthread

VIDEOSOCKSERVEROBJ = src/videosockserver.o
VIDEOSOCKSERVERBIN = bin/videosockserver

# Setup implicit pattern rules

src/%.o : src/%.cpp src/%.hpp
	$(CC) -c $(OFLAGS) $(CFLAGS) $(OPTFLAG) $(CPPPATH) $< -o $@ 

# Setup static rules

all: videosockserver 

binary_dir: 
	mkdir -p bin

videosockserver: $(VIDEOSOCKSERVEROBJ) binary_dir
	$(CC) $(OFLAGS) $(VIDEOSOCKSERVEROBJ) $(LIBS) -o $(VIDEOSOCKSERVERBIN) 

# Clean stuff

clean:
	rm -f src/*.o
