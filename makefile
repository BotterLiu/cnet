# Define source files
CPP_SRCS = vos_timer.cpp vos_thread.cpp vos_sem.cpp vos_event.cpp sockutil.cpp socket.cpp sockevent.cpp netop.cpp log.cpp \
    eventpool.cpp eventbase.cpp socket.cpp tcp_socket.cpp tprotocol.cpp runinfo.cpp sockmgr.cpp

###CPP_SRCS = 

BIN = 

DYNAMIC_LIB = 

STATIC_LIB = libnet.a

INSTALL_LIB_PATH = ../libs

CXXFLAGS  = -Wall -g -D__LINUX -fPIC -ggdb3 -Wno-deprecated

# Define header file paths
INCPATH = -I../net

CXX = g++

# Define the -L library path(s)
LDFLAGS = -L../libs

# Define the -l library name(s)
LIBS = -lpthread 

LIB_FLAGS = $(CXXFLAGS) $(LDFLAGS) $(LIBS) -shared -Wl,-soname,$(DYNAMIC_LIB)

# Only in special cases should anything be edited below this line
OBJS      = $(CPP_SRCS:.cpp=.o)


.PHONY = all clean distclean

# Main entry point
#

AR       = ar
ARFLAGS  = -ruv
RANLIB	 = ranlib

all:  $(BIN) $(STATIC_LIB) $(DYNAMIC_LIB)

# For linking object file(s) to produce the executable

$(BIN): $(OBJS)
	@echo Linking $@
	$(CXX) $^ $(LDFLAGS) $(LIBS)  $(CXXFLAGS) $(INCPATH) -o $@

${STATIC_LIB}: ${OBJS}
	@if [ ! -d ${INSTALL_LIB_PATH} ]; then mkdir -p ${INSTALL_LIB_PATH}; fi
	$(AR) ${ARFLAGS} $@ $(OBJS)
	$(RANLIB) $@

${DYNAMIC_LIB} : ${OBJS}
	@if [ ! -d ${INSTALL_LIB_PATH} ]; then mkdir -p ${INSTALL_LIB_PATH}; fi
	$(CXX) $(LIB_FLAGS) -o $(DYNAMIC_LIB) $(OBJS)

# For compiling source file(s)
#
.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LDFLAGS)  $<

install:
	@if [ "x${STATIC_LIB}" != "x" ]; then mkdir -p ${INSTALL_LIB_PATH}; cp $(STATIC_LIB) $(INSTALL_LIB_PATH); fi
	@if [ "x${DYNAMIC_LIB}" != "x" ]; then mkdir -p ${INSTALL_LIB_PATH}; cp $(DYNAMIC_LIB) $(INSTALL_LIB_PATH); fi
# For cleaning up the project
#
clean:
	$(RM) $(OBJS) core.* $(BIN)

distclean: clean
	$(RM) $(BIN) 


