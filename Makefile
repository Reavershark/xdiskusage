SHELL=/bin/sh

PROGRAM = xdiskusage
VERSION = 1.43

CXXFILES = panels.C xdiskusage.C

LIBS = -lfltk

MANPAGE = 1

################################################################

OBJECTS = $(CXXFILES:.C=.o)

all:	$(PROGRAM)

$(PROGRAM) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(PROGRAM) $(OBJECTS) $(LIBS) $(LDLIBS)

makeinclude: configure
	./configure
include makeinclude

.SUFFIXES : .fl .do .C .c .H

.C.o :
	$(CXX) $(CXXFLAGS) -c $<
.C :
	$(CXX) $(CXXFLAGS) -c $<
.fl.C :
	-fluid -c $<
.fl.H :
	-fluid -c $<

clean :
	-@ rm -f *.o $(PROGRAM) $(CLEAN) core *~ makedepend
	@touch makedepend

depend:
	$(MAKEDEPEND) -I.. $(CXXFLAGS) $(CXXFILES) $(CFILES) > makedepend
makedepend:
	touch makedepend
include makedepend

install: $(PROGRAM)
	$(INSTALL) -s $(PROGRAM) $(bindir)/$(PROGRAM)
	$(INSTALL) $(PROGRAM).$(MANPAGE) $(mandir)/man$(MANPAGE)/$(PROGRAM).$(MANPAGE)

uninstall:
	-@ rm -f $(bindir)/$(PROGRAM)
	-@ rm -f $(mandir)/man$(MANPAGE)/$(PROGRAM).$(MANPAGE)

dist:
	cat /dev/null > makedepend
	-@mkdir $(PROGRAM)-$(VERSION)
	-@ln README Makefile configure install-sh makedepend *.C *.H *.in *.fl $(PROGRAM).$(MANPAGE) $(PROGRAM)-$(VERSION)
	tar -cvzf $(PROGRAM)-$(VERSION).tgz $(PROGRAM)-$(VERSION)
	-@rm -r $(PROGRAM)-$(VERSION)

################################################################

PROGRAM_D = $(PROGRAM)_d

debug: $(PROGRAM_D)

OBJECTS_D = $(CXXFILES:.C=.do) $(CFILES:.c=.do)

.C.do :
	$(CXX) -I.. $(CXXFLAGS_D) -c -o $@ $<
.c.do :
	$(CC) -I.. $(CFLAGS_D) -c -o $@ $<

$(PROGRAM_D) : $(OBJECTS_D)
	$(CXX) $(CXXFLAGS_D) -o $(PROGRAM_D) $(OBJECTS_D) $(LIBS) $(LDLIBS)
