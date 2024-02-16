// DO NOT REMOVE THIS MESSAGE. The mess that follows is a compressed build of
// https://github.com/purplesyringa/blazingio. Refer to the repository for
// a human-readable version and documentation.
// Config options: AVX2 LUT CHAR_WITH_SIGN_IS_GLYPH BITSET FLOAT COMPLEX SPLICE INTERACTIVE STDIN_EOF LATE_BINDING CERR
#define L$(x)_mm256_loadu_si256(x)
#define M$(x,...)_mm256_##x##_epi8(__VA_ARGS__)
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
#include<immintrin.h>
#include<sys/mman.h>
#include<unistd.h>
#define $s __attribute__((target("avx2")))
#define $I __attribute__((always_inline))
#define $F M(),
#define E$(x)if(!(x))abort();
namespace $f{using namespace std;struct A{A&$O=(A){$r}};struct B{enum $c A:char{};A c;B&$O=(char x){c=A{x};$r}$O char(){$R(char)c;}};const long C=-1ULL/255;struct D{string&K;};static B E[65568];$T bool F>struct G{B*H;B*I;void K(off_t C){(C+=4095)&=-4096;char*D=(char*)mmap(0,C+4096,1,2,0,0);E$(D!=(void*)-1)E$(~madvise(D,C,22))H=(B*)D+C;E$(mmap(H,4096,3,50,-1,0)!=(void*)-1)H[1]=48;I=(B*)D;}void L(){H=I=E;}$I void M(){if(F&&__builtin_expect(I==H,0)){off_t A=0;B*C=E;asm volatile("syscall;movb $0,(%%rsi,%%rax);":"+a"(A),"+S"(C):"D"(0),"d"(65536):"rcx","r11");I=C;E$(A>=0)H=I+A;if(!A){E[0]=0;E[1]=48;H=0;}}}$T $c T>$I void N(T&x){while($F(*I&240)==48)x=x*10+(*I++-48);}$T $c T,T=1>$I void O(T&x){bool A=is_signed_v<T>&&($F*I==45);I+=A;N(x=0);x=A?-x:x;}$T $c T,$c=decltype(T{1.})>$I void O(T&x){bool A=($F*I==45);I+=A;$F I+=*I==43;uint64_t n=0;int i=0;for(;i<18&&($F*I&240)==48;i++)n=n*10+*I++-48;int B=20;bool C=*I==46;I+=C;for(;i<18&&($F*I&240)==48;i++){n=n*10+*I++-48;B-=C;}x=n;while(($F*I&240)==48){x=n*10+*I++-48;B-=C;}if(*I==46){I++;C=1;}while(($F*I&240)==48){x=n*10+*I++-48;B-=C;}if((*I|32)==101){I++;$F I+=*I==43;int D;O(D);B+=D;}if(0<=B&&B<41){static $C auto D=[]{array<T,41>D{};T x=1;for(int i=21;i--;){D[40-i]=x;D[i]=1/x;x*=10;}$R D;}();x*=D[B];}else{while(B-->20)x*=10;while(++B<20)x*=.1;}x=A?-x:x;}$I void O(bool&x){$F x=*I++==49;}$I void O(char&x){$F x=*I++;}$I void O(uint8_t&x){$F x=*I++;}$I void O(int8_t&x){$F x=*I++;}$s void P(string&K,B*(*C)(B*)){auto G=I;I=C(I);((basic_string<A>&)K).resize(I-G);memcpy(K.data(),G,I-G);while(F&&I==H){if($F!H){H=I;break;}I=C(I);K.append(E,I);}}$s void O(string&K){P(K,[](B*I)$s{auto p=(__m256i*)I;__m256i A,C=M$(set1,32);while(A=M$(cmpeq,C,_mm256_max_epu8(C,L$(p))),_mm256_testz_si256(A,A))p++;$R(B*)p+__builtin_ctz(M$(movemask,A));});}$s void O(D&A){if($F!*I){H=0;$R;}P(A.K,[](B*I)$s{auto p=(__m256i*)I;auto E=_mm_set_epi64x(0xff0000ff0000,255);__m256i A,C,D;while(A=L$(p),_mm256_testz_si256(C=M$(cmpgt,M$(set1,16),A),D=M$(shuffle,_mm256_set_m128i(E,E),A)))p++;$R(B*)p+__builtin_ctz(M$(movemask,C&D));});I+=*I==13;I+=*I==10;}$T $c T>$I void O(complex<T>&K){T A,B{};if($F*I==40){I++;O(A);if($F*I++==44){Q(B);I++;}}else{O(A);}K={A,B};}$T size_t N>$s void O(bitset<N>&K){if(N>=4096&&!*this)$R;ssize_t i=N;while(i){if($F i%32||H-I<32){K[--i]=*I++==49;}else{long a=-1ULL/65025;auto p=(__m256i*)I;for(size_t j=0;j<min(i,H-I)/32;j++){i-=32;((uint32_t*)&K)[i/32]=__bswap_32(M$(movemask,M$(shuffle,L$(p++)<<7,_mm256_set_epi64x(a+C*24,a+C*16,a+C*8,a))));}I=(B*)p;}}}$T $c T>$I void Q(T&K){if(!is_same_v<T,D>)while($F 0<=*I&&*I<=32)I++;O(K);}$O bool(){$R!!*this;}bool $O!(){$R I>H;}};struct $i{off_t C;G<0>A;G<1>B;$i(){C=lseek(0,0,2);~C?A.K(C):B.L();}void*tie(nullptr_t){$R 0;}$T $c T>$I $i&$O>>(T&K){__builtin_expect(~C,1)?A.Q(K):B.Q(K);$r}$O bool(){$R!!*this;}bool $O!(){$R __builtin_expect(~C,1)?!A:!B;}};struct $o{char*D;B*I;inline static char A[200];$o(){D=(char*)mmap(0,0x40000000,3,16418,-1,0);E$(D!=(void*)-1)I=(B*)D;for(int i=0;i<100;i++){A[i*2]=48+i/10;A[i*2+1]=48+i%10;}}~$o(){E();}void E(){auto G=D;ssize_t A=1;while(A>0){iovec B{G,(size_t)I-(size_t)G};G+=(A=vmsplice(1,&B,1,8));}if(A){G++;do{G+=(A=write(1,G,(char*)I-G));}while(A>0);E$(~A)}I=(B*)D;}void F(char K){*I++=K;}void F(uint8_t K){*I++=K;}void F(int8_t K){*I++=K;}void F(bool K){*I++=48+K;}$T $c T,int B,int C,T D=1>void G(T K,T E){if $C(C==1){if(B>=1||K>=D)*I++=48+E;}else if $C(C==2){if(B>=2||K>=10*D)F(A[E*2]);if(B>=1||K>=D)F(A[E*2+1]);}else{$C auto A=[]{int F=1;T H=10;while((F*=2)<C)H*=H;$R pair{F/2,H};}();$C int F=A.first;$C T H=A.second;G<T,max(0,B-F),C-F,T(D*H)>(K,E/H);G<T,min(B,F),F,D>(K,E%H);}}$T $c T,T=1>void F(T K){make_unsigned_t<T>A=K;if(K<0){F('-');A=-A;}G<decltype(A),1,array{3,5,10,20}[__builtin_ctz(sizeof(K))]>(A,A);}$T $c T,$c=decltype(T{1.})>void F(T K){if(K<0){F('-');K=-K;}auto A=[&]{G<uint64_t,12,12>(K*1e12,K*1e12);};if(!K)$R F('0');if(K>=1e16){K*=1e-16;int B=16;while(K>=1){K*=.1;B++;}F("0.");A();F('e');F(B);}else if(K>=1){uint64_t B=K;F(B);if(K-=B){F('.');A();}}else{F("0.");A();}}void F(const char*K){I=(B*)stpcpy((char*)I,K);}void F(const uint8_t*K){F((char*)K);}void F(const int8_t*K){F((char*)K);}void F(string_view K){memcpy(I,K.data(),K.size());I+=K.size();}$T $c T>void F(complex<T>K){*this<<'('<<K.real()<<','<<K.imag()<<')';}$T size_t N>$s void F(const bitset<N>&K){auto i=N;while(i%32)*I++=48+K[--i];auto p=(__m256i*)I;i/=32;auto b=_mm256_set1_epi64x(-3ULL/254);while(i)_mm256_storeu_si256(p++,M$(sub,M$(set1,48),M$(cmpeq,M$(shuffle,_mm256_set1_epi32(((uint32_t*)&K)[--i]),_mm256_set_epi64x(0,C,C*2,C*3))&b,b)));I=(B*)p;}$T $c T>$o&$O<<(const T&K){F(K);$r}$o&$O<<($o&(*A)($o&)){$R A(*this);}};struct $e{$T $c T>$e&$O<<(const T&K){$r}$e&$O<<($e&(*A)($e&)){$R A(*this);}};}namespace std{$f::$i i$;$f::$o o$;$f::$e e$;$f::$i&getline($f::$i&B,string&K){$f::D A{K};$R B>>A;}$f::$o&flush($f::$o&B){if(__builtin_expect(!~i$.C,0))B.E();$R B;}$f::$o&endl($f::$o&B){$R B<<'\n'<<flush;}$f::$e&endl($f::$e&B){$R B;}$f::$e&flush($f::$e&B){$R B;}}
#define freopen(...)if(freopen(__VA_ARGS__)==stdin)std::i$=$f::$i{}
#define cin i$
#define cout o$
#ifdef ONLINE_JUDGE
#define cerr e$
#define clog e$
#endif
// End of blazingio
