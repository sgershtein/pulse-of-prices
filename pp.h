//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// main include file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 11-Oct-95
//-----------------------------------------------------------------------------

#include "ioevents.h"	// mouse and keyboard events
#include "screen.h"     // screen routines and classes
#include "menu.h"
#include "bases.h"
#include "varray.h"

#define VERSION "1.11"
#define INTERNAL 0   // 1 - not for distribution outside of Ural-Relcom

//#define NOKEYTHROW

/******************* menu commands definitions ******************************/

enum MENUCOMMANDS {cmNone=0,cmBaseInfo,cmExit,cmDemand,cmSupply,cmFirms,
	cmSelect,cmSelectAll,cmDeselectAll,cmSelReverse,cmDelSel,cmSaveCur,cmStat,
	cmShowBrief,cmEditFilter,cmLoadFilter,cmSaveFilter,cmDirSetup,cmUserSetup,
	cmAbout,cmRelcom,cmCurrency,cmRoublePrice,cmSaveOptions,cmNoSort,
	cmSortPrices,cmSortPricesDesc,cmApplyFilter,cmFind,cmFindNext,
	/* ----- now not real menu, but action commands ----- */
	caNone=127,
	caCloseDataWin,         // close data window
	caOpenBaseWin           // close groups window and open base window
	};

/****************************************************************************/

extern PPI ppi;
extern PPD ppd;
extern PPG ppg;
extern PPF ppf;

class AboutBox : public ControlWindow {
	public:
		AboutBox(BOOL b=FALSE);
		virtual void draw();
		virtual BOOL ChewEvent(Event *);
	private:
		BOOL timeout;  // clear window after timeout?
		void *savebuf; // for saving screen
		char *oldstat; // for old status line message
		virtual char attrcode() {
			return attrMesBox;
		}
};

class RelcomInfo : public ControlWindow {
	public:
		RelcomInfo();
		virtual void draw();
	private:
		void *savebuf; // for saving screen
		char *oldstat; // for old status line message
		virtual char attrcode() {
			return attrMesBox;
		}
};

class BaseInfoBox : public ControlWindow { // display info about the bases
	public:
		BaseInfoBox();
		virtual void draw();
	private:
		void *savebuf; // for saving screen
		char *oldstat; // for old status bar message
		virtual char attrcode() {
			return attrMesBox;
		}
};

class KeyCheck { // check copy protection key
	public:
//		KeyCheck(char *dir=NULL) { if (dir) keypath=dir; }
//		long DecodeKey(char key[5]);	// decode key to CIC[4]
		long getCIC();				// returns 4 bytes of CIC
		void FindKey();			// finds a key or throws 1.0 if not found
		void check() {
#ifndef NOKEYTHROW
//			if (getCIC()!=DecodeKey(key)) throw 1.1;
#endif
		}
		void printCIC();			// print this computer's CIC on stdout
		BOOL warnCIC();			// warn about CIC on screen. TRUE is warned
	private:
//		static char* keypath;
//		static char key[5];
};

class Setup { // read/store setup info
	public:
		enum Code {all=0,grMouseCur,UserPal,HiRes,LeftHandMouse,ShowBrief,
			RoublePrice,TempPath,ExpiryDays,DataPath,FOverWrite,FShortForm,
			BChar, FilterPath, EasyExit,
			LAST};
		static int easyexit; // 1 - allow exit by ESC, otherwise only Alt-X
		static int frewrite;
		static int fshort;
      static int grmousecur;
      static int userpal;
      static int hires;
      static int lefthandmouse;
      static int showbrief;
      static int roubleprice;
      static int expirydays;
		static int bchar;    // background character
      static char temppath[70];
      static char filterpath[70];
      static char datapath[70];
      static long freemem;               // initial free memory in bytes
      Setup(char *dir) { inipath=dir; }; // dir - .ini file directory
      Setup() {};
      BOOL read(Code code);   // read setup
      BOOL save(Code code);   // save setup
	private:
		static char *inipath;

};

class DataWindow : public ListWindow { // base class for the Data Window
   public:
		enum WinType {base,firms,groups,ads};
      // TopMenu&, rect, number of elements, title, frame, shadow,
      // closebutton, parent
      DataWindow(TopMenu *mt, Rect r, long n, char *t=NULL, char *f=NULL,
            BOOL s=FALSE, BOOL cb=FALSE, Window *p=NULL) : nsel(0),
            mtop(mt), ListWindow(r,n,t,f,s,cb,p) {
         savesb=statusbar.get();
      }
      virtual ~DataWindow() {
         statusbar.say(savesb);
      }
      virtual BOOL valid()=0;              // returns FALSE in case of error
      virtual BOOL ChewMenuCommand(char);  // return TRUE if command chown
      virtual void draw();
      virtual which() {                   // which window is this?
         return base;
      }
      virtual void idlefunc() {}          // to be called for idle processing
   protected:
      Statusbar statusbar;
      char *savesb;                       // save status bar string
      TopMenu *mtop;
      long nsel;                          // number of selected items;
};

class GroupWindow : public DataWindow {    // groups selection window
   public:
      GroupWindow(TopMenu *mt,BOOL sup);
      virtual ~GroupWindow();
      virtual BOOL selected(long n);      // is n-th line selected?
      virtual void select(long, BOOL);    // select or deselect n-th line
      virtual BOOL ChewEvent(Event*);     // should handle <Enter> key
      virtual BOOL ChewMenuCommand(char); // return TRUE if command chown
      virtual which() {                   // which window is this?
         return groups;
      }
      virtual BOOL valid() {              // returns FALSE in case of error
         return grpname!=NULL;
      }
      virtual void draw();                // draw the window
      BOOL issupply() { return supply; }
      long Gsize(long l) {
         if (gsize)
            return gsize[(int)l];
         else
            return -1;
      }
      long Gfirst(long l) {
			if (gfirst)
            return gfirst[(int)l];
         else
            return -1;
      }
      int Gno(long l) {
         if (gno)
            return gno[(int)l];
         else
            return -1;
      }
      char* CurName() {
         return grpname?grpname[(int)lcur]:NULL;
      }
   protected:
      virtual char* getline(long n);      // return n-th line
   private:
      char tpos;                          // "typing" position (speed search)
      char **grpname;                     // group names
      BOOL *sel;                          // is the group selected
      long *gsize;                        // group sizes
      long *gfirst;                       // first record in group no.
      int *gno;                           // original group number
      BOOL supply;                        // supply or demand?
      BOOL ReadData(long&, int&); // FALSE if failed, long - number of groups,
                                  // int - max length of group string
};

class FirmWindow : public DataWindow {    // firms selection window
   public:
      FirmWindow(TopMenu *mt);
      virtual ~FirmWindow();
      virtual BOOL selected(long n);      // is n-th line selected?
      virtual void select(long, BOOL);    // select or deselect n-th line
      virtual BOOL ChewEvent(Event*);     // should handle <Enter> key
      virtual BOOL ChewMenuCommand(char); // return TRUE if command chown
      virtual which() {                   // which window is this?
         return firms;
      }
      virtual BOOL valid() {              // returns FALSE in case of error
         return sel!=NULL;
      }
      virtual void draw();                // draw the window
      virtual void idlefunc() {           // to be called for idle processing
         if (--drinfo==1)
            draw();
      }
   protected:
      virtual char* getline(long n);      // return n-th line
   private:
      long oldcur;                        // used by draw() function
      int drinfo;                        // used by draw() function
      char tpos;                          // "typing" position (speed search)
      unsigned *sel;                      // firms selection array
};

class FilterWindow;  // forward definition

class BaseWindow : public DataWindow {    // groups selection window
   public:
      enum SortOrder { srtUnknown=1,srtNatural,
         srtAscendPrices, srtDescendPrices };
      BaseWindow(TopMenu *mt,GroupWindow *gw,FilterWindow *filt);
      BaseWindow(TopMenu *mt,FirmWindow *gwm,FilterWindow *filt);
      virtual ~BaseWindow();
      virtual BOOL selected(long n);      // is n-th line selected?
      virtual void select(long, BOOL);    // select or deselect n-th line
      virtual BOOL ChewEvent(Event*);     // should handle <Enter> key
      virtual BOOL ChewMenuCommand(char); // return TRUE if command chown
      virtual which() {                   // which window is this?
         return base;
      }
      virtual BOOL valid() {              // returns FALSE in case of error
         return is_valid;
      }
      virtual void draw();                // draw the window
      virtual void idlefunc();            // to be called for idle processing
   protected:
      virtual char* getline(long n);      // return n-th line
      inline void setbool(long,BOOL);     // functions for operating
      inline void setsel(long,BOOL);      //  bitwise tables bool and sel
      inline BOOL getbool(long);          //  (both in "data" array)
      inline BOOL getsel(long);
      BOOL getfullinfo(long);             // reads full info into ppf and ppd
   private:
      BOOL roubleprice;                   // Show all prices in roubles
      SortOrder srtorder;                 // current base sort order
      long oldcur;                        // used by draw() function
      BOOL showbrief;                     // Full/brief info
      BOOL applyfilter;                   // apply filter?
      BOOL sbtoggled;                     // showbrief was just toggled
      char mytitle[50];                   // to hold the title
      VArrayL *data;                      // the data
      long pbool,                         // positions of bool and
           psel,                          // selection bit tables in "data"
           pidx;                          // position of index table in "data"
      BOOL is_valid;                      // was ititialization ok?
      BOOL supply;                        // supply or demand?
      long iready;                        // number of index elements ready
      long icur;                          // current index in bool bit array
      long *rlast;                        // last record for each paym-t type
      long *gfirst;                       // first and last records
      long *glast;                        // for each payment type
      float *rprice;                      // last record price for each paym
      long pcdone;                        // to show percentage done
      int lastpc;                         // last percentage shown
      BOOL breverse;                      // shall we simply reverse the order?
      char tempname[130];                 // to hold the name of temp file
      char *findstr;                      // string to find
      char *savename;                     // filename to save selection
      BOOL ReadData(long n=1);            // read n index elements
      long OpenFiles();   // returns max number of data records, 0 if failure
      FilterWindow *filter;
      BOOL filtering;                     // are we filtering now?
      BOOL frewrite;                      // wnen saving - overwrite old file?
      BOOL fshort;                        // save short info only?
      long gfound;                        // rec no. for which group name found
};

class SetRates : public ListWindow {
   public:
      SetRates(Window *p=NULL); // only parent
      virtual BOOL ChewEvent(Event*);  // return TRUE if event chown
      virtual void draw();
      virtual BOOL selected(long);
      virtual void select(long, BOOL) {};    // select or deselect n-th line
   protected:
      Statusbar statusbar;
      virtual char* getline(long n);      // return n-th line
      void GetRate(int);                  // input the rate from user
   private:
      void *savebuf;                      // for saving screen underneath
      char s[65]; // for temporary use
      virtual char attrcode() {
         return attrCurRates;
      }
};

class Rates {
   public:
      Rates();
      BOOL ReadRates();
      BOOL isvalid() { return valid; }
      float operator[](int i) {   // get i-th rate
         return valid?rates[i]:0;
      }
      void setuser(int i, float f) { // set user rate for i
         if (!valid || i<0 || i>=npaym)
            return;
         rates[i]=f;
         user[i]=TRUE;
      }
      void clearuser(int i);  // clear user rate for i
      BOOL iscustom(int i) {
         return valid?user[i]:FALSE;
      }
      static int npaym;    // number of payment methods
   private:
      static BOOL valid;   // are the rates valid
      static float *rates; // the rates.
      static BOOL *user;   // are these user rates?
};

//------------------------ Filters stuff -------------------------------------

class OrCondition { // filter or-condition base class
   friend class AndCondition;
   friend class Filter;
   public:
      enum CTYPE {
         equal,      // ==
         nequal,     // !=
         contains,
         ncontains,
         less,       // <
         greater,    // >
         nless,      // >=
         ngreater,   // <=
         unknown=255
      };
      CTYPE ctype;
      virtual ~OrCondition() {}  // virtual destructor mechanizm
      OrCondition() : next(NULL) {}
      virtual BOOL operator()(char *, int, float) = 0;
      // check if given record passes thru the condition.  TRUE is yes.
      OrCondition *next;
};

class FilFirm : public OrCondition {
   public:
      FilFirm(CTYPE c, char* s) : str(s), bf(NULL) { ctype=c; lct=unknown; }
      ~FilFirm() {
         if (str) delete str;
         if (bf) delete bf;
      }
      virtual BOOL operator()(char *, int fcode, float);
      // check if given record passes thru the condition.  TRUE is yes.
      char* getstr() { return str; }
      void setstr(char *s) { str=s; lct=unknown; }
   private:
      char *str;
      CTYPE lct;  // if lct==ctype => can do spead search
      unsigned *bf;  // firm array (2 bits for firm)
};

class FilAd : public OrCondition {
   public:
      FilAd(CTYPE c, char* s) : str(s) { ctype=c; }
      ~FilAd() { if (str) delete str; }
      virtual BOOL operator()(char *ad, int, float);
      // check if given record passes thru the condition.  TRUE is yes.
      char* getstr() { return str; }
      void setstr(char *s) { str=s; }
   private:
      char *str;

};

class FilPrice : public OrCondition {
   public:
      FilPrice(CTYPE c, float val) : value(val) { ctype=c; }
      virtual BOOL operator()(char *, int, float price);
      // check if given record passes thru the condition.  TRUE is yes.
      float getvalue() { return value; }
      void setvalue(float f) { value=f; }
   private:
      float value;
};

class AndCondition { // filter and-condition
   friend class Filter;
   public:
      BOOL isand;    // TRUE - and, FALSE - and (for conditions in the list!!)
      enum filtype { filFirm=1, filPrice=2, filAd=0
      }; // on which field is this condition
      filtype field;
      AndCondition(filtype ft) : head(NULL), field(ft), next(NULL), ncond(0),
         isand(FALSE) {}
      ~AndCondition();
      BOOL operator()(char *ad, int fcode, float price);
      // check if given record passes thru the condition.  TRUE is yes.
      filtype Field() { return field; }
      void insert(OrCondition *here, OrCondition *cond);
      // insert new condition before *here into the list
      void remove(OrCondition *cond);
      // remove this condition from the list
      OrCondition *head;
      AndCondition *next;
      int ncond; // number of OrCondition's in the list
};

class Filter { // filter for records
   public:
      BOOL isand;    // TRUE - and, FALSE - and (for conditions in the list!!)
      Filter() : head(NULL), isand(TRUE), ncond(0) {}
      ~Filter();
      BOOL operator()(char *ad, int fcode, float price);
      // check if given record passes thru the filter.  TRUE is yes.
      void insert(AndCondition *here, AndCondition *cond);
      // insert new condition before *here into the list
      void remove(AndCondition *cond);
      // remove this condition from the list and delete it!
   protected:
      AndCondition *head;
      int ncond; // number of conditions in the list
      char filname[31];
};

// ----------------------- end of filters stuff ------------------------------

class FilterWindow : public ListWindow, public Filter {
   public:
      FilterWindow();
      ~FilterWindow();
      virtual void draw();
      virtual BOOL selected(long n) { return (n==0); }// is n-th line selected?
      virtual void select(long, BOOL) {}  // select or deselect n-th line
      virtual BOOL ChewEvent(Event*);
      void edit();                        // edit the filter
      BOOL isempty() { return head==NULL; }// is the filter empty?
      BOOL operator[](long ofs);
      // check if given data ofs passes through filter
      BOOL write();              // save filter to disk
      void manage();             // manage saved filters
   protected:
      void countlno();                    // count number of lines in filter
      void insand(AndCondition *here);    // ask user and insert and-condition
      void chor(OrCondition *here,
         AndCondition::filtype type);     // modify given or-condition
      void insor(OrCondition *here,
         AndCondition *ac);               // ask user and insert or-condition
      enum ACTION {
         none=0, ins, del, ret
      };
      virtual char* getline(long n);      // get n-th line
      BOOL doedit(int n,ACTION act);      // do the action with given line
      BOOL read(unsigned ofs);   // read filter from disk from given ofs
      BOOL erase(unsigned ofs); // remove from file filter on given ofs
   private:
      Button *bnew;                       // new filter button
      virtual char attrcode() {
         return attrListWin;
      }
      void FileName(char*);               // returns filter file name
      BOOL chkad;                         // are there ad conditions?
      BOOL chkfirm;                       // are there firm conditions?
      BOOL chkprice;                      // are there price conditions?
      BOOL chkvalid;                      // are the above 3 varibles valid?
};

void drawuser();	// draw licensee's name
