#include "parallel.h"

using namespace parlay;

bool debug = 0;

// A serial implementation for checking correctness.
//
// Work = O(n)
// Depth = O(n)
template <class T, class F>
T scan_inplace_serial(T *A, size_t n, const F& f, T id) {
  T cur = id;
  for (size_t i=0; i<n; ++i) {
	T m = std::max(A[i],id);
    T next = f(cur, m);
    A[i] = cur;
    cur = next;
  }
  return cur;
}

// Parallel in-place prefix sums. Your implementation can allocate and
// use an extra n*sizeof(T) bytes of memory.
//
// The work/depth bounds of your implementation should be:
// Work = O(n)
// Depth = O(\log(n))

template <class T, class F>
T scan_up(T *A, size_t n, T *L, const F& f) {
	if (n == 1) return std::max(A[0],0);
	auto mc = n/2 + (n % 2 != 0);
	//printf("mc: %zu\n", mc);
	T l, r;
	auto f1 = [&]() {l = scan_up(A,mc,L,f);};
	auto f2 = [&]() {r = scan_up(A+mc,n-mc,L+mc,f);};
    par_do(f1, f2);
	//printf("l: %llu\nr: %llu\n",l,r);
	L[mc-1] = l;
	return f(l,r);
}

template <class T, class F>
void scan_down(T *R, size_t n, T *L, const F& f,T s) {
	if (n == 1){
		R[0] = s; 
		return;
	}
	auto m = n/2 + (n % 2 != 0);
	auto sprime = f(s,L[m-1]);
	auto f1 = [&]() {scan_down(R,m,L,f,s);};
	auto f2 = [&]() {scan_down(R+m,n-m,L+m,f,sprime);};
    par_do(f1, f2);
	return;
}

template <class T, class F>
T scan_inplace(T *A, size_t n, const F& f, T id) {
	/*
	for (size_t i = 0; i < n; i++){
		printf("%llu ",A[i]);
	}*/
	//printf("\n");	
	T* L = (T*)malloc((n-1) * sizeof(T));
	size_t total = scan_up(A,n,L,f);
	scan_down(A,n,L,f,id);
	free(L);
	return total;  
}
