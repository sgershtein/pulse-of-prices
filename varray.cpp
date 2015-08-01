//-----------------------------------------------------------------------------
// Pulse of Prices
// Virtual Arrays classes
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 12-Oct-95
//-----------------------------------------------------------------------------

#include "varray.h"
#include <io.h>

//#define DEBUG // if defined there's lots of debug output to screen

#ifdef DEBUG
#include <iostream.h>
#endif

//-------------------- VBlockL class implementation ---------------------------

VBlockL::VBlockL(uint bsz) : size(bsz), array(NULL), status(vbEmpty),
   lastofs(-1) {
}

VBlockL::~VBlockL() {
   if (status==vbReady)
      delete[] array;
}

void VBlockL::Clear() {
   if (array)
      delete[] array;
   status=vbEmpty;
   lastofs=-1;
}

void VBlockL::allocate() { // allocate memory for the block if status==vbEmpty
   if (array || status!=vbEmpty)
      return;
   array = new ulong[size];
   status=vbReady;
   lastofs = -1;
#ifdef DEBUG
   cout << "VBlockL::allocate() - " << size << " elements allocated\n";
#endif
}

inline void VBlockL::Access() { // set last access time
//   ulong *time=(ulong*)0x46c; // current time
   tlast=*((ulong*)0x46c);
}

ulong VBlockL::operator()(uint i) {
                                      // get n-th element of the block
   if (status!=vbReady)
      throw VArrayErr(vaBlockFault);   // accessing block which is not ready
   if (i>=size)
      throw VArrayErr(vaIndexRange);  // array index out of range
   Access();
   return array[i];
}

ulong& VBlockL::operator[](uint i) {
                                       // set n-th element of the block
   if (status!=vbReady)
      throw VArrayErr(vaBlockFault);   // accessing block which is not ready
   if (i>=size)
      throw VArrayErr(vaIndexRange);  // array index out of range
   Access();
   lastofs=-1;  // need to re-save before deleting block
   return array[i];
}

void VBlockL::Flush(fstream &f,ulong ofs) {
   // flush block to disk
   if (status!=vbReady)
      return;
   if (lastofs<0 || (ofs*sizeof(*array))!=lastofs) { // block was modified
      lastofs=ofs*sizeof(*array);
      f.seekp(lastofs);
      f.write((char*)array,sizeof(*array)*size);
      if (f.fail()) {
         f.clear();
         throw VArrayErr(vaFWriteFault);   // error writing to file
      }
      #ifdef DEBUG
      cout << "VBlockL::Flush - block flushed to offset " << ofs << endl;
      #endif
   }
   #ifdef DEBUG
     else cout << "VBlockL::Flush - block discarded (wasn't modified)\n";
   #endif
   status = vbSaved;
   delete[] array;
   array = NULL;
}

void VBlockL::Get(fstream &f) {    // get block from disk
   if (status!=vbSaved)
      return;
   if (array!=NULL)  // I don't know how it may happen, but let it be [sg]
      delete[] array;
   array = new unsigned long[size];
   f.seekg(lastofs);
   f.clear();  // we might've been at eof...
   f.read((char*)array,sizeof(*array)*size);
   if (f.fail()) {
      f.clear();
      throw VArrayErr(vaFReadFault);    // error reading from file
   }
   status=vbReady;
   Access();   // set last access time
#ifdef DEBUG
   cout << "VBLockL::Get - block retrieved from offset " << lastofs << endl;
#endif
}

//-------------------- VArrayL class implementation ---------------------------

// Initialization:
//    no - number of elements in the array
//    fn - name of temporary file to create (Must be valid!)
//    bsz - number of elements per data block
//    bm - maximum allowed number of blocks im memory

VArrayL::VArrayL(ulong no,char *fn,uint bsz,uint bm) :
   n(no), fname(fn), bsize(bsz), bmax(bm), fuse(FALSE), bcur(0) {
   bno=(uint)(no/bsize+(n%bsize?1:0));
   if (bsize!=2048)
      throw VArrayErr(vaUnknownErr);   // only block size of 2048 is persimmed
   blk=new VBlockL[bno];
   for (uint i=0; i<bno; i++)
      blk[i].SetSize(bsize);
   if (n%bsize)
      blk[bno-1].SetSize((uint)(n%bsize));  // last block smaller
#ifdef DEBUG
   cout << "VArrayL initialized with " << bno << " blocks\n";
#endif
}

VArrayL::~VArrayL() { // must delete the temp file
   delete[] blk;  // delete all the blocks from memory
   if (fuse)
      FClose();   // close and delete the file
#ifdef DEBUG
   cout << "VArrayL removed\n";
#endif
}

ulong& VArrayL::operator[](ulong i) {
   uint b=(uint)(i>>11); //(i/bsize);
   if (b>=bno)
      throw VArrayErr(vaIndexRange);   // Index out of range
   if (blk[b].Status()==vbSaved) {
      if (bcur==bmax)
         save(1);
#ifdef DEBUG
      cout << "Retrieving block " << b << "...\n";
#endif
      blk[b].Get(f);
      bcur++;
   } else if (blk[b].Status()==vbEmpty) {
      if (bcur==bmax)
         save(1);
      blk[b].allocate();
      bcur++;
   }
   return blk[b][(uint)(/*i%bsize*/i&2047)];
}

ulong VArrayL::operator()(ulong i) {
   uint b=(uint)(i>>11); //(i/bsize);
   if (b>=bno)
      throw VArrayErr(vaIndexRange);   // Index out of range
   if (blk[b].Status()==vbSaved) {
      if (bcur==bmax)
         save(1);
#ifdef DEBUG
      cout << "Retrieving block " << b << "...\n";
#endif
      blk[b].Get(f);
      bcur++;
   } else if (blk[b].Status()==vbEmpty) {
      return 0;
/*      if (bcur==bmax)
         save(1);
      blk[b].allocate();
      bcur++;  */
   }
   return blk[b]((uint)(/*i%bsize*/i&2047));
}

BOOL VArrayL::flush(ulong bt) {
   if (!bt) {
      save(bcur); // flush all blocks
      return TRUE;
   }
   if (bt>bsize*bcur) {
      save(bcur); // flush all blocks, still not enough :(
      return FALSE; // did all we can...
   }
   save((uint)((bt>>11)/*bt/bsize*/+(bt&2047/*bt%bsize*/?1:0)));   // We can do it!
   return TRUE;
}

void VArrayL::FCreate() {   // create file
   f.open(fname,ios::in | ios::out | ios::binary);
   if (!f)  // open failure
      throw VArrayErr(vaFCreateFault);
#ifdef DEBUG
   cout << "VArrayL temp file created\n";
#endif
   fuse=TRUE;
}

void VArrayL::FClose() {    // close and delete the file
   f.close();
   unlink(fname); // delete the file
#ifdef DEBUG
   cout << "VArrayL temp file removed\n";
#endif
   fuse=FALSE;
}


void VArrayL::save(uint k) {  // save k blocks to file
   for (uint i=0; i<k; i++) { // for each block to flush
      if (bcur==0)
         return;
      ulong t=(ulong)(-1); // Max possible number :)
      uint bm=0;
     // looking for a block which has oldest last access time
      for (uint b=0; b<bno; b++)
         if (blk[b].Status()==vbReady && blk[b].LastAccess()<t) {
            t=blk[b].LastAccess();
            bm=b;
         }
      if (!fuse)
         FCreate();
#ifdef DEBUG
      cout << "Flushing block " << bm << "...\n";
#endif
      blk[bm].Flush(f,(ulong)bm*bsize); // flush this block
      bcur--;
   }
}
