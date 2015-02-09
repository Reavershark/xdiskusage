SHELL=/bin/sh

PROGRAM = xdiskusage
VERSION = 1.51

CXXFILES = panels.C xdiskusage.C

# You may need this to link with a shared version of fltk:
LIBS =

MANPAGE = 1

################################################################

OBJECTS = $(CXXFILES:.C=.o)

all:	$(PROGRAM)

$(PROGRAM) : $(OBJECTS)
	$(CXX) -o $(PROGRAM) $(OBJECTS) `fltk-config --ldflags`

configure: configure.in
	autoconf

makeinclude: configure makeinclude.in
	./configure
include makeinclude

.SUFFIXES : .fl .do .C .c .H

.C.o :
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<
.C :
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<
.fl.C :
	-fluid -c $<
.fl.H :
	-fluid -c $<

clean :
	-@ rm -f *.o $(PROGRAM) $(CLEAN) core *~ makedepend
	@touch makedepend

depend:
	$(MAKEDEPEND) -I.. $(CPPFLAGS) $(CXXFILES) $(CFILES) > makedepend
makedepend:
	touch makedepend
include makedepend

install: $(PROGRAM)
	-@mkdir -p $(bindir)
	$(INSTALL) -s $(PROGRAM) $(bindir)/$(PROGRAM)
	-@mkdir -p $(mandir)/man$(MANPAGE)
	$(INSTALL) $(PROGRAM).$(MANPAGE) $(mandir)/man$(MANPAGE)/$(PROGRAM).$(MANPAGE)

uninstall:
	-@ rm -f $(bindir)/$(PROGRAM)
	-@ rm -f $(mandir)/man$(MANPAGE)/$(PROGRAM).$(MANPAGE)

dist:
	cat /dev/null > makedepend
	-@mkdir $(PROGRAM)-$(VERSION)
	-@ln README Makefile configure install-sh makedepend *.C *.H *.in *.fl $(PROGRAM).spec $(PROGRAM).$(MANPAGE) $(PROGRAM)-$(VERSION)
	tar -cvzf $(PROGRAM)-$(VERSION).tgz $(PROGRAM)-$(VERSION)
	-@rm -r $(PROGRAM)-$(VERSION)

################################################################

PROGRAM_D = $(PROGRAM)_d

debug: $(PROGRAM_D)

OBJECTS_D = $(CXXFILES:.C=.do) $(CFILES:.c=.do)

.C.do :
	$(CXX) $(CPPFLAGS) $(CXXFLAGS_D) -c -o $@ $<
.c.do :
	$(CC) $(CFLAGS_D) -c -o $@ $<

$(PROGRAM_D) : $(OBJECTS_D)
	$(CXX) -o $(PROGRAM_D) $(OBJECTS_D) `fltk-config --ldflags`

# Used to make the version that is gzipped and uploaded to web site
static : $(OBJECTS)
	$(CXX) -o $(PROGRAM) $(OBJECTS) `fltk-config --ldstaticflags`
