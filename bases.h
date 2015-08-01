//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// definitions of classes for working with data files
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 13-Oct-95
//-----------------------------------------------------------------------------

#include <fstream.h>
#include <except.h>
#include "date.h"

typedef char BOOL;
#define TRUE 1
#define FALSE 0

#pragma warn -inl

class BaseKey {
	public:
		char username[50];	// licensee's name
		BaseKey() : ready(FALSE), key(-1) {};
		void set(long k) {
			key = k;
			ready = FALSE;
		};
		void getready();	// make the key ready
		char operator[](short i) {	// i ranges from 1 to 8
			if (!ready) getready();
			return kb[i-1];
		}
		long getkey() { return key; }
	private:
		char kb[8];	// xor value for 1-8 bit accord
		BOOL ready;
		long key;
};

extern BaseKey basekey;

class StrPack {   // encoding and decoding strings
   public:
      char* encode(char*);
/*      char* decode(char*); */
      void decode(char *from, char *to);
   private:
      char out[256];
      char *in;
      int inbit, outbit;      // bit positions in in and out strings
      BOOL rus;               // is current language Russian?
      char getbits(char n);   // get n more bits
      void putbits(char n, char bits);   // put n more bits;
      inline char quote(); // returns quote char (5bit) for current language
      inline char numode(); // returns number mode char (5bit) for cur language
      char dechar(char c); // decode character from current language
      char enchar(char c); // encode character:
                           // returns 255 if char must be quoted
                           // otherwise highest bit sets language
                           // (1 - Russian, 0 - English)
};

typedef unsigned short ushort;

class PPF { // pulse.ppf file - firms info
   public:
      PPF() : firm(NULL), fullname(NULL), address(NULL), is_open(FALSE),
         f(NULL) {};
      ~PPF() {
         if (is_open)
            close();
      }
      long area;      // 000-999 are valid
      long  phone[4];   // the last one - fax number
      char *firm;       // firm name
      char *fullname;   // full name
      char *address;    // firm address
      BOOL open(char *path);  // argument - the path, return TRUE is sucess
      BOOL create(char *path,ushort nf); // nf - number of firms
      void close() {
         if (f)
            f->close();
         is_open = FALSE;
         if (!fread) return;
         if (firm)
            delete firm;
         if (fullname)
            delete fullname;
         if (address)
            delete address;
         firm=address=fullname=0;
         if (f)
            delete f;
         f = NULL;
      }
      BOOL isopen() { return is_open; }
      long fsize() { return fsz; } // file size
      ushort Nfirms() { return nfirms; }
      ushort Fno() { return fno; }
      BOOL readn(ushort n,BOOL fonly=FALSE); // read n-th rec, TRUE if success
      // if fonly==TRUE => only read firm name
   /***************************************************************************
     WARNING: readn() will try to delete firm, fullname and address if they
	  aren't nulls.  If they ain't supposed to be deleted, set them to nulls
	  before calling readn()!
   ***************************************************************************/
      BOOL write();   // write next record, TRUE if success
   private:
      ushort nfirms;  // number of firms
      ushort fno;     // current firm number
      fstream *f;
      BOOL is_open;
      BOOL fread;    // file open for reading?
      long fsz; // file size
};

class PPG { // pulse.ppg file - groups info
	public:
		PPG() : gname(NULL), is_open(FALSE), f(NULL) {};
		~PPG() {
		}
		long gsize;       // number of records in the group
		long gfirst;      // first record of this group in pulse.ppd
		char *gname;      // group name
		BOOL isdemand,    // are there demand records?
			  issupply;    // are there supply records?
		BOOL open(char *path);  // argument - the path, return TRUE is sucess
		BOOL create(char *path,ushort ng); // ng - number of groups
		void close() {
			if (f)
				f->close();
			is_open = FALSE;
			if (!fread) return;
			if (gname)
				delete[] gname;
			gname=NULL;
			if (f)
				delete f;
			f=NULL;
		}
		long fsize() { return fsz; } // file size
		BOOL isopen() { return is_open; }
		ushort Ngroups() { return ngroups; }
		ushort Gno() { return gno; }
		void rewind() { gno=0; }   // rewinds to the beginning
		BOOL read();   // read next record, TRUE if success
		long getKEY(long CIC);	// find KEY with given CIC. Return -1 if cannot
	/***************************************************************************
	  WARNING: readn() will try to delete gname if it isn't null. If it isn't
	  supposed to be deleted, set it to null before calling readn()!
	***************************************************************************/
		BOOL write();   // write next record, TRUE if success
	private:
		ushort ngroups;  // number of groups
		ushort gno;     // current group number
		fstream *f;
		BOOL is_open;
		BOOL fread;    // file open for reading?
		long fsz; // file size
		long keyofs;	// offset of the keys database
};

struct Paym {  // payment method structure
   char abbr[4];  // abbreviation
   float coef;    // conversion to roubles coeficient
   long isupply;  // offset of sorted supply index for this payment type
   long idemand;  // same for demand index;
};

// Index constants (for iselect() function)
#define iSupply 1
#define iDemand 2
//#define iSrtSupply 3   EXCLUDED! 02.11.95
//#define iSrtDemand 4   EXCLUDED! 02.11.95
#define iPaym 5   // + method_code*2 + (supply? 0 : 1)  [method_code - 0..?]

class PPI { // pulse.ppi file - file of indexes
   public:
      char npaym; // Number of payment methods
      Paym *paym; // payment methods info
      PPI() : is_open(FALSE), npaym(0), idx(0), prec(NULL),
               pofs(NULL), f(NULL), buffer1(NULL), buffer2(NULL), bsize1(0),
               bsize2(0) {}
      ~PPI() {
         if (isopen())
            close();
         if (prec)
            delete[] prec;
         if (pofs)
            delete[] pofs;
         if (buffer1)
            delete[] buffer1;
         if (buffer2)
            delete[] buffer2;
      }
      BOOL open(char *path);  // argument - the path, return TRUE is sucess
      BOOL create(char *path);
      void close();
      long fsize() { return fsz; } // file size
      BOOL isopen() { return is_open; }
      long RecNo() { return recno; } // number of records in index
      BOOL write();           // write payment methods info (npaym total)
      BOOL write(long);       // write next index element
      BOOL iselect(char);     // select index (before writing or reading)
      BOOL writeg(int gno,long gfirst, long glast); // write first and last
      // used indexes for group gno for current payment method
      BOOL readg(int gno, long &gfirst, long &glast); // read first and last
      long operator[](long);  // get index element
      long operator()(long,BOOL); // get offset of i-th record in main index
         // BOOL: TRUE for supply, FALSE for demand.
      unsigned Bsize1() { return bsize1; }
      unsigned Bsize2() { return bsize2; }
      BOOL setbuf1(unsigned size) {
         if (buffer1)
            delete[] buffer1;
         try {
            buffer1 = new char[size];
            bsize1=size;
				bufofs1=-1;
			} catch (xalloc) {
				buffer1=NULL;
				bsize1=0;
				return FALSE;
			}
			return TRUE;
		}
		BOOL setbuf2(unsigned size) {
			if (buffer2)
				delete[] buffer2;
			try {
				buffer2 = new char[size+25];
				*((long*)(buffer2+4*0))=-1;
				*((long*)(buffer2+4*1))=-1;
				*((long*)(buffer2+4*2))=-1;
				*((long*)(buffer2+4*3))=-1;
				*((long*)(buffer2+4*4))=-1;
				bsize2=size;
				return TRUE;
			} catch (xalloc) {
				buffer2=NULL;
				bsize2=0;
				return FALSE;
         }
      }
      void delbuf1() {
         if (buffer1) {
            delete buffer1;
            buffer1=NULL;
            bsize1=0;
         }
      }
      void delbuf2() {
         if (buffer2) {
            delete buffer2;
            buffer2=NULL;
            bsize2=0;
         }
      }
   private:
      long nsup, ndem;  // number of supply and demand elements 
      long curofs;   // current file offset
      long recno;   // number of records in index
      char idx;   // current selected index
      long *prec; // number of records for each payment method
      long *pofs; // offset for each payment method
      long iofs;  // curent index ofs
      long sup_ofs, dem_ofs; // main supply and demand index offsets
      fstream *f;
      BOOL is_open;
      BOOL fread;    // file open for reading?
      long fsz; // file size
      char *buffer2;
      char *buffer1;
      unsigned bsize1;   // buffer size
      unsigned bsize2;   // buffer size
      long bufofs2;      // offset in buffer
      long bufofs1;      // offset in buffer
};

struct DataRec {
   float price;            // requested price
   unsigned short fcode;   // firm code
	char pcode;             // payment method code
//	Date dsubmit;           // submition date (or 0.0.0 if too old)
	char ad[161];           // decoded ad text
};

class PPD { // pulse.ppd file - main data file
	public:
		char nupdat;   // Number of updates
		Date dcreate,  // date of creation
			  dupdate;  // date of updation
	  DataRec drec;   // data record
		PPD() : is_open(FALSE), f(NULL), buffer(NULL), bsize(0) {};
		~PPD() {
			if (isopen())
				close();
			if (buffer)
				delete[] buffer;
		}
		long fsize() { return fsz; } // file size
		BOOL open(char *path);  // argument - the path, return TRUE is sucess
		BOOL create(char *path);
		void close() {
			if (f)
				f->close();
			is_open = FALSE;
			if (f)
				delete f;
			f = NULL;
		}
		BOOL isopen() { return is_open; }
		long write();  // write next element, returns its offset
		BOOL read(long ofs,BOOL readad=TRUE); // read next element from ofs.
			// if readad==FALSE, then do not read as string!
		unsigned Bsize() { return bsize; }
		BOOL setbuf(unsigned size) {
			if (buffer)
            delete[] buffer;
         try {
            buffer = new char[size];
            bsize=size;
         } catch (xalloc) {
            buffer=NULL;
            bsize=0;
            return FALSE;
         }
         bufofs=-size-1;
         return TRUE;
      }
      void delbuf() {
         if (buffer) {
            delete buffer;
            buffer=NULL;
            bsize=0;
         }
      }
   private:
      long curofs;      // current file offset
      fstream *f;
      BOOL is_open;
      BOOL fread;       // file open for reading?
      long fsz;         // file size
      char *buffer;
      unsigned bsize;   // buffer size
      long bufofs;      // offset in buffer
};

#pragma warn .inl

