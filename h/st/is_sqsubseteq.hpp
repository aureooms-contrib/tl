#ifndef H_ST_IS_SQSUBSETEQ
#define H_ST_IS_SQSUBSETEQ

#include "st/min_A_idx.hpp"
#include "array/is_equal.hpp"
#include "array/is_all_ones.hpp"

namespace st {
	// Check if A \sqsubsteq B
	template <typename T, typename SIZE>
	bool is_sqsubseteq( const T * const A, const T * const B, const SIZE length) {
	    T min_A(st::min_A_idx(A, length));
	    return array::is_equal(B+min_A,A+min_A,length-min_A);
	}

	// Check if A \sqsubsteq 1111111111111111111
	template <typename T, typename SIZE>
	bool is_sqsubseteq_all_ones( const T * const A, const SIZE length) {
	    T min_A(st::min_A_idx(A, length));
	    return array::is_all_ones(A+min_A,length-min_A);
	}

}

#endif // H_ST_IS_SQSUBSETEQ
