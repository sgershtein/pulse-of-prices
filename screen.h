//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Screen classes header file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 5-Oct-95
//-----------------------------------------------------------------------------

#ifndef _SCREEN_H_
#define _SCREEN_H_
// only include once

#include "ioevents.h"
#pragma warn -dup
#include <vfcrt/vfcrt.h>
#pragma warn .dup
#include "schemes.h"

class Rect {   // Coordinates of a rectangle
   public:
      char x1,y1,x2,y2;
      Rect() {}   // default constructor
      Rect(char xx1, char yy1, char xx2, char yy2) :
         x1(xx1),y1(yy1),x2(xx2),y2(yy2) { // coordinate constructor
         if (xx1>xx2) {       // normalizing
            x1=xx2; x2=xx1;
         }
         if (yy1>yy2) {
            y1=yy2; y2=yy1;
         }
      }
};

class Attr {   // class, keeping all the attributes
   public:
      Attr() {
         if (scheme==scCheck)
            check();
      }
      char& operator[](unsigned no) { // return color No. for the cur. scheme
         return attr[no][scheme];
      }
      char& operator()(unsigned no) { // color No. for user defined scheme
         return attr[no][scUser];
      }
      void Scheme(s) {  // choose a cheme
         if (scheme<scMaxScheme)
            scheme =s;
      }
   private:
      static char scheme;   // scMono, scColor, scLaptop, scUser, scCheck
      static char attr[attrMax+1][scMaxScheme+1];
      void check();  // if video mode is 7 => scheme:=scMono, else :=scColor
};

class Cursor {
   public:
      char x() { return VWhereX(); }
      char y() { return VWhereY(); }
      void moveto(char x, char y) { VGotoXY(x,y); }
      void hide() { VGotoXY(0,80); }   // will this work?
};

class ScreenObject { // an object, that can be drawn on the screen
   public:
//      ScreenObject() {}
      virtual ~ScreenObject() {} // virtual destructor mechanism
      virtual void draw() = 0;
   protected:
      Mouse mouse;      // a static class
      Display display;  // another static class
      Attr attr;        // same here (static class)
      Cursor cursor;    // this class doesn't have any variables at all
};

class Statusbar : public ScreenObject { // status bar line
   public:
      void clear();
      void say(char*);     // output the string on the status bar
      void working(BOOL);  // show/hide "Working" sign
      virtual void draw();
      char *get() {        // return current status bar contence
         return saying;
      }
   private:
      static char *saying;
      static BOOL wrk;
};

class Background : public ScreenObject {  // Screen background
   public:
      Background() : filler(206) {}  // default constructor
      Background(char c) : filler(c) {};
      virtual void draw();
   private:
      char filler;   // filler character
};

class VisibleObject;
class Window;

class Screen { // contains a list of screenelements
   public:
      Screen() { wlist=NULL; }
      virtual ~Screen();   // must delete all the windows
   protected:
      VisibleObject *wlist;
};

class VisibleObject : public ScreenObject {
      friend class Screen;
      friend class Window;
   public:
      VisibleObject(Rect r, Window *p=NULL) : rect(r), parent(p) {}
      char x1() { return rect.x1; } // window coordinates
      char y1() { return rect.y1; }
      char x2() { return rect.x2; }
      char y2() { return rect.y2; }
      char l() { return x2()-x1()+1; } // window length
      char h() { return y2()-y1()+1; } // window height
      void Save(void*&);    // save rectangle rect into buffer
      void Restore(void*);  // restore rectangle rect from buffer

   protected:
      Rect rect;            // window coordinates (relative in parent window)
      Window *parent;
      VisibleObject *next;  // to be queued by Screen class
      void SetParent(Window *p) { parent=p; }
};

class Window : public Screen, public VisibleObject {
   public:
      Window(Rect r, char *t=NULL, char *f=NULL, BOOL s=FALSE, Window *p=NULL) :
         VisibleObject(r,p), title(t), frame(f), shadow(s) {}
      virtual ~Window();
      virtual void draw();
      virtual void AddChild(VisibleObject *child); // Add a child
      void SetTitle(char *t) {   // Set window title
         title = t;
      }
      char cx1() { return rect.x1+1; } // client coordinates
      char cy1() { return rect.y1+1; }
      char cx2() { return rect.x2-1; }
      char cy2() { return rect.y2-1; }
      char cl() { return x2()-x1()-1; } // window client length
      char ch() { return y2()-y1()-1; } // window client height
   protected:
      BOOL shadow; // is it a window with shadow?
      char *frame;
      char *title;
      virtual char attrcode() {
         return attrWindow;
      }
};

class Control { // a control element
   public:
      Control() : focus(FALSE) {}
      virtual ~Control() {    // virtual destructor mechanism
         LostFocus();
      }
      virtual void GotFocus() {
         focus=TRUE;
      }
      virtual void LostFocus() {
         focus=FALSE;
      }
      virtual BOOL ChewEvent(Event*) {
         return FALSE;  // by default event not chown
      }

   protected:
      BOOL focus;
};

class ControlWindow;

class ControlObject : public Control, public VisibleObject {
   friend class ControlWindow;
   public:
      ControlObject(Rect r,char *s=NULL,ControlWindow *p=NULL) :
         VisibleObject(r,(Window*)p), saying(s), olds(NULL),
         next(NULL), tab(NULL), left(NULL), right(NULL), up(NULL),
         down(NULL) {}
      void SetTTLRUD(ControlObject *t, ControlObject *st,
         ControlObject *l=NULL,ControlObject *r=NULL,
         ControlObject *u=NULL,ControlObject *d=NULL) {
         stab = st;
         tab = t;
         left = l;
         right = r;
         up = u;
         down =d;
      }
      virtual BOOL ChewEvent(Event*);
      ControlWindow* GetParent() {
         return (ControlWindow*)parent;
      }
      virtual void GotFocus();
      virtual void LostFocus();
   protected:
      ControlObject *next, // to be queued by ControlWindow
         *stab, *tab, *left, *right, *up, *down; // where to move focus on keys
      char *saying;  // to display on status bar when focused
      char *olds;    // old saying
      Statusbar statusbar;

};

class ControlWindow : public Control, public Window {
   public:
      ControlWindow(Rect r, char *t=NULL, char *f=NULL, BOOL s=FALSE,
          BOOL cb=FALSE, Window *p=NULL) : clist(NULL), focus(NULL),
          Window(r,t,f,s,p), closebutton(cb) {}
      virtual ~ControlWindow();
      virtual void AddControl(ControlObject*);
      virtual BOOL ChewEvent(Event*); // event may be processed by children
      virtual void draw();
      void SetFocus(ControlObject *f) {
         focus = f;
      }
      void MoveFocus(ControlObject *f) {  // change focus and notify all
         if (focus)
            focus->LostFocus();
         SetFocus(f);
         if (focus)
            focus->GotFocus();
      }
      virtual void Notify(char code) {
         ncode = code;
      }
      virtual char GoModal(char *saying="");
         // saying - for status bar. Returns ncode
   protected:
      BOOL closebutton; // does window has a close button?
      char ncode;    // child event notification code
      ControlObject *clist, // list of objects
         *focus;  // object that has focus
};

class Button : public ControlObject {
   public:
      Button(char* s, char sel, char x1, char x2, char c, unsigned h=0,
         BOOL d=FALSE, char *say=NULL, ControlWindow *p=NULL);
            // name, sel.char.pos, x,y , ncode, hotkey, def, saying, parent
      virtual void draw();
      virtual BOOL ChewEvent(Event*);
      void Press(BOOL b) {
         pressed = b;
         draw();
      }
      BOOL Toggle() {
         pressed = !pressed;
         draw();
         return pressed;
      }
      virtual void GotFocus() {
         ControlObject::GotFocus();
         draw();
      }
      virtual void LostFocus() {
         ControlObject::LostFocus();
         draw();
      }
   protected:
      BOOL def;      // is this default button
      char ncode;    // event notification code to send to parent
      unsigned hotkey;   // hot key scan+key code
      char *name;    // button name
      char selchar;  // selection char No. in name. None if zero.
      BOOL pressed;  // is it pressed?
};

class MessageBox : public ControlWindow {
   public:
      MessageBox(char *title, char *txt, char *sbtext=NULL);
      virtual void draw();
   private:
      char *text;
      void *savebuf;
      virtual char attrcode() {
         return attrMesBox;
      }
};

class ScrollBar : public ControlObject {
   public:
      ScrollBar(char x, char y, char l, char c, long mi, long ma,
         BOOL v=TRUE, ControlWindow *p=NULL);
            // x,y,l, ncode, min, max, vert (orientation), parent
      void setpos(long p);
      void resize(char x, char y, char l, long mi, long ma);
      long getpos() { return pos; }
      virtual void draw();
      virtual BOOL ChewEvent(Event*);
   private:
      char ncode;    // event notification code to send to parent
      BOOL vert;     // vertical or horizontal?
      long pmin, pmax, pos;   // minimal, maximal and current positions
      char hl;       // on-screen handle coordinate (0..len-1)
      char len;      // on-screen scroll bar length
};

class ListWindow : public ControlWindow {
   public:
      ListWindow(Rect r, long n, char *t=NULL, char *f=NULL, BOOL s=FALSE,
          BOOL cb=FALSE, Window *p=NULL);
      // rect, number of elements, title, frame, shadow, closebutton, parent
      virtual ~ListWindow();
      void moveto(long n);    // choose n-th element
      void down(long n=1);    // down n times
      void up(long n=1);      // up n times
      void pgdn();            // page down
      void pgup();            // page up
      virtual void draw();
      virtual BOOL ChewEvent(Event*);
      virtual BOOL selected(long n)=0;       // is n-th line selected?
      virtual void select(long n, BOOL b)=0; // select or deselect n-th line
      void select(long n) {   // reverse selection settings for n-th line
         select(n,!selected(n));
      }
      void resize(Rect);
      void setlno(long l) {
         lno=l;
      }
      void repaint(char c=255) { // 255 - full redraw.  1 - only reoutput lines
         redraw=c;
         draw();
      }
      BOOL visible(long l) {
         return (l>=lfirst && l-lfirst<ch()); 
      }
      virtual void Notify(char code);  // redefining Notify function
      long Lno() { return lno; }
      long Lcur() { return lcur; }
   protected:
      long lno;      // number of lines in the list
      long lcur,     // current list line
           lfirst;   // first displayd line in the window
      virtual char* getline(long n)=0;   // get n-th line
      char redraw;   // >0 if repaint is needed, 255 = full repaint
   private:
      long  ocur,    // old values of first and cur
            ofirst;  // use by draw() function
      ScrollBar *scb;
      virtual char attrcode() {
         return attrListWin;
      }
};

class InputField : public ControlObject {
   public:
      enum InpType {
         it_caps = 128, // automatic input capitalization
         it_any = 0, // any characters allowed for input
         it_int = 1, // only '0'..'9' are allowed
         it_float = 3, // '0'..'9' + '.' + 'e'
         it_filename = 4 // filenames, no spaces allowed
      };
      InputField(char x, char y, char l, char *&s, InpType it=it_any,
         char *say=NULL, ControlWindow *p=NULL);
            // x,y,l, s, inp. type, saying, parent
            // if s!=NULL => s must have enough space for l characters!
            // if s==NULL, space will be allocated, but won't be deleted!
            // It must be deleted manually when no longer needed
      ~InputField() {
         if (focus)
            LostFocus();
      }
      virtual void draw();
      virtual BOOL ChewEvent(Event*);
      virtual void GotFocus();
      virtual void LostFocus();
      char* GetBuf() {
         return buf;
      }
   private:
      BOOL clear;    // if TRUE and key pressed => clear old string
      char len;      // buffer length
      char xc,yc;    // Cursor position
      char *buf;
      char pos;      // cursor position in buffer
      InpType itype; // input string type
};

class PercentBar : public ControlObject {
   public:
      PercentBar(char x, char y, char l, long mi, long ma,
         ControlWindow *p=NULL);
            // x,y,l, min, max, parent
      void setpos(long p);
      void resize(char x, char y, char l, long mi, long ma);
      long getpos() { return pos; }
      virtual void draw();
//      virtual BOOL ChewEvent(Event*);
   private:
      long pmin, pmax, pos;   // minimal, maximal and current positions
      char hl;       // on-screen handle coordinate (0..len-1)
      char len;      // on-screen scroll bar length
};

class RadioButton {  // does nothing itself, handled by RadioButtonGroup class
   friend class RadioButtonGroup;
   public:
      RadioButton(int x1, int y1,char *txt1=NULL, int xt1=0, int yt1=0) :
         x(x1), y(y1), txt(txt1), tx(xt1), ty(yt1) {}
   private:
      int x,y;    // button coordinates
      int tx,ty;  // text coordinates (if applicable)
      char *txt;  // text (if any)
};

class RadioButtonGroup : public ControlObject { // a group of radio buttons
   public:
      RadioButtonGroup(RadioButton *grp,int no,int sel,
         char *say=NULL, ControlWindow *p=NULL) :
         ControlObject(Rect(1,2,3,4),say,p), cx(-1), cy(-1),
         buttons(grp), bno(no), bsel(sel) {}
      virtual void draw();
      virtual BOOL ChewEvent(Event*);
      virtual void GotFocus() {
         ControlObject::GotFocus();
         draw();
      }
      virtual void LostFocus() {
         ControlObject::LostFocus();
         draw();
      }
      int selected() { return bsel; }
      void select(int sel) {
         bsel=sel;
         draw();
      }
   private:
//      char *saying;           // statusbar saying
      int bno;                // number of buttons
      int bsel;               // no. of currently selected one
      RadioButton *buttons;   // array of the buttons
      int cx,cy;
};


#endif
