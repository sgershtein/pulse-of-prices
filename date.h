//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// definitions of date classes
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 17-Oct-95
//-----------------------------------------------------------------------------

#ifndef _DATE_H_
#define _DATE_H_

typedef char BOOL;

class Date { // for keeping date
   public:
      Date() : d(0), m(0), y(0) {} // default constructor
      Date(int dd,int mm, int yy);
      Date operator+(int); // what date is in n days?
      Date operator-(int i) {
         return *this+(-i);
      }
      int operator-(Date); // how many days between these dates?
      int operator<(Date dat);
      BOOL correct() { return d==0; }
      int day() { return d; }
      int month() { return m; }
      int year() { return y; }
      int yy() { return y%100; }
      void Today();  // set the date to today's date.
   private:
      int d;   // day
      int m;   // month
      int y;   // year   [1974, not just 74!]
};

#endif