// xdiskusage.C

const char* copyright = 
"xdiskusage version 1.51\n"
"Copyright (C) 2014 Bill Spitzak\n"
"Based on xdu by Phillip C. Dykstra\n"
"\n"
"This program is free software; you can redistribute it and/or modify "
"it under the terms of the GNU General Public License as published by "
"the Free Software Foundation; either version 2 of the License, or "
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License "
"along with this software; if not, write to the Free Software "
"Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 "
"USA.";

#if defined(DF_COMMAND)
// assumme DU_COMMAND is also defined
#elif defined(__hpux)
# define DF_COMMAND "df -P"
# define DU_COMMAND "du -kx"
#elif defined(__bsdi__)
# define DF_COMMAND "df"
# define DU_COMMAND "du -x"
#elif defined(__FreeBSD__)
# define DF_COMMAND "df -k -t noprocfs,devfs,fdescfs"
# define DU_COMMAND "du -kx"
#elif defined(SVR4) || defined(__sun)
# define DF_COMMAND "/bin/df -k"
# define DU_COMMAND "/bin/du -kd"
#else // linux
# define DF_COMMAND "df -k -x usbfs -x tmpfs"
# define DU_COMMAND "du -kx"
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "panels.H"
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/fl_ask.H>
#include <FL/math.h>

typedef unsigned long long ull;

// turn number of K into user-friendly text:
const char* formatk(ull k) {
  static char buffer[32];
  int index = 0;
  double dk = k;
  while (dk >= 1024.0) {
    index++;
    dk /= 1024.0;
  }
  if (index)
    sprintf(buffer, "%.4g%c", dk, "KMGTPEZY"[index]);
  else
    sprintf(buffer, "%dK", (int)k);
  return buffer;
}

// Holds data read from 'df' for each disk
struct Disk {
  const char* mount;
  ull total;
  ull used;
  ull avail;
  Disk* next;
};

Disk* firstdisk;
int quiet;

void alert(const char* message)
{ if (quiet) fprintf(stderr, "%s\n", message);
  else fl_alert("%s", message);
}

void alert(const char* fmt, const char* s1)
{ if (quiet) {fprintf(stderr, fmt, s1); fputc('\n', stderr);}
  else fl_alert(fmt, s1);
}

void alert(const char* fmt, const char* s1, const char* s2)
{ if (quiet) {fprintf(stderr, fmt, s1, s2); fputc('\n', stderr);}
  else fl_alert(fmt, s1, s2);
}

void alert(const char* fmt, const char* s1, unsigned n, const char* s2)
{ if (quiet) {fprintf(stderr, fmt, s1, n, s2); fputc('\n', stderr);}
  else fl_alert(fmt, s1, n, s2);
}

// Run 'df' and create a list of Disk structures, fill in browser:
void reload_cb(Fl_Button*, void*) {
  FILE* f = popen(DF_COMMAND, "r");
  if (!f) {
    alert("Can't run df, %s\n", strerror(errno));
    return;
  }
  Disk** pointer = &firstdisk;
  for (;;) {
    char buffer[1024];
    if (!fgets(buffer, 1024, f)) break;
    int n = 0; // number of words
    char* word[10]; // pointer to each word
    for (char* p = buffer; n < 10;) {
      // skip leading whitespace:
      while (*p && isspace(*p)) p++;
      if (!*p) break;
      // skip over the word:
      word[n++] = p;
      while (*p && !isspace(*p)) p++;
      if (!*p) break;
      *p++ = 0;
    }
    if (n < 5 || word[n-1][0] != '/') continue;
    // ok we found a line with a /xyz at the end:
    Disk* d = new Disk;
    d->mount = strdup(word[n-1]);
    d->total = strtoul(word[n-5],0,10);
    d->used  = strtoul(word[n-4],0,10);
    d->avail = strtoul(word[n-3],0,10);
    *pointer = d;
    d->next = 0;
    pointer = &d->next;
  }
  pclose(f);

  if (!firstdisk) {
    alert("Something went wrong with df, no disks found.");
    return; // leave browser as-is
  }

  // There is no browser widget if a file was given on the command line
  if (!disk_browser)
    return;

  disk_browser->clear();

  for (Disk* d = firstdisk; d; d = d->next) {
    char buf[512];
    int pct;
    ull sum = d->used + d->avail;
    if (!sum) {
      pct = 100;
    } else {
      pct = int((d->used*100.0)/sum+.5);
      if (!pct && d->used) pct = 1;
    }
    const char* mount = d->mount;
    if (strlen(mount) > 255) mount = d->mount + strlen(mount) - 255;
#if FL_MAJOR_VERSION > 1 || FL_MINOR_VERSION >= 3
    static int widths[] = {0, 0, 0};
    if (d==firstdisk) {
      if (widths[0] == 0) {
        int n = disk_browser->textsize();
        fl_font(disk_browser->textfont(), n);
        widths[0] = fl_width("2.222Mn");
        widths[1] = fl_width("00% n");
      }
      disk_browser->column_widths(widths);
    }
    sprintf(buf, "@r%s\t@r%d%% \t@b%s", formatk(d->total), pct, d->mount);
#else
    sprintf(buf, "%s  %02d%%  %s", formatk(d->total), pct, d->mount);
#endif
    disk_browser->add(buf);
  }
  disk_browser->position(0);
#if FL_MAJOR_VERSION > 1
  disk_browser->deselect();
#endif
}

Fl_Window *copyright_window;
void copyright_cb(Fl_Button*, void*) {
  if (!copyright_window) {
    copyright_window = new Fl_Window(400,360,"About xdiskusage");
    copyright_window->color(FL_WHITE);
    Fl_Box *b = new Fl_Box(10,0,380,360,copyright);
#ifdef FL_NORMAL_SIZE
    b->labelsize(12);
#endif
    b->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
    copyright_window->resizable(b);
    copyright_window->end();
  }
  copyright_window->hotspot(copyright_window);
  copyright_window->set_non_modal();
  copyright_window->show();
}

////////////////////////////////////////////////////////////////

struct Node {
  Node* child;
  Node* brother;
  const char* name;
  ull size;
  long ordinal; // sign bit indicates hidden
  bool hidden() const {return ordinal < 0;}
};

// default sizes of new windows, changed by user actions on other windows:
int window_w = 600;
int window_h = 480;
int ncols = 5;
#define MAXDEPTH 100

class OutputWindow : public Fl_Window {
  void draw();
  int handle(int);
  void resize(int,int,int,int);
  Node* root;
  Node* current_root;
  Node* path[MAXDEPTH];
  Node* current_node;
  int root_depth;
  int current_depth;
  int ncols;
  Fl_Menu_Button menu_button;
  OutputWindow(int w, int h, const char* l) : Fl_Window(w,h,l),
    menu_button(0,0,w,h) {box(FL_NO_BOX);}
  void finish_drawn_row();
  void draw_tree(Node* n, int column, ull row, double scale, double offset);
  void print_tree(FILE* f, Node* n, int column, ull row, double scale, double offset, int W, int H);
  void setcurrent(Node*, int);
  void setroot(Node*, int);
public:
  static void root_cb(Fl_Widget*, void*);
  static void out_cb(Fl_Widget*, void*);
  static void hide_cb(Fl_Widget*, void*);
  static void unhide_cb(Fl_Widget*, void*);
  static void in_cb(Fl_Widget*, void*);
  static void next_cb(Fl_Widget*, void*);
  static void previous_cb(Fl_Widget*, void*);
  static void setroot_cb(Fl_Widget*, void*);
  static void sort_cb(Fl_Widget* o, void*);
  static void columns_cb(Fl_Widget* o, void*);
  static void copy_cb(Fl_Widget* o, void*);
  static void print_cb(Fl_Widget* o, void*);
  static Node* sort(Node* n, int (*compare)(const Node*, const Node*));
  static OutputWindow* make(const char*, Disk* = 0);
  static void print_file(OutputWindow* d, FILE *f, bool portrait, bool fill);
  ~OutputWindow();
};

int all_files;
char *outfile;
int arg_cb(int, char **argv, int &i) {
  const char *s = argv[i];
  if (!s) return 0;
  if (*s != '-') return 0;
  if (!s[1]) return 0; // plain "-" for pipe
  for (s++; *s; s++) {
    if (*s == 'a') all_files = 1;
    else if (*s == 'q') quiet = 1;
    else if (*s == 'o' && argv[i+1]) { 
     outfile = strdup(argv[i+1]);
     i++;
     quiet = 1;
    } else return 0;
  }
  i++;
  return 1;
}

int main(int argc, char**argv) {
#if FL_MAJOR_VERSION < 2
  // Make fltk look more like KDE/Windoze:
#ifndef FL_NORMAL_SIZE // detect new versions of fltk where this is a variable
  FL_NORMAL_SIZE = 12;
#endif
  Fl::set_color(FL_SELECTION_COLOR,0,0,128);
#endif
  // Parse and -x switches understood by fltk:
  int n; Fl::args(argc, argv, n, arg_cb);
  // Any remaining words are files/directories:
  if (n < argc) {
    if (argv[n][0] == '-') {
      if (argv[n][1]) { // unknown switch
	fprintf(stderr,
"xdiskusage		display browser of disks to run du on\n"
"xdiskusage directory...	run du on each named directory\n"
"xdiskusage file...	assume the file contains du output\n"
"du ... | xdiskusage -	pipe du output to xdiskusage\n"
" -a		show all files\n"
" -o file	immediately send postscript output to <file> (- means stdout)\n"
" -q		(quiet) don't show the progress slider\n"
"%s\n"
" -help\n", Fl::help);
	return 1;
      } else {
	// plain "-" indicates pipe
	argv[n] = 0;
      }
    }
    while (n < argc) {
      OutputWindow* d = OutputWindow::make(argv[n++]);
      if (outfile) {
        if (!d) return 1;
        FILE *f;
        if (strcmp(outfile,"-")==0)
          f = stdout;
        else
          f = fopen(outfile, "w");
        if (!f) {
          perror("Cannot open output file");
          return 1;
        }
        d->print_file(d, f, true, true);
        return 0;
      }
      else if (d) d->show();
    }
  } else {
    // normal gui:
    make_diskchooser();
    reload_cb(0,0);
    disk_chooser->show(argc,argv);
  }
  return Fl::run();
}

void disk_browser_cb(Fl_Browser*b, void*) {
  int i = b->value();
  //printf("disk browser cb %d\n", i);
#if FL_MAJOR_VERSION < 2
  i--;
#endif
  if (i < 0) return;
  Disk* d;
  for (d = firstdisk; i > 0; i--) d = d->next;
  all_files = all_files_button->value();
  OutputWindow* w = OutputWindow::make(d->mount, d);
  if (w) w->show();
  //b->value(0);
}

#include <FL/filename.H>

void disk_input_cb(Fl_Input* i, void*) {
  all_files = all_files_button->value();
  OutputWindow* w = OutputWindow::make(i->value(), 0);
  if (w) w->show();
}

void close_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)o;
  delete d;
}

static int cancelled;

void cancel_cb(Fl_Button*, void*) {
  cancelled = 1;
}

void delete_tree(Node* n) {
  while (n) {
    if (n->child) delete_tree(n->child);
    Node* next = n->brother;
    free((void*)n->name);
    delete n;
    n = next;
  }
}

// fill in missing totals by adding all sons:
void fix_tree(Node* n) {
  if (n->size) return;
  ull total = 0;
  for (Node *x = n->child; x; x = x->brother) total += x->size;
  n->size = total;
}

static long ordinal;

Node* newnode(const char* name, ull size, Node* parent, Node* & brother) {
  Node* n = new Node;
  n->child = n->brother = 0;
  n->name = strdup(name);
  n->size = size;
  n->ordinal = ++ordinal;
  if (parent->child) {
    brother->brother = n;
  } else {
    parent->child = n;
  }	
  brother = n;
  return n;
}

static Fl_Menu_Item menutable[] = {
  {"set root", '\r', OutputWindow::setroot_cb},
  {"in", FL_Right, OutputWindow::in_cb},
  {"next", FL_Down, OutputWindow::next_cb},
  {"previous", FL_Up, OutputWindow::previous_cb},
  {"out", FL_Left, OutputWindow::out_cb},
  {"root out", '/', OutputWindow::root_cb},
  {"hide", 'h', OutputWindow::hide_cb},
  {"unhide", FL_SHIFT|'H', OutputWindow::unhide_cb},
  {"sort", 0, 0, 0, FL_SUBMENU},
    {"largest first", 's', OutputWindow::sort_cb, (void*)'s'},
    {"smallest first", 'r', OutputWindow::sort_cb, (void*)'r'},
    {"alphabetical", 'a', OutputWindow::sort_cb, (void*)'a'},
    {"reverse alphabetical", 'z', OutputWindow::sort_cb, (void*)'z'},
    {"unsorted", 'u', OutputWindow::sort_cb, (void*)'u'},
    {0},
  {"columns", 0, 0, 0, FL_SUBMENU},
    {"2", '2', OutputWindow::columns_cb, (void*)2},
    {"3", '3', OutputWindow::columns_cb, (void*)3},
    {"4", '4', OutputWindow::columns_cb, (void*)4},
    {"5", '5', OutputWindow::columns_cb, (void*)5},
    {"6", '6', OutputWindow::columns_cb, (void*)6},
    {"7", '7', OutputWindow::columns_cb, (void*)7},
    {"8", '8', OutputWindow::columns_cb, (void*)8},
    {"9", '9', OutputWindow::columns_cb, (void*)9},
    {"10", '0', OutputWindow::columns_cb, (void*)10},
    {"11", '1', OutputWindow::columns_cb, (void*)11},
    {0},
  {"copy to clipboard", 'c', OutputWindow::copy_cb},
  {"print", 'p', OutputWindow::print_cb},
  {0}
};

static int largestfirst(const Node* a, const Node* b) {
  return (a->size > b->size) ? -1 : 1;
}

OutputWindow* OutputWindow::make(const char* path, Disk* disk) {

  cancelled = 0;

  FILE* f;
  bool true_file;
  char buffer[2048];
  char pathbuf[1024];

  if (!path) {
    // it is a pipe
    true_file = true;
    f = stdin;
  } else {
    if (!disk) {
      // follow all symbolic links...
      strncpy(pathbuf, path, 1024);
      for (int i=0; i<10; i++) {
	char *p = (char*)fl_filename_name(pathbuf);
	int j = readlink(pathbuf, p, 1024-(p-pathbuf));
	if (j < 0) {
	  if (errno != EINVAL) {
	    alert("%s: no such file", pathbuf);
	    return 0;
	  }
	  break;
	}
	if (*p == '/') {memmove(pathbuf, p, j); p = pathbuf;}
	p[j] = 0;
	path = pathbuf;
      }
      // See if the path we were given is really a mountpoint, in which case
      // we can figure out freespace.
      reload_cb(0,0);
      for (Disk* d=firstdisk; d; d=d->next) {
        if(strcmp(d->mount, path) == 0) {
          disk = d;
          break;
        }
      }
    }
    // First, determine if the path we're given is an actual file, and
    // not a directory.  If it is a file, assume it's the output from a
    // du command the user ran by hand.
    struct stat stbuf;
    if (stat(path, &stbuf) < 0) {
      alert("%s : %s", path, strerror(errno));
      return 0;
    }
    true_file = S_ISREG(stbuf.st_mode);
    if (true_file) {
      f = fopen(path, "r");
      if (!f) {
	alert("%s : %s", path, strerror(errno));
	return 0;
      }
    }
  }

  if (!true_file) {
    // It's a directory; popen a du command starting at that directory.
    
#ifdef __sgi
    // sgi has the -x and -L switches somehow confused.  You cannot turn
    // off descent into mount points (with -x) without turning on descent
    // into symbolic links (at least for a stat()).  At DD all the symbolic
    // links are on the /usr disk so I special case it:
    if (!strcmp(path, "/usr"))
      sprintf(buffer, "du -k%c \"%s\"", all_files ? 'a' : ' ', path);
    else
#endif
      snprintf(buffer, 2048,
               DU_COMMAND"%c \"%s\"", all_files ? 'a' : ' ', path);
    
    f = popen(buffer,"r");
    if (!f) {
      alert("Problem running '%s' : %s", buffer, strerror(errno));
      return 0;
    }
  }

  if (!quiet) {
    if (!wait_window) make_wait_window();
    wait_slider->type(disk ? FL_HOR_FILL_SLIDER : FL_HOR_SLIDER);
    wait_slider->value(0.0);
#if FL_MAJOR_VERSION > 1
    wait_slider->box(FL_DOWN_BOX);
    wait_slider->color(0);
    wait_slider->selection_color(12);
#endif
    if (!(path && true_file)) {
      wait_window->show();
      while (wait_window->damage()) Fl::wait(.1);
    }
  }

  Node* root = new Node;
  root->child = root->brother = 0;
  root->name = 0; //if (!true_file) root->name = strdup(path);
  root->size = 0;
  root->ordinal = 0;
  ordinal = 0;

  Node* lastnode[MAXDEPTH];
  ull runningtotal;
  ull totals[MAXDEPTH];
  lastnode[0] = root;
  runningtotal = 0;
  totals[0] = 0;
  int currentdepth = 0;

  for (size_t line_no = 1;; ++line_no) {
    if (!(path && true_file)) Fl::check();
    if (cancelled) {
      delete_tree(root);
      if (true_file) {
	if (path) fclose(f);
      } else {
	pclose(f);
      }
      wait_window->hide();
      return 0;
    }

    // FIXME: don't limit line length to 2048
    if (!fgets(buffer, 2048, f)) {
      if (!runningtotal) {
	alert("%s: empty or bad file", path ? path : "stdin");
	cancelled = 1;
	continue;
      }
      break;
    }

    // If the line was longer than the maximum, warn about it,
    // and discard the line.
    size_t len = strlen (buffer);
    if (buffer[len-1] != '\n') {
      fprintf (stderr, "%s:%u: line too long, skipping it\n",
               path ? path : "stdin", (unsigned)line_no);
      // Read until end of line or EOF.
      while (1) {
        char c = getc (f);
        if (c == '\n' || c == EOF)
          break;
      }
      continue;
    }

    // null-terminate the line:
    char* p = buffer+len;
    if (p > buffer && p[-1] == '\n') p[-1] = 0;

    ull size = strtoull(buffer, &p, 10);
    if (!isspace(*p) || p == buffer) {
      if (!*p || *p=='#') continue; // ignore blank lines or comments (?)
      alert("%s:%u: does not look like du output: %s",
            path ? path : "stdin", (unsigned)line_no, p);
      cancelled = 1;
      continue;
    }

    while (isspace(*p)) p++;

    // split the path into parts:
    int newdepth = 0;
    const char* parts[MAXDEPTH];
    if (*p == '/') {
      if (!root->name) root->name = strdup("/");
      p++;
    }
    for (newdepth = 0; newdepth < MAXDEPTH && *p;) {
      parts[++newdepth] = p++;
      while (*p && *p != '/') p++;
      if (*p == '/') *p++ = 0;
    }

    // find out how many of the fields match:
    int match = 0;
    for (; match < newdepth && match < currentdepth; match++) {
      if (strcmp(parts[match+1],lastnode[match+1]->name)) break;
    }

    // give a total to any subdirectories we are leaving that du did
    // not report a total for:
    for (int j = currentdepth; j > match; j--) fix_tree(lastnode[j]);

    if (match == newdepth) {
      Node* p = lastnode[newdepth];
      ull t = totals[newdepth]+size;
      p->size = size;
      runningtotal = t;
    } else {
      Node* p = lastnode[match];
      for (int j = match+1; j <= newdepth; j++) {
	totals[j] = runningtotal;
	p = newnode(parts[j], 0, p, lastnode[j]);
      }
      p->size = size;
      runningtotal += size;
    }

    currentdepth = newdepth;
    totals[newdepth] = runningtotal;

    // percent done is rounded to nearest pixel so slider does
    // not continuousely redraw:
    if (!quiet) {
      double v = disk ? (double)runningtotal/disk->used :
	(double)(ordinal%1024)/1024;
      v = rint(v*wait_slider->w())/wait_slider->w();
      wait_slider->value(v);
    }
  }
  if (!root->name && path) root->name = strdup(path);

  if (true_file) {
    if (path) fclose(f);
  } else {
    pclose(f);
  }
  if (!quiet)
  wait_window->hide();

  // remove all the common roots that were not given sizes by du:
  while (root->child && !root->size && !root->child->brother) {
    Node* child = root->child;
    if (root->name) {
      char buffer[1024];
      char* p = buffer;
      const char* q = root->name;
      while (*q) *p++ = *q++;
      if (p[-1] != '/') *p++ = '/';
      strcpy(p, child->name);
      free((void*)(root->name));
      root->name = strdup(buffer);
      free((void*)(child->name));
    } else {
      root->name = child->name;
    }
    root->size = child->size;
    root->child = child->child;
    delete child;
    for (int j = 1; j < currentdepth; j++) lastnode[j] = lastnode[j+1];
    currentdepth--;
  }
    
  // give a total to any subdirectories we are leaving that du did
  // not report a total for:
  for (int j = currentdepth; j > 0; j--) fix_tree(lastnode[j]);

  // Add dummy nodes to report more information we learned from df:
  if (disk) {
    // find last child so we can add after it:
    Node* n = root->child;
    while (n && n->brother) n = n->brother;
    if (disk->used > runningtotal) {
      // missing stuff (probably permission denied errors):
      newnode("(permission denied)", disk->used-runningtotal, root, n);
      runningtotal = disk->used;
    }
    if (disk->avail) {
      newnode("(free)", disk->avail, root, n);
      runningtotal += disk->avail;
    }
    if (disk->total > runningtotal) {
      newnode("(inodes)", disk->total-runningtotal, root, n);
      runningtotal = disk->total;
    }
  }

  root->size = runningtotal;

  OutputWindow* d = new OutputWindow(window_w, window_h, root->name);
  d->ncols = ::ncols;
  d->root = d->current_root = sort(root, largestfirst);
  d->resizable(d);
  d->menu_button.menu(menutable);
  d->menu_button.box(FL_NO_BOX);
  d->callback(close_cb);
  d->root_depth = 0;
  d->current_node = d->root;
  d->current_depth = 0;

  return d;
}

// These point at the top-left most pixel not drawn. This avoids drawing
// over already made drawings:
static int drawn_row;
static int undrawn_column;

#define BACKGROUND FL_LIGHT2

// erase the reset of drawn_row
void OutputWindow::finish_drawn_row() {
  if (undrawn_column < w()) {
    fl_color(BACKGROUND);
    fl_rectf(undrawn_column, drawn_row, w()-undrawn_column, 1);
  }
}

int depth(Node* m, int quit_after) {
  int found = 0;
  for (Node* c = m->child; c; c=c->brother) if (!c->hidden()) {
    int t = depth(c, quit_after-1);
    if (t > found) {found = t; if (found+1 >= quit_after) break;}
  }
  return found+1;
}

// Returns first pixel row not drawn into:
void OutputWindow::draw_tree(Node* n, int column, ull row, double scale, double offset) {
  int X = (w()-1)*column/ncols;
  int W = (w()-1)*(column+1)/ncols - X;
  int Y = int(row*scale+offset+.5);
  for (; n; n = n->brother) {
    int NEXTY;
    if (n == current_root) NEXTY = h()-1;
    else if (n->hidden()) continue;
    else NEXTY = int((row+n->size)*scale+offset+.5);
    int H = NEXTY-Y;
    // if we have gone to next pixel row, erase end of previous one:
    if (Y > drawn_row) {
      finish_drawn_row();
      drawn_row = Y;
      undrawn_column = X+1;
    }
    // anything more than 1 pixel tall draws a box:
    if (H > 1) {
      fl_color(FL_WHITE);
      fl_rectf(X+1,Y+1,W-1,H-1);
      fl_color(FL_BLACK);
      if (n->size*scale > 10 && n->name && n->name[0]) {
	char buffer[256];
#if FL_MAJOR_VERSION > 1
	snprintf(buffer, 256, "%s%c%s", n->name,
		 n->size*scale > 20 ? '\n' : ' ',
		 formatk(n->size));
	fl_draw(buffer, X+3, Y, W-3, H,
		Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_CLIP|fltk::RAW_LABEL));
#else
	// we need to double all the @ signs in the filename, and do
	// something with unprintable letters:
	int i,j; for (i = j = 0; i < 254;) {
	  unsigned char c = (unsigned char)(n->name[j++]);
	  if (!c) break;
	  if (c < 32 || c==255) c = '?';
	  buffer[i++] = c; if (c=='@' || c=='&') buffer[i++] = c;
	}
	snprintf(buffer+i, 256-i, "%c%s",
		 n->size*scale > 20 ? '\n' : ' ',
		 formatk(n->size));
	fl_draw(buffer, X+3, Y, W-3, H,
		Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_CLIP));
#endif
      }
      fl_rect(X,Y,W+1,H+1);
      if (undrawn_column < X+W+1) undrawn_column = X+W+1;
      if (column+1 < ncols) {
	// figure out how much is not hidden:
	ull hidden = 0;
	for (Node* c = n->child; c; c = c->brother)
	  if (c->hidden()) hidden += c->size;
	if (hidden > 0) {
	  int yy = int((row+hidden)*scale+offset+.5);
	  fl_color(FL_BLACK);
	  fl_line(X,yy,X+10,yy);
	  fl_line(X+10,yy,X+W,Y);
	}
	if (hidden < n->size) {
	  double s2 = scale*n->size/(n->size-hidden);
	  draw_tree(n->child, column+1, row, s2, offset+row*(scale-s2));
	}
	if (drawn_row < NEXTY) {
	  finish_drawn_row(); drawn_row++;
	  if (drawn_row < NEXTY) {
	    fl_color(BACKGROUND);
	    fl_rectf(X+W+1, drawn_row, w()-(X+W+1), NEXTY-drawn_row);
	  }
	  drawn_row = NEXTY;
	  undrawn_column = X+W+1;
	}
      } else {
	drawn_row = NEXTY;
	undrawn_column = X+W+1;
      }
      if (n == current_node) {
	fl_color(FL_RED);
	fl_rect(X+1,Y+1,W-1,H-1);
      }
    } else {
      // draw a line for children less than 2 pixels tall
      int R = (w()-1)*(column+::depth(n,ncols-column))/ncols;
      if (R >= undrawn_column) {
	fl_color(FL_DARK2);
	fl_rectf(undrawn_column, Y, R+1-undrawn_column, 1);
	undrawn_column = R+1;
      }
      if (H > 0) {
	finish_drawn_row();
	fl_color(FL_DARK2);
	fl_rectf(X+1, NEXTY, W+1, 1);
	drawn_row = NEXTY;
	undrawn_column = X+W+1;
      }
      if (n == current_node) {
	fl_color(FL_RED);
	fl_rectf(X,Y,W+1,H+1);
      }
    }
    if (n == current_root) {finish_drawn_row(); break;}
    row += n->size;
    Y = NEXTY;
  }
}

void OutputWindow::draw() {
  //fl_draw_box(box(),0,0,w(),h(),color());
  double scale = (double)(h()-1)/current_root->size;
  fl_font(0,10);
  drawn_row = undrawn_column = 0;
  draw_tree(current_root, 0, 0, scale, 0);
}

int OutputWindow::handle(int event) {
  switch (event) {
  case FL_PUSH:
  case FL_DRAG:
  case FL_RELEASE:
    break;
  default:
    return Fl_Window::handle(event);
  }
  // Figure out what node we are pointing at:
  int X = Fl::event_x();
  int Y = Fl::event_y();
  if (X < 0 || X >= w() || Y < 0 || Y >= h()) return 1;
  int column = X * ncols / w();
  if (column <= 0) {
    setcurrent(current_root, root_depth);
  } else {
    column += root_depth;
    double scale = (double)(h()-1)/current_root->size;
    double offset = 0;
    Node* n = current_root;
    ull row = 0;
    int d = root_depth;
    while (n && d < column) {
      path[d] = n;
      ull hidden = 0;
      for (Node* c = n->child; c; c = c->brother)
	if (c->hidden()) hidden += c->size;
      if (hidden >= n->size) {n = 0; break;}
      double s2 = scale*n->size/(n->size-hidden);
      offset += row*(scale-s2);
      scale = s2;
      n = n->child;
      for (; n; n = n->brother) {
	if (n->hidden()) continue;
	int Y1 = int((row+n->size)*scale+offset+.5);
	if (Y < Y1) break;
	row += n->size;
      }
      d++;
    }
    if (n) setcurrent(n,d);
  }
  // make menus popup now that we have current node:
  if (event == FL_PUSH && Fl::event_button() != 1)
    return Fl_Window::handle(event);
  // double-click opens the item:
  if (event == FL_RELEASE && Fl::event_clicks()) {
    if (current_node == current_root && current_depth)
      setcurrent(path[current_depth-1], current_depth-1);
    else
      setroot(current_node, current_depth);
  }
  // otherwise just navigate to it:
  return 1;
}

void OutputWindow::setcurrent(Node* n, int newdepth) {
  if (current_node == n) return;
  current_node = n;
  current_depth = newdepth;
  if (newdepth <= root_depth) setroot(n, newdepth);
  int m = current_depth-root_depth+1;
  if (m > ncols) ncols = m;
  n->ordinal = labs(n->ordinal); // unhide
  redraw();
}

void OutputWindow::setroot(Node* n, int newdepth) {
  if (n == current_root) return;
  current_root = n;
  root_depth = newdepth;
  char buffer[1024];
  buffer[0] = 0;
  char* p = buffer;
  for (int i = 0; i < root_depth; i++) {
    const char* q = path[i]->name;
    if (q && *q) {
      while (*q) *p++ = *q++;
      if (p[-1] != '/') *p++ = '/';
    }
  }
  strcpy(p,n->name);
  label(buffer);
  int m = current_depth-root_depth+1;
  if (m > ncols) ncols = m;
  redraw();
}

void OutputWindow::copy_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  char buffer[1024];
  char* p = buffer;
  for (int i = 0; i < d->current_depth; i++) {
    const char* q = d->path[i]->name;
    while (*q) *p++ = *q++;
    if (p[-1] != '/') *p++ = '/';
  }
  strcpy(p, d->current_node->name);
  Fl::selection(*d, buffer, strlen(buffer));
}
  
OutputWindow::~OutputWindow() {
  delete_tree(root);
}

void OutputWindow::setroot_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  d->setroot(d->current_node, d->current_depth);
}

void OutputWindow::root_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  if (d->root_depth)
    d->setroot(d->path[d->root_depth-1], d->root_depth-1);
}

void OutputWindow::out_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  if (d->current_depth)
    d->setcurrent(d->path[d->current_depth-1], d->current_depth-1);
}

void OutputWindow::in_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  for (Node* c = d->current_node->child; c; c = c->brother)
    if (!c->hidden()) {
      d->path[d->current_depth] = d->current_node;
      d->setcurrent(c, d->current_depth+1);
      break;
    }
}

void OutputWindow::next_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  for (Node* c = d->current_node->brother; c; c = c->brother)
    if (!c->hidden()) {
      d->setcurrent(c, d->current_depth);
      break;
    }
}

void OutputWindow::previous_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  if (!d->current_depth) return;
  Node* prev = 0;
  for (Node* c = d->path[d->current_depth-1]->child; c; c = c->brother) {
    if (c == d->current_node) {
      if (prev) d->setcurrent(prev, d->current_depth);
      return;
    }
    if (!c->hidden()) prev = c;
  }
}

void OutputWindow::hide_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  d->current_node->ordinal = -labs(d->current_node->ordinal);
  for (Node* c = d->current_node->brother; c; c = c->brother)
    if (!c->hidden()) {
      d->setcurrent(c, d->current_depth);
      return;
    }
  if (d->current_depth)
    d->setcurrent(d->path[d->current_depth-1], d->current_depth-1);
}

void unhide(Node* n) {
  n->ordinal = labs(n->ordinal);
  for (Node* m = n->child; m; m = m->brother) unhide(m);
}
void OutputWindow::unhide_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  unhide(d->current_node);
  d->redraw();
}

static int smallestfirst(const Node* a, const Node* b) {
  return (a->size < b->size) ? -1 : 1;
}

static int alphabetical(const Node* a, const Node* b) {
  return strcmp(a->name, b->name);
}

static int zalphabetical(const Node* a, const Node* b) {
  return -strcmp(a->name, b->name);
}

static int unsorted(const Node* a, const Node* b) {
  return labs(a->ordinal) - labs(b->ordinal);
}

Node* OutputWindow::sort(Node* n, int (*compare)(const Node*, const Node*)) {
  if (!n) return 0;
  Node* head = 0;
  while (n) {
    Node* n1 = n->brother;
    Node** p = &head;
    while (*p) {if (compare(n, *p) < 0) break; p = &(*p)->brother;}
    n->brother = *p;
    *p = n;
    n->child = sort(n->child, compare);
    n = n1;
  }
  return head;
}

void OutputWindow::sort_cb(Fl_Widget* o, void*v) {
  OutputWindow* d = (OutputWindow*)(o->window());
  int (*compare)(const Node*, const Node*);
  switch ((char)(long)v) {
  case 's': compare = largestfirst; break;
  case 'r': compare = smallestfirst; break;
  case 'a': compare = alphabetical; break;
  case 'z': compare = zalphabetical; break;
  default: compare = unsorted; break;
  }
  d->root = sort(d->root, compare);
  d->redraw();
}

void OutputWindow::columns_cb(Fl_Widget* o, void*v) {
  OutputWindow* d = (OutputWindow*)(o->window());
  int n = (int)(long)v;
  ::ncols = n;
  if (n == d->ncols) return;
  if (d->current_depth > d->root_depth+n-1) {
    d->current_depth = d->root_depth+n-1;
    d->current_node = d->path[d->current_depth];
  }
  d->ncols = n;
  d->redraw();
}

void OutputWindow::resize(int X, int Y, int W, int H) {
  window_w = W;
  window_h = H;
  Fl_Window::resize(X,Y,W,H);
}

////////////////////////////////////////////////////////////////
// PostScript output

void OutputWindow::print_tree(FILE*f,Node* n, int column, ull row,
			      double scale, double offset,
			      int bboxw, int bboxh)
{
  int X = bboxw*column/ncols;
  int W = bboxw*(column+1)/ncols - X;
  for (; n; n = n->brother) {
    double Y,H;
    if (n == current_root) {Y = 0; H = bboxh;}
    else if (n->hidden()) continue;
    else {Y = row*scale+offset; H = n->size*scale;}
    fprintf(f, "%d %g %d %g rect", X, -Y, W, -H);
    if (H > 20) {
      fprintf(f, " %d %g moveto (%s) %d fitshow", X+3, -Y-H/2+2, n->name, W-6);
      fprintf(f, " %d %g moveto (%s) %d fitshow", X+3, -Y-H/2-8, formatk(n->size), W-6);
    } else if (H > 10) {
      fprintf(f, " %d %g moveto (%s %s) %d fitshow", X+3, -Y-H/2-4, n->name, formatk(n->size), W-6);
    } else if (H > 2) {
      fprintf(f, " /Helvetica findfont %g scalefont setfont\n", H*0.95);
      fprintf(f, " %d %g moveto (%s %s) %d fitshow\n", X+3, -Y-H*0.85, n->name, formatk(n->size), W-6);
      fprintf(f, " /Helvetica findfont 10 scalefont setfont");
    }
    fprintf(f, "\n");
    if (n->child && column+1 < ncols) {
      ull hidden = 0;
      for (Node* c = n->child; c; c = c->brother)
	if (c->hidden()) hidden += c->size;
      if (hidden > 0) {
	double yy = (row+hidden)*scale+offset;
	fprintf(f, " %d %g moveto %d %g lineto %d %g lineto stroke\n",
		X, -yy, X+10, -yy, X+W, -Y);
      }
      if (hidden < n->size) {
	double s2 = scale*n->size/(n->size-hidden);
	print_tree(f, n->child, column+1, row, s2, offset+row*(scale-s2), bboxw, bboxh);
      }
    }
    if (n == current_root) return;
    row += n->size;
  }
}

#if FL_MAJOR_VERSION > 1
static void ok_cb(Fl_Widget* w, void*) {
  w->window()->set();
  w->window()->hide();
}
#endif

void OutputWindow::print_cb(Fl_Widget* o, void*) {
  OutputWindow* d = (OutputWindow*)(o->window());
  if (!print_panel) make_print_panel();
#if FL_MAJOR_VERSION > 1
  print_ok_button->callback(ok_cb);
  if (!print_panel->exec()) return;
#else
  print_panel->show();
  for (;;) {
    if (!print_panel->shown()) return;
    Fl_Widget* o = Fl::readqueue();
    if (o) {
      if (o == print_ok_button) break;
    } else {
      Fl::wait();
    }
  }
  print_panel->hide();
#endif
  FILE *f;
  if (print_file_button->value()) {
    f = fopen(print_file_input->value(), "w");
  } else {
    f = popen(print_command_input->value(), "w");
  }
  print_file(d, f, print_portrait_button->value(), fill_page_button->value());
  if (print_file_button->value()) {
    fclose(f);
  } else {
    pclose(f);
  }
}

void OutputWindow::print_file(OutputWindow* d, FILE *f, bool portrait, bool fill) {
  if (!f) {
    alert("Can't open printer output stream");
    return;
  }
  fprintf(f, "%%!PS-Adobe-2.0\n");
  int W = 7*72+36;
  int H = 10*72;
  if (!portrait) {int t = W; W = H; H = t;}
  int X = 0;
  int Y = 0;
  if (!fill) {
    if (d->w()*H > d->h()*W) {int t = d->h()*W/d->w(); Y = (H-t)/2; H = t;}
    else {int t = d->w()*H/d->h(); X = (W-t)/2; W = t;}
  }
  if (portrait)
    fprintf(f, "%%%%BoundingBox: 36 36 %d %d\n",36+X+W,36+Y+H);
  else
    fprintf(f, "%%%%BoundingBox: 36 36 %d %d\n",36+Y+H,36+X+W);
  fprintf(f, "/rect {4 2 roll moveto dup 0 exch rlineto exch 0 rlineto neg 0 exch rlineto closepath stroke} bind def\n");
  fprintf(f, "/pagelevel save def\n");
  fprintf(f, "/Helvetica findfont 10 scalefont setfont\n");
  fprintf(f, "/fitshow {gsave 1 index stringwidth pop div dup 1 lt {dup sqrt scale} {pop} ifelse show grestore} bind def\n");
  fprintf(f, "0 setlinewidth\n");
  if (portrait)
    fprintf(f, "%d %d translate\n", 36+X, 36+Y+H);
  else
    fprintf(f, "%d %d translate 90 rotate\n", 36+Y, 36+X);
  double scale = (double)H/d->current_root->size;
  d->print_tree(f, d->current_root, 0, 0, scale, 0, W, H);
  fprintf(f,"showpage\npagelevel restore\n%%%%EOF\n");
}

// End of xdiskusage.C
