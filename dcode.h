// This defines D key processing

#define N 64769l

   // e=28451 = 14,13,11,10,9,8,5,1,0 - степени двойки
   // d=131   = 7,1,0
   // p=1439; q=1187 - primes

inline unsigned DCODE(unsigned NUM) {
	unsigned long n0=(NUM), n=(NUM);
   n%=N; n*=n;  n%=N; n*=n; 
   n%=N; n*=n;  n%=N; n*=n; 
   n%=N; n*=n;  n%=N; n*=n; 
   n%=N; n*=n;  n%=N; n*=n0; 
   n%=N; n*=n0; n%=N; n*=n0; 
   n%=N;
   return (unsigned)n;
}

