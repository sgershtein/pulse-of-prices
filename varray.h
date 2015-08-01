//-----------------------------------------------------------------------------
// Pulse of Prices
// Virtual Arrays header file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 12-Oct-95
//-----------------------------------------------------------------------------

#include <fstream.h>

typedef unsigned long ulong;
typedef unsigned int uint;

typedef char BOOL;
#define TRUE 1
#define FALSE 0

// USES EXCEPTIONS MECHANISM FOR ERROR SIGNALLING!!!!!

enum VA_EXCEPTION_CODES {
   vaUnknownErr=0,   // unknown error
   vaIndexRange,     // array index out of range
   vaFCreateFault,   // failure to create temp file
   vaFWriteFault,    // file write failure
   vaFReadFault,     // file read failure
   vaBlockFault      // trying to access block which is not in memory
};

class VArrayErr { // exception class which may be thrown by VArray
   public:
      VArrayErr(char n) : no(n) {}
      char code() {
         return no;
      }
   private:
      char no; // exception code
};

// virtual block status constants
#define vbEmpty 0
#define vbReady 1
#define vbSaved 2

// IN PERSPECTIVE THESE CLASSES SHOULD ALSO USE XMS IF AVAILABLE

// NOTE THAT [] AND () OPERATORS HAVE HIGHER PRIORITY THAN * !!!
// Hence, the syntax is (*VBlockL)[i], not just *VBlockL[i] !!!

class VBlockL { // block of virtual array (page)
   public:
      VBlockL(uint bsz=0);   // Initialize a block of a given size
      ~VBlockL();
      void allocate();     // allocate memory for the block if status==vbEmpty
      ulong& operator[](uint);
                                 // set n-th element of the block
                                 // STATUS MUST BE vbReady !!!
      ulong operator()(uint);
                                 // get n-th element of the block
                                 // STATUS MUST BE vbReady !!!
/************************** IMPORTANT !!! ********************************
   There is an important difference between the [] and () operators.
   The first one should be used for setting the value to the array
   while the second one can only be used for getting data.  If the
   block was already saved to disk once and since then [] operator was not
   called, the block does not need to be saved again before reusing it!!
   Great savings of the time!!!
**************************************************************************/
      char Status() { return status; }    // current status
      ulong LastAccess() { return tlast; }  // last access time
      void Flush(fstream &f,ulong ofs); // flush block to disk
      void Get(fstream &f); // get block from disk
      void Clear();  // clear the block (discard everything in it)
                     // status becomes vbEmpty
      void SetSize(uint sz) {    // change block size
         if (status==vbEmpty)
            size=sz;
      }
   private:
      long lastofs; // offset of last save. if non-negative then no changes
                     // were made since last save
      ulong tlast;   // last access time
      char status;   // vbEmpty, vbReady, vbSaved
      uint size;
      ulong *array;  // array of data;
      inline void Access(); // sets tlast to current time
};

class VArrayL { // virtual array of long elements
   public:
      // Initialization:
      //    no - number of elements in the array
      //    fn - name of temporary file to create (Must be valid!)
      //    bsz - number of elements per data block
      //    bm - maximum allowed number of blocks im memory
      VArrayL(ulong no,char *fn,uint bsz,uint bm);
      ~VArrayL(); // must delete the temp file
      ulong& operator[](ulong);
      ulong operator()(ulong);
/************************** IMPORTANT !!! ********************************
   There is an important difference between the [] and () operators.
   The first one should be used for setting the value to the array
   while the second one can only be used for getting data.  If the
   block was already saved to disk once and since then [] operator was not
   called, the block does not need to be saved again before reusing it!!
   Great savings of the time!!!
**************************************************************************/
      void SetBMax(uint bm) { // set new max number of blocks in memory
         bmax = bm;
         if (bcur>bmax)
            save(bcur-bmax);  //  flush();

      }
      uint BMax() { return bmax; }
      BOOL flush(ulong bt=0); // try to free up bt bytes memory by flushing
                              // to disk.  Return TRUE if succeeded.
                              // If bt==0 then flush everything.
   private:
      ulong n;       // number of elements
      char *fname;   // temp file name
      uint  bsize;   // number of elements per block
      uint  bmax;    // maximum number of block in memory
      uint  bcur;    // current number of blocks in memory
      uint  bno;     // number of blocks in blk[]
      BOOL  fuse;    // Is temp file already created and open?
      VBlockL *blk;  // array of data blocks
      void FCreate();   // create file
      void FClose();    // close and delete the file
      void save(uint k);  // save k blocks to file
      fstream f;
};