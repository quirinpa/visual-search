IDIR := include
SDIR := src

VPATH = $(IDIR) $(SDIR)

# DEBUG := -O
DEBUG := -g -pg

CXXFLAGS := $(DEBUG) -I$(IDIR) -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-declarations -Winline -Wno-long-long \
	-Wuninitialized -Wconversion -Wredundant-decls -Wdouble-promotion

CFLAGS := -ansi -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes $(CXXFLAGS)

.PHONY: all clean todolist

EXES := cap_test match

all: $(EXES) convert

cap_test: cross_match.o subspace_clustering.o
match: cross_match.o subspace_clustering.o get_match_buckets.o

convert: convert.cpp
	@echo G++ $@
	@$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs --cflags opencv`

$(EXES): % : %.o
	@echo G++ $@
	@$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs opencv`

%.o: %.cpp
	@echo G++ -c $@
	@$(CXX) -c $< $(CXXFLAGS) -std=c++11 `pkg-config --cflags opencv`

clean:
	-@$(RM) -r $(wildcard output* *.o $(EXES) convert proj.zip tags)

todolist:
	-@for file in $(wildcard $(SRC:%=$(SDIR)/%) $(IDIR)/*/*.h $(IDIR)/*.h); do\
	 fgrep -H -e TODO -e FIXME $$file; done; true

