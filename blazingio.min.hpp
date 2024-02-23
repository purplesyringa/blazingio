// DO NOT REMOVE THIS MESSAGE. The mess that follows is a minified build of
// https://github.com/purplesyringa/blazingio. Refer to the repository for
// a human-readable version and documentation.
// Options: cbfoiedrhWLMXaIaAn
#define L$(x)_mm256_loadu_si256(x)
#define M$(x,...)_mm256_##x##_epi8(__VA_ARGS__)
#define $u(...)__VA_ARGS__
#if __APPLE__
#define $m(A,B)A
#else
#define $m(A,B)B
#endif
#if _WIN32
#define $w(A,B)A
#else
#define $w(A,B)B
#endif
#if __i386__|_M_i386
#define $H(A,B)A
#else
#define $H(A,B)B
#endif
#if __x86_64__|__i386__|_M_X64|_M_I386
#define $S(A,B)A
#else
#define $S(A,B)B
#endif
#define $T template<
#define $c class
#define $C constexpr
#define $R return
#define $O operator
#define $r $R*this;
#include<array>
#include<bitset>
#include<complex>
#include<cstring>
#include $S(<immintrin.h>,<arm_neon.h>)
#include<stdint.h>
#include $w(<windows.h>,<sys/mman.h>)
#include<sys/stat.h>
#include $w(<io.h>,<unistd.h>)
#ifdef _MSC_VER
#include<__msvc_int128.hpp>
using $Z=_Signed128;
#define $s
#else
#define $Z __int128
#define $s $S(__attribute__((target("avx2"))),)
#endif
#define $z $S(32,16)
#define $t $S(__m256i,uint8x16_t)
#define $I $w(__forceinline,__attribute__((always_inline)))
#define $F M(),
#define E$(x)if(!(x))abort();
$w(LONG WINAPI $x(_EXCEPTION_POINTERS*);,)namespace $f{using namespace std;struct B{enum $c A:char{};A c;B&$O=(char x){c=A{x};$r}$O char(){$R(char)c;}};const uint64_t C=~0ULL/255;struct D{string&K;};static B E[65568];$T bool F>struct G{B*H;B*I;void K(off_t C){$w(char*D=(char*)VirtualAlloc(0,(C+8191)&-4096,8192,1);E$(D)E$(VirtualFree(D,0,32768))DWORD A=(C+4095)&-65536;if(A)E$(MapViewOfFileEx(CreateFileMapping(GetStdHandle(-10),0,2,0,A,0),4,0,0,0,D)==D)E$(VirtualAlloc(D+A,65536,12288,4)==D+A)E$(~_lseek(0,A,0))DWORD E=0;ReadFile(GetStdHandle(-10),D+A,65536,&E,0);,char*D=(char*)mmap(0,C+4096,3,2,0,0);E$(D!=(void*)-1)E$(mmap(D+((C+4095)&-4096),4096,3,$m(4114,50),-1,0)!=(void*)-1))H=(B*)D+C;*H=10;H[1]=48;I=(B*)D;}void L(){H=I=E;}$I void M(){if(F&&I==H){$w(DWORD A=0;ReadFile(GetStdHandle(-10),I=E,65536,&A,0);,$S(off_t A=$H(3,$m($S(33554435,3),$S(0,63)));B*D=E;asm volatile($H("int $128","syscall"):"+a"(A),$H("+c"(D):"b","+S"(D):"D")(0),"d"(65536)$H(,$u(:"rcx","r11")));I=D;,$u(register long A asm("x0")=0,D asm("x1")=(long)E,G asm("x2")=65536,C asm($m("x16","x8"))=$H(3,$m($S(33554435,3),$S(0,63)));asm volatile("svc 0"$m("x80",):"+r"(A),"+r"(D):"r"(C),"r"(G));I=(B*)D;)))H=I+A;*H=10;if(!A)E[1]=48,H=0;}}$T $c T>$I void N(T&x){while($F(*I&240)==48)x=x*10+(*I++-48);}$T $c T>$I enable_if_t<is_integral_v<T>>O(T&x){bool A=is_signed_v<T>&&($F*I==45);I+=A;N(x=0);x=A?1+~x:x;}$T $c T>$I decltype((void)T{1.})O(T&x){bool A=($F*I==45);I+=A;$F I+=*I==43;uint64_t n=0;int i=0;for(;i<18&&($F*I&240)==48;i++)n=n*10+*I++-48;int B=20;bool C=*I==46;I+=C;for(;i<18&&($F*I&240)==48;i++)n=n*10+*I++-48,B-=C;x=(T)n;while(($F*I&240)==48)x=x*10+*I++-48,B-=C;if(*I==46)I++,C=1;while(($F*I&240)==48)x=x*10+*I++-48,B-=C;int D;if((*I|32)==101)I++,$F I+=*I==43,O(D),B+=D;static $C auto E=[](){array<T,41>E{};T x=1;for(int i=21;i--;)E[40-i]=x,E[i]=1/x,x*=10;$R E;}();if(0<=B&&B<41)x*=E[B];else{while(B-->20)x*=10;while(++B<20)x*=.1;}x=A?-x:x;}$I void O(bool&x){$F x=*I++==49;}$I void O(char&x){$F x=*I++;}$I void O(uint8_t&x){$F x=*I++;}$I void O(int8_t&x){$F x=*I++;}$T $c T>$s void P(string&K,T C){M();auto G=I;C();K.assign((const char*)G,I-G);while(F&&I==H){if($F!H){H=I;break;}C();K.append(E,I);}}$s void O(string&K){P(K,[&]()$s{$t*p=($t*)I;$w(ULONG index;,)$S($u(int J;__m256i C=M$(set1,32);while(!(J=M$(movemask,M$(cmpeq,C,_mm256_max_epu8(C,L$(p))))))p++;I=(B*)p+$w((_BitScanForward(&index,J),index),__builtin_ctz(J));),$u(uint64x2_t A;while(A=(uint64x2_t)(*p<=32),!(A[0]|A[1]))p++;I=(B*)p+(A[0]?0:8)+$w((_BitScanForward64(&index,A[0]?:A[1]),index),__builtin_ctzll(A[0]?:A[1]))/8;))});}$s void O(D&A){P(A.K,[&](){I=(B*)memchr(I,10,H-I+1);});if(A.K.size()&&A.K.back()==13)A.K.pop_back();if(A.K.empty()||I<H)I+=*I==10;}$T $c T>$I void O(complex<T>&K){T A,B{};if($F*I==40){I++;O(A);if($F*I++==44)Q(B),I++;}else O(A);K={A,B};}$T size_t N>$s void O(bitset<N>&K){if(N>=4096&&!*this)$R;ptrdiff_t i=N;while(i)if($F i%$z||H-I<$z)K[--i]=*I++==49;else{auto p=($t*)I;for(int64_t j=0;j<min(i,H-I)/$z;j++){i-=$z;$S(uint64_t a=~0ULL/65025;((uint32_t*)&K)[i/32]=$w(_byteswap_ulong,__builtin_bswap32)(M$(movemask,M$(shuffle,_mm256_slli_epi32(L$(p++),7),_mm256_set_epi64x(a+C*24,a+C*16,a+C*8,a))));,$u(auto B=(uint8x16_t)vdupq_n_u64(~2ULL/254)&(48-*p++);auto C=vzip_u8(vget_high_u8(B),vget_low_u8(B));((uint16_t*)&K)[i/16]=vaddvq_u16((uint16x8_t)vcombine_u8(C.val[0],C.val[1]));))}I=(B*)p;}}$T $c T>$I void Q(T&K){if(!is_same_v<T,D>)while($F 0<=*I&&*I<=32)I++;O(K);}$O bool(){$R!!*this;}bool $O!(){$R I>H;}};struct $i{G<0>A;G<1>B;$i(){struct stat D;E$(~fstat(0,&D))(D.st_mode>>12)==8?A.K(D.st_size):B.L();}void*tie(nullptr_t){$R 0;}$T $c T>$I $i&$O>>(T&K){A.I?A.Q(K):B.Q(K);$r}$O bool(){$R!!*this;}bool $O!(){$R A.I?!A:!B;}};uint16_t A[100];char L[64]{1};struct
$o{char*D;B*I;$o(){$w(E$(D=(char*)VirtualAlloc(0,536870912,8192,4))E$(VirtualAlloc(D,4096,4096,260))AddVectoredExceptionHandler(1,$x);,D=(char*)mmap(0,536870912,3,$m(4162,16418),-1,0);E$(D!=(void*)-1))I=(B*)D;for(int i=0;i<100;i++)A[i]=(48+i/10)|((48+i%10)<<8);for(int i=1;i<64;i++)L[i]=L[i-1]+(0x8922489224892249>>i&1);}~$o(){flush(1);}void flush(int F=0){$w(auto E=GetStdHandle(-11);auto C=F?ReOpenFile(E,1073741824,7,2684354560):(void*)-1;DWORD A;E$(C==(void*)-1?WriteFile(E,D,DWORD((char*)I-D),&A,0):(WriteFile(C,D,DWORD(((char*)I-D+4095)&-4096),&A,0)&&~_chsize(1,int((char*)I-D)))),auto G=D;ssize_t A;while((A=write(1,G,(char*)I-G))>0)G+=A;E$(~A))I=(B*)D;}void F(char K){*I++=K;}void F(uint8_t K){*I++=K;}void F(int8_t K){*I++=K;}void F(bool K){*I++=48+K;}$T $c T>enable_if_t<is_integral_v<T>>F(T K){using D=make_unsigned_t<T>;D C=K;if(K<0)F('-'),C=1+~C;static $C auto N=[](){array<D,5*sizeof(T)/2>N{};D n=1;for(size_t i=1;i<N.size();i++)n*=10,N[i]=n;$R N;}();$w(ULONG M;,)int G=L[$w((_BitScanReverse64(&M,C|1),M),63^__builtin_clzll(C|1))];G-=C<N[G-1];uint16_t H[20];if $C(sizeof(T)==2){auto n=33555U*C-C/2;uint64_t H=A[n>>25];n=(n&33554431)*25;H|=A[n>>23]<<16;H|=uint64_t(48+(((n&8388607)*5)>>22))<<32;H>>=40-G*8;memcpy(I,&H,8);}else if $C(sizeof(T)==4){$H($u(auto n=((1441151881ULL*C)>>25)+1;for(int i=0;i<5;i++)H[i]=A[n>>32],n=(n&~0U)*100;),$u(auto n=1441151881ULL*C;int K=57;auto J=~0ULL>>7;for(int i=0;i<5;i++)H[i]=A[n>>K],n=(n&J)*25,K-=2,J>>=2;))memcpy(I,(B*)H+10-G,16);}else{$H($u(if(C<(1ULL<<32)){$R F((uint32_t)C);}uint64_t J=1e10;auto x=C/J,y=C%J;int K=1e5,b[]{int(x/K),int(x%K),int(y/K),int(y%K)};B H[40];for(int i=0;i<4;i++){uint32_t n=((429497ULL*b[i])>>7)+1;B*p=H+i*5;*p=48+(n>>25);n=(n&(~0U>>7))*25;memcpy(p+1,A+(n>>23),2);memcpy(p+3,A+(((n&(~0U>>9))*25)>>21),2);}),$u(auto n=$Z{18}*C+(($Z{8240973594166534376}*C)>>64)+1;for(int i=0;i<10;i++)H[i]=A[int(n>>64)],n=(n&~0ULL)*100;))memcpy(I,(B*)H+20-G,20);}I+=G;}$T $c T>decltype((void)T{1.})F(T K){if(K<0)F('-'),K=-K;auto G=[&](){auto x=uint64_t(K*1e12);$H($u(uint32_t n[]{uint32_t((x/1000000*429497)>>7)+1,uint32_t((x%1000000*429497)>>7)+1};int K=25,J=~0U>>7;for(int i=0;i<3;i++){for(int j=0;j<2;j++)memcpy(I+i*2+j*6,A+(n[j]>>K),2),n[j]=(n[j]&J)*25;K-=2;J>>=2;}I+=12;),$u(auto n=($Z{472236648287}*x>>8)+1;for(int i=0;i<6;i++)memcpy(I,A+int(n>>64),2),I+=2,n=(n&~0ULL)*100;))};if(!K)$R F('0');if(K>=1e16){K*=1e-16;int B=16;while(K>=1)K*=.1,B++;F("0.");G();F('e');F(B);}else if(K>=1){auto B=(uint64_t)K;F(B);if(K-=B)F('.'),G();}else F("0."),G();}void F(const char*K){$w(size_t A=strlen(K);memcpy((char*)I,K,A);I+=A;,I=(B*)stpcpy((char*)I,K);)}void F(const uint8_t*K){F((char*)K);}void F(const int8_t*K){F((char*)K);}void F(string_view K){memcpy(I,K.data(),K.size());I+=K.size();}$T $c T>void F(complex<T>K){*this<<'('<<K.real()<<','<<K.imag()<<')';}$T size_t N>$s void F(const bitset<N>&K){auto i=N;while(i%$z)*I++=48+K[--i];auto p=($t*)I;i/=$z;while(i){$S(auto b=_mm256_set1_epi64x(~2ULL/254);_mm256_storeu_si256(p++,M$(sub,M$(set1,48),M$(cmpeq,_mm256_and_si256(M$(shuffle,_mm256_set1_epi32(((uint32_t*)&K)[--i]),_mm256_set_epi64x(0,C,C*2,C*3)),b),b)));,auto A=(uint8x8_t)vdup_n_u16(((uint16_t*)&K)[--i]);*p++=48-vtstq_u8(vcombine_u8(vuzp2_u8(A,A),vuzp1_u8(A,A)),(uint8x16_t)vdupq_n_u64(~2ULL/254));)}I=(B*)p;}$T $c T>$o&$O<<(const T&K){F(K);$r}$o&$O<<($o&(*A)($o&)){$R A(*this);}};struct $e{$T $c T>$e&$O<<(const T&K){$r}$e&$O<<($e&(*A)($e&)){$R A(*this);}};}namespace std{$f::$i i$;$f::$o o$;$f::$e e$;$f::$i&getline($f::$i&B,string&K){$f::D A{K};$R B>>A;}$f::$o&flush($f::$o&B){if(!i$.A.I)B.flush();$R B;}$f::$o&endl($f::$o&B){$R B<<'\n'<<flush;}$f::$e&endl($f::$e&B){$R B;}$f::$e&flush($f::$e&B){$R B;}}$w(LONG WINAPI $x(_EXCEPTION_POINTERS*A){auto C=A->ExceptionRecord;char*B=(char*)C->ExceptionInformation[1];if(C->ExceptionCode==2147483649&&(size_t)(B-std::o$.D)<0x40000000){E$(VirtualAlloc(B,16777216,4096,4)&&VirtualAlloc(B+16777216,4096,4096,260))$R-1;}$R 0;},)
#define freopen(...)if(freopen(__VA_ARGS__)==stdin)std::i$=$f::$i{}
#define cin i$
#define cout o$
#ifdef ONLINE_JUDGE
#define cerr e$
#define clog e$
#endif
// End of blazingio
