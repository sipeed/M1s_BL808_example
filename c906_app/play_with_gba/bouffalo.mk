#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCS := 	main.c \
					port-bl808/emu.cpp \
					44vba/src/gba.cpp \
					44vba/src/memory.cpp \
					44vba/src/sound.cpp \
					44vba/src/system.cpp

COMPONENT_ADD_INCLUDEDIRS += 44vba/src

COMPONENT_OBJS := $(patsubst %.cpp,%.o, $(COMPONENT_SRCS))
COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_OBJS))

COMPONENT_SRCDIRS := . 44vba/src port-bl808

CPPFLAGS +=  -DFRONTEND_SUPPORTS_RGB565 -DHAVE_STDINT_H -DINLINE=inline -DLSB_FIRST  -O3