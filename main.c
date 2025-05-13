#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <stdio.h>

int main() {
    Display *display;
    Window root, win;
    XEvent ev;
    XRenderPictFormat *pictformat;
    XRenderPictureAttributes pa;
    Picture picture;
    XRenderColor clear_color = { 0, 0, 0, 0 }; // Fully transparent color
    XRenderColor dim_color = { 0, 0, 0, 0x8000 }; // Semi-transparent black (50% alpha)
    int screen;
    Atom atomWmDeleteWindow;

    int win_width = 400;
    int win_height = 300;
    
    display = XOpenDisplay(NULL);
    if (!display) return 1;

    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    
    int event_base, error_base;
    if (!XCompositeQueryExtension(display, &event_base, &error_base)) {
        fprintf(stderr, "Composite extension not supported.\n");
        return 1;
    }
    
    int render_event_base, render_error_base;
    if (!XRenderQueryExtension(display, &render_event_base, &render_error_base)) {
        fprintf(stderr, "Render extension not supported.\n");
        return 1;
    }

    // Get ARGB visual
    XVisualInfo vinfo;
    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No 32-bit TrueColor visual found.\n");
        return 1;
    }
    
    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(display, root, vinfo.visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = 0; // Transparent background
    attr.override_redirect = False; // Allow window manager control

    win = XCreateWindow(display, root, 100, 100, win_width, win_height, 0, 
                        vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
    
    atomWmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, win, &atomWmDeleteWindow, 1);
    
    XStoreName(display, win, "Dimming Window");
    
    XSelectInput(display, win, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
    XMapWindow(display, win);
    
    pictformat = XRenderFindVisualFormat(display, vinfo.visual);
    if (!pictformat) {
        fprintf(stderr, "No render format found for visual.\n");
        return 1;
    }

    pa.subwindow_mode = IncludeInferiors;
    picture = XRenderCreatePicture(display, win, pictformat, CPSubwindowMode, &pa);
    
    while (1) {
        XNextEvent(display, &ev);

        if (ev.type == Expose || ev.type == ConfigureNotify) {
            if (ev.type == ConfigureNotify) {
                win_width = ev.xconfigure.width;
                win_height = ev.xconfigure.height;
            }
            
            XRenderFillRectangle(display, PictOpSrc, picture, &clear_color, 0, 0, win_width, win_height);
            XRenderFillRectangle(display, PictOpOver, picture, &dim_color, 0, 0, win_width, win_height);
        }

        if (ev.type == ClientMessage && (Atom)ev.xclient.data.l[0] == atomWmDeleteWindow) {
            break;
        }
    }

    // Cleanup
    XRenderFreePicture(display, picture);
    XDestroyWindow(display, win);
    XCloseDisplay(display);

    return 0;
}
