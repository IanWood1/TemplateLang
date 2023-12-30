// TemplateLang.cpp : Defines the entry point for the application.
//

#include "TemplateLang.h"

using namespace std;

int use(int x) {
  // turn off formatting for this section
  // clang-format off
  //using f = function< variable<0>, variable<0>>;
  // 
  // Recursive Fibonacci
  //using f = function<
		//  if_<
  //      lteq<variable<0>, value<1>>,
  //      value<1>,
  //      add<
  //        recurse<
  //          add<variable<0>, value<-1>>>,
  //        recurse<
  //          add<variable<0>, value<-2>>>>>,
		//variable<0>>;

  // sum from 1 to n
  using f = function<
    reduce<
			function< add< variable<0>, variable<1> >, variable<0>, variable<1> >,
			value<1>,
			variable<0>>,
variable<0>>;

  auto exp = program<
      call<
        f, 
        variable<0>>,
        1>{};
  return exp.run(x);
  // clang-format on
}

int main() {
  std::vector<int> x;
  static auto val = use(10);
  std::cout << val << std::endl;
  return 0;
}