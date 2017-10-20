#ifndef H_BASE_CONSTRUCT_GROUND_V
#define H_BASE_CONSTRUCT_GROUND_V

#include <stdio.h>
#include <cstring> // std::memcpy, std::memset, std::fill

#include "../alloc.hpp"

namespace base {
	template <typename T>
	void construct_ground_V(T ** ground_V, const T D, const T verbose) {
        int i,j,k;
        T * count;
        alloc(count,D,T);
        // initialize count to 0
        //std::memset(count,0,D * sizeof(T));
        bool carry;
        
        alloc(ground_V[0],D,T);
        ground_V[0][0] = 1;
        std::memset(ground_V[0]+1,0,(D-1) * sizeof(T));
        
        if (verbose != 0) {
            // Print point
            printf("[");
            for (i = 0; i < D; ++i){
                printf("%d",ground_V[0][i]);
                if (i != D-1) printf(",");
            }
            printf("] ");//
        }
        
        k = 1;
        for (i = D-1; i > 0; --i) {
            std::memset(count,0,D * sizeof(T));
            
            while (count[i] == 0) {
                alloc(ground_V[k],D,T);
                ground_V[k][0] = 1;
                std::memset(ground_V[k]+1,0,(i-1) * sizeof(T));
                ground_V[k][i] = 1;
                std::memset(ground_V[k]+i+1,0,(D-i-1) * sizeof(T));
                
                // Extract a vector in {-1,0,1}^{D-i-1} to fill the vector
                for (j = i+1; j < D; ++j) ground_V[k][j] = count[j] - 1;
                
                if (verbose != 0) {
                    // Print point
                    printf("[");
                    for (j = 0; j < D; ++j) {
                        printf("%d",ground_V[k][j]);
                        if (j != D-1) printf(",");
                    }
                    printf("] ");//
                }
                ++k;
                // Increase counter, by performing mod-3 computation
                j = D-1;
                do {
                    carry = (count[j] == 2);
                    count[j] = (count[j] + 1) % 3;
                    j--;
                } while (carry && j >= 0);
            }
        }        
        free(count);
	}
}

#endif // H_BASE_CONSTRUCT_GROUND_V