IDIR := include
SDIR := src

VPATH = $(IDIR) $(SDIR)

ifeq ($(DEBUG),)
	DEBUG := -O
else
	DEBUG := -g -pg -D DEBUG
endif

CXXFLAGS := $(DEBUG) -I$(IDIR) -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-declarations -Winline -Wno-long-long \
	-Wuninitialized -Wconversion -Wredundant-decls -Wdouble-promotion

ifneq ($(KNN_MATCH),)
	CXXFLAGS := $(CXXFLAGS) -D KNN_MATCH
endif

ifneq ($(EXPERIMENTAL_FLANN),)
	CXXFLAGS := $(CXXFLAGS) -D EXPERIMENTAL_FLANN
endif

# CFLAGS := -ansi -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes $(CXXFLAGS)

.PHONY: all clean todolist

EXES := match convert

all: $(EXES)

match: cross_match.o subspace_clustering.o

$(EXES): % : %.o
	@echo CXX $@
	@$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs opencv`

%.o: %.cpp
	@echo CXX -c $@
	@$(CXX) -c $< $(CXXFLAGS) `pkg-config --cflags opencv`

clean:
	-@$(RM) -r $(wildcard output* *.o $(EXES) convert proj.zip tags)

todolist:
	-@for file in $(wildcard $(SRC:%=$(SDIR)/%) $(IDIR)/*/*.h $(IDIR)/*.h); do\
	 fgrep -H -e TODO -e FIXME $$file; done; true

