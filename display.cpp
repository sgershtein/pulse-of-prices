//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Display and Screen routines
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 30-Sep-95
//-----------------------------------------------------------------------------

#include "ioevents.h"
#include "screen.h"
#include <conio.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>

// ---------------------------- Display class implementation ------------------
char Display::adapter=adaCHECK;	// adaEGA, adaVGA, adaOTHER
char Display::mx,Display::my;

Display::Display() {
   if (adapter!=adaCHECK)
      return;
	adapter = chkada();
	struct text_info ti;
	gettextinfo(&ti);
	my = ti.screenheight-1;
	if ((mx = ti.screenwidth-1)<70)
		NormRes();	// We don't like 40x25 text modes, just don't like them :)
}

void Display::Done(char *s) {
   if (my>25)
      NormRes();
   VClrScr(' ',LIGHTGRAY);
   VPuts(0,23,s?s:"");
   Cursor c;
   c.moveto(0,24);
};

BOOL Display::HiRes() {
	if (adapter) {
		textmode(C4350);
   	struct text_info ti;
	   gettextinfo(&ti);
	   my = ti.screenheight-1;
	   mx = ti.screenwidth-1;
		return TRUE;
	}
	return FALSE;
}

BOOL Display::NormRes() {
	textmode(C80);
	struct text_info ti;
	gettextinfo(&ti);
	my = ti.screenheight-1;
	mx = ti.screenwidth-1;
	return TRUE;
}

char Display::chkada() {
	char aa=adaOTHER;
	asm {
		  MOV     AX,1A00H
		  int     10h
		  CMP     AL,1AH
		  JNZ     NONVGA               	// not VGA
		  CMP     BL,0FFH
		  JZ      NONVGA                	// not VGA
		  CMP     BL,08H
		  JNZ     NONVGA                	// not VGA
		  MOV     aa,adaVGA   			  	// It is VGA, 16 scan lines
		  JMP     SHORT DONEVA
NONVGA: MOV     Ah,12H             		// OK, it is not VGA, maybe EGA?
		  mov     bl,10h
		  int     10h
		  CMP     BL,10H
		  JZ      DONEVA                	// not EGA
		  CMP     CL,6
		  JC      DONEVA                	// not EGA
		  CMP     CL,0AH
		  JNC     DONEVA                	// not EGA
		  MOV     aa,adaEGA   			  	// It is EGA, 14 scan lines
DONEVA:
	}
	return aa;
}

// ---------------------------- Attr class implementation ---------------------

char Attr::scheme=scCheck; // must be checked
char Attr::attr[attrMax+1][scMaxScheme+1] = {ATTRIBUTES};

void Attr::check() { // check video mode: color/mono
   char* vm=(char*)(0x0449);  // Current video mode [BIOS by Lukach, p. 131]
   scheme = (*vm==7?scMono:*vm==2?scLaptop:scColor);
   for (int i=0; i<=attrMax; i++)
      attr[i][scUser]=attr[i][scheme];// copying current scheme to user defined 
}

// ---------------------- Background class implementation ---------------------

void Background::draw() {
   Attr at;
   mouse.hide();
   VClrScr (filler,at[attrBkgr]);
//   VStena(at[attrBkgr]);
   mouse.show();
}

// ---------------------- Statusbar class implementation ---------------------

char *Statusbar::saying = "";
BOOL Statusbar::wrk = FALSE;

void Statusbar::clear() {
   saying = "";
   draw();
}

void Statusbar::say(char* s) {  // output the string on the status bar
   saying = s;
   draw();
}

void Statusbar::working(BOOL b) {  // show/hide "Working" sign
   wrk = b;
   draw();
}

void Statusbar::draw() {
   mouse.hide();
   VPutchColor(0,display.yMax(),' ',attr[attrStatus]);
   VPutsColorLen(1,display.yMax(),saying,attr[attrStatus],80);
   char swrk[]="РАБОТАЮ";
   if (wrk)
      VPutsColor(81-sizeof(swrk),display.yMax(),swrk,attr[attrWorking]);
   mouse.show();
}

// ---------------------- Screen class implementation ---------------------

Screen::~Screen() {   // must delete all the windows
   while (wlist) {
      VisibleObject *w=wlist;
      wlist=wlist->next;
      delete w;
   }
}

// ---------------------- Window class implementation ---------------------

void Window::AddChild(VisibleObject *child) {
   child->next=wlist;
   child->SetParent(this);
   wlist=child;
}

void Window::draw() {
   mouse.hide();
   char dx=0,dy=0; // relative coordinates offset
   if (parent) {
      dx = parent->cx1();
      dy = parent->cy1();
   }
   VClrFrame(x1()+dx,y1()+dy,cl(),ch(),' ',attr[attrcode()]);
   if (frame)  // draw window with a frame
      VFrame(x1()+dx,y1()+dy,cl(),ch(),frame,attr[attrcode()]);
   VPutsColorMid((x1()+x2())/2,y1(),title,attr[attrcode()]);
   if (shadow)
      VTen(x1()+dx,y1()+dy,cl(),ch());
   for (VisibleObject *w=wlist; w; w=w->next)
      w->draw();
   mouse.show();
}

Window::~Window() {
   VisibleObject *o;
   while ((o=wlist)!=NULL) {
      wlist=wlist->next;
      delete o;
   }
}

// ------------------ ControlWindow class implementation ---------------------

ControlWindow::~ControlWindow() {
//   Window::~Window();
   ControlObject *o;
   while ((o=clist)!=NULL) {
      clist=clist->next;
      delete o;
   }
}

void ControlWindow::AddControl(ControlObject* child) {
   child->next=clist;
   child->SetParent(this);
   clist=child;
}

BOOL ControlWindow::ChewEvent(Event *e) { // event may be processed by children
   if (closebutton)
      if (e->type==evMouseClick) {
         MouseEvent *m=(MouseEvent*)e;
         if (m->x==x1()+2 && m->y==y1()) {
            ncode = ncCloseButton;
            ClickSnd();
            return TRUE;
         }
      } else if (e->type==evKeyPress) {
         KeyEvent *k=(KeyEvent*)e;
         if (k->keycode==27) {  // ESC
            ncode = ncCloseButton;
            return TRUE;
         }
      }
   if (focus && focus->ChewEvent(e))
      return TRUE;   // focus receives event first
   for (ControlObject *c=clist; c; c=c->next) // then all the others
      if (c!=focus && c->ChewEvent(e))
         return TRUE;
   return FALSE;
}

void ControlWindow::draw() {
   Window::draw();   // draw the window and non-control children
   mouse.hide();
   if (closebutton)
      VPutsColor(x1()+1,y1(),"[\xfe]",attr[attrcode()]);
   for (ControlObject *c=clist; c; c=c->next) // draw control children
      c->draw();
   mouse.show();
}

char ControlWindow::GoModal(char *saying) {
   EventQueue eq;
   Statusbar st;
   char *s=st.get();
   if (saying)
      st.say(saying);
   Cursor c;
   c.hide();
   ncode=ncNone;
   if (!focus) {// if focus not set => set it to the first control in the list
      SetFocus(clist);
      clist->GotFocus();
   }
   while (!ncode)
      ChewEvent(eq.wait());
   st.say(s);
   return ncode;
}

// ------------------ VisibleObject class implementation ----------------------

void VisibleObject::Save(void* &savebuf) {    // save rectangle and cursor
   if (!savebuf) {
      mouse.hide();
      savebuf = new short[(l()+1)*(h()+1)+1];
      short *saveb=(short*)savebuf;
      VGetText(x1(),y1(),l()+1,h()+1,(short*)saveb+1);
      mouse.show();
      Cursor c;
      saveb[0]=(c.x() << 8) + c.y();
   }
}

void VisibleObject::Restore(void *savebuf) { // restore rectangle and cursor
   if (!savebuf)
      return;
   mouse.hide();
   Cursor c;
   short *saveb=(short*)savebuf;
   c.moveto(saveb[0]>>8,saveb[0]&0xff);
   VPutText(x1(),y1(),l()+1,h()+1,saveb+1);
   delete savebuf;
   savebuf = NULL;
   mouse.show();
}

// ------------------ ControlObject class implementation ----------------------

BOOL ControlObject::ChewEvent(Event *e) { // handle focus movement keys
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      switch (k->scancode) {
      case S_LEFT:
         if (!focus)
            return FALSE;
         if (left) {
            LostFocus();
            left->GotFocus();
            if (parent)
               GetParent()->SetFocus(left);
            return TRUE;
         }
         return FALSE;
      case S_RIGHT:
         if (!focus)
            return FALSE;
         if (right) {
            LostFocus();
            right->GotFocus();
            if (parent)
               GetParent()->SetFocus(right);
            return TRUE;
         }
         return FALSE;
      case S_UP:
         if (!focus)
            return FALSE;
         if (up) {
            LostFocus();
            up->GotFocus();
            if (parent)
               GetParent()->SetFocus(up);
            return TRUE;
         }
         return FALSE;
      case S_DOWN:
         if (!focus)
            return FALSE;
         if (down) {
            LostFocus();
            down->GotFocus();
            if (parent)
               GetParent()->SetFocus(down);
            return TRUE;
         }
         return FALSE;
      case S_TAB: // both tab and shift-tab
         if (!focus)
            return FALSE;
         if (k->keycode==9) {// tab
            if (tab) {
               LostFocus();
               tab->GotFocus();
               if (parent)
                  GetParent()->SetFocus(tab);
               return TRUE;
            }
         } else { // shift-tab
            if (stab) {
               LostFocus();
               stab->GotFocus();
               if (parent)
                  GetParent()->SetFocus(stab);
               return TRUE;
            }
         }
         return FALSE;
      } // switch
   }
   return FALSE;
}

void ControlObject::GotFocus() {
   if (!focus && saying) {
      olds=statusbar.get();
      statusbar.say(saying);
   }
   focus=TRUE;
}

void ControlObject::LostFocus() {
   if (focus && saying && olds) {
      statusbar.say(olds);
   }
   focus=FALSE;
}


// ---------------------- Button class implementation -------------------------

Button::Button(char* s, char sel, char x1, char y1, char c, unsigned h,
   BOOL d, char *say, ControlWindow *p) :
   ControlObject(Rect(x1,y1,x1+strlen(s)+1,y1+1),say,p), name(s), selchar(sel),
   pressed(FALSE), ncode(c), hotkey(h), def(d) {
}

void Button::draw() {
   mouse.hide();
   char dx=0,dy=0; // relative coordinates offset
   if (parent) {
      dx = parent->cx1();
      dy = parent->cy1();
   }
   char na0=(char)(VGetch(x1()+dx,y1()+dy+1)>>8);
   char l=strlen(name);
   if (!pressed) {
      VPutsColor(x1()+dx,y1()+dy,name,focus?attr[attrButtonFocus]:
         def?attr[attrDefButton]:attr[attrButton]);
      if (selchar)
         VSetColor1(x1()+dx+selchar-1,y1()+dy,1,attr[attrButtonSelChar]);
      char cc=x1()+dx+1+l, dd=y1()+dy+1;
      for (char i=x1()+dx+1; i<cc; i++)  // draw shadow
         VPutchColor(i,dd,223,(na0&0xf0)+DARKGRAY);
      VPutchColor(cc-1,dd-1,220,(na0&0xf0)+DARKGRAY);
   } else { // draw pressed button
      VPutsColor(x1()+dx+1,y1()+dy,name,focus?attr[attrButtonFocus]:
         attr[attrButton]);
      char cc=x1()+dx+1+l, dd=y1()+dy+1;
      for (char i=x1()+dx+1; i<cc; i++)  // clear shadow
         VPutchColor(i,dd,' ',na0);
      VPutchColor(x1()+dx,y1()+dy,' ',na0);
   }
   mouse.show();
}

BOOL Button::ChewEvent(Event* e) {
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      if ((k->scancode==S_RET && (def || focus)) || k->code==hotkey) {
         Press(TRUE);   // Press the button
         delay(150);     // wait for a moment
         Press(FALSE);  // Release the button
         if (parent)
            GetParent()->Notify(ncode); // tell parent button was pressed
         return TRUE;
      } // if S_RET or hotkey
   } else {
      MouseEvent *m=(MouseEvent*)e;
      char dx=0,dy=0; // relative coordinates offset
      if (parent) {
         dx = parent->cx1();
         dy = parent->cy1();
      }
      if (m->type==evMouseDown && m->y==(y1()+dy) &&
         m->x<x2()+dx-1 && m->x>=x1()+dx) { // mouse clicked
         Press(TRUE);
         for(;;) { // capture mouse
            EventQueue eq;
            Event *e=eq.wait();
            if (e->type==evKeyPress) {
               if (pressed)
                  Press(FALSE);
               return TRUE;
            }
            m=(MouseEvent*)e;
            if (m->y==(y1()+dy) && m->x<x2()+dx-1 && m->x>=x1()+dx) {
               if (!pressed)
                  Press(TRUE);
               if (m->type==evMouseUp) {
                  Press(FALSE);
                  GetParent()->Notify(ncode);
                  return TRUE;
               }
            } else { // not above the button
               if (pressed)
                  Press(FALSE);
               if (m->type==evMouseUp) // mouse released ouside of button
                  return TRUE;
            }
         }
      }
   }
   return ControlObject::ChewEvent(e);
}

//--------------------- ScrollBar class implementation -----------------------

ScrollBar::ScrollBar(char x, char y, char l, char c, long mi, long ma,
   BOOL v, ControlWindow *p) : ncode(c), pmin(mi), pmax(ma), len(l),
   vert(v), ControlObject(v?Rect(x,y,x,y+l):Rect(x,y,x+l,y),NULL,p) {

   if (pmin>pmax) {  // exchange pmin and pmax if necessary
      long pp=pmin;
      pmin=pmax;
      pmax=pp;
   }
   pos=pmin;
   hl=0;
}

void ScrollBar::resize(char x, char y, char l,long mi, long ma) {
   len=l;
   pmin=mi;
   pmax=ma;
   rect=vert?Rect(x,y,x,y+l):Rect(x,y,x+l,y);
}

void ScrollBar::setpos(long p) {
   if (p<pmin)
      p=pmin;
   else if (p>pmax)
      p=pmax;
   pos=p;
   char sp=(float)(p-pmin+1)/(pmax-pmin+1)*(len-1)+0.5;
   if (p==pmin)
      sp=0;
   else if (sp==len-1 && p<pmax)
      sp--;
   if (sp!=hl) {
      hl=sp;
      draw();
   }
   if (parent)
      GetParent()->Notify(ncode);   // tell parent that handle was moved
}

void ScrollBar::draw() {
   mouse.hide();
   char dx=0,dy=0; // relative coordinates offset
   if (parent) {
      dx = parent->cx1();
      dy = parent->cy1();
   }
   char na0=attr[attrSBHandle];
   if (vert) { // vertical
      VSetChar1Ver(x1()+dx,y1()+dy,len,' ');
      VSetColor1Ver(x1()+dx,y1()+dy,len,attr[attrScrollBar]);
      VPutchColor(x1()+dx,y1()+hl+dy,9,na0);
      if (hl>0)
         VPutchColor(x1()+dx,y1()+hl-1+dy,30,na0);
      if (hl<len-1)
         VPutchColor(x1()+dx,y1()+hl+1+dy,31,na0);
   } else { // horizontal
      VSetColor1(x1()+dx,y1()+dy,len,attr[attrScrollBar]);
      VSetChar1(x1()+dx,y1()+dy,len,' ');
      VPutchColor(x1()+dx+hl,y1()+dy,9,na0);
      if (hl>0)
         VPutchColor(x1()+dx+hl-1,y1()+dy,17,na0);
      if (hl<len-1)
         VPutchColor(x1()+dx+hl+1,y1()+dy,16,na0);
   }
   mouse.show();
}

BOOL ScrollBar::ChewEvent(Event *e) {
   volatile long *time = (long*)0x46c; // current tick value
   if (e->type==evMouseDown) {
      MouseEvent *m=(MouseEvent*)e;
      char dx=0,dy=0; // relative coordinates offset
      if (parent) {
         dx = parent->cx1();
         dy = parent->cy1();
      }
      if ( (vert && m->x==x1()+dx && m->y==y1()+dy+hl) ||
          (!vert && m->y==y1()+dy && m->x==x1()+dx+hl)) { // on handle
      // capture mouse to move handle
         EventQueue eq;
         Event *e;
         do {
            e=eq.wait();
            if (e->type==evKeyPress) {
               setpos((float)(hl)/(len-1)*(pmax-pmin+1)+pmin);
               return TRUE;
            }
            m=(MouseEvent*)e;
            if (vert) {
               if (abs(m->x-x1()-dx)<2 && m->y>=y1()+dy && m->y<y1()+dy+len)
                  if (hl!=m->y-y1()-dy) {
                     hl=m->y-y1()-dy;
                     draw();
                  }
            } else
               if (abs(m->y-y1()-dy)<2 && m->x>=x1()+dx && m->x<x1()+dx+len)
                  if (hl!=m->x-x1()-dx) {
                     hl=m->x-x1()-dx;
                     draw();
                  }
         } while (m->type!=evMouseUp);
         setpos((float)(hl)/(len-1)*(pmax-pmin+1)+pmin);
         return TRUE;
      } else if ( (vert && m->x==x1()+dx && m->y==y1()+dy+hl+1) ||
                 (!vert && m->y==y1()+dy && m->x==x1()+dx+hl+1)) { // increment
      // increment until next event...
         EventQueue eq;
         Event *e;
         char hl0=hl;
         setpos(pos+1);
         if (hl!=hl0) {
            if (vert)
               mouse.moveto(x1()+dx,y1()+dy+hl+1);
            else
               mouse.moveto(x1()+dx+hl+1,y1()+dy);
            eq.get();   // get new mousemove event
         }
         long t0=*time+5;
         do {
            if (*time-t0>1) { // not too fast
               t0=*time;
               char hl0=hl;
               setpos(pos+1);
               if (hl!=hl0) {
                  if (vert)
                     mouse.moveto(x1()+dx,y1()+dy+hl+1);
                  else
                     mouse.moveto(x1()+dx+hl+1,y1()+dy);
                  eq.get();   // get new mousemove event
               }
            }
            e=eq.get();
         } while (e==NULL);
         return TRUE;
      } else if ( (vert && m->x==x1()+dx && m->y==y1()+dy+hl-1) ||
                 (!vert && m->y==y1()+dy && m->x==x1()+dx+hl-1)) { // decrement
      // decrement until next event...
         Event *e;
         EventQueue eq;
         char hl0=hl;
         setpos(pos-1);
         if (hl!=hl0) {
            if (vert)
               mouse.moveto(x1()+dx,y1()+dy+hl-1);
            else
               mouse.moveto(x1()+dx+hl-1,y1()+dy);
            eq.get();   // get new mousemove event
         }
         long t0=*time+5;
         do {
            if (*time-t0>1) { // not too fast
               t0=*time;
               char hl0=hl;
               setpos(pos-1);
               if (hl!=hl0) {
                  if (vert)
                     mouse.moveto(x1()+dx,y1()+dy+hl-1);
                  else
                     mouse.moveto(x1()+dx+hl-1,y1()+dy);
                  eq.get();   // get new mousemove event
               }
            }
            e=eq.get();
         } while (e==NULL);
         return TRUE;
      } else if ((vert && m->x==x1()+dx) || (!vert && m->y==y1()+dy)) {
         if ((vert && m->y>=y1()+dy && m->y<y1()+dy+hl) ||
            (!vert && m->x>=x1()+dx && m->x<x1()+dx+hl)) { // page down
            setpos(pos-(pmax-pmin+1)/(len));
            return TRUE;
         } else if ((vert && m->y<y1()+dy+len && m->y>y1()+dy+hl) ||
            (!vert && m->x<x1()+dx+len && m->x>x1()+dx+hl)) { // page up
            setpos(pos+(pmax-pmin+1)/(len));
            return TRUE;
         }
      }
   }
   return ControlObject::ChewEvent(e);
}

//--------------------- MessageBox class implementation -----------------------

MessageBox::MessageBox(char *title, char *txt, char *sbtext) :
      savebuf(NULL), text(txt),
      ControlWindow(Rect(14,8,65,14),title,F_DOUBLE,TRUE,TRUE) {
   if (display.yMax()>25) {
      rect.y1=display.yMax()/2-4;
      rect.y2=rect.y1+6;
   }
   char ll=max(strlen(txt),strlen(title));
   if (ll>l()) {
      char dx=(ll-l()+5)/2;
      rect.x1-=dx;
      rect.x2+=dx;
   }
   AddControl(new Button("  OK  ",0,(cl()-6)/2,3,ncOkButton,0,TRUE));
   draw();
   GoModal(sbtext);
   Restore(savebuf);
}

void MessageBox::draw() {
   Save(savebuf);
   mouse.hide();
   ControlWindow::draw();
   VPutsColorMid((x1()+x2())/2,y1()+2,text,attr[attrcode()]);
   mouse.show();
}

//--------------------- ListWindow class implementation -----------------------


ListWindow::ListWindow(Rect r, long n, char *t, char *f, BOOL s,BOOL cb,
      Window *p) : ControlWindow(r,t,f,s,cb,p), lno(n), lfirst(0),
      lcur(0), redraw(255) {
// rect, number of elements, title, frame, shadow, closebutton, parent
   AddControl(scb=new ScrollBar(cl(),0,ch(),ncVScrollBar,0,n-1,TRUE,this));
}

void ListWindow::resize(Rect r) {
   rect=r;
   scb->resize(cl(),0,ch(),0,lno-1);
}

ListWindow::~ListWindow() {
//   delete scb;
}

void ListWindow::moveto(long n) {    // choose n-th element
   if (n<0)
      n=0;
   else if (n>lno-1)
      n=lno-1;
   lcur=n;
   if (lcur<lfirst)
      lfirst=lcur;
   else if (lcur>=lfirst+ch())
      lfirst=lcur-ch()+1;
   scb->setpos(lcur);
   draw();
}

void ListWindow::down(long n) {    // down n times
   moveto(lcur+n);
}

void ListWindow::up(long n) {      // up n times
   moveto(lcur-n);
}

void ListWindow::pgdn() {            // page down
   lcur+=ch();
   if (lno>ch())
      lfirst+=ch();
   if (lfirst>=lno) {
      lfirst=lno-ch()-1;
      if (lcur>=lno)
         lcur=lno-1;
   }
   moveto(lcur);
}

void ListWindow::pgup() {            // page up
   lcur-=ch();
   if (lfirst-ch()<0) {
      lfirst=0;
      if (lcur<0)
         lcur=0;
   } else
      lfirst-=ch();
   moveto(lcur);
}

void ListWindow::draw() {
   mouse.hide();
   if (redraw) {
      ocur=ofirst=-100; // to be far enough from any possible value
      if (redraw==255)  // window redraw
         ControlWindow::draw();
      redraw=0;
   }
   long shift=ofirst-lfirst;  // if positive => scroll down
   if ((shift<0?-shift:shift)<ch() ) { // must scroll!
      if (ocur!=lcur) {
         if (ocur>=lfirst && ocur<lfirst+ch()) // ex-cur line is still visible
            // clear current-line attribute
            VSetColor1(cx1()+1,(char)(cy1()+ocur-ofirst),cl()-2,
               selected(ocur)?attr[attrListWinSel]:attr[attrListWin]);
         if (lcur>=ofirst && lcur<ofirst+ch()) // new cur line already visible
            // set current-line attribute
            VSetColor1(cx1()+1,(char)(cy1()+lcur-ofirst),cl()-2,
               selected(lcur)?attr[attrListWinCurSel]:attr[attrListWinCur]);
      }
      // now scroll window...
      if (shift>0)
         while (shift--) {
            VScr_d(x1(),y1(),cl(),ch()); // scrolling down
            long cur=--ofirst;
            VPutsColorLen(cx1()+1,cy1(),getline(cur),attr[
               selected(cur)?
                  (cur==lcur? attrListWinCurSel : attrListWinSel) :
                  (cur==lcur? attrListWinCur    : attrListWin)], cl()-2);
            VPutchColor(cx1(),cy1(),selected(cur)?16:' ',attr[
               selected(cur)? attrListWinSel : attrListWin]);
         }
      else
         while (shift++) {
            VScr_u(x1(),y1(),cl(),ch()); // scrolling up
            long cur=(ofirst++)+ch();
            VPutsColorLen(cx1()+1,y1()+ch(),getline(cur),attr[
               selected(cur)?
                  (cur==lcur? attrListWinCurSel : attrListWinSel) :
                  (cur==lcur? attrListWinCur    : attrListWin)], cl()-2);
            VPutchColor(cx1(),y1()+ch(),selected(cur)?16:' ',attr[
               selected(cur)? attrListWinSel : attrListWin]);
         }
   } else  // full redraw
      for (char c=0; c<ch(); c++) {
         long cur=lfirst+c;
         VPutsColorLen(cx1()+1,cy1()+c,(cur<lno?getline(cur):""),attr[
            (cur<lno&&selected(cur))?
               (cur==lcur? attrListWinCurSel : attrListWinSel) :
               (cur==lcur? attrListWinCur    : attrListWin)], cl()-2);
         VPutchColor(cx1(),cy1()+c,selected(cur)?16:' ',attr[
            selected(cur)? attrListWinSel : attrListWin]);
      }
   ocur=lcur;     // keep current values for using next time
   ofirst=lfirst;
   mouse.show();
}

BOOL ListWindow::ChewEvent(Event *e) {
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      if (!k->keycode)
         switch (k->scancode) {
            case S_END:
            case S_C_PGDN:
               moveto(lno-1);
               return TRUE;
            case S_HOME:
            case S_C_PGUP:
               moveto(0);
               return TRUE;
            case S_DOWN:
               down();
               return TRUE;
            case S_UP:
               up();
               return TRUE;
            case S_PGDN:
               pgdn();
               return TRUE;
            case S_PGUP:
               pgup();
               return TRUE;
         }
   } else {
      MouseEvent *m=(MouseEvent*)e;
      if ((m->type==evMouseDown) &&
          (m->x>x1() && m->x<x2() && m->y>y1() && m->y<y2())) {
         EventQueue eq;
         Event *e;
         BOOL ib=!selected(m->y-cy1()+lfirst);
         volatile long *time=(long*)0x46c;
         do {
            long t0=*time;
            do {
               e=eq.get();
            } while (!e && (*time-t0<1));
            if (e && e->type==evKeyPress)
               return TRUE;
            if (e)
               m=(MouseEvent*)e;
            if (m->x>x1() && m->x<x2())
               if (m->y>=cy1() && m->y<=cy2()) {
                  if (m->status==mbRight)
                     select(m->y-cy1()+lfirst,ib);
                  if (m->type==evMouseMove && m->status==mbRight)
                     if ((long)m->y-cy1()+lfirst-lcur>1) {
                        for (long l=lcur+1; l<m->y-cy1()+lfirst; l++)
                           select(l,ib);
                     } else if (lcur-((long)m->y-cy1()+lfirst)>1) {
                        for (long l=m->y-cy1()+lfirst+1; l<lcur; l++)
                           select(l,ib);
                     }
                  if (lcur!=m->y-cy1()+lfirst)
                     moveto(m->y-cy1()+lfirst);
               }
               else if (m->type==evMouseMove)
                  if (m->y>cy2()) { // scroll down
                     down();
                     if (m->status==mbRight)
                        select(lcur,ib);
                  } else { //scroll up
                     up();
                     if (m->status==mbRight)
                        select(lcur,ib);
                  }
         } while (m->type!=evMouseUp);
         return TRUE;
      }
   }
   return ControlWindow::ChewEvent(e);
}

void ListWindow::Notify(char code) {  // redefining Notify function
   if (code==ncVScrollBar) {
      // moveto(scb->getpos());
      lfirst+=scb->getpos()-lcur;
      lcur=scb->getpos();
      if (lfirst>lno-ch())
         lfirst=lno-ch();
      if (lfirst<0)
         lfirst=0;
      if (lcur<lfirst)
         lfirst=lcur;
      else if (lcur>=lfirst+ch())
         lfirst=lcur-ch()+1;
      draw();
   } else
      ControlWindow::Notify(code);
}

// ----------------------------- InputField -----------------------------------

InputField::InputField(char x, char y, char l, char *&s, InpType it,
         char *say, ControlWindow *p) : buf(s), len(l), itype(it), clear(TRUE),
         ControlObject(Rect(x,y,x+l+1,x),say,p) {
// x,y,l, ncode, inp. type, s, parent
// if s!=NULL => s must have enough space for l characters!
   if (!buf) {
      buf = new char[l+1];
      *buf=0;
      s=buf;
   }
}

void InputField::draw() {
   mouse.hide();
   char dx=0,dy=0; // relative coordinates offset
   if (parent) {
      dx = parent->cx1();
      dy = parent->cy1();
   }
   VPutsColorLen(x1()+dx,y1()+dy,buf,attr[attrInputBox],len);
   if (focus) {
      Cursor c;
      c.moveto(x1()+dx+pos,y1()+dy);
   }
   mouse.show();
}

BOOL InputField::ChewEvent(Event *e) {
   if (e->type==evKeyPress && focus) {
      KeyEvent *k=(KeyEvent*)e;
      char *s; // temporary variable
      if (k->keycode<32)
         switch (k->scancode) {
            case S_LEFT:
               if (pos>0)
                  pos--;
               draw();
               clear=FALSE;
               return TRUE;
            case S_RIGHT:
               if (pos<strlen(buf))
                  pos++;
               draw();
               clear=FALSE;
               return TRUE;
            case S_HOME:
               pos=0;
               draw();
               clear=FALSE;
               return TRUE;
            case S_END:
               pos=strlen(buf);
               draw();
               clear=FALSE;
               return TRUE;
            case S_DEL:
               if (clear) {
                  pos=0;
                  buf[0]=0;
                  clear=FALSE;
               } else if (pos<strlen(buf))
                  for (s=buf+pos; (*s=*(s+1))!=0; s++) ;
               draw();
               return TRUE;
            case S_BS:
               if (pos>0) {
                  for (s=buf+pos-1; (*s=*(s+1))!=0; s++) ;
                  pos--;
               }
               clear=FALSE;
               draw();
               return TRUE;
         }
      BOOL allowed=FALSE;
      switch (k->keycode) {
         case ' ':   // symbols, forbidden in filenames
         case '>':
         case '<':
         case ',':
         case ';':
         case '/':
            if (itype&it_filename)
               return FALSE;
         case '.':
         case 'e':
         case 'E':
         case '-':
         case '+':   // special symbols for floating numbers
            if (itype==it_int)
               return FALSE;
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':   // digits only
            allowed=TRUE;  // this character is allowed
         default:    // all the rest
            if (!allowed) { // need to check
               if (itype==it_int || itype==it_float)
                  break;
               if (k->keycode<' ')
                  break;
            }
            if (strlen(buf)>=len) {
               beep();
               return TRUE;
            }
         char c=(itype&it_caps)?upcase(k->keycode):k->keycode;
         if (clear) {
            clear=FALSE;
            pos=buf[0]=0;
         }
         for (s=buf+strlen(buf)+1; s>buf+pos; s--)
            *s=*(s-1);
         *s=c; // char inserted!
         pos++;
         draw();
         return TRUE;
      }
   } else if (e->type==evMouseDown) {
      MouseEvent *m=(MouseEvent*)e;
      char dx=0,dy=0; // relative coordinates offset
      if (parent) {
         dx = parent->cx1();
         dy = parent->cy1();
      }
      if (m->y==dy+y1() && m->x>=dx+x1() && m->x<=dx+x2()) {
         if (focus) {
            pos=m->x-dx-x1();
            if (pos>strlen(buf))
               pos=strlen(buf);
            draw();
            clear=FALSE;
         } else if (parent)
            GetParent()->MoveFocus(this);
         else
            focus=TRUE;
         return TRUE;
      }
   }
   return ControlObject::ChewEvent(e);
}

void InputField::GotFocus() {
   pos=0;
   ControlObject::GotFocus();
   Cursor c;
   xc=c.x();
   yc=c.y();
   draw();
}

void InputField::LostFocus() {
   pos=0;
   ControlObject::LostFocus();
   Cursor c;
   c.moveto(xc,yc);
}

// ----------------------------- PercentBar -----------------------------------

PercentBar::PercentBar(char x, char y, char l, long mi, long ma,
   ControlWindow *p) : pmin(mi), pmax(ma), len(l), pos(0), hl(0),
   ControlObject(Rect(x,y,x+l,y),NULL,p) {
}

void PercentBar::setpos(long p) {
   if (p<pmin)
      p=pmin;
   else if (p>pmax)
		p=pmax;
	if (pos==p)
		return;
	pos=p;
	char sp;
/*	if (pos==pmax)
		sp = len-1;
	else */
		sp=(float)(p-pmin+1)/(pmax-pmin+1)*(len-1);
	if (sp==0 && p>pmin)
		sp++;
	if (sp!=hl) {
		hl=sp;
		draw();
	}
}

void PercentBar::resize(char x, char y, char l, long mi, long ma) {
	len=l;
	pmin=mi;
	pmax=ma;
	rect=Rect(x,y,x+l,y);
}

void PercentBar::draw() {
	mouse.hide();
	char dx=0,dy=0; // relative coordinates offset
	if (parent) {
		dx = parent->cx1();
		dy = parent->cy1();
	}
	VSetColor1(x1()+dx,y1()+dy,hl+1,attr[attrPercent1]);
	if (len>hl+1)
		VSetColor1(x1()+dx+hl,y1()+dy,len-hl,attr[attrPercent2]);
	char s[5];
	sprintf(s,"%3d%%",(int)(((double)(pos-pmin+1)*100)/(pmax-pmin+1)));
	VPutsMid(x1()+dx+len/2,y1()+dy,s);
	mouse.show();
}

// ----------------------- pathes to VFSoft ----------------------------------

void far _Cdecl  VSetColor1(byte x, byte y, word length, byte attr) {
/* установить строку на экране длиной length, начиная с x, y в аттрибут attr */
	if (y>24)
		for (int i=0; i<length; i++)
         VSetColor(x+i,y,1,attr);
   else
      VSetColor(x,y,length,attr);
}

void far _Cdecl  VSetColor1Ver(byte x, byte y, word hight, byte attr) {
   if (y+hight>24)
      for (int i=0; i<hight; i++)
         VSetColor(x,y+i,1,attr);
   else
      VSetColorVer(x,y,hight,attr);
}

void far _Cdecl  VSetChar1(byte x, byte y, word length, char ch) {
  /* установить строку на экране длиной length, начиная с x, y в символ ch */
   if (y>24)
		for (int i=0; i<length; i++)
         VSetChar(x+i,y,1,ch);
   else
      VSetChar(x,y,length,ch);
}

void far cdecl VSetChar1Ver(byte x, byte y, byte hight, char ch) {
  /* установить столбец на экране высотой high, начиная с x, y в символ ch */
   if (y+hight>24)
      for (int i=0; i<hight; i++)
         VSetChar(x,y+i,1,ch);
   else
      VSetCharVer(x,y,hight,ch);
}

//------------------------- RadioButtonGroup -----------------------------

void RadioButtonGroup::draw() {
   mouse.hide();
   Cursor cur;
   int dx,dy;
   if (parent) {
      dx=GetParent()->cx1();
      dy=GetParent()->cy1();
   }
   if (focus) {
      if (cx<0) {
         cx=cur.x();
         cy=cur.y();
      }
      cur.moveto(buttons[bsel].x+dx,buttons[bsel].y+dy);
   } else if (!focus && cx>=0) {
      cur.moveto(cx,cy);
      cx=-1;
   }
   for (int i=0; i<bno; i++) {
      VPutch(buttons[i].x+dx-1,buttons[i].y+dy,'(');
      VPutch(buttons[i].x+dx,buttons[i].y+dy,(i==bsel?7:' '));
      VPutch(buttons[i].x+dx+1,buttons[i].y+dy,')');
      if (buttons[i].txt) {
         VPuts(buttons[i].tx+dx,buttons[i].ty+dy,buttons[i].txt);
      }
   }
   mouse.show();
}

BOOL RadioButtonGroup::ChewEvent(Event *e) {
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      if (focus && !k->keycode)
         switch (k->scancode) {
            case S_LEFT:
            case S_UP:
               if (--bsel<0)
                  bsel=bno-1;
               draw();
               return TRUE;
            case S_RIGHT:
            case S_DOWN:
               if (++bsel>=bno)
                  bsel=0;
               draw();
               return TRUE;
         }
   } else {
      MouseEvent *m=(MouseEvent*)e;
      int dx,dy;
      if (parent) {
         dx=GetParent()->cx1();
         dy=GetParent()->cy1();
      }
      if (m->type==evMouseDown)  {
         for (int i=0; i<bno; i++)
            if (m->x==buttons[i].x+dx && m->y==buttons[i].y+dy) {
               bsel=i;
               if (parent)
                  GetParent()->MoveFocus(this);
               draw();
               return TRUE;
            }
         for (int i=0; i<bno; i++)
            if (buttons[i].txt && m->x>=buttons[i].tx+dx &&
               m->y==buttons[i].ty+dy &&
               m->x<buttons[i].tx+dx+strlen(buttons[i].txt)) {
               bsel=i;
               if (parent)
                  GetParent()->MoveFocus(this);
               draw();
               return TRUE;
            }
      }
   }
   return ControlObject::ChewEvent(e);
}

