//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// date classes implementation
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 17-Oct-95
//-----------------------------------------------------------------------------

#include "date.h"
#include <dos.h>

// number of days in a month
char mdays[]={0,31,28,31,30,31,30,31,31,30,31,30,31};

Date::Date(int dd,int mm, int yy) : d(0),m(0),y(0) {
   if (mm<1 || mm>12) return;
   if (yy%4==0)
      mdays[2]=29;
   else
      mdays[2]=28;
   if (dd<1 || (dd>mdays[mm])) return;
   d=dd;
   y=yy;
   m=mm;
}

Date Date::operator+(int n) { // what date is in n days?
   int d1=d,m1=m,y1=y;
   int dy = n/365;            // n/4 - number of leap years;
   int dd = n%365-n/365/4;    // n/365/4 - number of leap-years
   if ((y+(dy%4))/4!=y/4 && (y+dy)%4!=0)
      dd--;                   // one more leap-year
   if (y%4==0 && m<=2)
      dd--;                   // this is already a leap-year
   y1 += dy;
   if (y1%4==0)
      mdays[2]=29;
   else
      mdays[2]=28;
   d1 += dd;
   while (d1<=0) {             // normalization
      d1+=mdays[--m1];
      if (m1==0) {
         y1--;
         if (y1%4==0)
            mdays[2]=29;
         else
            mdays[2]=28;
         m1=12;
      }
   }
   while (d1>mdays[m1]) {
      d1-=mdays[m1++];
      if (m1==13) {
         y1++;
         if (y1%4==0)
            mdays[2]=29;
         else
            mdays[2]=28;
         m1=1;
      }
   }
   return Date(d1,m1,y1);
}

int Date::operator<(Date dat) {
   return (y<dat.y) ||
          (y==dat.y && m<dat.m) ||
          (y==dat.y && m==dat.m && d<dat.d);
}

int Date::operator-(Date dat) { // how many days between these dates?
   if (*this<dat)
      return -(dat-*this);
   int dy=y-dat.y;
   int i, dd1=d, dd2=dat.d;
   if (y%4==0)
      mdays[2]=29;
   else
      mdays[2]=28;
   for (i=m-1; i>0; i--)
      dd1+=mdays[i];
   if (dat.y%4==0)
      mdays[2]=29;
   else
      mdays[2]=28;
   for (i=dat.m-1; i>0; i--)
      dd2+=mdays[i];
   int dd;
   if (dd2>dd1) {
      dy--;
      if (dat.y%4!=0 && dat.m<2)
         dd2++;
      dd2=365-dd2;
      dd=dd1+dd2;
   } else
      dd = dd1-dd2;
   dd+=dy*365+(dy/4);
   if ((y/4!=(y-(dy%4))/4) && y%4 && dat.y%4)
      dd++;
   return dd;
}

void Date::Today() { // set the date to today's date
   struct date dat;
   getdate(&dat);
   y=dat.da_year;
   d=dat.da_day;
   m=dat.da_mon;
}
