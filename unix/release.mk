CFLAGS = -O3 -march=pentium4 -mtune=generic -msse -msse2 -mfpmath=sse -s -fomit-frame-pointer
DEFINES = -DNDEBUG

include build.mk
