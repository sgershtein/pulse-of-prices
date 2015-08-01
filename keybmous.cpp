//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Keyboard and Mouse Events routines
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 30-Sep-95
//-----------------------------------------------------------------------------

#include "ioevents.h"
#include <conio.h>
#include <bios.h>
#include <dos.h>
#include <string.h>

// ----------------------- EventQueue class implementation --------------------

Event *EventQueue::head=NULL,
      *EventQueue::tail=NULL;	               // head and tail of the queue
Handler *EventQueue::handlers;	            // list of all event handlers
char EventQueue::exm=0;                      // current copy of the class
char EventQueue::command=0;                  // command to execute

EventQueue::~EventQueue() {
   if (--exm>0)
      return; // not the last copy yet
	while (head) {  // delete the event queue
		Event *e=head->next;
		delete head;
		head = e;
	}
	while (handlers) {
		Handler *h = handlers->next;
		delete handlers;
		handlers = h;
	}
}

void EventQueue::addhandler(Handler* h) {
	h->next=handlers;
	handlers=h;
}

Event* EventQueue::get() {
	CheckEvents();
	Event *e=head;
	head = head? head->next : NULL;
	if (!head)
		tail=NULL;
	return e;
}

Event* EventQueue::next() { // return next event without removing it 
	CheckEvents();
	return head;
}

Event* EventQueue::wait() {
	while (!head)
		CheckEvents();
	return get();
}

void EventQueue::CheckEvents() {
	Event *e;
	for (Handler *h=handlers; h; h=h->next)
		while ((e=h->newevent())!=NULL) {
			// adding the event to our queue
			if (tail) {
				if (tail->type==evMouseMove && e->type==evMouseMove) {
					// replacing old mouse move event with the new one
					*tail = *e;
					delete e;
				} else { // adding event to the queue's tail
					tail->next=e;
					tail = e;
				}
			} else
				tail = head = e;
		}
}

// ----------------------- Keyboard class implementation ----------------------

void Keyboard::Flush() {
	char c;
	while (getkey(c,c)) ;
}

BOOL Keyboard::getkey(char &scancode, char &keycode) { // get next key or NULL
	if (!_bios_keybrd(_KEYBRD_READY))
		return FALSE;	// No key was pressed
	unsigned short key = _bios_keybrd(_KEYBRD_READ); // Fetch the key
	scancode = key >> 8;
	keycode = key & 0xff;
	return TRUE;	// a key was pressed and fetched
}

Event* Keyboard::newevent() { // check for a new keyboard event
	char key,scan;
	if (!getkey(scan,key))
		return NULL;
	KeyEvent *e = new KeyEvent(scan,key);
	return e;
}

// ----------------------- Mouse class implementation -------------------------

// Local coorditates of graphical cursor hot point (5,1)
#define grxHot 5
#define gryHot 1

// Symbols used for mouse cursor emulatign
const char pat[6] = {208,215,216,202,203,207};

Display *Mouse::display=NULL;
BOOL Mouse::firsttime = TRUE;
char Mouse::hidden;	   // mouse cursor: 0 - visible, 1-... hidden
BOOL Mouse::inited = FALSE; // Is mouse already initialized?
BOOL Mouse::grcur;		// are we using graphical cursor?
BOOL Mouse::cangr;		// can we use graphical cursor?
char Mouse::x,Mouse::y,
     Mouse::lastx,Mouse::lasty; // when last <> current => evMouseMove event
char Mouse::status;	   // button status
int  Mouse::grx,Mouse::gry; // graphical cursor coordinates (hot spot)
BOOL Mouse::ismouse=FALSE;	// is there a mouse driver installed?
char Mouse::ydiv;		   // number of scan lines per symbol
BOOL Mouse::b3;			// Is this a 3-button mouse?
BOOL Mouse::exb;		   // left-handed mouse?
char Mouse::cc[6];		// six characters affected by graphical cursor
char Mouse::emc[16][6];	// patterns of symbols used for cursor emulation

Mouse::Mouse(Display *d) {
   if (!firsttime)
   	return;
   hidden = 0;
   inited = FALSE;
   status = 0;
   exb = FALSE;
   firsttime = FALSE;
	display = d;
	BOOL mouse=FALSE;
	int bbx=0;	// for bx value returned by mouse initialization call
	asm {	// Checking if mouse driver is present
		  cmp     word ptr ds:[33h*4],0 // Testing if mouse int handler exists
		  jnz    ismou
		  cmp    word ptr ds:[33h*4+2],0
		  jz     nomou
ismou:
		  xor 	ax,ax
		  int    33h             	// With AX=0h - Initialization
		  cmp    ax,-1           	// Was mouse initialization correct?
		  jnz    nomou            	// No => no correct mouse driver
		  mov		mouse,TRUE			// Mouse is present
		  mov		bbx,bx
nomou:
	}
	ismouse = mouse;
	b3 = (bbx==3 && mouse);
	grcur = cangr = (display->Adapter()!=adaOTHER) && !iswin() &&
         (display->yMax()<30);
	ydiv = display->Adapter()==adaVGA? 16 : (cangr? 14 : 0);
	lastx = lasty = -1;
	grx = gry = 0;
}

Mouse::~Mouse() {
// DON'T DO ANYTHING, SINCE IT'S JUST AN INSTANCE!
//	Restore();
}

void Mouse::Restore() {
	if (!inited)
		return;
	if (grcur) {
		grUndraw();
		// here we restore the original pattern of pat[] symbols
		getMset();	// Switching screen to special mode
		short int p32[6];
		for (int i=0; i<6; i++)
			p32[i]=pat[i]<<5;
		for (int j=0; j<ydiv; j++)
			for (int i=0; i<6; i++) {
				short aa=p32[i]++;
				char c=emc[j][i];
				asm {
					push 	es
					mov   ax,0a000h
					push	ax
					pop	es
					mov	di,aa
					mov	al,c
					mov	ah,byte ptr es:[di]
					mov	byte ptr es:[di],al
					pop	es
				}
			 }
		getMcl();	// Switching screen back to normal mode
		// original pattern restored
	}
	inited = FALSE;
}

void Mouse::Emulation(BOOL b) {	// toggle cursor emulation on/off
	if (b==grcur || (b && !cangr)) return;
	BOOL bb=inited;
	Restore();
	grcur=b;
	if (bb)    // If it was inited already - re-init it.
		Init();
}

void Mouse::Init() {
	if (!ismouse) return;
	if (inited)
		Restore();
	asm {	// Initialize mouse
		xor	ax,ax
		int	33h
	}
	hidden = 0;
	if (grcur && display->yMax()>25)
		grcur = FALSE; // can't make it on HiRes
	if (!grcur)
		asm { // show built-in cursor
			mov	ax,01h
			int	33h
		}
	else {
		// Here we preserve the original pattern of pat[] symbols
		getMset();	// Switching screen into special mode
			// loading the pattern into emc
		short int p32[6];
		for (int i=0; i<6; i++)
			p32[i]=pat[i]<<5;
		for (int j=0; j<ydiv; j++)
			for (int i=0; i<6; i++) {
				short aa=p32[i]++;
				char c;
				asm {
					push 	es
					mov   ax,0a000h
					push	ax
					pop	es
					mov	di,aa
					mov	ah,byte ptr es:[di]
					mov	c,ah
					pop	es
				}
				emc[j][i]=c;
			 }
			// Pattern loaded.
		getMcl();	// Switching screen back to normal mode
		grDraw();
	}
	inited = TRUE;
}

void Mouse::hide() {
	if (!inited) return;
	if (grcur)
		grUndraw();
	hidden++;	// potential bug if hidden==255! Consider it impossible. [SG]
	asm {	// code to hide mouse
		mov	ax,02h
		int	33h
	}
}

void Mouse::show() {
	if (!inited) return;
	if (hidden) {
		hidden--;
		if (grcur)
			grDraw(); // draws the cursor only if necessary
		asm {	// code to show the mouse
			mov	ax,01h
			int	33h
		}
	}
}

void Mouse::moveto(char xx, char yy) {	// move mouse cursor
	if (!inited || !ismouse) return;
	if (x>display->xMax() || y>display->yMax())
		return;
	if (grcur) {
		grUndraw();	// undraw graphical cursor (if shown)
		grx=xx*8+4;
		gry=yy*ydiv+ydiv/2;
		grDraw();	// draw graphical cursor (if not hidden)
      unsigned x = grx;
      unsigned y = gry/ydiv*8;
      asm {	// tell mouse driver that cursor has been moved
         mov	cx,x
         mov	dx,y
         mov   ax,4
         int   33h		// Mouse cursor to <cx>:<dx>
      }
	} else {
		unsigned x0=xx*8,y0=yy*8;
		asm {
			mov	ax,04h
			mov	cx,x0
			mov	dx,y0
			int	33h
		}
	}
}

BOOL Mouse::iswin() {	// are we under Windows?
	return FALSE; //  TEMPORARILY!!!! NEEDS TO BE CHECKED!!!!
}

void Mouse::getxy() {	// get current coordinates
	if (grcur) {
		x = grx / 8;
		y = gry / ydiv;
	} else {
		unsigned xx,yy;
		asm {
			mov	ax,03h
			int	33h		// Mouse position in CX,DX
			mov	xx,cx
			mov	yy,dx
		}
		x = xx/8;
		y = yy/8;
	}
}

Event* Mouse::newevent() {	// check for a new mouse event
	if (!inited || !ismouse) return NULL;
	static volatile const unsigned long *time=
		(volatile const unsigned long *)0x46c;
	static char lx=0,ly=0;	// coordinates of last button event
	static char b=0;     	// last pressed/released button
	static int ltype=0;		// last event type
	static char clk=0;	// which click is this? down - 0, up - 1, down - 2...
	static BOOL cdone=TRUE;	// was click event generated?
	static long tt=0;		// time of the first button down in the serie
	// Do we need to generate Click or DblClick events?
	if (grcur) { // calculating new mouse position and redisplaying
		short x,y;
		asm {
			mov 	ax,0bh
			int  	33h 		// Mouse displacement
			mov	x,cx
			mov	y,dx
		}
		if (x || y) {
			grUndraw();
			if ((grx+=x)>=((display->xMax()+1)*8))
				grx=display->xMax()*8+7;
			else if (grx<0)
				grx=0;
			if ((gry+=y)>=((display->yMax()+1)*ydiv))
				gry=(display->yMax()+1)*ydiv-1;
			else if (gry<0)
				gry=0;
			grDraw();
			x = grx;
			y = gry/ydiv*8;
			asm {	// tell mouse driver that cursor has been moved
				mov	cx,x
				mov	dx,y
				mov   ax,4
				int   33h		// Mouse cursor to <cx>:<dx>
			}
		}
	}
	if (clk==1 && !cdone) {	// click must be generated!
		MouseEvent *m=new MouseEvent;
		m->x=lx;
		m->y=ly;
		m->buttons=b;
		m->status=status;
		m->type=evMouseClick;
		cdone=TRUE;
		return m;
	}
	if (clk && (*time-tt>DblClkInterval))
		clk-=2;
	if (clk==3 && !cdone) {
		MouseEvent *m=new MouseEvent;
		m->x=lx;
		m->y=ly;
		m->buttons=b;
		m->status=status;
		m->type=evMouseDblClick;
		cdone=TRUE;
		clk=lx=ly=b=ltype=0;
		return m;
	}
	char stt;	// buttons status
	int xx,yy;	// mouse position
	unsigned int t;	// number if times the even accured
	for (int bb=0; bb<=(b3?2:1); bb++)  // bb - button number
		for (int aa=5; aa<=6; aa++) {// 5 - 'buttons downs', 6 - 'buttons ups'
			asm {
				mov	ax,aa		// 05h or 06h
				mov	bx,bb		// left/right/middle button (1/2/3)
				int	33h
				mov	xx,cx
				mov	yy,dx
				mov	t,bx
				mov	stt,al
			}
			if (t) {	// >0 times
				MouseEvent *m=new MouseEvent;
				m->x=xx/8;
				m->y=yy/8;
				if (exb)	// bb:= 2 -> 2, 1 -> 0, 0 -> 1  - Left Handed Mouse
					bb = (bb==0)? 1: (bb==1)? 0 : 2;
				m->buttons = 1 << bb;     		// which button pressed/released
				m->status = stt;					// current status
				status = stt;
				m->type = (aa==5)? evMouseDown : evMouseUp;
				if (m->x==lx && m->y==ly && b==m->buttons && m->type!=ltype) {
					if (clk || m->type==evMouseUp) {
						if (!clk++)
							tt=*time;
						cdone = FALSE;
						ltype = m->type;
					}
				} else {	// this is somewhere else
					clk=lx=ly=b=ltype=0;
					if (m->type==evMouseDown) {
						lx = m->x;
						ly = m->y;
						b = m->buttons;
						ltype = evMouseDown;
					}
				}
				lastx = x = m->x;
				lasty = y = m->y;
				return m;
			}
		}
	lastx=x;
	lasty=y;
	getxy();
	if (x!=lastx || y!=lasty) { // mouse has been moved!
		clk=lx=ly=b=ltype=0;	// since it moved => click shouldn't be generated
		MouseEvent *m=new MouseEvent;
		m->x=x;
		m->y=y;
		m->buttons=0;
		m->status=status;
		m->type=evMouseMove;
		return m;
	}
	return NULL;
}

void Mouse::getMset() {	// set character definitions access mode
	unsigned short seqparms1[4]={0x300,0x402,0x704,0x300};
	unsigned short gcparms1[3]={0x204,0x5,0x6};
	int i;
	for (i=0; i<4; i++) {
		short a=seqparms1[i];
		asm {
			mov	dx,03c4h
			mov	ax,a
			out	dx,ax
		}
	}
	for (i=0; i<3; i++) {
		short a=gcparms1[i];
		asm {
			mov	dx,03ceh
			mov	ax,a
			out	dx,ax
		}
	}
}

void Mouse::getMcl() {		// clear character definitions access mode
	unsigned short seqparms2[4]={0x300,0x302,0x304,0x300};
	unsigned short gcparms2[3]={0x004,0x1005,0x0e06};
	int i;
	for (i=0; i<4; i++) {
		short a=seqparms2[i];
		asm {
			mov	dx,03c4h
			mov	ax,a
			out	dx,ax
		}
	}
	for (i=0; i<3; i++) {
		short a=gcparms2[i];
		asm {
			mov	dx,03ceh
			mov	ax,a
			out	dx,ax
		}
	}
}

void Mouse::grUndraw() {	// undraw the graphical cursor
	if (!grcur || hidden)
		return;
	short x = (grx-grxHot)/8, y = (gry-gryHot)/ydiv; // text pattern coordinates
	if (grx<grxHot)
		x=-1;
	if (gry<gryHot)
		y=-1;
	// restore the pattern ---------------------------------
	char *scr = (char*)(0xb8000000L + y*160 + x*2);
	if ((y*160+x*2)<0)	// this neutralizes some strange effects, I don't know
		scr+=16; 			// why it works, but it does work. [sg]
	if (y>=0) {
		if (x>=0)
			scr[0] = cc[0];
		if (x<79) {
			scr[2] = cc[1];
			if (x<78)
				scr[4] = cc[2];
		}
	}
	if (y<24) {
		if (x>=0)
			scr[160] = cc[3];
		if (x<79) {
			scr[162] = cc[4];
			if (x<78)
				scr[164] = cc[5];
		}
	}
}

void Mouse::grDraw() {		// draw the graphical cursor
	if (!grcur || hidden)
		return;
	short x = (grx-grxHot)/8, y = (gry-gryHot)/ydiv; // text pattern coordinates
	if (grx<grxHot)
		x=-1;
	if (gry<gryHot)
		y=-1;
	// save the pattern ---------------------------------
	char *scr = (char*)(0xb8000000L + y*160 + x*2);
	if ((y*160+x*2)<0)	// this neutralizes some strange effects, I don't know
		scr+=16;				// why it works, but it does work. [sg]
	if (y>=0) {
		if (x>=0)
			cc[0] = scr[0];
		if (x<79) {
			cc[1] = scr[2];
			if (x<78)
				cc[2] = scr[4];
		}
	}
	if (y<24) {
		if (x>=0)
			cc[3] = scr[160];
		if (x<79) {
			cc[4] = scr[162];
			if (x<78)
				cc[5] = scr[164];
		}
	}
	// ------------------------------- the pattern saved
	// create the new characters ------------------------
	unsigned short orpic[14]={	// `or' picure of the hand mouse cursor
		0x0000,	//   	  0000 0000 0000 0000b  ................
		0x0400,	//      0000 0100 0000 0000b  .....*..........
		0x0c00,	//      0000 1100 0000 0000b  ....**..........
		0x0c00,	//      0000 1100 0000 0000b  ....**..........
		0x0c90,	//      0000 1100 1001 0000b  ....**..*..*....
		0x0db2,	//      0000 1101 1011 0010b  ....**.**.**..*.
		0x2db6,	//      0010 1101 1011 0110b  ..*.**.**.**.**.
		0x6db6,	//      0110 1101 1011 0110b  .**.**.**.**.**.
		0x6ffe,	//      0110 1111 1111 1110b  .**.***********.
		0x7ffe,	//      0111 1111 1111 1110b  .**************.
		0x3ffc,	//      0011 1111 1111 1100b  ..************..
		0x3ffc,	//      0011 1111 1111 1100b  ..************..
		0x0ff0,	//      0000 1111 1111 0000b  ....********....
		0x0000};	//      0000 0000 0000 0000b  ................
	unsigned short andpic[14]={ // `and' picure of the hand mouse cursor
		0xf1ff,	//      1111 0001 1111 1111b
		0xe5ff,	//      1110 0101 1111 1111b
		0xedff,	//      1110 1101 1111 1111b
		0xec6f,	//      1110 1100 0110 1111b
		0xec94,	//      1110 1100 1001 0100b
		0x0db2,	//      0000 1101 1011 0010b
		0x2db6,	//      0010 1101 1011 0110b
		0x6db6,	//      0110 1101 1011 0110b
		0x6ffe,	//      0110 1111 1111 1110b
		0x7ffe,	//      0111 1111 1111 1110b
		0xbffd,	//      1011 1111 1111 1101b
		0xbffd,	//      1011 1111 1111 1101b
		0xcff3,	//      1100 1111 1111 0011b
		0xf00f};	//      1111 0000 0000 1111b
	char pic[16][6];  // place for creating the new characters
	getMset();	// Switching screen into special mode
		// loading the old pattern into pic
	short int p32[6];
	for (int i=0; i<6; i++)
		p32[i]=cc[i]<<5;
	for (int j=0; j<ydiv; j++)
		for (int i=0; i<6; i++) {
			short aa=p32[i]++;
			char c;
			asm {
				push 	es
				mov   ax,0a000h
				push	ax
				pop	es
				mov	di,aa
				mov	ah,byte ptr es:[di]
				mov	c,ah
				pop	es
			}
			pic[j][i]=c;
		 }
		// old patter loaded.  Now creating new one.
	int dy=(gry-gryHot+ydiv)%ydiv;  // first scan line affected
	int dx=(grx-grxHot+8)%8;	// delta x (how much to shift left)
	for (int line=0; line<14; line++) {	// for each line of the mask
		unsigned short orm =(orpic[line] >> dx), // shifting pictures by dx
			andm=(andpic[line] >> dx) | (~((1<<(16-dx))-1));  // to the right
			asm {	// swap bytes in orm and andm
				mov	ax,orm
				xchg	ah,al
				mov	orm,ax
				mov	ax,andm
				xchg	ah,al
				mov	andm,ax
			}
		int ldy=line+dy;
		unsigned short *ups = (unsigned short*)(&pic[ldy%ydiv][ldy<ydiv?0:3]);
			// ups - pointer to the two first characters in pic together.
			// vertical lines 0..ydiv-1 belong to chars 0..2; tre rest - to 3..5
		*ups &= andm;
		*ups |= orm;
		char *cps = &pic[ldy%ydiv][ldy<ydiv?2:5]; // pointer to the last char
		*cps &= (char)(((andpic[line]<<(8-dx))|((1<<(8-dx))-1)) & 0xff);
		*cps |= (char)((orpic[line]<<(8-dx)) & 0xff);
	}
		// New pattern seems to be created. Well, let's output it there.
	for (int i=0; i<6; i++)
		p32[i]=pat[i]<<5;
	for (int j=0; j<ydiv; j++)
		for (int i=0; i<6; i++) {
			short aa=p32[i]++;
			char c=pic[j][i];
			asm {
				push 	es
				mov   ax,0a000h
				push	ax
				pop	es
				mov	di,aa
				mov	al,c
				mov	ah,byte ptr es:[di]
				mov	byte ptr es:[di],al
				pop	es
			}
		 }
	getMcl();	// Switching screen back to normal mode
	// -------------------------- new characters created
	// output the new pattern ---------------------------
	if (y>=0) {
		if (x>=0)
			scr[0] = pat[0];
		if (x<79) {
			scr[2] = pat[1];
			if (x<78)
				scr[4] = pat[2];
		}
	}
	if (y<24) {
		if (x>=0)
			scr[160] = pat[3];
		if (x<79) {
			scr[162] = pat[4];
			if (x<78)
				scr[164] = pat[5];
		}
	}
// --------------------------- the new pattern shown
}

// ---------------------------- IniFile Class ---------------------------------

IniFile::IniFile(char* path, char* name) { // name without extention!
	char *q;
	for (q=fname; (*q++=*path++)!=0;) ; // copying the string
	q--;
   if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
      *q++='\\';
      *q=0;
   }
   strcat(fname,name); // adding the file name
   strcat(fname,".ini"); // adding the extention
   fi.open(fname);
   is_open = FALSE;
   if (!fi)
      return; // file does not exists
   is_open = TRUE;
}

BOOL IniFile::findsec(char *sec) {
   if (!is_open)
      return FALSE;
   fi.seekg(0);
   fi.clear();
   char buf[50];
   do {
      fi.getline(buf,50);
      if (fi.eof())
         return FALSE;
   } while (buf[0]!='[' || strstr(buf,sec)!=buf+1 ||
         buf[strlen(sec)+1]!=']');
   return TRUE;   // section found!
}

BOOL IniFile::findopt(char *opt) {
   fi.clear();
   for(;;) {
      char *c=opt;
      char ch;
      do {
         fi.get(ch);
         if (fi.fail())
            return FALSE;
         if (*c==0 && ch=='=')
            return TRUE; // found! Equal sign already eaten!
         if (c==opt && ch=='[')
            return FALSE; // next section encountered
      } while (ch==*c++);
      fi.ignore(1000,'\n'); // skip to the next line
      if (fi.eof())
         return FALSE;
   }
}

BOOL IniFile::fwsec(char *sec) {
   if (!is_open)
      return FALSE;
   fi.seekg(0);
   fi.clear();
   char buf[50];
   do {
      fi.getline(buf,50);
      fo << buf << endl;
      if (fi.eof())
         return FALSE;
   } while (buf[0]=='[' && strstr(buf,sec)==buf+1 &&
         buf[strlen(sec)+2]==']');
   return TRUE;   // section found!
}

BOOL IniFile::fwopt(char *opt) {
   fi.clear();
   for(;;) {
      char *c=opt;
      char ch;
      do {
         fi.get(ch);
         if (fi.fail())
            return FALSE;
         if (c==opt && ch=='[') {
            fi.putback(ch);
            return FALSE; // next section encountered
         }
         fo.put(ch);
         if (*c==0 && ch=='=')
            return TRUE; // found! Equal sign already eaten!
      } while (ch==*c++);
      fi.ignore(1000,'\n'); // skip to the next line
      if (fi.eof())
         return FALSE;
   }
}

BOOL IniFile::GetInt(char *sec, char *opt, int &arg) {
   if (!findsec(sec) || !findopt(opt))
      return FALSE;
   fi >> arg;
   return !(fi.fail());
}

BOOL IniFile::GetDouble(char *sec, char *opt, double &arg) {
   if (!findsec(sec) || !findopt(opt))
      return FALSE;
   fi >> arg;
   return !(fi.fail());
}

BOOL IniFile::GetStr(char *sec, char *opt, char* arg, int bufsize) {
   if (!findsec(sec) || !findopt(opt))
      return FALSE;
   fi.get(arg,bufsize);
   return !(fi.fail());
}

BOOL IniFile::PutInt(char *sec, char *opt, int arg) {
   char fn[127];
   strcpy(fn,fname);
   fn[strlen(fname)-1]='!'; // temp file
   fo.open(fn,ios::out);
   if (!fo)
      return FALSE;
   if (!findsec(sec))
      fo << '[' << sec << ']' << endl;
   else if (!findopt(opt))
      fo << opt << '=';
   fo << arg;
   findsec("\1"); // just to copy all the file
   fo.close();
   return TRUE;
}

BOOL IniFile::PutDouble(char *sec, char *opt, double arg) {
   char fn[127];
   strcpy(fn,fname);
   fn[strlen(fname)-1]='!'; // temp file
   fo.open(fn,ios::out);
   if (!fo)
      return FALSE;
   if (!findsec(sec))
      fo << '[' << sec << ']' << endl;
   else if (!findopt(opt))
      fo << opt << '=';
   fo << arg;
   findsec("\1"); // just to copy all the file
   fo.close();
   return TRUE;
}

BOOL IniFile::PutStr(char *sec, char *opt, char* arg) {
   char fn[127];
   strcpy(fn,fname);
   fn[strlen(fname)-1]='!'; // temp file
   fo.open(fn,ios::out);
   if (!fo)
      return FALSE;
   if (!findsec(sec))
      fo << '[' << sec << ']' << endl;
   else if (!findopt(opt))
      fo << opt << '=';
   fo << arg;
   findsec("\1"); // just to copy all the file
   fo.close();
   return TRUE;
}

// ------------------------ Hard I/O error handler ----------------------------

#define IGNORE  0
#define RETRY   1
#define ABORT   2
#define FAIL    3

void my_io_handler(unsigned int ax, unsigned int, unsigned int*) {
   if (ax & 0X80) { // device error
      beep(); beep(); beep();
      _hardretn(ABORT);
   }
 /* otherwise it was a disk error */
   if (ax & 0x0800) // FAIL answer is allowed
      _hardresume(FAIL);
   else
      _hardresume(IGNORE);
}

IOHardErr::IOHardErr() {   // register the handler
   _harderr(my_io_handler);
}

// -------------------------- Ctrl-Break Handler ------------------------------

static CtrlBreak *instance = NULL;  // current CtrlBreak instance


void interrupt CtrlBreak::CBreak () {
//      Control-break interrupt handler.
//      This calls the current instance's "controlBreak" member function.
   instance->controlBreak();
}


CtrlBreak::CtrlBreak () {
   if (!instance)
      clear();
   else
      cbFlag=instance->cbFlag;
   oldInstance = instance;
   instance = this;
   oldCtrlBreak = getvect (0x23);
   setvect (0x23, Handler (&CtrlBreak::CBreak));
}

CtrlBreak::~CtrlBreak () {
   setvect (0x23, oldCtrlBreak);
   instance = oldInstance;
}


// --------------------------- simple functions -------------------------------

void beep() {
   sound(2000);
   delay(100);
   nosound();
}

void ClickSnd() {
   sound(100);
   delay(5);
   nosound();
   delay(10);
   sound(70);
   delay(5);
   nosound();
}

char upcase(char c) {
   if ((c>=97 && c<=122) ||   // English
      (c>=160 && c<=175))     // first Russian half
      return (c&~32);
   if (c>=224 && c<=239)      // second Russian half
      return (c-80);
   return c;
}

