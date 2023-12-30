// TemplateLang.cpp : Defines the entry point for the application.
//

#include "TemplateLang.h"

// clang-format off
double sequence_sum_reduce(double x) {

  // sum from 1 to n
  using f = function<
              reduce_range<
                function<
                  add<variable<0>, variable<1>>, 
                  variable<0>, 
                  variable<1>>,
                value<1.>, 
                add<value<1.>, variable<0>>>,
              variable<0>>;

  return program<
              call<f, variable<0>>, 
              1>{}.run(x);
}

double sequence_sum_generator(double x) {

  using range = gen<
                  function<variable<0>, variable<0>>,
                  value<1.>,
                  add<value<1.>, variable<0>>>;

	return program<
							sum<range>,
							1>{}.run(x);
}

double fib_recursive(double x) {
  using f = function<
              if_<
                lteq<variable<0>, value<1.>>, 
                variable<0>,
                add<
                  recurse<add<variable<0>, value<-1.>>>,
                  recurse<add<variable<0>, value<-2.>>>>>,
              variable<0>>;
  return program<
            call<f, variable<0>>, 
            1>{}.run(x);
}

double e_approx(double n_terms) {
	using range_gen = gen<
		                function<variable<0>, variable<0>>,
		                value<1.>,
		                add<value<1.>, variable<0>>>;
  
  using f_factorial = function<
												reduce_range<
													function<
														mul<variable<0>, variable<1>>, 
														variable<0>, 
														variable<1>>,
													value<1.>, 
													add<value<1.>, variable<0>>>,
                          value<1.>,
												variable<0>>;
  using fact_gen = compose<
										range_gen, 
										f_factorial>;
  using inv_gen = compose<
                    fact_gen, 
                    function<inv<variable<0>>, variable<0>>>;
  return program<sum<fact_gen>, 1>{}.run(n_terms); 
}
// clang-format on

int main() {
  std::cout << "reduce:   sum of 1 to 100 = " << sequence_sum_reduce(100)
            << "\n";
  std::cout << "gen:      sum of 1 to 100 = " << sequence_sum_generator(100)
            << "\n";
  std::cout << "recurse:  fib(10) = " << fib_recursive(10) << "\n";
  std::cout << "e approx: " << e_approx(10) << "\n";
  return 0;
}