VERSION := 0.1b-rc2

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
	-Wuninitialized -Wconversion -Wredundant-decls -Wdouble-promotion -D PROGRAM_VERSION=\"$(VERSION)\"

ifneq ($(KNN_MATCH),)
	CXXFLAGS := $(CXXFLAGS) -D KNN_MATCH
endif

ifneq ($(EXPERIMENTAL_FLANN),)
	CXXFLAGS := $(CXXFLAGS) -D EXPERIMENTAL_FLANN
endif

# CFLAGS := -ansi -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes $(CXXFLAGS)

# .PHONY: all install clean todolist
.PHONY: all clean todolist

EXES := vsmatch vsconvert

all: $(EXES)

vsmatch: cross_match.o subspace_clustering.o

$(EXES): % : %.o
	@echo CXX $@
	@$(CXX) -o $@ $^ $(CXXFLAGS) `pkg-config --libs opencv`

%.o: %.cpp
	@echo CXX -c $@
	@$(CXX) -c $< $(CXXFLAGS) `pkg-config --cflags opencv`

install: $(EXES)
	@echo installing executable files to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f vsmatch ${DESTDIR}${PREFIX}/bin
	@cp -f vsconvert ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vsmatch
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vsconvert
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < man/vsconvert.1 > ${DESTDIR}${MANPREFIX}/man1/vsconvert.1
	@sed "s/VERSION/${VERSION}/g" < man/vsmatch.1 > ${DESTDIR}${MANPREFIX}/man1/vsmatch.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/vsconvert.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/vsmatch.1


clean:
	-@$(RM) -r $(wildcard output* *.o $(EXES) convert proj.zip tags)

todolist:
	-@for file in $(wildcard $(SRC:%=$(SDIR)/%) $(IDIR)/*/*.h $(IDIR)/*.h); do\
	 fgrep -H -e TODO -e FIXME $$file; done; true

