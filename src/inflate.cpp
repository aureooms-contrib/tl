#include <iostream>
#include <vector>
#include <map>

#include "mem/alloc.hpp"
#include "mem/alloc_triangular_matrix.hpp"
#include "mem/alloc_matrix.hpp"

#include "linalg/is_id.hpp"
#include "linalg/invert.hpp"
#include "linalg/transpose.hpp"

#include "tl/load.hpp"
#include "tl/dump.hpp"

#include "nt/factor.hpp"
#include "simpl/slack_matrix_simplicial_2L.hpp"
#include "simpl/push_simplicial_core.hpp"

#include "tl/extractM.hpp"
#include "tl/checksimplicialcore.hpp"
#include "tl/construct_slack_matrix.hpp"

#include "tl/Polytope.hpp"
#include "base/construct_slab_point_sat.hpp"
#include "base/construct_d_aut_collection.hpp"
#include "base/construct_orbits.hpp"
#include "base/construct_base_V.hpp"
#include "base/construct_base_H.hpp"
#include "emb/VEmbedding.hpp"
#include "emb/V.hpp"
#include "base/Slabs.hpp"
#include "base/construct_facets_base.hpp"
#include "emb/HEmbedding.hpp"
#include "emb/X.hpp"
#include "emb/Xr.hpp"
#include "base/construct_incompatibility_adjM.hpp"

#include "array/is_all_ones.hpp"
#include "array/pack.hpp"

#include "st/inc.hpp"
#include "st/is_sqsubseteq.hpp"
#include "clops/cl.hpp"

int main () {

    std::ios_base::sync_with_stdio(false);

    int tot_N_closed_sets = 0;

    std::map<int,emb::VEmbedding<int,int>> Vs;

    // start the popcorn machine
    std::vector<tl::Polytope<int>> facets;
    while ( true ) {

        if ( !tl::load(std::cin, facets) ) break ;

        auto& facet = facets[0]; // we use a length-1 vector
        int D = facet.dimension + 1;

        // use the characterization of simplicial 2-level polytopes [Grande, Sanyal]
        if (linalg::is_id(facet.dimension+1, facet.matrix)) {
            for (auto N : nt::factor(D)) {

                const int K = D/N;

                void * mem_S_new;
                int ** S_new;
                int num_rows_S_new,num_cols_S_new;

                simpl::slack_matrix_simplicial_2L(K,N,mem_S_new,S_new,num_rows_S_new,num_cols_S_new);
                simpl::push_simplicial_core(S_new,num_rows_S_new,num_cols_S_new,facet.matrix,D);

                tl::dump(std::cout, D, num_rows_S_new, num_cols_S_new, S_new);

                free(mem_S_new);
            }
        }
        // non simplicial case
        else {
            int num_rows_S(facet.rows);
            int num_cols_S(facet.columns);

            // Cheking for simplicial core
            if (!tl::checksimplicialcore(facet.matrix,D)) {
                std::cerr << "Fail. Simplicial core not found.\n" << std::endl ;
                return 1;
            }

            // Extract embedding transformation matrix M_d(0) and invert it
            void * mem_M, * mem_Minv;
            int ** M, ** Minv;
            mem::alloc_matrix(mem_M,M,D,D);
            mem::alloc_matrix(mem_Minv,Minv,D,D);

            tl::extractM(facet.matrix,M,D);
            linalg::invert(M,Minv,D);

            // Constructing H-embedding of facets of the base
            void * mem_facets_base;
            int ** facets_base;
            mem::alloc_matrix(mem_facets_base,facets_base,num_rows_S,D);
            const int num_facets_base = base::construct_facets_base(facets_base,facet.matrix,num_rows_S,D);

            // Constructing automorphism group of the base and extending it to R^D
            void * mem_d_aut_collection;
            int ** d_aut_collection;
            const int num_autom_base = base::construct_d_aut_collection(mem_d_aut_collection,d_aut_collection,facet.matrix,num_rows_S,num_cols_S,D);

            // Create Vert(P_0) (in V-embedding)
            void * mem_base_V;
            int ** base_V;
            mem::alloc_matrix(mem_base_V,base_V,num_cols_S,D);
            base::construct_base_V(base_V,facet.matrix,num_cols_S,D);

            // Create Vert(P_0) (H-embedding this time)
            void * mem_base_H;
            int ** base_H;
            mem::alloc_matrix(mem_base_H,base_H,num_cols_S,D);
            base::construct_base_H(base_H,base_V,Minv,num_cols_S,D);

            // Compute the slabs: inequalities x(E) <= 1, x(E) >= 0 that are valid for the base_H
            base::Slabs<int,int> slabs(D, num_cols_S, base_H);

            // Create V-embedding
            if (Vs.count(D) == 0) Vs.emplace(D, emb::V(D));
            auto& V = Vs.at(D);

            // Create H-embedding
            auto X = emb::X(D,V,facets_base,num_facets_base,slabs,Minv);

            // It is possible to free the memory used for the mem_base_V, we will use the H-embedding
            free(mem_base_V);

            // Generating orbits of point of the ground set
            void * mem_base_Ht;
            int ** base_Ht;
            mem::alloc_matrix(mem_base_Ht,base_Ht,D,num_cols_S);
            linalg::transpose(base_H, base_Ht,num_cols_S,D);
            void * mem_orbits;
            int ** orbits;
            mem::alloc_matrix(mem_orbits,orbits,num_autom_base,X.compsize);
            base::construct_orbits(orbits,num_autom_base,base_Ht,d_aut_collection,X,D);
            free(mem_base_Ht);

            // Compute Xr - its Xcomp is the discrete convexhull of Xfinal in Xcomp
            auto Xr = emb::Xr(D, X, slabs.n_rows, slabs.n_rows_64);

            // array::dump(X.list_accepted,X.compsize);

            // array::dump_matrix(X.comp,X.compsize,D);
            // std::cerr << std::endl;
            // array::dump_matrix(Xr.comp,Xr.compsize,D);
            // std::cerr << std::endl;

            // std::cerr << "-"<< "  " << X.fullsize << std::endl;
            // std::cerr << Xr.compsize << "  " << X.compsize << std::endl;
            // std::cerr << Xr.finalsize << "  " << X.finalsize << std::endl;
            // std::cerr << Xr.e1 <<  "  " << X.e1 << std::endl;

            // std::cerr << "#automorphisms of the base = " << num_autom_base << std::endl;
            // array::dump_matrix(orbits,num_autom_base,X.compsize);

            // Free mem_d_aut_collection, since we constructed the orbits
            free(mem_d_aut_collection);

            // Construct the incompatibility matrix
            void * mem_IM;
            int ** IM;
            mem::alloc_triangular_matrix(mem_IM,IM,Xr.finalsize);
            base::construct_incompatibility_adjM(IM,Xr.final,facets_base,Xr.finalsize,num_facets_base,D);

            // Pack-64
            void * mem_IM_64;
            uint64_t ** IM_64;
            mem::alloc_triangular_matrix_64(mem_IM_64,IM_64,Xr.finalsize);
            array::pack64_matrix_triangular(IM,IM_64,Xr.finalsize);

            // Lauching Ganter's next-closure algorithm and checking 2-levelness
            int * A;
            mem::alloc(A,Xr.finalsize);

            int * B;
            mem::alloc(B,slabs.n_rows);
            uint64_t * B_64;
            mem::alloc(B_64,slabs.n_rows_64);

            int * I;
            mem::alloc(I,Xr.finalsize);

            int * CI;
            mem::alloc(CI,Xr.finalsize);
            uint64_t * CI_64;
            mem::alloc(CI_64,Xr.n_rows_64);

            int * CI_big;
            mem::alloc(CI_big,Xr.compsize);
            uint64_t * CI_big_64;
            mem::alloc(CI_big_64,Xr.n_rows_big_64);

            // special case for e_1 only
            std::memset(A,0,Xr.finalsize * sizeof(int));
            A[0] = 1;
            std::fill(B,B + slabs.n_rows,1);
            ++tot_N_closed_sets;

            // construct the slack matrix S with embedding transformation matrix in top left position
            void * mem_S_new;
            int ** S_new;
            int num_rows_S_new, num_cols_S_new;
            tl::construct_slack_matrix(base_H,Xr.final,A,B,slabs.matrix,facet.matrix,mem_S_new,S_new,Xr.finalsize,slabs.n_rows,num_cols_S,num_rows_S_new,num_cols_S_new,D);
            tl::dump(std::cout, D, num_rows_S_new, num_cols_S_new,S_new);
            free(mem_S_new);

            while (true) {
                int i = 1;
                while (true) {
                    while ( A[i] == 1 ) ++i;
                    st::inc(A, i, I, Xr.finalsize); // I = inc(A,i)
                    ++i;

                    if (clops::cl(I, B_64, CI_64, CI_big_64, CI, CI_big, IM_64, X, Xr, slabs, orbits, num_autom_base)) {
                        if (st::is_sqsubseteq(I, CI, Xr.finalsize)) break;
                    }
                    else if (st::is_sqsubseteq_all_ones(I, Xr.finalsize)) {
                        std::fill(CI,CI+Xr.finalsize,1);
                        break;
                    }
                }
                int *tmp(A);
                A = CI;
                CI = tmp;
                ++tot_N_closed_sets;

                if ( array::is_all_ones(A, Xr.finalsize) ) break;
                array::unpack64(B,slabs.n_rows,B_64);

                // std::cerr << "A = ";
                // array::dump(A,X.finalsize);

                // construct the slack matrix S with embedding transformation matrix in top left position
                if ( tl::construct_slack_matrix(base_H,Xr.final,A,B,slabs.matrix,facet.matrix,mem_S_new,S_new,Xr.finalsize,slabs.n_rows,num_cols_S,num_rows_S_new,num_cols_S_new,D) ) {
                    tl::dump(std::cout, D, num_rows_S_new, num_cols_S_new, S_new);
                    free(mem_S_new);
                }
                
            }

            free(I);
            free(B);
            free(B_64);
            free(CI);
            free(CI_64);
            free(CI_big);
            free(CI_big_64);
            free(A);

            free(mem_IM);
            free(mem_IM_64);

            slabs.teardown();
            X.teardown();
            Xr.teardown();
            free(mem_orbits);
            free(mem_base_H);
            free(mem_facets_base);
            free(mem_M);
            free(mem_Minv);
        }
        facet.teardown();
        facets.clear(); // we use a vector containing a single element
    }

    // Cleanup for V-embeddings
    for (auto pair : Vs) pair.second.teardown();
    Vs.clear();
    std::cerr << "Total #closed sets = " << tot_N_closed_sets << std::endl;
    return 0;

}
