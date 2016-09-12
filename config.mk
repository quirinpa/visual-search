PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

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

EXES := vsmatch vsconvert
