#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MIN(A, B)	((A) < (B) ? (A) : (B))

/* --------- XEMBED and systray stuff */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static int trapped_error_code = 0;
static int (*old_error_handler) (Display *, XErrorEvent *);

#define BC 0 // dark green
#define BD 1 // dark red
#define FC 2 // white
#define FD 3 // white

XColor colors[4];

static int
error_handler(Display * display, XErrorEvent * error) {
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

void init_colors(){
	
	// orange background charging
	colors[BC].flags = DoRed | DoGreen | DoBlue;
	colors[BC].red   = 0xF5 * 255;
	colors[BC].green = 0xB0 * 255;
	colors[BC].blue  = 0x65 * 255;
	
	// blue background discharging
	colors[BD].flags = DoRed | DoGreen | DoBlue;
	colors[BD].red   = 0xAE * 255;
	colors[BD].green = 0xD6 * 255;
	colors[BD].blue  = 0xF1 * 255;
	
	// black foreground charging
	colors[FC].flags = DoRed | DoGreen | DoBlue;
	colors[FC].red   = 0x00 * 255;
	colors[FC].green = 0x00 * 255;
	colors[FC].blue  = 0x00 * 255;
	
	// black forground discharging
	colors[FD] = colors[FC];
	
}

void print_usage(){
	
	printf("\n\
btry v0.1 (20180101) system tray battery monitor\n\
\n\
Usage: btry [-h] [-b FFFFFF] [-f FFFFFF] [-B FFFFFF] [-F FFFFFF]\n\
\n\
  -h --help          print this dialog\n\
  -b --bg-charge     background color while charging\n\
  -f --fg-charge     foreground color while charging\n\
  -B --bg-discharge  background color while discharging\n\
  -F --fg-dischasrge  foreground color while discharging\n\
\n");
	
}

int parse_color(char * color, int c_i){
	
	// fail if flag wasn't passed a real color
	if (strlen(color) != 6){
		
		printf("E: invalid color '%s', must be 6 hex digits -- FFFFFF\n", color);
		print_usage();
		return 1;
	}
	
	uint8_t values[6];
	
	for (int i = 0; i < 6; i++ ) {
		
		if (color[i] >= '0' && color[i] <= '9'){
			values[i] = color[i] - '0';
			continue;
		}
		
		if(color[i] >= 'A' && color[i] <= 'F'){
			values[i] = color[i] - 'A' + 10;
			continue;
		}
		
		if(color[i] >= 'a' && color[i] <= 'f'){
			values[i] = color[i] - 'a' + 10;
			continue;
		}
		
		return 1;
	}
	
	// convert 6 0-15 numbers to 3 0-65535 numbers
	// hopefully xlib converts against floor values
	// because this will turn 0-255 -> 0-65280
	colors[c_i].red   = (values[0] * 0x10 + values[1]) * 0x100;
	colors[c_i].green = (values[2] * 0x10 + values[3]) * 0x100;
	colors[c_i].blue  = (values[4] * 0x10 + values[5]) * 0x100;
	
	printf("%d\n", c_i);
	printf("%d - %d - %d - %d - %d - %d\n", values[0], values[1], values[2], values[3], values[4], values[5]);
	printf("r: %d\ng: %d\nb: %d\n", colors[c_i].red, colors[c_i].green, colors[c_i].blue);
	printf("\n");
	
	return 0;
}

int parse_args(int argc, char **argv){
	
	int i;
	for(i = 1; i < argc; i++){
		
		if ( ! strcmp(argv[i], "-h") || ! strcmp(argv[i], "--help") ) {
			print_usage();
			return 1;
		} else if ( ! strcmp(argv[i], "-b") || ! strcmp(argv[i], "--bg-charge") ) {
			
			// ensure required argument
			if (argc == i + 1){
				printf("E: %s requires argument [COLOR] -- FFFFFF\n", argv[i]);
				print_usage();
				return 1;
			}
			
			if (parse_color(argv[i + 1], BC)) {
				printf("E: failed to parse colors\n");
				return 1;
			}
			
			i += 1;
			continue;
		} else if ( ! strcmp(argv[i], "-f") || ! strcmp(argv[i], "--fg-charge") ) {
			
			// ensure required argument
			if (argc == i + 1){
				printf("E: %s requires argument [COLOR]\n", argv[i]);
				print_usage();
				return 1;
			}
			
			if (parse_color(argv[i + 1], FC)) {
				printf("E: failed to parse colors\n");
				return 1;
			}
			
			i += 1;
			continue;
		} else if ( ! strcmp(argv[i], "-B") || ! strcmp(argv[i], "--bg-discharge") ) {
			
			// ensure required argument
			if (argc == i + 1){
				printf("E: %s requires argument [COLOR]\n", argv[i]);
				print_usage();
				return 1;
			}
			
			if (parse_color(argv[i + 1], BD)) {
				printf("E: failed to parse colors\n");
				return 1;
			}
			
			i += 1;
			continue;
		} else if ( ! strcmp(argv[i], "-F") || ! strcmp(argv[i], "--fg-discharge") ) {
			
			// ensure required argument
			if (argc == i + 1){
				printf("E: %s requires argument [COLOR]\n", argv[i]);
				print_usage();
				return 1;
			}
			
			if (parse_color(argv[i + 1], FD)) {
				printf("E: failed to parse colors\n");
				return 1;
			}
			
			i += 1;
			continue;
		} else {
			printf("E: unrecognized flag '%s'\n", argv[i]);
			print_usage();
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
	//~ Colormap cm;
	//~ XVisualInfo vi;
	Display *dpy;
	//~ GC gc;
	int screen;
	Window root, win;
	int rc = 0;
	
	init_colors();
	
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
	
	XColor throwaway;
	
	throwaway.flags= DoRed | DoGreen | DoBlue;
	throwaway.red = 65535;
	throwaway.green = 32767;
	throwaway.blue = 0;
	
	if(XAllocColor(dpy,wa.colormap,&throwaway) == 0){
		printf("Failed to allocate color\n");
		return 1;
	}
	
	for(int i = 0; i < 4; i++){
		if(XAllocColor(dpy, wa.colormap, &(colors[i])) == 0){
			printf("Failed to allocate color\n");
			return 1;
		}
	}
	
	win = XCreateSimpleWindow(dpy, root, 0, 0, width, height, 0, 0, colors[0].pixel);
	
	//~ gc = XCreateGC(dpy, win, NULL, NULL);
	
	//~ final_colors[0] = throwaway;
	
	//~ printf("%x\n", final_colors[0].pixel);
	
	// this is probably wrong
	// XDrawPoint(dpy, win, root, x, y);

	//~ XSetForeground(d, gc, xcolour.pixel);
	//~ XFillRectangle(d, w, gc, 0, 0, winatt.width, 30);
	//~ XFlush(d);
	
	send_systray_message(dpy, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0); // pass win only once
	XMapWindow(dpy, win);
	
	XSync(dpy, False);
	
	/* run */
	while(1) {
		while(XPending(dpy)) {
			XNextEvent(dpy, &ev); /* just waiting until we error because window closed */
		}
	}
	
	// XFreeColormap()
}
