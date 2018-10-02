#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "global.h"

#define MIN(A, B)               ((A) < (B) ? (A) : (B))

/* --------- XEMBED and systray stuff */
#define SYSTEM_TRAY_REQUEST_DOCK   0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static int trapped_error_code = 0;
static int (*old_error_handler) (Display *, XErrorEvent *);

static int
error_handler(Display     *display, XErrorEvent *error) {
    trapped_error_code = error->error_code;
    return 0;
}

void
trap_errors(void) {
    trapped_error_code = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

int
untrap_errors(void) {
    XSetErrorHandler(old_error_handler);
    return trapped_error_code;
}

void
send_systray_message(Display* dpy, long message, long data1, long data2, long data3) {
    XEvent ev;

    Atom selection_atom = XInternAtom (dpy,"_NET_SYSTEM_TRAY_S0",False);
    Window tray = XGetSelectionOwner (dpy,selection_atom);

    if ( tray != None)
        XSelectInput (dpy,tray,StructureNotifyMask);

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = tray;
    ev.xclient.message_type = XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False );
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1; // <--- your window is only here
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

    trap_errors();
    XSendEvent(dpy, tray, False, NoEventMask, &ev);
    XSync(dpy, False);
    usleep(10000);
    if (untrap_errors()) {
        /* Handle errors */
    }
}

void print_colors(){
	
	int i;
	size_t len = (sizeof (x_colors) / sizeof (const char *));
	
	printf("\nColors:\n\n");
	
	for (i = 0; i < len; i++ )
		printf("  '%s'\n", x_colors[i]);
	
	printf("\n");
	
}

void print_usage(char * bin){
	
	printf("\n\
btry v0.1 (20180101) system tray battery monitor\n\
\n\
Usage: %s [-hc] [-b COLOR] [-f COLOR] [-B COLOR] [-F COLOR]\n\
\n\
  -h --help          print this dialog\n\
  -c --colors        print all valid color names\n\
  -b --bg-charge     background color while charging\n\
  -f --fg-charge     foreground color while charging\n\
  -B --bg-discharge  background color while discharging\n\
  -F --fg-discharge  foreground color while discharging\n\
\n",
	bin);
	
}

int validate_color(char * color){
	
	int i;
	size_t len = (sizeof (x_colors) / sizeof (const char *));
	
	for (i = 0; i < len; i++ ) {
		
		if ( ! strcmp(x_colors[i], color) )
			return 0;
	}
	
	return 1;
}

int parse_args(int argc, char **argv){
	
	int i;
	for(i = 1; i < argc; i++){
		
		if ( ! strcmp(argv[i], "-h") || ! strcmp(argv[i], "--help") ) {
			print_usage(argv[0]);
			return 1;
		} else if ( ! strcmp(argv[i], "-c") || ! strcmp(argv[i], "--colors") ) {
			print_colors();
			return 1;
		} else if ( ! strcmp(argv[i], "-b") || ! strcmp(argv[i], "--bg-charge") ) {
			
			// ensure required argument
			if (argc == i + 1){
				printf("E: %s requires argument [COLOR]\n", argv[i]);
				print_usage(argv[0]);
				return 1;
			}
			
			// fail if flag wasn't passed a real color
			if (validate_color(argv[i + 1])){
				
				printf("E: invalid color '%s'\n", argv[i]);
				print_colors();
				return 1;
			}
			
			i++;
			continue;
		} else {
			printf("E: unrecognized flag '%s'\n", argv[i]);
			print_usage(argv[0]);
			return 1;
		}
		
	}
	
	return 0;
	
}

/* ------------ Regular X stuff */
int
main(int argc, char **argv) {
    int width, height;
    XWindowAttributes wa;
    XEvent ev;
    Display *dpy;
    int screen;
    Window root, win;
    int rc;
    
    if (argc > 1) // no args -> defaults
		rc = parse_args(argc, argv);
		
	if (rc) // help or colors
		return 0;

    /* init */
    if (!(dpy=XOpenDisplay(NULL)))
        return 1;
    screen = DefaultScreen(dpy);    
    root = RootWindow(dpy, screen);
    if(!XGetWindowAttributes(dpy, root, &wa))
        return 1;
    width = height = MIN(wa.width, wa.height);
    
    //~ if (! XLookupColor(dpy, colormap, ""))
    
    
    //~ white = WhitePixel(dpy, screen);
    //~ black = BlackPixel(dpy, screen);

    /* create window */
    win = XCreateSimpleWindow(dpy, root, 0, 0, width, height, 0, 0, 0x708090);

    send_systray_message(dpy, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0); // pass win only once
    XMapWindow(dpy, win);

    XSync(dpy, False);

    /* run */
    while(1) {
        while(XPending(dpy)) {
            XNextEvent(dpy, &ev); /* just waiting until we error because window closed */
        }
    }
}
