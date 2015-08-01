//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// classes for working with data files
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 13-Oct-95
//-----------------------------------------------------------------------------

#include <string.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <share.h>
#include <io.h>
#include <stdlib.h>
#include "bases.h"
#include <limits.h>
#include "dcode.h"

//#include <dos.h>

#define SHARE

//#define BDEBUG

BaseKey basekey;	// basekey the only instance (global for all)

void BaseKey::getready() {	// make the key ready
	char *aa = (char*)(&key);
	kb[7] = *aa;
	aa++;
	kb[4] = *aa & 31;
	kb[2] = (*aa >> 5) & 7;
	aa++;
	kb[3] = *aa & 15;
	kb[1] = (*aa >> 4) & 3;
	kb[0] = (*aa >> 6) & 1;
	char cb = 0;
	for (long i=1; i<16777216l; i <<= 1)
		if (key & i)
			cb ^= 1;
	if (cb != 0) {	// parity error
		throw 5;
	}
	ready = TRUE;
}

unsigned P2[]={1,2,4,8,16,32,64,128,256}; // powers of two

char numpos(char c) { // returns char pos or 255 if not numeric
   if (c>='0' && c<='9')
      return c-'0';
   switch (c) {
      case '.': return 0xa;
      case '|': return 0xb;
      case '-': return 0xc;
      case '"': return 0xd;
      case ' ': return 0xe;
      default: return 255; // not a 'numerical' digit
   }
}

char* StrPack::encode(char *s) {
   in=s;
   inbit=outbit=0;
   for (int i=0; i<256; i++)
      out[i]=0; // clear buffer
   char len=(char)strlen(in); // can only handle strings of up to 255 symbols
   putbits(8,0);   // leaving place for string length
   // let's find out the initial language (by first letter)
   for (char *q=in; *q; q++) {
      char c=enchar(*q);
      if (c<255) {
         if (c&0x80) {
            rus = TRUE;
            putbits(1,1);
         } else {
            rus = FALSE;
            putbits(1,0);
         }
         break;
      }
   } // The initial language found
//   char *NUMS="0123456789.|-"""" "; // number mode digits
   for (q=in; *q; q++) { // now encoding each char
      char c=enchar(*q);
      if (c==255) {
         char sc;
         if (((sc=numpos(*q))!=255) && (numpos(*(q+1)!=255))) { // two digits
            putbits(5,numode()); // switch to number mode
            do {
               if (sc!=255)
                  putbits(4,sc);
               else { // have to quote
                  putbits(4,0xf);
                  putbits(1,0);        // quote character, not change language
                  putbits(8,*q);
               }
            } while (*(++q) && ((sc=numpos(*q))!=255 || enchar(*q)==255));
            if (*q--) { // return to regular mode, "--" because of inc in for()
               putbits(4,0xf);
               putbits(1,1);
            } else {
               if (outbit%8+4<8) // 4 more bits must be added
                  putbits(4,0xe); // so we can add an extra space...
               break; // string ended.
            }
            continue;
         }
         putbits(5,quote());  // quote symbol
         putbits(1,0);        // quote character, not change language
         putbits(8,*q);       // the quoted character
      } else if ((!(c&0x80))==(!rus)) { // meaning just !(c&0x80 XOR rus)
         // same language
         putbits(5,c&0x1f);
      } else { // either have to change language or quote char
         char sc;
         if (((sc=numpos(*q))!=255) && (numpos(*(q+1)!=255))) { // two digits
            putbits(5,numode()); // switch to number mode
            do {
               if (sc!=255)
                  putbits(4,(char)(sc));
               else { // have to quote
                  putbits(4,0xf);
                  putbits(1,0);        // quote character, not change language
                  putbits(8,*q);
               }
            } while (*(++q) && ((sc=numpos(*q))!=255 || enchar(*q)==255));
            if (*q--) { // return to regular mode, "--" because of inc in for()
               putbits(4,0xf);
               putbits(1,1);
            } else {
               if (outbit%8+4<8) // 4 more bits must be added
                  putbits(4,0xe); // so we can add an extra space...
               break; // string ended.
            }
            continue;
         }
         for (char *p=q; *p; p++) { // looking if there's two chars of new lang
            char cc=enchar(*p);
            if ((cc<255 && p!=q) || *(p+1)==0) { // if all other chars have to be quoted
                                       // we don't care about the language
               if ((!(cc&0x80))==(!rus)) { // there's only one foreign char
                  putbits(5,quote());  // let's quote it
                  putbits(1,0);        // quote character, not change language
                  putbits(8,*q);       // the quoted character
               } else { // need to change language
                  putbits(5,quote());
                  putbits(1,1);        // change language command
                  rus = !rus;
                  putbits(5,c&0x1f);   // output the character
               }
               break;   // can continue with our characters
            }
         }              // for (p)
      } // new language char
   }
   // ok, seem to be done with it.
   if (outbit%8+5<8) // 5 more bits must be added
      putbits(5,enchar(' ')); // so we can add an extra space...
   if (outbit/8<len)  // encoding is reasonable
      out[0]=outbit/8+(outbit%8?1:0);
   else { // encoding is not reasonable
      out[0]=255;
      strcpy(&out[1],in);
   }
   return out;
}

void StrPack::decode(char *from, char *to) { // new version of decode
   if (from[0]==255) {// string wasn't encoded
      strcpy(to,from+1);
      return;
   }
   in=from;
   inbit=outbit=0;
	getbits(8);	// skip lenth byte
	char len=from[0];   // encoded string length
	int l85=len*8-5;
	if (getbits(1))   // initial language code
		rus = TRUE;
   else
      rus = FALSE;
   char NUMS[]="0123456789.|-\x22 ";
   register char qt=quote(), nm=numode();
   for (char i=0; inbit<l85; ) {  // decode each character
      char c=getbits(5);
      if (c==qt)  // quote signal encountered
         if (getbits(1)) {
            rus = !rus; // changing language
            qt = quote();
            nm = numode();
         } else
            to[i++] = getbits(8);
      else if (c==nm) { // numerical mode
         do {
            char ch=getbits(4);
            if (ch==0xf) // quote char
               if (getbits(1))
                  break; // end numerical mode
               else
                  to[i++]=getbits(8);
            else
               to[i++]=NUMS[ch];
         } while (inbit<l85);
      } else // just regular char
         to[i++] = dechar(c); // decode character
   }
   to[i]=0;
   // done decoding
   return;
}

/*char* StrPack::decode(char *s) { // seems to be pretty fast...
   if (s[0]==255) {// string wasn't encoded
      strcpy(out,s+1);
      return out;
   }
   in=s;
   inbit=outbit=0;
   for (int j=0; j<256; j++)
      out[j]=0; // clear buffer
   char len=getbits(8);   // encoded string length
   if (getbits(1))   // initial language code
      rus = TRUE;
   else
      rus = FALSE;

   for (char i=0; (len*8-inbit)>5; ) {  // encode each character
      char c=getbits(5);
      if (c==quote())  // quote signal encountered
         if (getbits(1))
            rus = !rus; // changing language
         else
            out[i++] = getbits(8);
      else // just regular char
         out[i++] = dechar(c); // decode character
   }
   // done decoding
   return out;
} */

char StrPack::numode() { // returns quote char for current language
   if (rus)
      return 0x14;
   else
      return 0x1a;
}

char StrPack::quote() { // returns quote char for current language
   if (rus)
      return 0x1a;
   else
      return 0x1f;
}

char StrPack::enchar(char c)  {
// encode character:
// returns 255 if char must be quoted
// otherwise highest bit sets language (1 - Russian, 0 - English)
   switch (c) {
   case ' ':
      return rus?0x99:0x1e; // space depends on current encoding
      // 0x99 = 0x19 + 0x80 - the latter is sign of Russian language
   case '.':
      return 0x1c;
   case ',':
      return 0x1d;
   case '-':
      return 0x1b;
//   case '|':
//      return 0x1a;
   case 0x99:  // Russian sch' letter
   case 0x9a:  // Russian hard sign leter
   case 0x94:  // Russian ef letter
      return 255;
   default:
      if (c>=0x80 && c<=0x9f) // Russian alphabet
         return c;   // COOL!   0x80|(c-0x80) - I didn't plan it, really!
      if (c>=0x41 && c<=0x5a) // English alphabet
         return c-0x41;
   }
   return 255; // otherwise char has to be quoted
}

char StrPack::dechar(char c)  { // decode character from current language
   if (rus)
      switch (c) {
      case 0x19:
         return ' ';
      case 0x1a:
         return 0;   // quote char;
      default:
         return c+0x80;
      }
   else {
      if (c<=0x19)
         return c+0x41; // 'A'..'Z'
      switch (c) {
      case 0x1f:
         return 0;   // quote char
      case 0x1e:
         return ' ';
      case 0x1d:
         return ',';
      case 0x1c:
         return '.';
      case 0x1b:
         return '-';
//      case 0x1a:
//         return '|';   // vertical line (often used in bases)
      }
      return 0;
   }
}

char StrPack::getbits(char n) {   // get n more bits
   char pos=inbit>>3; //inbit/8;
   char ofs=inbit&7;  //inbit%8;
   unsigned short mask = (P2[n]-1)<<(16-n-ofs);
   unsigned int d=*(unsigned short*)(in+pos); // two bytes from pos
   asm {
      mov   ax,d
      xchg  ah,al // make it in natural order
      mov   bx,mask
      and   ax,bx
      mov   d,ax
   }
   inbit+=n;
	return ((char)(d >> (16-n-ofs))) ^ basekey[n];
}

void StrPack::putbits(char n, char bits) {   // put n more bits;
	char pos=outbit/8;
	char ofs=outbit%8;
	bits ^= basekey[n];
	unsigned short mask = (bits)<<(8-n+8-ofs);
	unsigned short d=*(unsigned short*)(out+pos); // two bytes from pos
	asm {
		mov   ax,d
		mov   bx,mask
		xchg  bh,bl // make it in natural order
		or    ax,bx
		mov   d,ax
	}
	*(unsigned short*)(out+pos) = d;
	outbit+=n;
}

#ifndef BDEBUG

// ----------------------------- PPF ---------------------------------

#define ppf_signat "SGpf\2" // 1 - file format version

char mystrlen(char *s)  { // returns length of the string w/o trailing spaces
   int sp=-1;
   for (char l=0; s[l]; l++)
      if (s[l]!=' ')
         sp=l;
   return (char)(sp+1);
}

BOOL PPF::open(char *path) {  // argument - the path, return TRUE is sucess
   if (f)
      close();
   char fname[127];
   for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
   q--;
   if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
      *++q='\\';
      *++q=0;
   }
   strcat(fname,"pulse.ppf"); // adding the file name
#ifndef SHARE
   f = new fstream;
   f->open(fname,ios::in | ios::binary); // open file read-only
   if (!f)
      return FALSE; // failed to open the file
#else
   int handle = sopen(fname, O_RDONLY | O_BINARY, SH_DENYNONE, S_IREAD);
   if (!handle)
      return FALSE;
   f = new fstream(handle);
#endif
   f->seekg(0,ios::end);
   firm=fullname=address=NULL;
   fsz=f->tellg();
   f->seekp(0);
   char buf[6];
   f->clear();
   f->read(buf,5);
   buf[5]=0;
   if (f->fail() || strcmp(buf,ppf_signat)) // failure || wrong file signature
      return FALSE;
   f->read((char*)&nfirms,2); // read number of firms
   if (f->fail())
      return FALSE;
   fno = nfirms;  // not a correct record number
   is_open = TRUE;
   fread = TRUE;
   return TRUE;   // success
}

BOOL PPF::create(char* /*path*/,ushort/* nf*/) { // nf - number of firms
/*   char fname[127];
   for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
   q--;
   if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
      *++q='\\';
      *++q=0;
   }
   strcat(fname,"pulse.ppf"); // adding the file name
   f = new fstream;
   f->open(fname,ios::in | ios::binary | ios::out | ios::trunc);
               // open file for read/write, discard contence
   if (!f)
      return FALSE;
   f->write(ppf_signat,5);  // write the signature
   nfirms = nf;
   f->write((char*)&nfirms,2);   // number of firms
   if (f->fail())
      return FALSE;
   fno = 0;
   is_open = TRUE;
	fread = FALSE;*/
	return TRUE;
}

BOOL PPF::readn(ushort n,BOOL fonly) {   // read n-th record
	if (!is_open || !fread || n>=nfirms)
      return FALSE;
   if (fno==n)
      return TRUE;
   f->clear();
   f->seekg(7+4*n);
   long ofs;
   f->read((char*)&ofs,4);
   if (!fonly) {  // read everything
      f->seekg(ofs);
      ushort ar;
      f->read((char*)&ar,2);
      area=ar;
      char buf[12];
      f->read(buf,12);
      phone[0]=((long)buf[0]<<16)+((long)buf[1]<<8)+(buf[2]);
      phone[1]=((long)buf[3]<<16)+((long)buf[4]<<8)+(buf[5]);
      phone[2]=((long)buf[6]<<16)+((long)buf[7]<<8)+(buf[8]);
      phone[3]=((long)buf[9]<<16)+((long)buf[10]<<8)+(buf[11]);
   } else
      f->seekg(ofs+14);  // only read name
	char l;
   f->read(&l,1);
   if (firm)
      delete[] firm;
   firm = new char[l+1];
   f->read(firm,l);
   firm[l] = 0;
   if (fullname)
      delete[] fullname;
   if (address)
      delete[] address;
   if (!fonly) {
      f->read(&l,1);
      fullname = new char[l+1];
      f->read(fullname,l);
      fullname[l] = 0;
      f->read(&l,1);
      address = new char[l+12];  // in case it needs expansion (E-burg) - 12 chars
      f->read(address,l);
      address[l] = 0;
      char *c=strchr(address,1); // chr(1) substitutes Ekaterinburg
      if (c) {// found!
         for (char *q=address+l; q>c ; q--)
            *(q+11)=*q; // shifting right 11 (+1 the code itself) = 12
         for (q="Екатеринбург"; *q; *c++=*q++) ;
		}
	} else
		fullname=address=NULL;
	if (f->bad() || f->fail()) {
		fno=nfirms;
		return FALSE;
	}
	if (!fonly)
		fno=n;
	else
		fno=nfirms;
	return TRUE;
}

BOOL PPF::write() {  // write next record
/*   if (!is_open || fread)
      return FALSE;
   f->clear();
   f->seekp(0,ios::end);
   long flen=f->tellg(); // determining the length of the file
   if (!fno)  // writing first record
      f->seekp(flen=7+4*nfirms);
   ushort ar=(ushort)area;
   f->write((char*)&ar,2);
   char buf[12];
	for (char i=0,j=0; j<4; j++) {
      buf[i++] = (char)((phone[j]>>16) & 0xff);
      buf[i++] = (char)((phone[j]>>8) & 0xff);
      buf[i++] = (char)((phone[j]) & 0xff);
   }
   f->write(buf,12);
   char l = mystrlen(firm);
   f->write(&l,1);
	f->write(firm,l);
   l = mystrlen(fullname);
   if (fullname[0]=='|') {
      f->write(&--l,1);
      f->write(fullname+1,l);
   } else {
      f->write(&l,1);
		f->write(fullname,l);
	}
	char *c = strstr(address,"Екатеринбург");
	if (c) { // string found
		*c++=1;
		for (char *q=c+11;  (*c++=*q++)!=0; ) ; // 12 - length of "E-burg"
	}
	l = mystrlen(address);
	f->write(&l,1);
	f->write(address,l);
	f->seekp(7+4*fno);
	f->write((char*)&flen,4);
	if (f->fail() || f->bad())
		return FALSE;
	fno++;*/
	return TRUE;
}


// ----------------------------- PPG ---------------------------------

#define ppg_signat "SGpg\3" // 3 - file format version

BOOL PPG::open(char *path) {  // argument - the path, return TRUE is sucess
	char fname[127];
	for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
	q--;
	if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
		*++q='\\';
		*++q=0;
	}
	strcat(fname,"pulse.ppg"); // adding the file name
#ifndef SHARE
	f = new fstream;
	f->open(fname,ios::in | ios::binary); // open file read-only
	if (!f)
		return FALSE; // failed to open the file
#else
	int handle = sopen(fname, O_RDONLY | O_BINARY, SH_DENYNONE, S_IREAD);
	if (!handle)
		return FALSE;
	f = new fstream(handle);
#endif
	f->seekg(0,ios::end);
	fsz=f->tellg();
	f->seekp(0);
	f->clear();
	char buf[6];
	f->read(buf,5);
	buf[5]=0;
	if (f->fail() || strcmp(buf,ppg_signat)) // failure || wrong file signature
		return FALSE;
	f->read((char*)&ngroups,2); // read number of firms
	f->read((char*)&keyofs,4);	// offset to the keys database
	if (f->fail())
		return FALSE;
	gno = 0;
	is_open = TRUE;
	fread = TRUE;
	gname=NULL;
	return TRUE;   // success
}

BOOL PPG::create(char* /*path*/,ushort/* ng*/) {
/*   char fname[127];
	for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
	q--;
	if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
		*++q='\\';
		*++q=0;
	}
	strcat(fname,"pulse.ppg"); // adding the file name
	f = new fstream;
   f->open(fname,ios::in | ios::binary | ios::out | ios::trunc);
               // open file for read/write, discard contence
   if (!f)
		return FALSE;
	f->write(ppg_signat,5);  // write the signature
   ngroups = ng;
   f->write(&ngroups,1);   // number of firms
   if (f->fail())
      return FALSE;
   gno = 0;
   is_open = TRUE;
   fread = FALSE;*/
	return TRUE;
}

BOOL PPG::read() {   // read n-th record
	if (!is_open || !fread || gno>=ngroups)
		return FALSE;
	f->clear();
	if (gno==0) // first time
		f->seekg(11);
	f->read((char*)&gsize,4); // combined field
	isdemand = ((gsize >> 31) & 1)!=0;
	issupply = ((gsize >> 30) & 1)!=0;
	gsize&=0x3fffffffL;
	f->read((char*)&gfirst,4); // first record
	char l;
	f->read(&l,1);
	if (gname)
		delete[] gname;
	gname = new char[l+1];
	f->read(gname,l);
	gname[l] = 0;
	if (f->bad() || f->fail()) {
		gno=ngroups;
		return FALSE;
   }
   gno++;
   return TRUE;
}

BOOL PPG::write() {  // write next record
/*   if (!is_open || fread)
		return FALSE;
	f->clear();
	long comb = ((long)isdemand << 31) + ((long)issupply << 30) + gsize;
	f->write((char*)&comb,4);
	f->write((char*)&gfirst,4); // first record
	char l = mystrlen(gname);
	f->write(&l,1);
	f->write(gname,l);
	if (f->fail() || f->bad())
		return FALSE;
	gno++;      */
	return TRUE;
}

long PPG::getKEY(long CIC) {	// find KEY with given CIC. Return -1 if cannot
	if (!is_open || !fread)
		throw 10;
	unsigned long KEY;
	ushort no;
	ushort CRC;
	f->seekg(keyofs);
	f->read((char*)&no,2);	// number of codes in the base
	f->read((char*)&CRC,2);	// Control Word

	if (f->bad()) {
		f->clear();
		return -1;
	}
	for (int i=0; i<no; i++) {
		f->read((char*)&KEY,4);
//		cerr << "\nKEY=" << KEY;
		KEY ^= CIC;
//		cerr << "\nKEY=" << KEY;
		ushort *us = (ushort*)&KEY;
		*us = DCODE(*us); us++;
		*us = DCODE(*us);
//		cerr << "\nKEY=" << KEY;
		char *cp = (char*)&KEY;
		ushort CW = ((cp[1] ^ cp[2] ^ cp[0]) << 8) +
		  ((cp[0] + cp[1] + cp[2]) & 255);
//		cerr << "\nCRC=" << CRC << " CW=" << CW << endl;
		if (cp[3] || CW!=CRC)
			continue;
		else {
			f->clear();
			f->seekg((no-i-1)*4,ios::cur);
			f->read(basekey.username,49);
			basekey.username[f->gcount()]=0;	// end of string
			for (char *c=basekey.username; *c; *c++^=131) ;
			return KEY;
		}
	}
	f->clear();
	return -1;
}

// ----------------------------- PPI ---------------------------------

#define ppi_signat "SGpi\2"

BOOL PPI::open(char *path) {
	// argument - the path, return TRUE is sucess
	curofs=-1;
   char fname[127];
   for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
   q--;
   if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
      *++q='\\';
      *++q=0;
   }
   strcat(fname,"pulse.ppi"); // adding the file name
#ifndef SHARE
   f = new fstream;
   f->open(fname,ios::in | ios::binary); // open file read-only
	if (!f)
		return FALSE; // failed to open the file
#else
   int handle = sopen(fname, O_RDONLY | O_BINARY, SH_DENYNONE, S_IREAD);
   if (!handle)
      return FALSE;
   f = new fstream(handle);
#endif
   f->seekg(0,ios::end);
   fsz=f->tellg();
   f->seekp(0);
   f->clear();
	char buf[6];
	f->read(buf,5);
   buf[5]=0;
   if (f->fail() || strcmp(buf,ppi_signat)) // failure || wrong file signature
      return FALSE;
   f->read((char*)&sup_ofs,4); // offset of supply main index
   f->read((char*)&dem_ofs,4); // offset of demand main index
   f->seekg(sup_ofs);
   f->read((char*)&nsup,4);
   f->seekg(dem_ofs);
   f->read((char*)&ndem,4);
   f->seekg(21);
   f->read(&npaym,1); // number of payment methods
   paym = new Paym[npaym];
	f->read((char*)paym,npaym*sizeof(Paym)); // read payments info
   if (f->fail())
      return FALSE;
   is_open = TRUE;
	fread = TRUE;
   if (pofs)
      delete[] pofs;
   if (prec)
      delete[] prec;
   pofs=new long[npaym*2];
   prec=new long[npaym*2];
   for (int i=0; i<npaym*2; i++)
		prec[i]=-1;
   return TRUE;   // success
}

BOOL PPI::create(char * /*path*/) {
/*	char fname[127];
	for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
	q--;
	if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
		*++q='\\';
		*++q=0;
	}
	strcat(fname,"pulse.ppi"); // adding the file name
	f = new fstream;
   f->open(fname,ios::in | ios::binary | ios::out | ios::trunc);
               // open file for read/write, discard contence
   if (!f)
      return FALSE;
   f->write(ppi_signat,5);  // write the signature
   if (f->fail())
      return FALSE;
   is_open = TRUE;
	fread = FALSE;
	nsup=ndem=0;*/
	return TRUE;
}

BOOL PPI::write() {   // write payment methods info (npaym total)
/*	if (!is_open || fread)
		return FALSE;
	f->seekp(21);
	f->write(&npaym,1);
	f->write((char*)paym,npaym*sizeof(Paym));
	return !f->fail();*/
	return TRUE;
}

BOOL PPI::write(long /*i*/) {       // write next index element
/*	if (!is_open || fread)
		return FALSE;
	f->seekp(0,ios::end);
	f->write((char*)&i,4);
	recno++;
	return !f->fail();*/
	return TRUE;
}

BOOL PPI::writeg(int /*gno*/,long /*gfirst*/, long /*glast*/) { // write first and last
/*	if (!is_open || fread)
		return FALSE;
	f->seekg(iofs+(recno+1)*4+gno*8); // first four bytes - number of records
	f->write((char*)&gfirst,4);
	f->write((char*)&glast,4);
	return !f->fail();*/
	return TRUE;
}

BOOL PPI::readg(int gno, long &gfirst, long &glast) { // read first and last
	if (!is_open || !fread)
		return FALSE;
	long ofs=iofs+(recno+1)*4+gno*8;
	if (ofs!=curofs)
		f->seekg(curofs=ofs);
			// first four bytes - number of records
	f->read((char*)&gfirst,4);
	f->read((char*)&glast,4);
	curofs+=8;
	return !f->fail();
}

void PPI::close() {
	if (!fread && is_open && idx) {
		// need to store number of records for previous idx
		f->seekp(iofs);
		f->clear();
		f->write((char*)&recno,4); // saving number of records
	}
	f->close();
	is_open = FALSE;
	if (fread)
		delete[] paym;
	if (f)
		delete f;
	f = NULL;
}

BOOL PPI::iselect(char i) {     // select index (before writing or reading)
	if (!is_open)
		return FALSE;
	if (fread && i>=iPaym) {
		if (i-iPaym>=npaym*2)
			return FALSE;
		if (prec[i-iPaym]!=-1) {
			iofs=pofs[i-iPaym];
			recno=prec[i-iPaym];
			idx=i;
			return TRUE;
		}
	}
	curofs=-1;
	if (!fread) { // determining current offset
		if (idx) { // need to store number of records for previous idx
			f->seekp(iofs);
			f->clear();
			f->write((char*)&recno,4); // saving number of records
		}
		f->seekg(0,ios::end);
		iofs=f->tellg();
	}
	if (i<iPaym)
      f->seekg(i*4+1);
	else {
		char j=i-iPaym;
      f->seekg(22+8+(j/2)*16+(j%2)*4);
   }
   f->clear();
   if (fread) { // reading data
      f->read((char*)&iofs,4);
      f->seekg(iofs);
      f->read((char*)&recno,4); // reading number of records
      if (i>=iPaym) {
         pofs[i-iPaym]=iofs;
         prec[i-iPaym]=recno;
      }
   } else { // storing data
      f->write((char*)&iofs,4);
      if (i==iSupply)
         sup_ofs=iofs;
		else if (i==iDemand)
         dem_ofs=iofs;
      recno=0;
      f->seekp(0,ios::end);
      f->write((char*)&recno,4); // space for number of records
   }
   idx = i;
   return !f->fail();
}

long PPI::operator[](long i) { // get i-th index element
	if (!fread || !is_open || !idx || i>=recno)
		return 0;
	long ofs=iofs+(i+1)*4;
	if (buffer2 && idx<5+iPaym && idx>=iPaym) { // catching only five first indexes
		int sz=bsize2/5;   // five buffers for five first indexes
		char *lbuf=buffer2+sz*(idx-iPaym)+20;
		bufofs2=*((long*)(buffer2+4*(idx-iPaym)));

		if (ofs<bufofs2 || ofs+5>bufofs2+sz || bufofs2==-1) {
			if (ofs<bufofs2)
				f->seekg(bufofs2=max(ofs-sz+5,(long)0)); // back indexing
			else
				f->seekg(bufofs2=ofs);
			curofs=bufofs2; //ofs+sz;
			f->read(lbuf,sz);
			f->clear();
			*((long*)(buffer2+4*(idx-iPaym)))=bufofs2;
      } // buffer ready
      return *((long*)(lbuf+(unsigned)(ofs-bufofs2)));
   }
   if (curofs!=ofs)
      f->seekg(ofs); // first four bytes - number of records
	long l;
   f->clear();
   f->read((char*)&l,4);
   curofs=ofs+4;
   return l;
}

long PPI::operator()(long i,BOOL b) {
   // get offset of i-th record in main index. b - TRUE for supply
   if (nsup)
		if (b && i>=nsup)
			return LONG_MAX;
		else if (!b && i>=ndem)
			return LONG_MAX;
	long ofs=(b?sup_ofs:dem_ofs)+((i+1)*4);
	if (buffer1) {
      if (ofs<bufofs1 || ofs+5>bufofs1+bsize1 || bufofs1==-1) {
			if (ofs<bufofs1)
				f->seekg(bufofs1=max(ofs-bufofs1+5,(long)0));
			else
				f->seekg(bufofs1=ofs);
			curofs=bufofs1+bsize1; //ofs+bsize1;
			f->read(buffer1,bsize1);
			f->clear();
//			bufofs1=ofs;
		} // buffer ready
      return *((long*)(buffer1+(unsigned)(ofs-bufofs1)));
   }
   if (curofs!=ofs)
      f->seekg(ofs);
   long l;
   f->clear();
   f->read((char*)&l,4);
   curofs=ofs+4;
   return l;
}

// ----------------------------- PPD ---------------------------------

#define ppd_signat "SGpd\2"

BOOL PPD::open(char *path) {  // argument - the path, return TRUE is sucess
	char fname[127];
	curofs=-1;  // no current offset yet
	for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
	q--;
	if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
		*++q='\\';
		*++q=0;
	}
	strcat(fname,"pulse.ppd"); // adding the file name
#ifndef SHARE
	f = new fstream;
	f->open(fname,ios::in | ios::binary); // open file read-only
	if (!f)
		return FALSE; // failed to open the file
#else
	int handle = sopen(fname, O_RDONLY | O_BINARY, SH_DENYNONE, S_IREAD);
	if (!handle)
		return FALSE;
	f = new fstream(handle);
#endif
	f->seekg(0,ios::end);
	fsz=f->tellg();
   f->seekp(0);
   char buf[6];
   f->clear();
   f->read(buf,5);
   buf[5]=0;
   if (f->fail() || strcmp(buf,ppd_signat)) // failure || wrong file signature
      return FALSE;
   unsigned short dt;
	f->read((char*)&dt,2);      // Database creation date
   dcreate = Date(1,1,1995)+dt; // Number of days since 1.01.95
   f->read((char*)&dt,2);      // Database update date
	dupdate = Date(1,1,1995)+dt;
	f->read(&nupdat,1);          // number of updates
	if (f->fail())
		return FALSE;
	is_open = TRUE;
	fread = TRUE;
	return TRUE;   // success
}

BOOL PPD::create(char */*path*/) {
/*	char fname[127];
   for (char *q=fname; (*q++=*path++)!=0;) ; // copying the string
	q--;
   if (--q!=fname && *q!='\\') { // adding the '\' to the end if not there
      *++q='\\';
      *++q=0;
   }
   strcat(fname,"pulse.ppd"); // adding the file name
   f = new fstream;
   f->open(fname,ios::in | ios::binary | ios::out | ios::trunc);
               // open file for read/write, discard contence
   if (!f)
      return FALSE;
   f->write(ppd_signat,5);  // write the signature
   unsigned short dt = dcreate - Date(1,1,1995); // days since 1.01.95
   f->write((char*)&dt,2);
   f->write((char*)&dt,2); // update date = create date for now
	nupdat=dt=0;
   f->write(&nupdat,1);   // number of updates = 0;
   if (f->fail())
      return FALSE;
   is_open = TRUE;
	fread = FALSE;*/
	return TRUE;
}

long PPD::write() {  // write next element
/*	if (fread || !is_open)
		return 0;
	f->seekp(0,ios::end);
	f->clear();
	long l = f->tellg();
	f->write((char*)&drec.price,4);
	f->write((char*)&drec.fcode,2);
	f->write(&drec.pcode,1);
	char d=dupdate-drec.dsubmit;
	f->write(&d,1);
	char sl=mystrlen(drec.ad);
	drec.ad[sl]=0;  // remove trailing spaces
	StrPack sp;
	char *s=sp.encode(drec.ad);
	f->write(s,s[0]==255?strlen(s)+1:s[0]);
	if (f->fail())
		return 0;
	else
		return l;*/
	return 1;
}

BOOL PPD::read(long ofs,BOOL readad) { // read next element from ofs.
	if (!fread || !is_open)
		return FALSE;
	if (buffer) {  // working with buffer!
		if (bufofs==-1 || ofs<bufofs || bufofs+bsize<ofs+10) { // reposition buffer
			if (ofs!=curofs)
				f->seekg(ofs);
			curofs=ofs+bsize;
			f->read(buffer,bsize);
			f->clear();
			bufofs=ofs;
		} // buffer ready
		char *pos=buffer+(unsigned)(ofs-bufofs);
//      _fmemcpy(&drec.price,pos,4); pos+=4;
		drec.price=*((float*)pos); pos+=4;
//      _fmemcpy(&drec.fcode,pos,2); pos+=2;
		drec.fcode=*((int*)pos); pos+=2;
		drec.pcode=*pos; pos+=1;
/*      char d=*pos; pos+=1;
		drec.dsubmit = dupdate-(int)d; */
		StrPack sp;
		if (readad) {  // read ad string...
			int rr=bsize-(int)(pos-buffer)-1; // remaining data in buffer
			if (*pos==255) {  // string not encoded
				if (rr>160)
					strcpy(drec.ad,++pos);
				else
					for (int i=0; (drec.ad[i++]=*++pos)!=0; rr--)
						if (!rr) { // buffer ended
							f->seekg(curofs=ofs+4+2+1+1+i);
							do {
								f->read(&drec.ad[i],1);
								curofs++;
							} while (drec.ad[i++]);
							if (f->bad()) {
								f->clear();
								curofs=-1;
								return FALSE;
							}
							break;
						}
			} else { // encoded string
				if (*pos<=rr)
					sp.decode(pos,drec.ad); // decode the string
				else {
					char bb[255];
					f->seekg(ofs+4+2+1+1);
					curofs=(bb[0]=*pos)+ofs+4+2+1+1;
					f->read(&bb[1],*pos);
					sp.decode(bb,drec.ad);
					if (f->bad()) {
						f->clear();
						curofs=-1;
						return FALSE;
					}
				}
			}
		}
		return TRUE;   // buffer is always true
	}

	if (curofs!=ofs) {
		f->seekg(ofs);
		curofs=ofs;
	}
	f->clear();
	f->read((char*)&drec.price,4);
	f->read((char*)&drec.fcode,2);
	f->read(&drec.pcode,1);
/*   char d;
	f->read(&d,1);
	curofs+=1+1+2+4; */
	curofs += 1+2+4;
//	drec.dsubmit = dupdate-(int)d;
	StrPack sp;
	if (readad) {
		f->read(&drec.ad[0],1);
		curofs++;
		int i=0;
		if (drec.ad[0]==255)
			do {
				f->read(&drec.ad[i],1);
				curofs++;
			} while (drec.ad[i++]);
		else {
			char bb[255];
			curofs+=(bb[0]=drec.ad[0]);
			f->read(&bb[1],drec.ad[0]);
			sp.decode(bb,drec.ad); // this should be more fast
		}
	}
	if (f->bad()) {
		f->clear();
		curofs=-1;
		return FALSE;
	}
	return TRUE;
}

#else

// if defined BDEBUG

#include <iostream.h>

void main() {

char *str1="TEST1235| 6/546 TE.,/S123T";
char str2[50];
char str3[50];

cout << "->" << str1 << "<- (len: " << strlen(str1) << endl;

StrPack sp;
strcpy(str2,sp.encode(str1));
sp.decode(str2,str3);

cout << "->" << str2 << "<- (len: " << strlen(str2) << endl;
cout << "->" << str3 << "<- (len: " << strlen(str3) << endl;


}

#endif
