#ifndef H_CLOPS_GET_ONES
#define H_CLOPS_GET_ONES

#include "alloc.hpp"

namespace array {
	template <typename T,typename SIZE>
	SIZE get_ones(T* char_v,const SIZE length,T*& one_indices) {
		alloc(one_indices,length,T);
		SIZE num_one_indices = 0;
		for (SIZE i = 0; i < length; ++i) if (char_v[i] == 1) one_indices[num_one_indices++] = i;
		return num_one_indices;
	}

	template <typename SIZE,typename T>
	SIZE get_ones_64(uint64_t* word,const SIZE nwords,T*& one_indices, const SIZE length) {
		alloc(one_indices,length,T);
		SIZE num_one_indices = 0;
		for (SIZE i = 0; i < nwords; ++i) {
			for (SIZE j = 0; j < 64; ++j) {
				auto k = i*64+j;
				if (k==length) break;
				if ( ( word[i] >> j ) & 1 ) one_indices[num_one_indices++] = k;
			}
		}
		return num_one_indices;
	}
}

#endif // H_CLOPS_GET_ONES