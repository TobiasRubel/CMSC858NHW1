#include "parallel.h"
#include "random.h"


// For timing parts of your code.
#include "get_time.h"

// For computing sqrt(n)
#include <math.h>



using namespace parlay;

// Some helpful utilities
namespace {

// returns the log base 2 rounded up (works on ints or longs or unsigned
// versions)
template <class T>
size_t log2_up(T i) {
  assert(i > 0);
  size_t a = 0;
  T b = i - 1;
  while (b > 0) {
    b = b >> 1;
    a++;
  }
  return a;
}

}  // namespace


struct ListNode {
  ListNode* next;
  size_t rank;
  ListNode(ListNode* next) : next(next), rank(std::numeric_limits<size_t>::max()) {}
};

// Serial List Ranking. The rank of a node is its distance from the
// tail of the list. The tail is the node with `next` field nullptr.
//
// The work/depth bounds are:
// Work = O(n)
// Depth = O(n)
void SerialListRanking(ListNode* head) {
  size_t ctr = 0;
  ListNode* save = head;
  while (head != nullptr) {
    head = head->next;
    ++ctr;
  }
  head = save;
  --ctr;  // last node is distance 0
  while (head != nullptr) {
    head->rank = ctr;
    head = head->next;
    --ctr;
  }

}

// Wyllie's List Ranking. Based on pointer-jumping.
//
// The work/depth bounds of your implementation should be:
// Work = O(n*\log(n))
// Depth = O(\log^2(n))
void WyllieListRanking(ListNode* L, size_t n) {
	//initialize D, successor arrays
	size_t *D = (size_t*)malloc(n*sizeof(size_t));
	size_t *Dprime = (size_t*)malloc(n*sizeof(size_t));
  	ListNode** succ = (ListNode**)malloc(n * sizeof(ListNode*));
  	ListNode** succprime = (ListNode**)malloc(n * sizeof(ListNode*));
	auto v1 = [&](size_t i){
		succ[i] = L[i].next;
		succprime[i] = succ[i];
		(L[i].next == nullptr) ? D[i] = 0 : D[i] = 1;
		Dprime[i] = D[i];
		//using rank as an id
		L[i].rank = i;
	};
	parallel_for(0,n,v1);
	/*
	printf("L: ");
	for (size_t i = 0; i < n; i++) {
		printf("%zu ", L[i].rank);
	}
	printf("\n");
	printf("D: ");
	for (size_t i = 0; i < n; i++) {
		printf("%zu ", D[i]);
	}
	printf("\n");
	printf("D': ");
	for (size_t i = 0; i < n; i++) {
		printf("%zu ", Dprime[i]);
	}
	printf("\n");

	printf("succ: ");
	for (size_t i = 0; i < n; i++) {
		(succ[i] != nullptr)? printf("%zu ", succ[i]->rank): printf("N ");
	}
	printf("\n");
	printf("succ': ");
	for (size_t i = 0; i < n; i++) {
		(succprime[i] != nullptr)? printf("%zu ", succprime[i]->rank): printf("N ");
	}
	printf("\n");
	*/
	auto v2 = [&](size_t i) { 
		if (succ[i] != nullptr) {
			Dprime[i] = D[i] + D[succ[i]->rank];
			succprime[i] = succ[succ[i]->rank];
		} else {
			succprime[i] = succ[i];
			Dprime[i] = D[i];
		}
	};
	size_t maxiter = log2_up(n);
	for (size_t k = 0; k < maxiter; k++) {
		parallel_for(0,n,v2);
		std::swap(succ,succprime); 
		std::swap(D,Dprime);
		//ughhhh debug stuff
	/*
	printf("k: %zu\n",k);
	
	printf("\n");

	printf("D: ");
	for (size_t i = 0; i < n; i++) {
		printf("%zu ", D[i]);
	}
	printf("\n");
	printf("D': ");
	for (size_t i = 0; i < n; i++) {
		printf("%zu ", Dprime[i]);
	}
	printf("\n");


	printf("succ: ");
	for (size_t i = 0; i < n; i++) {
		(succ[i] != nullptr)? printf("%zu ", succ[i]->rank): printf("N ");
	}
	printf("\n");
	printf("succ': ");
	for (size_t i = 0; i < n; i++) {
		(succprime[i] != nullptr)? printf("%zu ", succprime[i]->rank): printf("N ");
	}
	printf("\n");

*/
	}

	auto v3 = [&](size_t i) {
		L[i].rank = D[i];
	};
	parallel_for(0,n,v3);
	
	free(D);
	free(Dprime);
	free(succ);
	free(succprime);
	/*
	for (size_t i = 0; i < n; i++){
		printf("%zu ",L[i].rank);
	}
	printf("\n");
	*/
}


// Sampling-Based List Ranking
//
// The work/depth bounds of your implementation should be:
// Work = O(n) whp
// Depth = O(\sqrt(n)* \log(n)) whp
void SamplingBasedListRanking(ListNode* L, size_t n, long num_samples=-1, parlay::random r=parlay::random(0)) {
	// Perhaps use a serial base case for small enough inputs?

	if (num_samples == -1) {
    	num_samples = sqrt(n);
  	}
	// let's allocate some memory for stuff we need
	int* in_samp = (int*)malloc(n*sizeof(int));
	// there is a clever way of figuring out whether 
	// a node is the head using atomic xor operations
	// in O(1) space 
	// but we're going to do the naive thing and 
	// find it in O(n) space
	bool* not_head = (bool*)malloc(n*sizeof(bool));


	// initialize the ranks for indexing
	for(size_t i = 0; i < n; i++) {
		L[i].rank = i;
		not_head[i] = false; 
	}
	
	srand (time(NULL));
	auto gen_sample = [&](size_t i) {
		// not technically threadsafe, but I don't think it is a huge deal for our purposes...
		long rnd = rand() % n;
		// sample
		(rnd < num_samples) ? (in_samp[i] = i) : in_samp[i] = -1;
		// include the tail and figure out who the head is
		(L[i].next == nullptr) ? (in_samp[i] = i) : not_head[L[i].next->rank] = true;
	};
	parallel_for(0,n,gen_sample);
	// include head in samp
	size_t head_index = -1;
	auto include_head = [&](size_t i ) {
		if (not_head[i] == false) (in_samp[i] = i, head_index = i);
	};
	parallel_for(0,n,include_head);
	
	// let's check that this all worked
	/*	
	printf("L: ");
	for (size_t i = 0; i < n; i++) {
		printf("%zu ", L[i].rank);
	}
	printf("\n");

	printf("in_samp: ");
	for (size_t i = 0; i < n; i++) {
		printf("%d ", in_samp[i]);
	}
	printf("\n");

	printf("not_head: ");
	for (size_t i = 0; i < n; i++) {
		printf("%d ", not_head[i]);
	}
	printf("\n");
	*/
	
	// now we build a linked list on the sampled nodes. Because we
	// are indexing in using the ids in L we need to allocate some extra
	// memory, but that should be okay...
		
	ListNode* samplist = (ListNode*)malloc(n * sizeof(ListNode));
	
	auto build_list = [&](size_t i) {
		if (in_samp[i] >= 0) {
			ListNode *currnode = L[i].next;
			if (currnode != nullptr) samplist[in_samp[i]].rank++;
			while (currnode != nullptr && in_samp[currnode->rank] < 0) {
				samplist[in_samp[i]].rank++;
				currnode = currnode->next;
			}
			if (currnode != nullptr) samplist[in_samp[i]].next = &samplist[in_samp[currnode->rank]];
		}
	};

	parallel_for(0,n,build_list);

	/*
	printf("samplist: ");
	for (size_t i = 0; i < m; i++) {
		printf("%zu ", samplist[i].rank);
	}
	printf("\n");
	*/
	// now that we have the linked list with the relative ranks, we just need to 
	// correct the ranks using a serial weighted listrank
	
	
	size_t ctr = 0;
	size_t tmpval;
	size_t m = 0;
	ListNode* save = &samplist[in_samp[head_index]];
	ListNode* head = save;
	while (head != nullptr) {
		m++;
		ctr+= head->rank;
		head = head->next;
	}
	printf("m: %zu\n",m);
	head = save;
	while (head != nullptr) {
		tmpval = head->rank;
		head->rank = ctr;
		head = head->next;
		ctr -= tmpval;
	}

	// now that we have the sampled linked list perfected, we just need to rank the other
	// elements in parallel
	
	auto finalize_list = [&](size_t i) {
		if (in_samp[i] >= 0) {
			size_t curr = samplist[in_samp[i]].rank;
			L[i].rank = curr;	
			ListNode *currnode = L[i].next;
			while (currnode != nullptr && in_samp[currnode->rank] < 0) {
				L[currnode->rank].rank = --curr;
				currnode = currnode->next; 
			}
		}
	};
	parallel_for(0,n,finalize_list);
	

}

