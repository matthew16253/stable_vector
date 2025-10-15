#include <iterator>
#include <iterator>
#include <iostream>
#include <ranges>
#include "stable_vector.hpp"

int main()
{
	my_adt::stable_vector<int> vec({1, 2, 3});
	my_adt::stable_vector<int> vec2({1, 2, 3});
	vec2 = vec;
	std::copy(vec2.crbegin(), vec2.crend(), std::ostream_iterator<int>(std::cout, ", "));
}