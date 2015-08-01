//-----------------------------------------------------------------------------
// Pulse of Prices Database Creator
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 11-oct-95
//-----------------------------------------------------------------------------

#include "date.h"
#include "varray.h"
#include "bases.h"
#include <dbf/ap/dbf.h>
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <alloc.h>
#include <stdlib.h>
#include <string.h>
#include <except.h>

//#define DEBUG

#ifdef DEBUG
   #define b_address "final\\bases\\adress_.dbf"
   #define b_offer   "final\\bases\\offer_.dbf"
   #define b_product "final\\bases\\product_.dbf"
   #define b_prodtype "final\\bases\\prodtyp_.dbf"
   #define payments_ini "final\\payments.ini"
	#define final_dir "final\\bases"
//   #define CHECKSORT // check quicksort
//   #define NOPPF  // no .ppf file generation
//   #define NOPPG  // no .ppg file generation
	#define NOASORT // no alphabetical sort for records
#else
	#define NOASORT // no alphabetical sort for records
	#define b_address "adress_.dbf"
	#define b_offer   "offer_.dbf"
	#define b_product "product_.dbf"
	#define b_prodtype "prodtyp_.dbf"
	#define payments_ini "payments.ini"
#endif

//#define HEAPSORT // use heapsort instead of quicksort...

char upcase(char);
#ifndef NOASORT
void qsortads(PPD&,VArrayL&,ulong,ulong);  // quick sort by names
#endif
#ifdef HEAPSORT
void heapsort(PPD&,PPI&,VArrayL&,ulong,ulong,BOOL); // heapsort by prices
#else
void qsortprice(PPD&,PPI&,VArrayL&,ulong,ulong,BOOL,ulong);
	// quick sort by prices
#endif
int cmpgr(BSS *o_product, int n1, int n2); // compare two group names

PPI ppi;
PPD ppd;
PPF ppf; // firm info file
PPG ppg; // group info file
int nidx[600];
int nidx0[600];
long g1[600];  // group starting points
long gfirst[600]; // group first element
long glast[600];  // group last element

// extern unsigned _stklen=20000u;

int main(int argc, char *argv[]) {
	cout << "Pulse of Prices Databases Ver 1.10 creator of "
		  << __DATE__ << "\n"
			  "Copyright (c) 1995, 1996 by Sergey Gershtein. Ural-Relcom, Ltd.\n"
			  "---------------------------------------------------------------\n"
			  "This program converts Oferta's Pulse database files from "
			  "current directory\n"
			  "into Pulse of Prices Viewer format.\n\n";

#ifndef DEBUG
	char *final_dir=".\\";
	if (argc>1)
		if (argv[1][0]=='?' || argv[1][1]=='?') {
			cout << "Usage: " << argv[0] << " <dest_dir>\n\n"
			"All the Oferta *.dbf files and optionally payments.ini file\n"
			"must be in current directory.  The resulting pulse.* files will\n"
			"be stored in the <dest_dir> directory.\n"
			"Be prepared that it may take a long time to create the bases.\n"
			"You can stop the program at any time by pressing Ctrl-C.\n";
			return 0;
		} else
		final_dir=argv[1];
#else
	if (argc>1)
		cout << "### This version was compiled with DEBUG option set.\n"
		"### It does not understand your argument '"<<argv[1]<<"'\n";
#endif

	cout << "Opening files:\n";

// clearing all the read-only attrs and stuff
   _rtl_chmod(b_address,1,0);
   _rtl_chmod(b_offer,1,0);
   _rtl_chmod(b_product,1,0);
   _rtl_chmod(b_prodtype,1,0);
// opening the bases
   BSS *o_address=open_dbf(b_address);
   cout << "   " << b_address;
   if (!o_address) {
      cout << " - error opening file\n";
      return 1;
   } else
      cout << endl;
   BSS *o_offer = open_dbf(b_offer);
   cout << "   " << b_offer;
   if (!o_offer) {
      cout << " - error opening file\n";
      return 1;
	} else
      cout << endl;
   BSS *o_product = open_dbf(b_product);
   cout << "   " << b_product;
   if (!o_product) {
      cout << " - error opening file\n";
      return 1;
   } else
      cout << endl;
   BSS *o_prodtype = open_dbf(b_prodtype);
   cout << "   " << b_prodtype;
   if (!o_prodtype) {
      cout << " - error opening file\n";
      return 1;
   } else
      cout << endl;

// bases opened.  Now reading... ------------------------ PPF ---------------
#ifndef NOPPF
{
   cout << "Creating pulse.ppf... ";
   if (!ppf.create(final_dir,(ushort)o_address->Amount)) {
      cout << "file creation error!\n";
      return 1;
   }
   ppf.firm = new char[17];
   ppf.address = new char[61];
   ppf.fullname = new char[120];
   for (ushort n=1; n<=o_address->Amount; n++) { // for each record
		if (n%10==1) {
         cout.width(6);
         cout << n << ' ';
         cout << "\x8\x8\x8\x8\x8\x8\x8";
      }
      if (read_dbf(o_address,n)) {
         cout << "Error reading database record!\n";
         return 1;
      }
      if (read_buf(o_address,ppf.firm,NULL,ppf.address,ppf.fullname,
         &ppf.phone[0],&ppf.phone[1],&ppf.phone[2],&ppf.phone[3],&ppf.area)) {
         cout << "Error parsing database record!\n";
         return 1;
      }
      for (char *s=ppf.firm; (*s=upcase(*s))!=0; s++) ; // upcase firm name
      if (!ppf.write()) {
         cout << "Error writing record!\n";
         return 1;
      }
   }
   delete[] ppf.firm;
   delete[] ppf.address;
   delete[] ppf.fullname;
   ppf.close();
   cout << n << " records processed.\n";
}
#endif
// --------------------------- creating PPG file -----------------------------
#ifndef NOPPG
{  int n;
   long dsup=0, ddem=0;
   cout << "Reading groups information...         ";
   for (n=0; n<o_product->Amount; n++) {
      nidx[n]=n+1;
      cout << "\x8\x8\x8\x8\x8\x8\x8";
      cout.width(6);
      cout << (int)n << ' ';
      if (read_dbf(o_product,n+1)) {
         cout << "Error reading database record!\n";
         return 1;
      }
      unsigned int recno;                  
      long last;
      if (read_buf(o_product,NULL,&recno,&g1[n],&last)) {
         cout << "Error parsing database record!\n";
         return 1;
      }
      if (read_dbf(o_prodtype,recno)) {
         cout << "Error reading database prodtyp_.dbf!\n";
         return 1;
      }
      int supply;
      if (read_buf(o_prodtype,NULL,NULL,NULL,&supply)) {
         cout << "Error parsing database prodtyp_.dbf record!\n";
         return 1;
      }
      if (supply) {
         dsup+=last-g1[n]+1;
			g1[n]-=ddem;
      } else { // demand
         ddem+=last-g1[n]+1;
         g1[n]-=dsup;
      }
      g1[n]--; // we start from zero, not one
   }

   cout << "\nSorting groups with InsertSort... ";
   for (int k=(int)o_product->Amount-2; k>=0; k--) {
      char save=nidx[k];   // save k-th element
      long sav1=g1[k];
      for (int j=k+1; (j<=o_product->Amount-1) &&
            cmpgr(o_product,save,nidx[j])>0; j++) {
         nidx[j-1]=nidx[j];
         g1[j-1]=g1[j];
      }
      nidx[j-1] = save;
      g1[j-1] = sav1;
      if (k%7==0) {
         cout.width(3);
         cout << k << "\x8\x8\x8";
      }
   }
   cout << "Done sorting.  \n";

   cout << "Creating pulse.ppg...        ";
   if (!ppg.create(final_dir,(ushort)o_product->Amount)) {
      cout << "file creation error!\n";
		return 1;
   }
   ppg.gname = new char[61];
   for (n=0; n<o_product->Amount; n++) { // for each record
      cout << "\x8\x8\x8\x8\x8\x8\x8";
      cout.width(6);
      cout << (int)n << ' ';
      if (read_dbf(o_product,nidx[n])) {
         cout << "Error reading database record!\n";
         return 1;
      }
      unsigned int recno;
      if (read_buf(o_product,ppg.gname,&recno,&ppg.gfirst,&ppg.gsize)) {
         cout << "Error parsing database record!\n";
         return 1;
      }
      ppg.gsize-=(--ppg.gfirst); // number of records, not the last record
      long gf=ppg.gfirst;
      ppg.gfirst=g1[n];   // not absolute one, but address in the index
      if (read_dbf(o_prodtype,recno)) {
         cout << "Error reading database prodtyp_.dbf!\n";
         return 1;
      }
      int supply;
      if (read_buf(o_prodtype,NULL,NULL,NULL,&supply)) {
         cout << "Error parsing database prodtyp_.dbf record!\n";
         return 1;
      }
      ppg.issupply = !(ppg.isdemand=(supply==0));
		if (!ppg.write()) {
         cout << "Error writing record!\n";
         return 1;
      }
      g1[n]=gf; // now g1 contains what gfirst used to be
   }
   delete[] ppg.gname;
   ppg.close();
   cout << "records processed.\n";
}
#endif
// creating .ppd file (uff..)
{
   try {
      ppi.paym = new Paym[30];   // let's start with 30 payment methods
      ulong pmno[30];            // number of times payment methods are used
      ppi.npaym = 0;
#ifdef DEBUG
      cout << "## coreleft: " << coreleft() << endl;
#endif
		ushort cblno=(ushort)((coreleft()-20000)/2048/4);
		VArrayL ndx(o_offer->Amount*2+2,"s_index.$$$",2048,cblno);
		cout << "Virtual array created: " << cblno << " 2048-element blocks allocated"
				  " for buffer\n";
		cout << "Memory should left afterwards: " << (coreleft() - ((long)cblno)*2048*4) << endl;
		cout << "Creating pulse.ppd... ";
		ppd.dupdate.Today();
		ppd.dcreate = ppd.dupdate;
		if (!ppd.create(final_dir)) {
			cout << "file creation error!\n";
			return 1;
		}
		for (long n=1; n<=o_offer->Amount; n++) { // for each record
			if (n%19==1) {
				cout.width(7);
				cout << n << ' ';
				cout << "\x8\x8\x8\x8\x8\x8\x8\x8";
         }
         if (read_dbf(o_offer,n)) {
            cout << "Error reading database record!\n";
            return 1;
         }
         double dprice;
         char payment[4];
         char date[7];
         if (read_buf(o_offer,NULL,NULL,&ppd.drec.fcode,NULL,&dprice,
             payment,date,ppd.drec.ad)) {
            cout << "Error parsing database record!\n";
            return 1;
         }
         ppd.drec.price=dprice;
         ppd.drec.fcode--;
         date[2]=date[5]=0;
         ppd.drec.dsubmit = Date(atoi(date),atoi(date+3),ppd.dcreate.year());

         for (int i=0; i<ppi.npaym; i++)// do we know this payment method?
            if (upcase(ppi.paym[i].abbr[0])==upcase(payment[0]) &&
                upcase(ppi.paym[i].abbr[1])==upcase(payment[1]) &&
                upcase(ppi.paym[i].abbr[2])==upcase(payment[2]))
                  break;
         if (i>=ppi.npaym) { // new payment method
				ppi.npaym = i+1;
            pmno[i]=0;
            for (int j=0; j<4; j++)
               ppi.paym[i].abbr[j]=payment[j];
            ppi.paym[i].coef=1;  // roubles form by default.
         }
         ppd.drec.pcode = i;
         pmno[i]++;
         for (char *s=ppd.drec.ad; (*s=upcase(*s))!=0; s++) ;
         if ((ndx[n-1]=ppd.write())==0) {
            cout << "Error writing record!\n";
            return 1;
         }
      }
      ppd.close();
      cout << (n-1) << " records processed.\n";
      cout << (int)ppi.npaym << " payment methods were encountered:\n";
      for (int i=0; i<ppi.npaym; i++) {
         if (i>0)
           cout << ", ";
         for (int j=0; j<4; j++)
            cout << ppi.paym[i].abbr[j];
         cout << " - ";
         cout.width(7);
         cout << pmno[i];
      }
      cout << endl;

      // finding minimum course
		cout << "Looking for the USD course... ";
      if (!ppg.open(final_dir)) {
         cout << "Error opening pulse.ppg!\n";
         return 1;
      }
      for (int i0=0; i0<ppg.Ngroups(); i0++) {
         if (!ppg.read()) {
            cout << "Error reading pulse.ppg record "<<i0<<"!\n";
            return 1;
         }
         if (strstr(ppg.gname,"‚€‹ž’€") && ppg.issupply)
            break;
      }
      double usdk=1e+10;
      if(!ppd.open(final_dir)) {
         cout << "Error opening pulse.ppd!\n";
         return 1;
      }
      if (i0>=ppg.Ngroups()) {
         cout << "!!!!!!!!!!!!!!!!! Not found !!!!!!!!!!!!!!!!!!!\n";
         usdk=4700;         // ????
      } else {
         for (i=0;i<ppg.gsize;i++) { // looking for course
            if (!ppd.read(ndx[g1[i0]+i])) {
               cout << "Error reading pulse.ppd records!\n";
               return 1;
            }
            if (strstr(ppd.drec.ad,"Ž„€†€ USD") &&
                ppd.drec.price<usdk) // found
					 usdk = ppd.drec.price;
         }
         if (usdk<1e+9)
            cout << usdk << " roubles/$\n";
         else {
            cout << "Records not found!\n";
            usdk = 0;
         }
      }
      ppg.close();

      // working with payments.ini file

      cout << "Checking "<<payments_ini<<" file... ";
      ifstream inif(payments_ini,ios::in);
      if (!inif)
         cout << "File not found.\n";
      else { // working with the file
         cout << "File found\n";
         while (!inif.eof()) {
            char pmt[4];
            char line[30];
            inif.read(pmt,4); // read four characters of payment type
            if (inif.eof())
               break;
            inif.getline(line,29);
            for (i=0; i<ppi.npaym; i++)
               if (upcase(ppi.paym[i].abbr[0])==upcase(pmt[0]) &&
                   upcase(ppi.paym[i].abbr[1])==upcase(pmt[1]) &&
						 upcase(ppi.paym[i].abbr[2])==upcase(pmt[2])) { // found
                  char *c;
                  for (c=line; *c<=' '; c++); // skip leading blanks
                  if (*c=='$')  // dollar conversion coef
                     ppi.paym[i].coef = atof(++c)*usdk;
                  else
                     ppi.paym[i].coef = atof(c);
                  cout << "   ";
                  for (int j=0; j<4; j++)
                     cout << ppi.paym[i].abbr[j];
                  cout << " = " << ppi.paym[i].coef << " roubles\n";
                  break;
               } // payment methods found
         } // while not eof
         inif.close();
      } // .ini file found

      // building demand and supply lists
      cout << "Building demand and supply lists... ";
      long nsup=0, ndem=0; // number of supply and demand records
      if (!ppg.open(final_dir)) {
         cout << "Error opening pulse.ppg!\n";
         return 1;
      }
      for (i=0; i<ppg.Ngroups(); i++) {
         if (!ppg.read()) {
            cout << "Error reading pulse.ppg record "<<i<<"!\n";
            return 1;
         }
			if (ppg.issupply)
            nsup+=ppg.gsize;
         else if (ppg.isdemand) {
            ndem+=ppg.gsize;
            for (int j=0; j<ppg.gsize; j++) // set high bit to 1 for demand
               ndx[g1[i]+j]=ndx(g1[i]+j) | 0x80000000l;
         }
         cout.width(7);
         cout << (ndem+nsup) << "\x8\x8\x8\x8\x8\x8\x8";
      }
      cout << "Done           \n   " << nsup << " supply records.\n   "
           << ndem << " demand records.\n";
      ppg.close();
      // creating ppi file -------------------------------------

      cout << "Creating pulse.ppi:\n   ";
      if (!ppi.create(final_dir)) {
         cout << "File creation error!\n";
         return 1;
      }
      if (!ppi.write()) { // writing initial payment info records
         cout << "Error writing payment info records!\n";
         return 1;
      } else
         cout << "Payment information stored.\n";

      cout << "   Writing supply records list... ";
      if (!ppi.iselect(iSupply)) {
         cout << "Error storing index offset!\n";
			return 1;
      }
      for (n=0; n<o_offer->Amount; n++) // for each element
         if ((ndx(n) & 0x80000000l)==0) // supply
            if (!ppi.write(ndx(n))) {
               cout << "Error writing index element!\n";
               return 1;
            } else if (n%33==1) {
               cout.width(7);
               cout << n << "\x8\x8\x8\x8\x8\x8\x8";
            }
      cout << "Done.        \n";

      cout << "   Writing demand records list... ";
      if (!ppi.iselect(iDemand)) {
         cout << "Error storing index offset!\n";
         return 1;
      }
      for (n=0; n<o_offer->Amount; n++) // for each element
         if ((ndx(n) & 0x80000000l)!=0) // demand
            if (!ppi.write(ndx(n)&0x7fffffffl)) {
               cout << "Error writing index element!\n";
               return 1;
            } else if (n%33==1) {
               cout.width(7);
               cout << n << "\x8\x8\x8\x8\x8\x8\x8";
            }
      cout << "Done.        \n";
#ifndef NOASORT
		qsortads(ppd,ndx,0,o_offer->Amount-1);  // quick sort by names
      cout << "   Writing supply records list... ";
      if (!ppi.iselect(iSrtSupply)) {
         cout << "Error storing index offset!\n";
         return 1;
      }
      for (n=0; n<o_offer->Amount; n++) // for each element
         if ((ndx(n) & 0x80000000l)==0) // supply
            if (!ppi.write(n)) {
               cout << "Error writing index element!\n";
               return 1;
            } else if (n%33==1) {
               cout.width(7);
               cout << n << "\x8\x8\x8\x8\x8\x8\x8";
            }
      cout << "Done.        \n";

      cout << "   Writing demand records list... ";
      if (!ppi.iselect(iSrtDemand)) {
         cout << "Error storing index offset!\n";
         return 1;
      }
      for (n=0; n<o_offer->Amount; n++) // for each element
         if ((ndx(n) & 0x80000000l)!=0) // demand
            if (!ppi.write(n/*ndx(n)&0x7fffffffl*/)) {
               cout << "Error writing index element!\n";
               return 1;
            } else if (n%33==1) {
               cout.width(7);
					cout << n << "\x8\x8\x8\x8\x8\x8\x8";
            }
      cout << "Done.        \n";
#endif // alpha sort

      cout << "Building indexes for each payment type:\n";
      cout << "   Supply... ";
      ulong pmcur[30];  // index positions for each payment type
      pmcur[0]=0;
      for (n=0; n<ppi.npaym-1; n++) {
         pmcur[(int)n+1]=pmcur[(int)n]+pmno[(int)n];
         pmno[(int)n]=pmcur[(int)n];

#ifdef DEBUG
         cout << "pmcur[" << n << "]=" << pmcur[n] << " ";
#endif

      }

#ifdef DEBUG
      cout << endl;
#endif

      pmno[(int)n]=pmcur[(int)n];
// now pmno points to the beginnings of the lists, pmcur - to the endings
//      long ll=0;
      for (n=0; n<nsup; n++) {
         if (!ppd.read(ppi(n,TRUE),FALSE)) { // not reading ad string
            cerr << "Error reading data #1!\n";
				return 1;
         }
         union {
            ulong l;
            float f;
         } u;
         u.f = ppd.drec.price;

#ifdef DEBUG
   if (pmcur[ppd.drec.pcode]+o_offer->Amount+1>=o_offer->Amount*2+2) {
      cerr << "@1: pcode=" << (int)ppd.drec.pcode <<
              ", pmcur[pcode]=" << (long)pmcur[ppd.drec.pcode] <<
              ", Amount=" << o_offer->Amount << endl;
   }
#endif

         ndx[pmcur[ppd.drec.pcode]+o_offer->Amount+1]=u.l;
         ndx[pmcur[ppd.drec.pcode]++]=n;

         if (n%13==1) {
            cout.width(7);
            cout << n << "\x8\x8\x8\x8\x8\x8\x8";
         }
      }
      cout << n << " elements done.\n";
      cout << "   Sorting the lists:\n";

      for (n=0; n<ppi.npaym; n++) {  // sorting all lists
         if (pmno[(int)n]>=pmcur[(int)n])
				continue;   // no records for this payment type
#ifdef HEAPSORT
         cout << "     HeapSorting ";
#else
         cout << "     QuickSorting ";
#endif
         for (int j=0; j<4; j++)
            cout << ppi.paym[(int)n].abbr[j];
         cout << "... ";
#ifdef HEAPSORT
         heapsort(ppd,ppi,ndx,pmno[(int)n],pmcur[(int)n]-1,TRUE);
#else
         qsortprice(ppd,ppi,ndx,pmno[(int)n],pmcur[(int)n]-1,
            TRUE,o_offer->Amount+1);
#endif
#ifdef SORTCHECK
         cout << "DEBUG: Checking sort order...";
         if (!ppd.read(ppi(ndx(pmno[(int)n]),TRUE),FALSE)) {
            cout << "ee!";
            return -1;
         }
         for (long ll=pmno[(int)n]+1; ll<pmcur[(int)n]; ll++) {
            float pr=ppd.drec.price;
            union {
               ulong l;
               float f;
            } u1;
            u1.l=ndx(ll+o_offer->Amount+1);
            if (!ppd.read(ppi(ndx(ll),TRUE))) {
					cout << "ee!";
               return -1;
            }
            if (pr>ppd.drec.price) {
               cout << ll << ": SORT ERROR!\n";
         //      return -1;
            }
            cout.width(8);
            cout << ll << "\x8\x8\x8\x8\x8\x8\x8\x8";
         }
         cout << "Sort OK\n";
#endif
      }

      cout << "   Storing indexes... ";
      for (int jj=0; jj<o_product->Amount; jj++)
         nidx0[nidx[jj]-1]=jj;
      for (int pc=0; pc<ppi.npaym; pc++) {
         ppi.iselect(iPaym+pc*2);
         for (int gr=0; gr<o_product->Amount; gr++)
            gfirst[gr]=glast[gr]=-1;
         for (long n=pmno[pc]; n<pmcur[pc]; n++) {
            ppi.write(ndx(n));
            // finding to which group ndx(n) belongs... Binary search
            int left=0, right=(int)(o_product->Amount-1), mid;
            while (left<right) {
                  mid=(right+left)/2;
                  if (ndx(n)<g1[nidx0[mid]])
                     right=mid-1;
						else if (ndx(n)>=g1[nidx0[mid]] &&
                     (mid==o_product->Amount-1 || ndx(n)<g1[nidx0[mid+1]])) {
                     // found
                     left=mid;
                     break;
                  } else
                     left=mid+1;
               }
            // group found
            if (gfirst[nidx0[left]]==-1)
               gfirst[nidx0[left]]=n-pmno[pc];
            glast[nidx0[left]]=n-pmno[pc];
            // information stored in the array
            if (n%13==1) {
               cout.width(7);
               cout << n << "\x8\x8\x8\x8\x8\x8\x8";
            }
         }
         for (gr=0; gr<o_product->Amount; gr++)
            ppi.writeg(gr,gfirst[gr],glast[gr]);
      }
      cout << nsup << " elements done.\n";

      for (n=0; n<ppi.npaym; n++) {
         pmno[(int)n]=pmcur[(int)n];
      }

// now pmno points to the beginnings of the lists, pmcur - to the endings
      cout << "   Demand... ";
		for (n=0; n<ndem; n++) {
         if (!ppd.read(ppi(n,FALSE),FALSE)) { // demand records
            cerr << "Error reading data #1!\n";
            return 1;
         }
         union {
            ulong l;
            float f;
         } u;
         u.f = ppd.drec.price;
         ndx[pmcur[ppd.drec.pcode]+o_offer->Amount+1]=u.l;
         ndx[pmcur[ppd.drec.pcode]++]=n;
         if (n%13==1) {
            cout.width(7);
            cout << n << "\x8\x8\x8\x8\x8\x8\x8";
         }
      }
      cout << n << " elements done.\n";
      cout << "   Sorting the lists:\n";

      for (n=0; n<ppi.npaym; n++) {  // sorting all lists
         if (pmno[(int)n]>=pmcur[(int)n])
            continue;   // no records for this payment type
#ifdef HEAPSORT
         cout << "     HeapSorting ";
#else
         cout << "     QuickSorting ";
#endif
         for (int j=0; j<4; j++)
				cout << ppi.paym[(int)n].abbr[j];
         cout << "... ";
#ifdef HEAPSORT
         heapsort(ppd,ppi,ndx,pmno[(int)n],pmcur[(int)n]-1,FALSE);
#else
         qsortprice(ppd,ppi,ndx,pmno[(int)n],pmcur[(int)n]-1,
               FALSE,o_offer->Amount+1);
#endif
      }

      cout << "   Storing indexes... ";
      for (pc=0; pc<ppi.npaym; pc++) {
         ppi.iselect(iPaym+pc*2+1);
         for (long n=pmno[pc]; n<pmcur[pc]; n++) {
            ppi.write(ndx(n));
            if (n%13==1) {
               cout.width(7);
               cout << n << "\x8\x8\x8\x8\x8\x8\x8";
            }
         }
      }
      cout << ndem << " elements done.\n";

      ppd.close(); // NOT TO FORGET
      ppi.close();
      delete[] ppi.paym;
   } catch (VArrayErr err) {
      cout << "Virtual array exception #" << (int)err.code() << "\n";
      return 1;
	} catch (xalloc xa) {
      cout << "Memory allocation failure: " << xa.requested() << "bytes.\n";
      return 1;
   } catch (...) {
      cout << "Unhandled exception!\n";
      return 1;
   }
}

   cout << "Closing Oferta's files... ";
   if (close_dbf(o_address) || close_dbf(o_product) || close_dbf(o_prodtype) ||
      close_dbf(o_offer)) {
         cout << "Error closing Oferta's bases!\n";
         return 1;
      }

   cout << "Done.\n";
   return 0;
}

char upcase(char c) {
   if ((c>=97 && c<=122) ||   // English
      (c>=160 && c<=175))     // first Russian half
      return (c&~32);
   if (c>=224 && c<=239)      // second Russian half
      return (c-80);
   return c;
}

inline void swap(VArrayL &ndx, ulong i1, ulong i2,ulong Amount) {
   // swap two elements
   ulong sav=ndx(i1);
   ndx[i1]=ndx(i2);
   ndx[i2]=sav;
   sav=ndx(i1+Amount);
   ndx[i1+Amount]=ndx(i2+Amount);
   ndx[i2+Amount]=sav;
}

#ifndef NOASORT

int cmpa(PPD &ppd, ulong ofs1, ulong ofs2) {
   static ulong oo=0;
// compares ad strings at ofs1 and ofs2
   if ((ofs1 & 0x80000000l)==0 &&    // ofs1 - supply
       (ofs2 & 0x80000000l)!=0)      // ofs2 - demand
         return -1;
   if ((ofs1 & 0x80000000l)!=0 &&    // ofs1 - demand
       (ofs2 & 0x80000000l)==0)      // ofs2 - supply
         return 1;

   static char s[161];
   static float pr;
   if (oo!=ofs1) {
      if (!ppd.read(ofs1 & 0x7fffffffl)) {
         cout << "Error reading pulse.ppd offset " << ofs1 << "!\n";
         exit(1);
      }
		strcpy(s,ppd.drec.ad);
      pr=ppd.drec.price;
      oo=ofs1;
   }
   if (!ppd.read(ofs2 & 0x7fffffffl)) {
      cout << "Error reading pulse.ppd offset " << ofs2 << "!\n";
      exit(1);
   }
   int i=strcmp(s,ppd.drec.ad);
   if (i)
      return i;
   else
      return pr>ppd.drec.price?1:(pr==ppd.drec.price)?0:-1;
}

void qsortads(PPD &ppd,VArrayL &ndx,ulong left0, ulong right0) {
   struct Range {
      ulong l,r;
   };
   Range stack[35];  // should be enough for 2**35 records! See Data Structures
                     // book page 259 or (Hoare, 1962)
   int sptr = 0;     // stack pointer

   cout << "   QuickSorting records alphabetically... ";
   stack[sptr].l=left0;     // push initial values into stack
   stack[sptr++].r=right0;

   while (sptr) { // while stack is not empty
      ulong left=stack[--sptr].l;  // retrieve next range from stack
		ulong right=stack[sptr].r;
      cout.width(7);
      cout << (long)(right-left) << "\x8\x8\x8\x8\x8\x8\x8";
// here comes the median of three qsort method.  Figure 6.40 on page 258 of
// Data Structures Book
      swap(ndx,(left+right)/2,left+1,Amount); // finding the median of three
      if (cmpa(ppd,ndx(left+1),ndx(right))>0)
         swap(ndx,left+1,right,Amount);
#ifdef DEBUG
      if (cmpa(ppd,ndx(left+1),ndx(right))>0) {
            cout << "e1!\n";
            exit(1);
         }
#endif
      if (cmpa(ppd,ndx(left),ndx(right))>0)
         swap(ndx,left,right);
#ifdef DEBUG
      if (cmpa(ppd,ndx(left),ndx(right))>0) {
            cout << "e2!\n";
            cout << cmpa(ppd,ndx(left),ndx(right));
            exit(1);
         }
#endif
      if (cmpa(ppd,ndx(left+1),ndx(left))>0)
         swap(ndx,left+1,left);
#ifdef DEBUG
      if (cmpa(ppd,ndx(left+1),ndx(left))>0) {
            cout << "e3!\n";
            exit(1);
			}
      if (cmpa(ppd,ndx(left+1),ndx(left))>0 ||
         cmpa(ppd,ndx(left),ndx(right))>0 ||
         cmpa(ppd,ndx(left+1),ndx(right))>0) {
            cout << "ajsghkasjfgkasjfaskj!\n";
            exit(1);
         }
#endif
      ulong j=left+1, k=right;
      ulong nleft=ndx(left);
      do {
         do { j++;
#ifdef DEBUG
            if (j>right) {
               cout << cmpa(ppd,ndx(left+1),ndx(right));
         }
#endif
         } while (cmpa(ppd,nleft,ndx(j))>0);
         do { k--;
#ifdef DEBUG
            if (k==left) {
               cout << cmpa(ppd,nleft,ndx(left+1));
            }
#endif
         } while (cmpa(ppd,nleft,ndx(k))<0);
         if (j<k)
            swap(ndx,j,k);
      } while (j<=k);
      swap(ndx,left,k);
		if (k-left>right-k) {
         if (k-left>10) {
            stack[sptr].l=left;
            stack[sptr++].r=k-1;
         }
         if (right-k>10) {
            stack[sptr].l=k+1;
            stack[sptr++].r=right;
         }
      } else {
         if (right-k>10) {
            stack[sptr].l=k+1;
            stack[sptr++].r=right;
         }
         if (k-left>10) {
            stack[sptr].l=left;
            stack[sptr++].r=k-1;
         }
      }  // new values are in stack
      if (sptr>34) {
         cout << "Stack overflow error!\n";
         exit(1);
      }
   } // while

   cout << "Almost done.  \n";

// now insertsort on the remaining stuff...
   cout << "   Finishing sorting with InsertSort... ";
	for (long k=right0-1; k>=0; k--) {
      ulong save=ndx(k);   // save k-th element
      for (long j=k+1; (j<=right0) && cmpa(ppd,save,ndx(j))>0; j++)
         ndx[j-1]=ndx(j);
      ndx[j-1] = save;
      if (k%33==0) {
         cout.width(7);
         cout << k << "\x8\x8\x8\x8\x8\x8\x8";
      }
   }
   cout << "Done sorting.  \n";
}

#endif // ifndef NOASORT

int cmpp(VArrayL &ndx,ulong i1,ulong i2,BOOL /*b*/,ulong Amount) {
   union {
      ulong l;
      float f;
   } u1,u2;
#ifdef DEBUG
   if (i1>=Amount || i2>=Amount)
      return 0;
#endif
   u1.l=ndx(i1+Amount);
   u2.l=ndx(i2+Amount);
   ulong l1=ndx(i1);
   ulong l2=ndx(i2);

	return (u1.f>u2.f?1:u1.f<u2.f?-1:
      l1>l2?1:-1); // if prices are equal, comparing indexes...
}

#ifndef HEAPSORT


void qsortprice(PPD &/*ppd*/,PPI &/*ppi*/,VArrayL &ndx,ulong left0,
   ulong right0,BOOL b,ulong Amount) {
   struct Range {
      ulong l,r;
   };
   Range stack[35];  // should be enough for 2**35 records! See Data Structures
                     // book page 259 or (Hoare, 1962)
   int sptr = 0;     // stack pointer

//   cout << "   QuickSorting records by prices... ";
   if (right0-left0>10) {
      stack[sptr].l=left0;     // push initial values into stack
      stack[sptr++].r=right0;
   }

   while (sptr) { // while stack is not empty
      ulong left=stack[--sptr].l;  // retrieve next range from stack
      ulong right=stack[sptr].r;
      if (right-left>100) {
         cout.width(7);
         cout << (right-left) << "\x8\x8\x8\x8\x8\x8\x8";
      }
// here comes the median of three qsort method.  Figure 6.40 on page 258 of
// Data Structures Book
      swap(ndx,(left+right)/2,left+1,Amount); // finding the median of three
      if (cmpp(ndx,left+1,right,b,Amount)>0)
         swap(ndx,left+1,right,Amount);
#ifdef DEBUG
      if (cmpp(ndx,left+1,right,b,Amount)>0) {
            cout << "e1!\n";
            exit(1);
         }
#endif
      if (cmpp(ndx,left,right,b,Amount)>0)
         swap(ndx,left,right,Amount);
#ifdef DEBUG
      if (cmpp(ndx,left,right,b,Amount)>0) {
            cout << "e2!\n";
            exit(1);
         }
#endif
      if (cmpp(ndx,left+1,left,b,Amount)>0)
         swap(ndx,left+1,left,Amount);
#ifdef DEBUG
      if (cmpp(ndx,left+1,left,b,Amount)>0) {
            cout << "e3!\n";
            exit(1);
         }
#endif
      ulong j=left+1, k=right;
      ulong nleft=left;
		do {
         do { j++; } while (cmpp(ndx,nleft,j,b,Amount)>0);
         do { k--; } while (cmpp(ndx,nleft,k,b,Amount)<0);
         if (j<k)
            swap(ndx,j,k,Amount);
      } while (j<=k);
      swap(ndx,left,k,Amount);
      if (k-left>right-k) {
         if (k-left>10) {
            stack[sptr].l=left;
            stack[sptr++].r=k-1;
         }
         if (right-k>10) {
            stack[sptr].l=k+1;
            stack[sptr++].r=right;
         }
      } else {
         if (right-k>10) {
            stack[sptr].l=k+1;
            stack[sptr++].r=right;
         }
         if (k-left>10) {
            stack[sptr].l=left;
            stack[sptr++].r=k-1;
         }
		}  // new values are in stack
		if (sptr>34) {
			cout << "Stack overflow error!\n";
			exit(1);
		}
   } // while

//   cout << "Almost done.  \n";

// now insertsort on the remaining stuff...
//   cout << "   Finishing sorting with InsertSort... ";
   cout << "       ";
   for (long k=right0-1; k>=(long)left0; k--) {
/*      ulong save=ndx(k);   // save k-th element
      ulong spr=ndx(k+Amount); */
      swap(ndx,k,Amount-1,Amount);
      for (long j=k+1; (j<=right0) &&
         cmpp(ndx,Amount-1,j,b,Amount)>0; j++) {
         ndx[j-1]=ndx(j);
         ndx[j-1+Amount]=ndx(j+Amount);
      }
      swap(ndx,j-1,Amount-1,Amount);
/*      ndx[j-1] = save;
      ndx[j-1+Amount] = spr; */
      if (k%73==0) {
         cout << "\x8\x8\x8\x8\x8\x8\x8";
         cout.width(7);
         cout << k;
      }
   }
   cout << "\x8\x8\x8\x8\x8\x8\x8" << (right0-left0+1) << " elements done.\n";

}

#else

void siftdown(PPD &/*ppd*/,PPI &/*ppi*/,VArrayL &ndx,ulong left,ulong right,
   ulong m,BOOL b) {
// sift m-th element down into the correct position in the heap
// b is to be forwarded into cmpp. TRUE for supply, FALSE for demand
// Data Structures book, module 5.3 on page 225

   ulong save=ndx(m);   // save m-th element
   ulong parent=m, child=2*m-left+1; // 2*(m-left+1)+left-1
   while (child<=right) {
      if (child<right && cmpp(ppd,ppi,ndx(child),ndx(child+1),b)<0)
         child++;       // select the smallest of two children
      if (cmpp(ppd,ppi,save,ndx(child),b)>=0)
         break;         // insertion point is found
      ndx[parent]=ndx(child);
      parent = child; child = parent*2-left+1;
   }
   ndx[parent]=save;
}

void heapsort(PPD &ppd,PPI &ppi,VArrayL &ndx,ulong left,ulong right, BOOL b) {
// heapsort by prices
   long perc=0, oldperc=0; // percentage done

// creating a heap...
   for (long k=(right-left+1)/2+left;k>=(long)left; k--) { // left-1 ?!
      siftdown(ppd,ppi,ndx,left,right,k,b);
      perc = (1-(float)(k-left)/((right-left)/2))*50;
      if (perc>oldperc) {
         cout.width(3);
         cout << (oldperc=perc) << '%' << "\x8\x8\x8\x8";
      }
   }

// heap created.  Now sorting...
   for (k=right; k>left; ) {
      swap(ndx,left,k);
      siftdown(ppd,ppi,ndx,left,--k,left,b);
      perc=(2-(float)(k-left)/(right-left))*50;
      if (perc>oldperc) {
         cout.width(3);
         cout << (oldperc=perc) << '%' << "\x8\x8\x8\x8";
      }
   }

   cout << (right-left+1) << " elements done.\n";

}

#endif

int cmpgr(BSS *o_product, int n1, int n2) { // used to compare two group names
   char name1[61], name2[61];
   if (read_dbf(o_product,n1)) {
      cout << "Error reading database record!\n";
      return 1;
   }
   if (read_buf(o_product,name1,NULL,NULL,NULL)) {
      cout << "Error parsing database record!\n";
      return 1;
   }
   if (read_dbf(o_product,n2)) {
      cout << "Error reading database record!\n";
      return 1;
   }
   if (read_buf(o_product,name2,NULL,NULL,NULL)) {
      cout << "Error parsing database record!\n";
      return 1;
   }
   return strcmp(name1,name2);
}


