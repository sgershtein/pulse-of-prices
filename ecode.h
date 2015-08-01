// This defines E key processing

#define N 64769l

   // e=28451 = 14,13,11,10,9,8,5,1,0 - степени двойки
   // d=131   = 7,1,0
   // p=1439; q=1187 - primes

inline unsigned ECODE(unsigned NUM) {
   unsigned long n0=NUM;
   unsigned long n=n0, nres=n0; // 0
   nres%=N; n*=n; n%=N; nres*=n; // 1
   n*=n; n%=N; // 2
   n*=n; n%=N; // 3
   n*=n; n%=N; // 4
   nres%=N; n*=n; n%=N; nres*=n; // 5
   n*=n; n%=N; // 6
   n*=n; n%=N; // 7
   nres%=N; n*=n; n%=N; nres*=n; // 8
   nres%=N; n*=n; n%=N; nres*=n; // 9
   nres%=N; n*=n; n%=N; nres*=n; // 10
   nres%=N; n*=n; n%=N; nres*=n; // 11
   n*=n; n%=N; // 12
   nres%=N; n*=n; n%=N; nres*=n; // 13
   nres%=N; n*=n; n%=N; nres*=n; // 14
   nres%=N;
   return (unsigned)nres;
}
