EXE := test

IDIR := include
SDIR := src
ODIR := obj

ODIRS := obj
VPATH = $(IDIR) $(SDIR)

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith \
	-Wcast-align -Wwrite-strings -Wmissing-declarations -Winline \
	-Wno-long-long -Wuninitialized -Wconversion -Wredundant-decls -Wdouble-promotion

# ifeq (,$(wildcard $(SDIR)/main.c))
# 	AEXT := c
# 	EXT := cpp
# 	CC := $(CXX)
# 	CFLAGS := -O
# else
# 	EXT := c
# 	AEXT := cpp
# 	CFLAGS := -g
# 	WARNINGS :=	$(WARNINGS) -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes	
# 	CC := gcc
# endif

OPENCV_FLAGS=`pkg-config --libs --cflags opencv`
CFLAGS := $(CFLAGS) -ansi $(WARNINGS) -I$(IDIR)
SRC := $(patsubst $(SDIR)/%, %, $(wildcard $(SDIR)/*.cpp $(SDIR)/*/*.cpp $(SDIR)/*.c $(SDIR)/*/*.c))
$(info SRC $(SRC))
OBJ := $(patsubst %, $(ODIR)/%.o, $(basename $(SRC)))
$(info OBJ $(OBJ))

.PHONY: all dist todolist conv 
# tests

all: $(EXE)

$(EXE): $(OBJ)
	@echo $(CC) -o $@ $^ CFLAGS
	-@$(CXX) -o $@ $^ $(CFLAGS) $(OPENCV_FLAGS)

$(ODIR)/%: $(ODIRS)
	$(CXX) -c -o $@ $(wildcard $(patsubst $(ODIR)/%,$(SDIR)/%.*, $(basename $@))) -MMD $(CFLAGS)

-include $(OBJ:%.o=.d)

$(ODIRS):
	-@mkdir -p $@

clean:
	-@$(RM) -r $(wildcard $(ODIR) $(EXE) proj.zip tags)

dist:
	@zip proj.zip $(IDIR)/*.h $(IDIR)/*/*.h $(SRC:%=$(SDIR)/%) Makefile

# conv: clean
# 	@for file in $(SRC:%=$(SDIR)/%); do\
# 		mv $$file $${file/.$(EXT)/.$(AEXT)}; \
# 	done;\
# 	echo changed to $(AEXT) mode

todolist:
	-@for file in $(wildcard $(SRC:%=$(SDIR)/%) $(IDIR)/*/*.h $(IDIR)/*.h); do\
	 fgrep -H -e TODO -e FIXME $$file; done; true

# avl_test: tests/avl.$(EXT) $(ODIR)/avltree.o
# 	@echo $(CC) $@ 
# 	-@$(CC) -o $@ $^ -I$(IDIR)

# tests: avl_test
