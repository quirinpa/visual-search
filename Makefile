EXE := test

IDIR := include
SDIR := src

VPATH = $(IDIR) $(SDIR)

# DEBUG := -O
DEBUG := -g

CXXFLAGS := $(DEBUG) -I$(IDIR) -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-declarations -Winline -Wno-long-long \
	-Wuninitialized -Wconversion -Wredundant-decls -Wdouble-promotion

CFLAGS := -ansi -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes

.PHONY: all clean todolist

all: $(EXE)

$(EXE): list.o avl.o fifo.o main.o include/avl.h
	$(CXX) -o $@ list.o avl.o fifo.o main.o $(CXXFLAGS) `pkg-config --libs opencv`

list.o: src/list.c include/list.h
	$(CC) -c $< $(CXXFLAGS) $(CFLAGS)

avl.o: src/avl.c include/avl.h include/bool.h
	$(CC) -c $< $(CXXFLAGS) $(CFLAGS)

# avl_iot.o: src/avl_iot.cpp include/avl_iot.h avl.o
# 	$(CXX) -c $< $(CXXFLAGS)

fifo.o: src/fifo.c include/fifo.h
	$(CC) -c $< $(CXXFLAGS) $(CFLAGS)

main.o: main.cpp include/avl_iot.h
	$(CXX) -c $< $(CXXFLAGS) -std=c++11 `pkg-config --cflags opencv`

clean:
	-@$(RM) -r $(wildcard output* *.o $(EXE) proj.zip tags)

todolist:
	-@for file in $(wildcard $(SRC:%=$(SDIR)/%) $(IDIR)/*/*.h $(IDIR)/*.h); do\
	 fgrep -H -e TODO -e FIXME $$file; done; true

