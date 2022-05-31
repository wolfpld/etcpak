CFLAGS += -DNO_GZIP -DPNG_INTEL_SSE
CXXFLAGS := $(CFLAGS) -std=c++11
DEFINES +=
INCLUDES := -I../zlib
LIBS := -lpthread
IMAGE := etcpak

FILTER := ../getopt/getopt.c

BASE := $(shell egrep 'ClCompile.*cpp"' ../build/etcpak.vcxproj | sed -e 's/.*\"\(.*\)\".*/\1/' | sed -e 's@\\@/@g')
BASE2 := $(shell egrep 'ClCompile.*c"' ../build/etcpak.vcxproj | sed -e 's/.*\"\(.*\)\".*/\1/' | sed -e 's@\\@/@g')

SRC := $(filter-out $(FILTER),$(BASE))
SRC2 := $(filter-out $(FILTER),$(BASE2))

OBJ := $(SRC:%.cpp=%.o)
OBJ2 := $(SRC2:%.c=%.o)

all: $(IMAGE)

%.o: %.cpp
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(DEFINES) $< -o $@

%.d : %.cpp
	@echo Resolving dependencies of $<
	@mkdir -p $(@D)
	@$(CXX) -MM $(INCLUDES) $(CXXFLAGS) $(DEFINES) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(<:.cpp=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.c
	$(CC) -c -DHAVE_UNISTD_H $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

%.d : %.c
	@echo Resolving dependencies of $<
	@mkdir -p $(@D)
	@$(CC) -MM $(INCLUDES) $(CFLAGS) $(DEFINES) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(<:.c=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(IMAGE): $(OBJ) $(OBJ2)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(OBJ) $(OBJ2) $(LIBS) -o $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(SRC:.cpp=.d) $(SRC2:.c=.d)
endif

clean:
	rm -f $(OBJ) $(OBJ2) $(SRC:.cpp=.d) $(SRC2:.c=.d) $(IMAGE)

.PHONY: clean all
