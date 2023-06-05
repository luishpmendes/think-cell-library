// Including necessary libraries and files
#include "tc/range/meta.h"
#include "tc/range/filter_adaptor.h"
#include "tc/string/format.h"
#include "tc/string/make_c_str.h"
#include <boost/range/adaptors.hpp>
#include <vector>
#include <cstdio>

// Create a namespace to avoid polluting the global namespace
namespace {

// A helper function to print to the console
template <typename... Args>
void print(Args&&... args) noexcept {
	// Forward the arguments to tc::make_c_str and then to printf
	std::printf("%s", tc::implicit_cast<char const*>(tc::make_c_str<char>(std::forward<Args>(args)...)));
}

// Example of basic usage of think-cell library with vector
void basic () {
	// Define a vector with integers from 1 to 20
	std::vector<int> v = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

	// Filter out odd numbers and print the even numbers
	tc::for_each(
		tc::filter(v, [](const int& n){ return (n%2==0);}),
		[&](auto const& n) {
			print(tc::as_dec(n), ", ");
		}
	);
	print("\n"); // Print a newline for formatting
}

// Example of generator usage 
namespace {
	// A structure that generates a sequence of integers from 0 to 49
	struct generator_range {
		template< typename Func >
		void operator()( Func func ) const& {
			for(int i=0;i<50;++i) {
				func(i);
			}
		}
	};
}

// Example of using generator_range to filter out odd numbers
void ex_generator_range () {
	// Filter out odd numbers and print the even numbers
	tc::for_each( tc::filter( generator_range(), [](int i){ return i%2==0; } ), [](int i) {
		print(tc::as_dec(i), ", ");
	});
	print("\n"); // Print a newline for formatting
}

// Example of generator with a break condition
namespace {
	struct generator_range_break {
		template< typename Func >
		tc::break_or_continue operator()( Func func ) const& {
			using namespace tc;
			for(int i=0;i<5000;++i) {
				if (func(i)==break_) { return break_; }
			}
			return continue_;
		}
	};
}

// Example of using generator_range_break to filter out odd numbers
void ex_generator_range_break () {
	// Filter out odd numbers and print the even numbers. Break after printing 50
	tc::for_each( tc::filter( generator_range_break(), [](int i){ return i%2==0; } ), [](int i) -> tc::break_or_continue {
		print(tc::as_dec(i), ", ");
		return (i>=50)? tc::break_ : tc::continue_;
	});
	print("\n"); // Print a newline for formatting
}

// Example of stacking filters
void stacked_filters() {
	// Using generator_range_break and stacking three filters to filter out multiples of 2, 3, and 5. 
	// The resulting numbers are printed, breaking the loop if the number is greater than 25.
	tc::for_each( tc::filter( tc::filter( tc::filter(
								generator_range_break(),
								[](int i){ return i%2!=0; } ),
								[](int i){ return i%3!=0; } ),
								[](int i){ return i%5!=0; } )
			, [](int i) -> tc::break_or_continue
	{
		print(tc::as_dec(i), ", ");
		return (i>25)? tc::break_ : tc::continue_;
	});
	print("\n"); // Print a newline for formatting
}

} // End of the namespace

// Main function of the program
int main() {
	// Begin the output
	print("-- Running Examples ----------\n");

	// Call the four functions to execute their demonstrations
	basic();
	ex_generator_range();
	ex_generator_range_break();
	stacked_filters();

	// Initialize an array and then a vector with the same values
	using namespace tc;

	int av[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
	auto v = std::vector<int> (av, av+sizeof(av)/sizeof(int)); 

	// Demonstrate using tc::filter with a vector, including an example of using iterators 
	auto r =  tc::filter( tc::filter( tc::filter(
								v,
								[](int i){ return i%2!=0; } ),
								[](int i){ return i%3!=0; } ),
								[](int i){ return i%5!=0; } );

	for (auto it = std::begin(r),
				end = std::end(r);
		it != end;
		++it)
	{
		print(tc::as_dec(*it), ", ");
	}
	print("\n");

	// Demonstrate using Boost's filter equivalent for comparison
	auto br = v | boost::adaptors::filtered([](int i){ return i%2!=0; })
				| boost::adaptors::filtered([](int i){ return i%3!=0; })
				| boost::adaptors::filtered([](int i){ return i%5!=0; });


	for (auto it = std::begin(br),
		end = std::end(br);
		it != end;
		++it)
	{
		print(tc::as_dec(*it), ", ");
	}
	print("\n");

	// Finish the output
	print("-- Done ----------\n");
	std::fflush(stdout); // Flush the output buffer to make sure all output is printed

	return EXIT_SUCCESS; // Indicate successful execution
}
