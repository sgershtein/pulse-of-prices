//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Menu classes header file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 06-Oct-95
//-----------------------------------------------------------------------------

#include "screen.h"

#ifndef _MENU_H_
#define _MENU_H_

/*class Doer { // encapsulates function to be called when menu entry is chosen
   public:
      virtual void Do() = 0;
}; */

struct MenuStruct { // top menu structure format
   char *name;             // menu alternative
   char *status;           // status text
   char selch;             // Selection char in the name (0 is none)
//   Doer *doer;           // function to call when selected (doer->Do())
   BOOL disabled;          // is it disabled?
   BOOL checked;           // is it checked?
   unsigned hotkey;        // a hot key scan + key code
   char command;           // a command to execute
};

class Menu { // the very common base class
   public:
      Menu(char n, MenuStruct *ms) : no(n), entry(ms) {}
      virtual ~Menu() {}   // virtual destructor mechanism
      void enable(char n,BOOL b) {   // enable/disable a menu entry
         if (n<no)
            entry[n].disabled = b;
      }
   protected:
      char no;             // number of entries
      char cur;            // current selected entry;
      MenuStruct *entry;   // the entries
};

class PullDnMenu;

class TopMenu : public Menu, public ScreenObject {
   // top menu (first line on the screen)
   public:
      TopMenu(int n, MenuStruct *ms, PullDnMenu *pd[], char sp=5);
      virtual ~TopMenu();
      virtual void draw(); // draw the menu
      void Do();           // do the menu
      void Done();         // done working with menu
      void Right();        // go left
      void Left();         // go right
      BOOL ChewEvent(Event *e);  // if not chown return FALSE
      BOOL ToggleChecked(char cm); // toggle checked attribute, return new
      BOOL ToggleDisabled(char cm); // toggle disabled attribute, return new
      void SetChecked(BOOL, char cm);
      void SetDisabled(BOOL, char cm);
      BOOL IsChecked(char cm);
      BOOL IsDisabled(char cm);
      BOOL IsActive() { return cur!=255; }
   protected:
      char spacing;        // number of spaces between entries
      PullDnMenu **pdmenu; // pointers to submenus
      BOOL pdactive;       // is pull-down menu active?
      char xx[10];         // starts of entries
      char ll[10];         // lengths of entries
   private:
      BOOL HandleMouse(MouseEvent*); // returns TRUE if event was handled
      MenuStruct* FindCommand(char cm);
      char *oldstat;       // old status line
};

class PullDnMenu : public Menu, public Window { // Pull-down menu
   friend class TopMenu;
   public:
      PullDnMenu(char n, MenuStruct *ms);
      virtual ~PullDnMenu();
      virtual void draw(); // draw the menu
      void undraw();       // undraw the menu
      void SetPos(char x,char y); // set the rectangle
      void Down();
      void Up();
      char No() {
         return no;
      }
      void SetCur(char c) {
         cur = c;
      }
      void DoEntry();
      BOOL ChewKey(KeyEvent *k);  // if not chown return FALSE
   protected:
      void *savebuf;    // buffer for saving screen
};

#endif