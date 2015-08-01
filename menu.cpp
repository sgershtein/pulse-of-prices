//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Menu classes implementation file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 06-Oct-95
//-----------------------------------------------------------------------------

#include "menu.h"
#include <string.h>

// ---------------------- TopMenu class implementation ------------------------

TopMenu::TopMenu(int n, MenuStruct *ms, PullDnMenu *pd[], char sp) :
      Menu(n,ms),
      pdmenu(pd),
      spacing(sp) {
   cur = 255;
   oldstat=NULL;
   pdactive = FALSE;
   for (char i=0; i<no; i++) {
      xx[i]=(i==0)?spacing : xx[i-1]+spacing+ll[i-1];
      ll[i]=strlen(entry[i].name);
      pdmenu[i]->SetPos(xx[i],1);
   }
}

TopMenu::~TopMenu() {

}

void TopMenu::draw() { // draw the menu
   mouse.hide();
//   VPutsColorLen (0,0,"",attr[attrMenu],80);
   for (char i=0; i<no; i++) {
      int x=(i==0?0:xx[i-1]+ll[i-1]);
      VPutsColorLen(x,0,"",attr[attrMenu],xx[i]-x);
      VPutsColor(xx[i],0,entry[i].name,(i==cur)?attr[attrMenuSel]:attr[attrMenu]);
      if (cur!=i)
         VSetColor(xx[i]+entry[i].selch-1,0,1,attr[attrMenuChar]);
   }
   VPutsColorLen(xx[no-1]+ll[no-1],0,"",attr[attrMenu],80-xx[no-1]-ll[no-1]);
   if (!pdactive || pdmenu[cur]->cur==255) {
      Statusbar s;
      if (cur!=255)
/*         s.clear();
      else */
         s.say(entry[cur].status);
   }
   if (pdactive && cur<255)
      pdmenu[cur]->draw();  // draw pull-down menu
   mouse.show();
}

void TopMenu::Do() { // do the menu
   EventQueue eq;
   if (cur==255) {
      Statusbar s;
      oldstat=s.get();
      cur=0;
      pdactive=FALSE;
   }
   draw();
   while (cur!=255) {
      Event *e=eq.wait();
      if (!ChewEvent(e) && e->type==evKeyPress)
         beep();
      delete e;
   }
}

void TopMenu::Right() {       // go left
   if (pdactive)
      pdmenu[cur]->undraw();
   cur=++cur%no;
   if (pdactive)
      pdmenu[cur]->SetCur(0);
   draw();
}

void TopMenu::Left() {       // go right
   if (pdactive)
      pdmenu[cur]->undraw();
   cur=(cur+no-1)%no;
   if (pdactive)
      pdmenu[cur]->SetCur(0);
   draw();
}

BOOL TopMenu::ChewEvent(Event *e) {  // if not chown return FALSE
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      {
         for (char c=0; c<no; c++)  // checking for submenu hot keys
            if (pdmenu[c]->ChewKey(k)) {
               Done();
               pdmenu[c]->DoEntry();
               return TRUE;
            }
      }
      if (cur==255) {
         if (k->scancode==S_F10) {
            Do();
            return TRUE;
         }
         for (char i=0; i<no; i++) // check for hot keys of top menu (ALt+...)
            if (k->code==entry[i].hotkey) {
               cur=i;
               pdactive=TRUE;
               Statusbar st;
               oldstat=st.get();
               pdmenu[cur]->SetCur(0);
               Do();
               return TRUE;
            }
      } else  // already in menu
         switch (k->scancode) {
         case S_LEFT:
            Left();
            return TRUE;
         case S_RIGHT:
            Right();
            return TRUE;
         case S_DOWN:
            if (pdactive)
               pdmenu[cur]->Down();
            else {
               pdactive = TRUE;
               pdmenu[cur]->SetCur(0);
               draw();
            }
            return TRUE;
         case S_UP:
            if (pdactive) {
               pdmenu[cur]->Up();
               return TRUE;
            } else
               return FALSE;
         case S_RET:
            if (pdactive) {
               char cc=cur;
               Done();
               pdmenu[cc]->DoEntry(); // do the entry;
            } else {
               pdactive = TRUE;
               pdmenu[cur]->SetCur(0);
               draw();
            }
            return TRUE;
         case S_ESC:
            if (pdactive) {
               pdmenu[cur]->undraw();
               pdactive=FALSE;
            } else
               Done();
            return TRUE;
         }
      char i;
      if (cur!=255) {
         if (!pdactive) { // checking for top menu selection chars
            for (i=0; i<no; i++)
               if (entry[i].selch &&
                  (upcase(entry[i].name[entry[i].selch-1])==
                     upcase(k->keycode))) {
                  cur=i;
                  pdactive=TRUE;
                  pdmenu[cur]->SetCur(0);
                  draw();
                  return TRUE;
               }
         } else   // checking current pull-down menu for selection chars
            for (i=0; i<pdmenu[cur]->no; i++)
               if (pdmenu[cur]->entry[i].selch &&
                  (upcase(pdmenu[cur]->entry[i].name[
                     pdmenu[cur]->entry[i].selch-1])==
                        upcase(k->keycode))) {
                  pdmenu[cur]->SetCur(i);
                  char cc=cur;
                  Done();
                  pdmenu[cc]->DoEntry();
                  return TRUE;
               }
      }
   } else { // this is a mouse event
      MouseEvent *m=(MouseEvent*)e;
      if (m->type==evMouseDown && m->buttons==mbLeft) {
         if (HandleMouse(m)) {
/*            if (pdactive) {
               char cc=cur;
               Done();
               pdmenu[cc]->DoEntry();
            }                         */
            return TRUE;
         } else
            return (cur!=255); //FALSE; // event not handled
      }
      return (cur!=255); //FALSE;
   }
   return (cur!=255); //FALSE;
}

void TopMenu::Done() {        // done working with menu
   if (pdactive)
      pdmenu[cur]->undraw();
   pdactive=FALSE;
   cur=255;
   draw();
   if (oldstat) {
      Statusbar s;
      s.say(oldstat);
      oldstat=NULL;
   }
}

BOOL TopMenu::HandleMouse(MouseEvent *m) { // incoming event is evMouseDown!
//   if (m->y!=0)
//      return FALSE; // event not handled
   EventQueue eq;
   if (cur==255) {
      Statusbar s;
		oldstat=s.get();
	}
   do {  // waiting for evMouseUp event
      if (m->y==0) {
         for (char i=0; i<no; i++)
            if (m->x>=xx[i]-spacing/2 && m->x<xx[i]+ll[i]+spacing/2) {
               if (cur!=i || !pdactive || pdmenu[cur]->cur!=255 ||
                  m->type==evMouseUp) {
                  if (pdactive && cur!=i)
                     pdmenu[cur]->undraw();
                  else
                     pdactive=TRUE;
                  cur=i;
                  if (m->type==evMouseUp)
                     pdmenu[cur]->SetCur(0);
                  else
                     pdmenu[cur]->SetCur(255);
                  draw();
               }
               break;
            } else if (i==no-1 && m->type==evMouseDown)
               return FALSE; // event not handled
      } else if (pdactive) {
         if (m->x>pdmenu[cur]->x1() && m->x<pdmenu[cur]->x2() &&
               m->y>pdmenu[cur]->y1() && m->y<pdmenu[cur]->y2()) {
            if (pdmenu[cur]->cur!=m->y-pdmenu[cur]->y1()-1) {
               pdmenu[cur]->SetCur(m->y-pdmenu[cur]->y1()-1);
               pdmenu[cur]->draw();
            }
				if (m->type==evMouseUp && m->buttons==mbLeft) {
					if (eq.next()->type==evMouseClick)
						eq.get();	// throw out click event
					if (!pdmenu[cur]->entry[pdmenu[cur]->cur].disabled) {
						char cc=cur;
						Done();
						pdmenu[cc]->DoEntry();
						return TRUE;
					} else {
						pdmenu[cur]->SetCur(0);
						pdmenu[cur]->draw();
						return TRUE;
					}
				}
			} else {// not in the menu
				pdmenu[cur]->SetCur(255);
//            pdmenu[cur]->draw();
				draw();
				if (m->type==evMouseUp && m->buttons==mbLeft) {
					Done();
					if (eq.next()->type==evMouseClick)
						eq.get();	// throw out click event
					return TRUE;
				}
			}
		} else if (m->type==evMouseDown) {// not the first line and not pdactive
			Done();
			return FALSE;  // event not handled
		}
		if (m->type==evMouseUp && m->buttons==mbLeft) {
			if (eq.next()->type==evMouseClick)
				eq.get();	// throw out click event
			return TRUE; // finally (s)he released the button!
		}
		Event *e=NULL;
		do {
			if (e)
				delete e;
			e=eq.wait();
			if (e->type==evKeyPress) {
				delete e;
				return FALSE;
			}
		} while (e->type!=evMouseUp && e->type!=evMouseMove);
		m=(MouseEvent*)e;
	} while (TRUE);
}

BOOL TopMenu::ToggleChecked(char cm) { // toggle checked attribute
	MenuStruct *ms=FindCommand(cm);
	return (ms->checked=!ms->checked);
}

BOOL TopMenu::ToggleDisabled(char cm) { // toggle disabled attribute
   MenuStruct *ms=FindCommand(cm);
   if (ms)
      return (ms->disabled=!ms->disabled);
   return FALSE;

}

void TopMenu::SetChecked(BOOL b, char cm) {
   MenuStruct *ms=FindCommand(cm);
   if (ms)
      ms->checked=b;
}

void TopMenu::SetDisabled(BOOL b, char cm) {
   MenuStruct *ms=FindCommand(cm);
   if (ms)
      ms->disabled=b;
}

BOOL TopMenu::IsChecked(char cm) {
   MenuStruct *ms=FindCommand(cm);
   return ms?ms->checked:FALSE;
}

BOOL TopMenu::IsDisabled(char cm) {
   MenuStruct *ms=FindCommand(cm);
   return ms?ms->disabled:FALSE;
}

MenuStruct* TopMenu::FindCommand(char cm) {
   for (char i=0; i<no; i++)
      for (char j=0; j<pdmenu[i]->no; j++)
         if (pdmenu[i]->entry[j].command==cm)
            return &pdmenu[i]->entry[j];
   return NULL;
}

// -------------------- PullDownMenu class implementation ---------------------

PullDnMenu::PullDnMenu(char n, MenuStruct *ms) :
   Menu(n,ms), Window(Rect(0,0,0,0),"",F_SINGLE,TRUE), savebuf(NULL) {
   cur = 255;
}

PullDnMenu::~PullDnMenu() {
   if (savebuf)
      delete savebuf;
}

void PullDnMenu::draw() { // draw the menu
   mouse.hide();
   if (!savebuf) {
      savebuf = new short[(l()+1)*(h()+1)+1];
      short *saveb=(short*)savebuf;
      VGetText(x1(),y1(),l()+1,h()+1,saveb+1);
      Cursor c;
      saveb[0]=(c.x()<<8)+c.y();
      c.hide();
   }
//   Window::draw();
//   VClrFrame(x1(),y1(),cl(),ch(),' ',attr[attrMenu]);
   if (frame)  // draw the frame
      VFrame(x1(),y1(),cl(),ch(),frame,attr[attrMenu]);
   if (shadow)
      VTen(x1(),y1(),cl(),ch());
   for (char i=0; i<no; i++) {
      if (entry[i].name[0]==0) {// delimiter
         VSetColor(cx1(),cy1()+i,cl(),attr[attrMenu]);
         VSetChar(cx1(),cy1()+i,cl(),196);
         VPutch(cx1()-1,cy1()+i,195);
         VPutch(cx2()+1,cy1()+i,180);
         entry[i].disabled=TRUE;
         continue;
      }
      VPutsColorLen(cx1()+1,cy1()+i,entry[i].name,
         entry[i].disabled?
         ((i==cur)?attr[attrMenuSelDisab]:attr[attrMenuDisab]):
         ((i==cur)?attr[attrMenuSel]:attr[attrMenu]),cl()-1);
      if (cur!=i && !entry[i].disabled)
         VSetColor(cx1()+entry[i].selch,cy1()+i,1,attr[attrMenuChar]);
      VPutchColor(cx1(),cy1()+i,entry[i].checked?4:' ',
         (i==cur)?attr[attrMenuSel]:attr[attrMenu]);
   }
   Statusbar s;
   if (cur!=255)
      s.say(entry[cur].status);
   mouse.show();
}

void PullDnMenu::undraw() {       // undraw the menu
   if (!savebuf)
      return;
   mouse.hide();
   short *saveb=(short*)savebuf;
   Cursor c;
   c.moveto(saveb[0]>>8,saveb[0]&0xff);
   VPutText(x1(),y1(),l()+1,h()+1,saveb+1);
   delete savebuf;
   savebuf = NULL;
   mouse.show();
}

BOOL PullDnMenu::ChewKey(KeyEvent *k) {  // if not chown return FALSE
   // if it's a hot key => just set cur properly and nothing more!
   for (char i=0; i<no; i++)
      if (k->code==entry[i].hotkey && !entry[i].disabled) {
         cur=i;
         return TRUE;
      }
   return FALSE;
}

void PullDnMenu::SetPos(char x,char y) { // set the rectangle
   rect.x1 = x-1;
   rect.y1 = y;
   rect.y2 = y+no+1;
   rect.x2 = 0;
   int j;
   for (char i=0; i<no; i++)
      if ((j=strlen(entry[i].name))>rect.x2)
         rect.x2 = j;
   rect.x2+=x+2;
   if (rect.x2>=79) {
      rect.x1-=rect.x2-78;
      rect.x2=78;
   }
}

void PullDnMenu::DoEntry() {
   if (cur==255 || entry[cur].disabled)
      return;
   EventQueue eq;
   eq.setcommand(entry[cur].command);
}

void PullDnMenu::Down() {
   while (entry[cur=++cur%no].name[0]==0);
   draw();
}

void PullDnMenu::Up() {
   while (entry[cur=(cur+no-1)%no].name[0]==0);
   draw();
}

