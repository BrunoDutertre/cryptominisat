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
 * Alternative C wrappers
 */
#include "cryptominisat5/cmsat_c.h"
#include "cryptominisat5/cryptominisat.h"

using namespace CMSat;

extern "C" {

struct cmsat_solver {
  SATSolver solver;
  std::vector<Lit> lit_buffer;
  std::vector<uint32_t> var_buffer;
};

/*
 * Conversion: lbool to cmsat_status
 */
static const cmsat_status lbool_to_status[3] = {
  CMSAT_SAT,     // l_True = 0
  CMSAT_UNSAT,   // l_False = 1
  CMSAT_UNKNOWN, // l_Undef = 2
};

static cmsat_status_t lbool2status(lbool b) {
  uint8_t i = b.getValue();
  return i < 3 ? lbool_to_status[i] : CMSAT_UNKNOWN;
}


/*
 * Conversion: lbool to cmsat_value
 */
static const cmsat_value_t lbool_to_value[3] = {
  CMSAT_VAL_TRUE,
  CMSAT_VAL_FALSE,
  CMSAT_VAL_UNKNOWN
};

static cmsat_value_t lbool2value(lbool b) {
  uint8_t i = b.getValue();
  return i < 3 ? lbool_to_value[i] : CMSAT_VAL_UNKNOWN;
}
  
/*
 * Allocate a solver instance
 */  
CMS_DLL_PUBLIC cmsat_solver_t* cmsat_new_solver(void) {
  return new cmsat_solver();
}

/*
 * Free solver s
 */
CMS_DLL_PUBLIC void cmsat_free_solver(cmsat_solver_t *s) {
  delete s;
}

/*
 * Set the number of threads to use (default = one thread?).
 * This must be called before adding clauses.
 * - return -1 if there's an error
 * - return 0 otherwise
 */
CMS_DLL_PUBLIC int32_t cmsat_set_num_threads(cmsat_solver_t *s, uint32_t n) {
  int32_t code = 0;
  
  try {
    s->solver.set_num_threads(n);
  } catch (std::runtime_error) {
    code = -1;
  }

  return code;
}

/*
 * Solver options
 */
CMS_DLL_PUBLIC void cmsat_set_verbosity(cmsat_solver_t *s, uint32_t verbosity) {
  s->solver.set_verbosity(verbosity);
}
  
CMS_DLL_PUBLIC void cmsat_set_max_time(cmsat_solver_t *s, double max_time) {
  s->solver.set_max_time(max_time);
}

CMS_DLL_PUBLIC void cmsat_set_max_confl(cmsat_solver_t *s, int64_t max_confl) {
  s->solver.set_max_confl(max_confl);
}
  
CMS_DLL_PUBLIC void cmsat_set_default_polarity(cmsat_solver_t *s, bool polarity) {
  s->solver.set_default_polarity(polarity);
}

CMS_DLL_PUBLIC void cmsat_no_simplify(cmsat_solver_t *s) {
  s->solver.set_no_simplify();
}

CMS_DLL_PUBLIC void cmsat_no_simplify_at_startup(cmsat_solver_t *s) {
  s->solver.set_no_simplify_at_startup();
}

CMS_DLL_PUBLIC void cmsat_no_equivalent_lit_replacement(cmsat_solver_t *s) {
  s->solver.set_no_equivalent_lit_replacement();
}

CMS_DLL_PUBLIC void cmsat_no_bva(cmsat_solver_t *s) {
  s->solver.set_no_bva();
}

CMS_DLL_PUBLIC void cmsat_no_bve(cmsat_solver_t *s) {
  s->solver.set_no_bve();
}

/*
 * Add n variables to the solver
 * - return -1 if there's an error (i.e., too many variables)
 * - return 0 otherwise
 */
CMS_DLL_PUBLIC int32_t cmsat_new_vars(cmsat_solver_t *s, uint32_t n) {
  int32_t code = 0;

  try {
    s->solver.new_vars(n);
  } catch (TooManyVarsError) {
    code = -1;
  }
  return code;
}

/*
 * Get the number of variables in s
 */
CMS_DLL_PUBLIC uint32_t cmsat_nvars(const cmsat_solver_t *s) {
  return s->solver.nVars();
}

/*
 * Copy array of literal a[0 .. n-1] into s->lit_buffer
 */
static void build_lit_array(cmsat_solver_t *s, const uint32_t *a, uint32_t n) {
  s->lit_buffer.clear();
  for (uint32_t i=0; i<n; i++) {
    s->lit_buffer.push_back(Lit::toLit(a[i]));
  }
}

/*
 * Copy array of variables a[0 ... n-1] into s->var_buffer
 */
static void build_var_array(cmsat_solver_t *s, const uint32_t *a, uint32_t n) {
  s->var_buffer.clear();
  for (uint32_t i=0; i<n; i++) {
    s->var_buffer.push_back(a[i]);
  }
}
  
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
CMS_DLL_PUBLIC int32_t cmsat_add_clause(cmsat_solver_t *s, const uint32_t *a, uint32_t n) {
  build_lit_array(s, a, n);
  return s->solver.add_clause(s->lit_buffer) - 1;
}

/*
 * Add an xor clause
 * - n = number of variables in the clause
 * - a = array of n variables
 * - the clause is (xor a[0] .... a[n-1]) == rhs
 *   each element of a must then be a valid variable index
 * - return -1 is something goes wrong
 * - return 0 otherwise
 */
CMS_DLL_PUBLIC int32_t cmsat_add_xor_clause(cmsat_solver_t *s, const uint32_t *a, uint32_t n, bool rhs) {
  build_var_array(s, a, n);
  return s->solver.add_xor_clause(s->var_buffer, rhs) - 1;
}

/*
 * Check satisfiability of s
 */
CMS_DLL_PUBLIC cmsat_status_t cmsat_solve(cmsat_solver_t *s) {
  return lbool2status(s->solver.solve());
}

/*
 * Check with assumptions:
 * - n = number of assumptions
 * - a = array of n assumptions
 *   each assumption must be a literal
 */
CMS_DLL_PUBLIC cmsat_status_t cmsat_solve_with_assumptions(cmsat_solver_t *s, const uint32_t* a, uint32_t n) {
  build_lit_array(s, a, n);
  return lbool2status(s->solver.solve(&s->lit_buffer));
}

/*
 * Value of variable x in s
 */
CMS_DLL_PUBLIC cmsat_value_t cmsat_var_value(cmsat_solver_t *s, uint32_t x) {
  const std::vector<lbool> model = s->solver.get_model();
  return lbool2value(model[x]);
}

/*
 * Value of literal l in s
 */
CMS_DLL_PUBLIC cmsat_value_t cmsat_lit_value(cmsat_solver_t *s, uint32_t l) {
  Lit lit = Lit::toLit(l);
  uint32_t x = lit.var();
  const std::vector<lbool> model = s->solver.get_model();

  if (model[x] == l_Undef) {
    return CMSAT_VAL_UNKNOWN;
  }
  return lbool2value(model[x] ^ lit.sign());
}


/*
 * Construct the model
 * - this stores the model in m
 * - m->nvals = number of values (i.e., number of variables in s)
 * - for each variable x: m->val[x] = value of x in the model
 */
CMS_DLL_PUBLIC void cmsat_get_model(const cmsat_solver_t *s, cmsat_val_vector_t *m) {
  const std::vector<lbool> model = s->solver.get_model();
  uint32_t n = model.size();
  m->nvals = n;
  m->val = new int8_t[n];

  for (uint32_t i=0; i<n; i++) {
    m->val[i] = lbool2value(model[i]);
  }
}


/*
 * Delete value vector v: this frees array v->val
 */
CMS_DLL_PUBLIC void cmsat_free_val_vector(cmsat_val_vector_t *v) {
  delete[] v->val;
  v->val = 0;
}


/*
 * Return a conflict after solve_with_assumptions returns unsat
 * - the conflict is a subset of the assumptions.
 * - it is returned in vector *c:
 *   c->nlits = number of literals in the conflict
 *   c->lit = the literals
 */
CMS_DLL_PUBLIC void cmsat_get_conflict(const cmsat_solver_t *s, cmsat_lit_vector_t *c) {
  const std::vector<Lit> conflict = s->solver.get_conflict();
  uint32_t n = conflict.size();
  c->nlits = n;
  c->lit = new uint32_t[n];

  for (uint32_t i=0; i<n; i++) {
    c->lit[i] = conflict[i].toInt();
  }
}


/*
 * Delete literal vector v: this frees array v->lit
 */
CMS_DLL_PUBLIC void cmsat_free_lit_vector(cmsat_lit_vector_t *v) {
  delete[] v->lit;
  v->lit = 0;
}

  
}
