#include <chrono>
#include <stdexcept>
#include <vector>

#include <iterator>
#include <iterator>
#include <iostream>
#include <ranges>
#include "stable_vector.hpp"


template <typename Container>
void test_emplace_back(std::size_t iterations)
{
	Container container;

	auto start = std::chrono::high_resolution_clock::now();

	while (iterations --> 0)
	{
		container.emplace_back();
	}
	
	auto finish = std::chrono::high_resolution_clock::now();

	std::println(std::cout, "test finished in {}", finish - start);
}


int main(int argc, char** argv)
{
	std::size_t iterations = std::stoull(argv[2]);

	switch (*(argv[1]))
	{
		case 'v':
		{
			test_emplace_back<std::vector<int>>(iterations);
			break;
		}
		case 'l':
		{
			test_emplace_back<std::list<int>>(iterations);
			break;
		}
		case 's':
		{
			test_emplace_back<my_adt::stable_vector<int>>(iterations);
			break;
		}
		default:
		{
			throw std::runtime_error("invalid args");
		}
	}
}