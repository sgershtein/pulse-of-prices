//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Input/output events header file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 30-Sep-95
//-----------------------------------------------------------------------------

#ifndef _IOEVENTS_H_
#define _IOEVENTS_H_
// only include the file once

#include <stddef.h>
#include <fstream.h>

typedef char BOOL;
#define TRUE 1
#define FALSE 0

// Double click interval in ticks (1/18th of second)
#define DblClkInterval 5

// Event types
#define evKeyPress 1
#define evMouseClick 10		// down + up on the same character
#define evMouseDblClick 11	// down + up + down + up within small time, same pos
#define evMouseMove 12
#define evMouseDown 13
#define evMouseUp 14

// display adapter constants (used by Mouse and Display classes)
#define adaEGA	14
#define adaVGA	16
#define adaOTHER 0
#define adaCHECK 255

// Mouse buttons masks
#define mbLeft	1
#define mbRight 2
#define mbMiddle 4

class Event { // Just an event
	friend class EventQueue;
	public:
		int type;
		Event() : next(NULL) {};
	protected:
		Event *next;	// pointer to the next event in the queue
};

class MouseEvent : public Event { // an event by mouse
	public:
		int x,y;	// x,y - mouse cursor coordinates when the event accured
		char buttons;		// which button was affected (pressed/released)
		char status;		// buttons status when event accured
};

class KeyEvent : public Event { // an event by keyboard
	public:
		char scancode;
		char keycode;
		unsigned short code;
		KeyEvent(char s,char k) : scancode(s), keycode(k),
				code((s<<8) + k) {
			type = evKeyPress;
		};
};

class Handler {	// Event handler class
	friend class EventQueue;
	public:
		virtual ~Handler() {}   // mechanism for virtual destructor calling
	protected:
		virtual Event* newevent() = 0;	// returns new event or NULL if no event
		Handler *next;
};

class EventQueue {	// event queue class
	public:
		EventQueue()  { exm++; }
		~EventQueue();
		void addhandler(Handler*);	// add handler to the list
      BOOL iscommand() { return command!=0; }
      char getcommand() {
         char c=command;
         command=0; // no command
         return c;
      }
      void setcommand(char c) {
         command = c;
      }
		Event* get();	  // get next event (or NULL), remove it from the queue
		Event* next();	  // get next event (or NULL) without removing it
		Event* wait();		// wait for next event
		void CheckEvents();	// checks for new events.
	protected:
      static char command;       // command to execute
		static Event *head, *tail;	// head and tail of the queue
		static Handler *handlers;	// list of all event handlers
      static char exm;           // current copy of the class
};

class Display {
	public:
		Display();
      void Done(char *); // CLear screen and say good-bye
		char Adapter() { return adapter; }	// returns adapter type
		char xMax() { return mx; }			// returns x-resolution
		char yMax() { return my; }			// returns y-resolution
		BOOL HiRes();	// set high text resolution, returns FALSE if error
		BOOL NormRes();	// set normal text resolution (80x25)
	private:
		static char adapter;	// adaEGA, adaVGA, adaOTHER
		static char mx,my;
		static char chkada();	// determine adapter type
};

class Mouse : public Handler {
	public:
      Mouse() {} ;
		Mouse(Display*); // takes pointer to an existing display class
		~Mouse();
		void Init();	// Reinitialize the mouse
		void Restore();	// Restore everything (e.g. before quitting)
		void Emulation(BOOL);  // Use mouse cursor emulation?
		void LeftHand(BOOL b) { // Left-Handed mouse
			exb = b;             // (exchanged left and right buttons)
		}
		void moveto(char x, char y);	// Move mouse cursor to (x,y)
		void show();	// show cursor (dec level of hideness)
		void hide();	// hide cursor (inc level of hideness)
		BOOL IsMouse() { return ismouse; }
		BOOL ThreeButtons() {return b3; }	// Is it 3-button mouse?
		BOOL IsEmul() { return grcur; }	// Is curr. cursor emulated?
		BOOL CanEmul() { return cangr; }	// Can we emulate cursor?
		BOOL IsLeftHand() { return exb; }	// Is it left-handed mouse?
      BOOL shown() { return !hidden; } // Is mouse shown
	protected:
		static Display *display;
		virtual Event* newevent();	// check for a new mouse event
	private:
		static char hidden;	// mouse cursor: 0 - visible, 1-... hidden
		static BOOL inited;	// Is mouse already initialized?
		static BOOL grcur;		// are we using graphical cursor?
		static BOOL cangr;		// can we use graphical cursor?
		static char x,y,lastx,lasty; // when last <> current => evMouseMove event
		static char status;	// button status
		static int grx,gry;	// graphical cursor coordinates (hot spot)
		static BOOL ismouse;	// is there a mouse driver installed?
		static char ydiv;		// number of scan lines per symbol
		static BOOL b3;			// Is this a 3-button mouse?
		static BOOL exb;		// left-handed mouse?
		static char cc[6];		// six characters affected by graphical cursor
		static char emc[16][6];	// patterns of symbols used for cursor emulation
      static BOOL firsttime; // is this the first instance of the class?
		BOOL iswin();	   // are we under Windows? [NOT YET IMPLEMENTED]
		void getxy();	   // get current coordinates
		void getMset();	// set character definitions access mode
		void getMcl();		// clear character definitions access mode
		void grUndraw();	// undraw the graphical cursor
		void grDraw();		// draw the graphical cursor
};

class Keyboard : public Handler {
	public:
		void Flush();
	protected:
		virtual Event* newevent();	// check for a new keyboard event
	private:
		BOOL getkey(char &scancode, char &keycode);	// get next key or NULL
};

class IniFile { // .ini file handler
   public:
      IniFile(char* path, char* name); // name without extention!
      ~IniFile() {
         if (is_open)
            fi.close();
      };
      BOOL GetInt(char *sec, char *opt, int &arg);
      BOOL GetDouble(char *sec, char *opt, double &arg);
      BOOL GetStr(char *sec, char *opt, char* arg, int bufsize);
      BOOL PutInt(char *sec, char *opt, int arg);
      BOOL PutDouble(char *sec, char *opt, double arg);
      BOOL PutStr(char *sec, char *opt, char* arg);
      BOOL Is_open() { return is_open; }
   private:
      BOOL is_open;
      ifstream fi;
      ofstream fo;
      char fname[127];
      BOOL findsec(char*);
      BOOL fwsec(char*);
      BOOL fwopt(char*);
      BOOL findopt(char*);
};

class IOHardErr { // DOS I/O hard error handler
   public:
      IOHardErr();   // register the handler
};

class CtrlBreak { // Ctrl=Break handler
  typedef void interrupt (*Handler) (...);

  public:
    CtrlBreak ();                        // install an error handler
    virtual ~CtrlBreak ();               // uninstall handler
    BOOL flag() { return cbFlag; }
    void clear() {
      cbFlag=FALSE;
    }
  protected:
    virtual void  controlBreak() {
      cbFlag=TRUE;
    }

  private:
    void*  operator new (size_t) {      // ... to prevent heap allocation
      return 0;
    }
    BOOL cbFlag;
    static void interrupt CBreak();
    CtrlBreak *oldInstance;
    Handler   oldCtrlBreak;
};


void beep();         // produce a short tone
void ClickSnd();     // clicking sound
char upcase(char);   // upcase a char (Russian & English cp 866)

// pathes to VFSoft library **********************************************
void far _Cdecl  VSetColor1  (unsigned char x, unsigned char y,
   unsigned int length, unsigned char attr);
void far _Cdecl  VSetColor1Ver(unsigned char x, unsigned char y,
   unsigned int hight, unsigned char attr);
void far _Cdecl  VSetChar1(unsigned char x, unsigned char y,
   unsigned int, char);
void far _Cdecl  VSetChar1Ver(unsigned char x, unsigned char y,
   unsigned char, char);
// ***********************************************************************

// now scan+key codes constants

#define A_ALT_C 0x2e00
#define A_ALT_D 0x2000
#define A_ALT_A 0x1e00
#define A_ALT_LT 0x3300
#define A_ALT_B 0x3000
#define A_ALT_C 0x2e00
#define A_ALT_Y 0x1500
#define A_ALT_X 0x2d00
#define A_ALT_F 0x2100
#define A_ALT_J 0x2400
#define A_ALT_P 0x1900
#define A_CTRL_I 0x1709
#define A_CTRL_F 0x2106
#define A_CTRL_S 0x1f13
#define A_CTRL_L 0x260c
#define A_CTRL_R 0x1312
#define A_SHIFT_TAB 0x0f00
#define A_LEFT  0x4B00
#define A_RIGHT 0x4D00
#define A_UP    0x4800
#define A_DOWN  0x5000
#define A_F1    0x3B00
#define A_F2    0x3C00
#define A_CTRL_F2 0x5f00
#define A_F3    0x3D00
#define A_F4    0x3E00
#define A_F5    0x3F00
#define A_F6    0x4000
#define A_F7    0x4100
#define A_F8    0x4200
#define A_F9    0x4300
#define A_F10   0x4400
#define A_F11   0x5700
#define A_F12   0x3400
#define A_ESC   0x011B
#define A_RET   0x1C0D
#define A_SPACE 0x3920
#define A_BS    0x0E08
#define A_DEL   0x5300
#define A_INS   0x5200
#define A_HOME  0x4700
#define A_END   0x4F00
#define A_PGUP  0x4900
#define A_PGDN  0x5100
#define A_1     0x0231
#define A_2     0x0332
#define A_3     0x0433
#define A_4     0x0534
#define A_5     0x0635
#define A_6     0x0736
#define A_7     0x0837
#define A_8     0x0938
#define A_9     0x0A39
#define A_0     0x0B30
#define A_MI    0x0C2D
#define A_PL    0x0D3D
#define A_SL    0x2B5C
#define A_TAB   0x0F09
#define A_C_PGUP 0x8400
#define A_C_PGDN 0x7600
#define A_GREY_MI	0x4A2D
#define A_GREY_PL	0x4E2B
#define A_GREY_MU 0x372A

// now key codes constants

#define K_RET 13
#define K_ESC 27

#endif
