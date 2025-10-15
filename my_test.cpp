#include <iterator>
#include <iterator>
#include <iostream>
#include <ranges>
#include "stable_vector.hpp"

int main()
{
	my_adt::stable_vector<int> vec{std::from_range_t{}, std::views::iota(1, 10)};

	// for (auto it = vec.begin(); it != vec.end(); it++)
	// {
	// 	std::cout << *it << "\n";
	// }
	std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, ", "));
}