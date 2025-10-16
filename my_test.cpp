#include <iterator>
#include <iterator>
#include <iostream>
#include <ranges>
#include "stable_vector.hpp"

int main()
{
	my_adt::stable_vector<int> vec(std::from_range_t{}, std::views::iota(1, 10000));

	std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, ", "));
}