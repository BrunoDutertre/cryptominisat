/******************************************
Copyright (c) 2019, Bruno Dutertre

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

/*
 * Alternative C Wrappers to cryptominisat5.
 * These wrappers don't expose the C++ data structures.
 */

#ifndef CMSAT_C_H
#define CMSAT_C_H

#if defined _WIN32
#define CMS_DLL_PUBLIC __declspec(dllexport)
#else
#define CMS_DLL_PUBLIC __attribute__ ((visibility ("default")))
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opaque solver type
 */
typedef struct cmsat_solver cmsat_solver_t;


/*
 * Literal constructors:
 * - lit(x, false) = positive literal for variable x (encoded as 2x)
 *   lit(x, true) = negative literal for variable x (encoded as 2x+1)
 * - pos(x) = positive literal for x
 * - neg(x) = negative literal for x
 */
static inline uint32_t cmsat_lit(uint32_t var, bool is_inverted) {
  return var + var + is_inverted;
}

static inline uint32_t cmsat_pos(uint32_t var) {
  return cmsat_lit(var, false);
}

static inline uint32_t cmsat_neg(uint32_t var) {
  return cmsat_lit(var, true);
}


/*
 * Special constants for undefined variables and literals.
 */
#define cmsat_var_undef (0xffffffffU >> 4)
#define cmsat_lit_undef cmsat_pos(cmsat_var_undef)
#define cmsat_lit_error cmsat_neg(cmsat_var_undef)


/*
 * Result of solve and solve_with_assumptions
 */
typedef enum cmsat_status {
  CMSAT_UNKNOWN = 0,
  CMSAT_SAT = 10,
  CMSAT_UNSAT = 20
} cmsat_status_t;

/*
 * Value assigned to variables and literals
 */
typedef enum cmsat_value {
  CMSAT_VAL_UNKNOWN = -1,
  CMSAT_VAL_FALSE = 0,
  CMSAT_VAL_TRUE = 1
} cmsat_value_t;


/*
 * Vector of literals (filled in by get_conflict)
 * - nlits = number of literals returned
 * - lit = array of nlits literals
 */
typedef struct cmsat_lit_vector {
  uint32_t *lit;
  uint32_t nlits;
} cmsat_lit_vector_t;


/*
 * Vector of boolean values (filled in by get_model)
 * - nvals = number of values
 * - val = array of nvals values
 * - val is a mapping from variable index to values
 */
typedef struct cmsat_val_vector {
  int8_t *val;
  uint32_t nvals;
} cmsat_val_vector_t;



/*
 * Allocate and free a sat_solver instance
 */
extern CMS_DLL_PUBLIC cmsat_solver *cmsat_new_solver(void);
extern CMS_DLL_PUBLIC void cmsat_free_solver(cmsat_solver_t *s);

/*
 * Set the number of threads to use (default = one thread?).
 * This must be called before adding clauses.
 * - return -1 if there's an error
 * - return 0 otherwise
 */
extern CMS_DLL_PUBLIC int32_t cmsat_set_num_threads(cmsat_solver_t *s, uint32_t n);

/*
 * Solver options
 */
extern CMS_DLL_PUBLIC void cmsat_set_verbosity(cmsat_solver_t *s, uint32_t verbosity); // verbosity (default 0 means quiet)
extern CMS_DLL_PUBLIC void cmsat_set_max_time(cmsat_solver_t *s, double max_time); //max time to run to on next solve() call
extern CMS_DLL_PUBLIC void cmsat_set_max_confl(cmsat_solver_t *s, int64_t max_confl); //max conflict to run to on next solve() call
extern CMS_DLL_PUBLIC void cmsat_set_default_polarity(cmsat_solver_t *s, bool polarity); //default polarity when branching for all vars
extern CMS_DLL_PUBLIC void cmsat_no_simplify(cmsat_solver_t *s); //never simplify
extern CMS_DLL_PUBLIC void cmsat_no_simplify_at_startup(cmsat_solver_t *s); //don't simplify at start, faster startup time
extern CMS_DLL_PUBLIC void cmsat_no_equivalent_lit_replacement(cmsat_solver_t *s); //don't replace equivalent literals
extern CMS_DLL_PUBLIC void cmsat_no_bva(cmsat_solver_t *s); //No bounded variable addition
extern CMS_DLL_PUBLIC void cmsat_no_bve(cmsat_solver_t *s); //No bounded variable elimination


/*
 * Add n variables to the solver
 * - return -1 if there's an error (i.e., too many variables)
 * - return 0 otherwise
 */
extern CMS_DLL_PUBLIC int32_t cmsat_new_vars(cmsat_solver_t *s, uint32_t n);

/*
 * Get the number of variables in s
 */
extern CMS_DLL_PUBLIC uint32_t cmsat_nvars(const cmsat_solver_t *s);

/*
 * Add a clause:
 * - n = number of literals
 * - a = array of n literals
 * - each literal is a 32bit unsigned integer.
 *   the lower-order bit of each literal is its sign: 0 means positive literal, 1 means negative.
 *   the other bits are a variable index.
 *
 * - return -1 if something was wrong:
 *   either n is too large
 *   or some a[i] has a variable that's larger than the number of variables in s
 *   or the solver is already unsat
 *
 * - return 0 otherwise
 */
extern CMS_DLL_PUBLIC int32_t cmsat_add_clause(cmsat_solver_t *s, const uint32_t *a, uint32_t n);


/*
 * Add an xor close
 * - n = number of variables in the clause
 * - a = array of n variables
 * - the clause is (xor a[0] .... a[n-1]) == rhs
 *   each element of a must then be a valid variable index
 * - return -1 is something goes wrong
 * - return 0 otherwise
 */
extern CMS_DLL_PUBLIC int32_t cmsat_add_xor_clause(cmsat_solver_t *s, const uint32_t *a, uint32_t n, bool rhs);


/*
 * Check satisfiability of s
 */
extern CMS_DLL_PUBLIC cmsat_status_t cmsat_solve(cmsat_solver_t *s);

/*
 * Check with assumptions:
 * - n = number of assumptions
 * - a = array of n assumptions
 *   each assumption must be a literal
 */
extern CMS_DLL_PUBLIC cmsat_status_t cmsat_solve_with_assumptions(cmsat_solver_t *s, const uint32_t* a, uint32_t n);


/*
 * Value of variable x in s
 */
extern CMS_DLL_PUBLIC cmsat_value_t cmsat_lit_value(cmsat_solver_t *s, uint32_t l);


/*
 * Value of variable x in s
 */
extern CMS_DLL_PUBLIC cmsat_value_t cmsat_var_value(cmsat_solver_t *s, uint32_t x);


/*
 * Construct the model
 * - this allocates m->val and stores the model in m
 * - m->nvals = number of values (i.e., number of variables in s)
 * - for each variable x: m->val[x] = value of x in the model
 */
extern CMS_DLL_PUBLIC void cmsat_get_model(const cmsat_solver_t *s, cmsat_val_vector_t *m);


/*
 * Delete value vector v: this frees array v->val
 */
extern CMS_DLL_PUBLIC void cmsat_free_val_vector(cmsat_val_vector_t *v);

/*
 * Return a conflict after solve_with_assumptions returns unsat
 * - the conflict is a subset of the assumptions.
 * - it is returned in vector *c:
 *   c->nlits = number of literals in the conflict
 *   c->lit = the literals
 * - this function allocates an array c->lit large enough
 */
extern CMS_DLL_PUBLIC void cmsat_get_conflict(const cmsat_solver_t *s, cmsat_lit_vector_t *c);

/*
 * Delete literal vector v: this frees array v->lit
 */
extern CMS_DLL_PUBLIC void cmsat_free_val_vector(cmsat_val_vector_t *v);


#ifdef __cplusplus
} /* close extern "C" { */
#endif

#endif /* CMSAT_C_H */
