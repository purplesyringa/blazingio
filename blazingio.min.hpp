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
#define $s __attribute__((target("avx2")))
#define $I __attribute__((always_inline))
#define $F Z(),
#define E$(x)if(!(x))abort();
namespace $f{using namespace std;struct A{A&$O=(A){$r}};struct B{enum $c q:char{};q c;B&$O=(char x){c=q{x};$r}$O char(){$R(char)c;}};const long k=68719476736,Z=-1ULL/255;struct b{std::string&O;};static B C[65568];$T bool w>struct q{B*l;B*K;void E(off_t M){(M+=4095)&=-4096;char*N=(char*)mmap(0,M+4096,1,2,0,0);E$(N!=(void*)-1)E$(~madvise(N,M,22))l=(B*)N+M;E$(mmap(l,4096,3,50,-1,0)!=(void*)-1)l[1]=48;K=(B*)N;}void L(){l=K=C;}$I void Z(){if(w&&__builtin_expect(K==l,0)){off_t y=0;B*k=C;asm volatile("syscall;movb $0,(%%rsi,%%rax);":"+a"(y),"+S"(k):"D"(0),"d"(65536):"rcx","r11");E$(y>=0)K=k;l=K+y;if(!y){C[0]=0;C[1]=48;l=0;}}}$T $c T>$I void H(T&x){while($F(*K&240)==48)x=x*10+(*K++-48);}$T $c T,T=1>$I void u(T&x){bool l=is_signed_v<T>&&($F*K==45);K+=l;H(x=0);x=l?-x:x;}$T $c T,$c=decltype(T{1.})>$I void u(T&x){bool l=($F*K==45);K+=l;$F K+=*K==43;uint64_t n=0;int i=0;for(;i<18&&($F*K&240)==48;i++)n=n*10+*K++-48;int f=20;bool o=*K==46;K+=o;for(;i<18&&($F*K&240)==48;i++){n=n*10+*K++-48;f-=o;}x=n;while(($F*K&240)==48){x=n*10+*K++-48;f-=o;}if(*K==46){K++;o=1;}while(($F*K&240)==48){x=n*10+*K++-48;f-=o;}if((*K|32)==101){K++;$F K+=*K==43;int k;u(k);f+=k;}if(0<=f&&f<41){static $C auto m=[]{array<T,41>m{};T x=1;for(int i=21;i--;){m[40-i]=x;m[i]=1/x;x*=10;}$R m;}();x*=m[f];}else{while(f-->20)x*=10;while(++f<20)x*=.1;}x=l?-x:x;}$I void u(bool&x){$F x=*K++==49;}$I void u(char&x){$F x=*K++;}$I void u(uint8_t&x){$F x=*K++;}$I void u(int8_t&x){$F x=*K++;}$s void o(){auto p=(__m256i*)K;__m256i s,D=M$(set1,32);while(s=M$(cmpeq,D,_mm256_max_epu8(D,L$(p))),_mm256_testz_si256(s,s))p++;K=(B*)p+__builtin_ctz(M$(movemask,s));}$s void m(){auto p=(__m256i*)K;auto t=_mm_set_epi64x(0xff0000ff0000,255);__m256i s,q,r;while(s=L$(p),_mm256_testz_si256(q=M$(cmpgt,M$(set1,16),s),r=M$(shuffle,_mm256_set_m128i(t,t),s)))p++;K=(B*)p+__builtin_ctz(M$(movemask,q&r));}$s void k(string&O,void(q::*t)()){auto d=K;(this->*t)();((basic_string<A>&)O).resize(K-d);memcpy(O.data(),d,K-d);while(w&&K==l){Z();if(!l){l=K;break;}(this->*t)();O.append(C,K);}}$s void u(string&O){k(O,&q::o);}$s void u(b&e){if($F!*K){l=0;$R;}k(e.O,&q::m);K+=*K==13;K+=*K==10;}$T $c T>$I void u(complex<T>&O){T w,q{};if($F*K==40){K++;u(w);if($F*K++==44){t(q);K++;}}else{u(w);}O={w,q};}$T size_t N>$s void u(bitset<N>&O){if(N>=4096&&!*this)$R;ssize_t i=N;while(i){Z();if(i%32||l-K<32){O[--i]=*K++==49;}else{long a=-1ULL/65025;auto p=(__m256i*)K;for(size_t j=0;j<min(i,l-K)/32;j++){i-=32;((uint32_t*)&O)[i/32]=__bswap_32(M$(movemask,M$(shuffle,L$(p++)<<7,_mm256_set_epi64x(a+Z*24,a+Z*16,a+Z*8,a))));}K=(B*)p;}}}$T $c T>$I void t(T&O){if(!is_same_v<T,b>)while($F 0<=*K&&*K<=32)K++;u(O);}$O bool(){$R!!*this;}bool $O!(){$R K>l;}};struct $i{off_t M;q<0>k;q<1>w;$i(){M=lseek(0,0,2);~M?k.E(M):w.L();}void*tie(nullptr_t){$R 0;}$T $c T>$I $i&$O>>(T&O){__builtin_expect(~M,1)?k.t(O):w.t(O);$r}$O bool(){$R!!*this;}bool $O!(){$R __builtin_expect(~M,1)?!k:!w;}};struct $o{char*N;B*K;inline static char V[200];$o(){N=(char*)mmap(0,k,3,16418,-1,0);E$(N!=(void*)-1)K=(B*)N;for(int i=0;i<100;i++){V[i*2]=48+i/10;V[i*2+1]=48+i%10;}}~$o(){G();}void G(){auto d=N;ssize_t k=1;while(k>0){iovec v{d,(size_t)K-(size_t)d};d+=(k=vmsplice(1,&v,1,8));}if(k){d++;do{d+=(k=write(1,d,(char*)K-d));}while(k>0);E$(~k)}K=(B*)N;}void F(char O){*K++=O;}void F(uint8_t O){*K++=O;}void F(int8_t O){*K++=O;}void F(bool O){*K++=48+O;}$T $c T,int S,int U,T h=1>void W(T O,T R){if $C(U==1){if(S>=1||O>=h)*K++=48+R;}else if $C(U==2){if(S>=2||O>=10*h)F(V[R*2]);if(S>=1||O>=h)F(V[R*2+1]);}else{$C auto j=[]{int g=1;T Q=10;while((g*=2)<U)Q*=Q;$R pair{g/2,Q};}();$C int g=j.first;$C T Q=j.second;W<T,max(0,S-g),U-g,T(h*Q)>(O,R/Q);W<T,min(S,g),g,h>(O,R%Q);}}$T $c T,T=1>void F(T O){make_unsigned_t<T>P=O;if(O<0){F('-');P=-P;}W<decltype(P),1,array{3,5,10,20}[__builtin_ctz(sizeof(O))]>(P,P);}$T $c T,$c=decltype(T{1.})>void F(T O){if(O<0){F('-');O=-O;}auto w=[&]{W<uint64_t,12,12>(O*1e12,O*1e12);};if(!O)$R F('0');if(O>=1e16){O*=1e-16;int f=16;while(O>=1){O*=.1;f++;}F("0.");w();F('e');F(f);}else if(O>=1){uint64_t X=O;F(X);if(O-=X){F('.');w();}}else{F("0.");w();}}void F(const char*O){K=(B*)stpcpy((char*)K,O);}void F(const uint8_t*O){F((char*)O);}void F(const int8_t*O){F((char*)O);}void F(string_view O){memcpy(K,O.data(),O.size());K+=O.size();}$T $c T>void F(complex<T>O){*this<<'('<<O.real()<<','<<O.imag()<<')';}$T size_t N>$s void F(const bitset<N>&O){auto i=N;while(i%32)*K++=48+O[--i];auto p=(__m256i*)K;i/=32;auto b=_mm256_set1_epi64x(-3ULL/254);while(i)_mm256_storeu_si256(p++,M$(sub,M$(set1,48),M$(cmpeq,M$(shuffle,_mm256_set1_epi32(((uint32_t*)&O)[--i]),_mm256_set_epi64x(0,Z,Z*2,Z*3))&b,b)));K=(B*)p;}$T $c T>$o&$O<<(const T&O){F(O);$r}$o&$O<<($o&(*Y)($o&)){$R Y(*this);}};struct $e{$T $c T>$e&$O<<(const T&O){$r}$e&$O<<($e&(*Y)($e&)){$R Y(*this);}};}namespace std{$f::$i i$;$f::$o o$;$f::$e e$;$f::$i&getline($f::$i&J,string&O){$f::b e{O};$R J>>e;}$f::$o&flush($f::$o&J){if(__builtin_expect(!~i$.M,0))J.G();$R J;}$f::$o&endl($f::$o&J){$R J<<'\n'<<flush;}$f::$e&endl($f::$e&J){$R J;}$f::$e&flush($f::$e&J){$R J;}}
#define freopen(...)if(freopen(__VA_ARGS__)==stdin)std::i$=$f::$i{}
#define cin i$
#define cout o$
#ifdef ONLINE_JUDGE
#define cerr e$
#define clog e$
#endif
// End of blazingio
