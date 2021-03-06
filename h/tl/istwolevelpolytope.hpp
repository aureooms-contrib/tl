#ifndef H_TL_ISTWOLEVELPOLYTOPE
#define H_TL_ISTWOLEVELPOLYTOPE

#include <algorithm>
#include <utility>
#include "mem/alloc.hpp"
#include "mem/alloc_matrix.hpp"
#include "Polytope.hpp"
#include "CanonicalGraph.hpp"
#include "array/get_zeros.hpp"

namespace tl {

	template <typename T>
	bool is_subset(T* Sj, T* Sk, T* zeros,const T* const end) {

		while ( zeros != end ) {
			if (Sj[*zeros] == 0 && Sk[*zeros] != 0) return false;
			++zeros;
		}

	    return true;

	}

	// the only rows in S_Fi are the one containing the maximal sets of zeros
	template <typename T,typename SIZE>
	bool is_maximal(T** S,T* zeros,const T* const end,const T i,const T j,const SIZE rows) {
		T* Sj = S[j];
	    for (SIZE k = 0; k < rows; ++k) {
			T* Sk = S[k];
			// Check if the set of zeros of S[j][.] intersected with the one of S[i][.]
			// is a subset of the one S[k][.] intersected with the one of S[i][.]
			if ((k != j) && (k != i) && is_subset(Sj,Sk,zeros,end)) return false;
	    }
	    return true;
	}

	// Checks whether a given 0-1 matrix is the slack matrix of a D-dimensional 2-level polytope,
	// by using the list of (D-1)-dimensional 2-level polytopes.
	template <typename C, typename A, typename P>
	bool istwolevelpolytope(C& comp, A& cgs, P& poly) {
		const auto& S = poly.matrix;
		const auto& rows = poly.rows;
		const auto& cols = poly.columns;
		const auto& D = poly.dimension;
	    // First test: check that every column contains at least D zeros
	    // by construction, every row of S contains at least D zeros
	    for (int j = 0; j < cols; ++j) {
	        int num_facets_contain = 0;
	        for (int i = 0; i < rows; ++i) if (S[i][j] == 0) ++num_facets_contain;
	        if (num_facets_contain < D) return false;
	    }

		void * zero_indices_mem;
		int ** zero_indices;
	    int * num_zero_indices;
		mem::alloc_matrix(zero_indices_mem, zero_indices, rows, cols);
		mem::alloc(num_zero_indices,rows);
	    for (int i = 0; i < rows; ++i) {
			num_zero_indices[i] = array::get_zeros_skip_alloc(S[i],cols,zero_indices[i]);
		}

	    void* rows_S_Fi_mem;
	    int** rows_S_Fi;
	    int* num_rows_S_Fi;
		mem::alloc_matrix(rows_S_Fi_mem, rows_S_Fi, rows, rows);
		mem::alloc(num_rows_S_Fi,rows);

	    for (int i = 0; i < rows; ++i) {
	        int l = 0; // current number of rows of S_Fi
			int* zeros = zero_indices[i];
			const int * const end = zeros + num_zero_indices[i];
			int j;
	        for (j = 0; j < i; ++j)
	            if (is_maximal(S,zeros,end,i,j,rows)) rows_S_Fi[i][l++] = j;
	        for (++j; j < rows; ++j)
	            if (is_maximal(S,zeros,end,i,j,rows)) rows_S_Fi[i][l++] = j;
	        num_rows_S_Fi[i] = l;
	    }

	    // Go through all the rows and build the corresponding submatrix for each of them. If the input is a slack matrix,
	    // this will compute the slack matrix of the corresponding facet and test is it appears in the list L_{D-1}

	    bool found = true;

	    for (int i = 0; i < rows; ++i) {
			const auto zero_indices_i = zero_indices[i];
			const auto rows_S_Fi_i = rows_S_Fi[i];
	    	const int fdimension = D - 1;
	    	const int frows = num_rows_S_Fi[i];
	    	const int fcolumns = num_zero_indices[i];
	    	void* fdata;
			int** fmatrix;
			mem::alloc_matrix(fdata,fmatrix,frows,fcolumns);
	        int* fpt(*fmatrix);

	        for (int j = 0; j < frows; ++j) {
				const auto rows_S_Fi_ij = rows_S_Fi_i[j];
				const auto Si = S[rows_S_Fi_ij];
				for (int k = 0; k < fcolumns; ++k) {
					*(fpt++) = Si[zero_indices_i[k]];
				}
			}

			tl::Polytope<int> facet(fdimension,frows,fcolumns,fdata,fmatrix);
			tl::CanonicalGraph<int> cg(facet);

			std::pair<setword*, setword*> pair(cg.begin, cg.end);

			found = std::binary_search(cgs.begin(), cgs.end(), pair, comp);

			cg.teardown();
			facet.teardown();

			if ( !found ) break ;

	    }

	    free(rows_S_Fi_mem);
	    free(num_rows_S_Fi);

	    free(zero_indices_mem);
	    free(num_zero_indices);

	    return found;
	}
}

#endif // H_TL_ISTWOLEVELPOLYTOPE
