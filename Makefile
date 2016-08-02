EXE := test

IDIR := include
SDIR := src

VPATH = $(IDIR) $(SDIR)

# DEBUG := -O
DEBUG := -g -pg

CXXFLAGS := $(DEBUG) -I$(IDIR) -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-declarations -Winline -Wno-long-long \
	-Wuninitialized -Wconversion -Wredundant-decls -Wdouble-promotion

CFLAGS := -ansi -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes

.PHONY: all clean todolist

all: $(EXE)

$(EXE): cross_match.o subspace_clustering.o main.o
	@echo G++ $@
	@$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs opencv`

%.o: %.cpp
	@echo G++ -c $@
	@$(CXX) -c $< $(CXXFLAGS) -std=c++11 `pkg-config --cflags opencv`

clean:
	-@$(RM) -r $(wildcard output* *.o $(EXE) proj.zip tags)

todolist:
	-@for file in $(wildcard $(SRC:%=$(SDIR)/%) $(IDIR)/*/*.h $(IDIR)/*.h); do\
	 fgrep -H -e TODO -e FIXME $$file; done; true

