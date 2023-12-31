// TemplateLang.cpp : Defines the entry point for the application.
//

#include "TemplateLang.h"

// clang-format off
constexpr double sequence_sum_reduce(double x) {

  // sum from 1 to n
  using f = function<
              reduce_range<
                function<
                  add<variable<0>, variable<1>>, 
                  2>,
                value<1>, 
                add<value<1>, variable<0>>>,
              1>;

  return program<
              call<f, variable<0>>, 
              1>{}.run(x);
}

constexpr double sequence_sum_generator(double x) {

  using range = gen<
                  function<variable<0>, 1>,
                  value<1>,
                  add<value<1>, variable<0>>>;

	return program<
							sum<range>,
							1>{}.run(x);
}

constexpr double fib_recursive(double x) {
  using f = function<
              if_<
                lteq<variable<0>, value<1>>, 
                variable<0>,
                add<
                  recurse<add<variable<0>, value<-1>>>,
                  recurse<add<variable<0>, value<-2>>>>>,
              1>;
  return program<
            call<f, variable<0>>, 
            1>{}.run(x);
}

constexpr double e_approx(double n_terms) {
	using range_gen = gen<
		                function<variable<0>, 1>,
		                value<0>,
		                add<value<1>, variable<0>>>;
  
  using f_factorial = function<
												reduce_range<
													function<
														mul<variable<0>, variable<1>>, 
														2>,
													value<1>, 
													add<value<1>, variable<0>>, 
                          value<1>>,
												1>;
  using fact_gen = compose<
										range_gen, 
										f_factorial>;
  using inv_gen = compose<
                    fact_gen, 
                    function<
                      inv<variable<0>>, 
                      1>>;
  return program<sum<inv_gen>, 1>{}.run(n_terms); 
}
// clang-format on

int main() {
  std::cout << "reduce:   sum of 1 to 100 = " << sequence_sum_reduce(100)
            << "\n";
  std::cout << "gen:      sum of 1 to 100 = " << sequence_sum_generator(100)
            << "\n";
  std::cout << "recurse:  fib(10) = " << fib_recursive(10) << "\n";
  std::cout << "e approx: " << e_approx(1000) << "\n";

  constexpr static auto reduce = sequence_sum_reduce(100);
  constexpr static auto gen = sequence_sum_generator(100);
  constexpr static auto fib = fib_recursive(10);
  constexpr static auto e = e_approx(100);
  std::cout << "\n";
  std::cout << "constexpr reduce:   sum of 1 to 100 = " << reduce << "\n";
  std::cout << "constexpr gen:      sum of 1 to 100 = " << gen << "\n";
  std::cout << "constexpr recurse:  fib(10) = " << fib << "\n";
  std::cout << "constexpr e approx: " << e << "\n";
  return 0;
}