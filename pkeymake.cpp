#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include "ecode.h"

long DecodeKey(unsigned char key[5]) {
   // decoding
   unsigned char kk[4];
	kk[0]=key[1]; kk[1]=key[2]; kk[2]=key[3]; kk[3]=key[4];

	unsigned *up=(unsigned*)kk;
   *up=ECODE(*up);   up++;
   *up=ECODE(*up);

	// removing solt
	kk[3] ^= ~kk[0];
	kk[2] ^= kk[0];
	kk[1] ^= ~key[0];
	kk[0] ^= key[0];

//   printf("Decoded CIC: %03d%03d-%03d%03d\n",kk[1],kk[2],kk[0],kk[3]);

   long *lp=(long*)kk;

   return *lp;

}

void main() {
   unsigned char key[5];
   cerr << "Reading CICs from standard input, writing KEYs to standard output\n";
   unsigned char line[51];
   randomize();
   while (!cin.eof()) {
      cin.getline(line,50);
      unsigned char *s;
      for(s=line; *s<'0' || *s>'9'; *s++) ;  // finding where the key start
      if (s[3]!='-' || s[10]!='-')     // not a valid key
         continue;
      s[17]='\0';    // chop all the rest
      key[4]=atoi(s+14); s[14]='\0';
      key[1]=atoi(s+11); s[10]='\0';
      key[3]=atoi(s+7); s[7]='\0';
      key[2]=atoi(s+4); s[3]='\0';
      key[0]=atoi(s);

//   printf("Read code: %03d%03d-%03d%03d\n",key[2],key[3],key[1],key[4]);

      unsigned char CIC[4];
      long *pCIC = (long*)CIC;
      *pCIC=DecodeKey(key);

//      printf("Encoding CIC: %03d%03d-%03d%03d\n",CIC[1],CIC[2],CIC[0],CIC[3]);
      unsigned char solt=random(255);

      // adding solt
      CIC[0] ^= solt;
      CIC[1] ^= ~solt;
      CIC[2] ^= CIC[0];
      CIC[3] ^= ~CIC[0]; 

      // encoding
      unsigned *up=(unsigned*)CIC;
      *up=ECODE(*up);   up++;
      *up=ECODE(*up);

      unsigned char ss[50];
      sprintf(ss,"%03d-%03d%03d-%03d%03d",solt,CIC[1],CIC[2],CIC[0],CIC[3]);

      cout << ss << endl;

   }  // while

}
