
// think-cell public library
//
// Copyright (C) 2016-2023 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once
#include "../base/assert_defs.h"
#include "../base/explicit_cast.h"
#include "../base/bit_cast.h"
#include "../algorithm/for_each.h"
#include "../algorithm/empty.h"
#include "../algorithm/minmax.h"
#include "../range/subrange.h"
#include "../range/concat_adaptor.h"
#include "../range/repeat_n.h"
#include "value_restrictive.h"

// The following code disables warnings about the use of deprecated declarations when the code is compiled with Clang.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations" // sprintf is deprecated in Xcode14.1 RC
#endif
#include <boost/lexical_cast.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace tc {
	///////////////
	// Wrapper to print integers as decimal

    // Begin namespace `integral_as_padded_dec_adl`
    // This namespace includes templates for padding decimal integers.
 	namespace integral_as_padded_dec_adl {
        // Template for a struct `integral_as_padded_dec_impl`, which prints integers as decimal.
        // Uses template specialization to differentiate between base case and recursive case.
		
        // Recursive case
        template< typename T, std::size_t N>
		struct integral_as_padded_dec_impl;

        // Base case
		template< typename T>
		struct integral_as_padded_dec_impl<T,1>;

        // Recursive case is defined here.
        // This struct is used to print an integer as a padded decimal by recursively calling itself.
		template< typename T, std::size_t N>
		struct [[nodiscard]] integral_as_padded_dec_impl : protected integral_as_padded_dec_impl<T,N-1> {
			friend auto range_output_t_impl(integral_as_padded_dec_impl const&) -> tc::type::list<tc::char_ascii>; // declaration only
			static constexpr unsigned long long c_nTenPow=integral_as_padded_dec_impl<T,N-1>::c_nTenPow*10;
			constexpr integral_as_padded_dec_impl( T n ) noexcept : integral_as_padded_dec_impl<T,N-1>(n) {}

			template<typename Sink>
			auto operator()(Sink sink) const& MAYTHROW -> tc::common_type_t<
				decltype(tc::continue_if_not_break(std::declval<Sink&>(), std::declval<tc::char_ascii>())),
				decltype(std::declval<integral_as_padded_dec_impl<T,N-1> const&>()(std::declval<Sink>()))
			> {
				static_assert( std::is_unsigned<T>::value );
				if( this->m_n<integral_as_padded_dec_impl::c_nTenPow ) {
					tc_yield(sink, tc::char_ascii('0'));
				}
				return tc::base_cast< integral_as_padded_dec_impl<T,N-1> >(*this)(tc_move(sink));
			}

			constexpr bool empty() const& noexcept { return false; }
		};

        // Base case is defined here.
        // The struct provides a definition for a wrapper to print a single-digit integer as a padded decimal.
 		template< typename T>
		struct [[nodiscard]] integral_as_padded_dec_impl<T,1> {
			friend auto range_output_t_impl(integral_as_padded_dec_impl const&) -> tc::type::list<tc::char_ascii>; // declaration only
			T m_n;
			static constexpr unsigned long long c_nTenPow=1;
			constexpr integral_as_padded_dec_impl( T n ) noexcept : m_n(n) {}

			template<typename Sink>
			auto operator()(Sink&& sink) const& MAYTHROW {
				return tc::for_each(tc::transform(tc::ptr_begin( boost::lexical_cast< std::array<char,50> >(m_n+0/*force integral promotion, otherwise unsigned/signed char gets printed as character*/) ), tc::fn_explicit_cast<tc::char_ascii>()), std::forward<Sink>(sink));
			}

			constexpr bool empty() const& noexcept { return false; }
		};
	}
    // End namespace `integral_as_padded_dec_adl`

    // as_dec is a function that takes an integer and returns a decimal representation of it.
	template< tc::actual_integer T>
	constexpr auto as_dec(T t) return_ctor_noexcept(
		TC_FWD(integral_as_padded_dec_adl::integral_as_padded_dec_impl<T, 1>),
		(t)
	)

	template< typename T >
	constexpr auto as_dec(tc::size_proxy<T> const& t) return_decltype_noexcept(
		tc::as_dec(t.m_t)
	)

	template< std::size_t N, tc::actual_integer T>
	constexpr auto as_padded_dec(T t) return_ctor_noexcept(
		TC_FWD(integral_as_padded_dec_adl::integral_as_padded_dec_impl<std::make_unsigned_t<T>, N>),
		(tc::as_unsigned(t))
	)

	template< std::size_t N, typename T >
	constexpr auto as_padded_dec(tc::size_proxy<T> const& t) return_decltype_noexcept(
		tc::as_padded_dec<N>(t.m_t)
	)

	TC_DEFINE_ENUM(casing, BOOST_PP_EMPTY(), (uppercase)(lowercase));

    // The following set of functions and structures define a wrapper to print integers as hexadecimal.
	namespace as_hex_adl {
		///////////////
		// Wrapper to print integers as hex
		template< typename T, unsigned int nWidth, tc::casing c>
		struct [[nodiscard]] as_hex_impl final {
			friend auto range_output_t_impl(as_hex_impl const&) -> tc::type::list<tc::char_ascii>; // declaration only
		private:
			typename boost::uint_t< CHAR_BIT*sizeof(T) >::exact m_n;
		public:
			constexpr as_hex_impl( T const& n ) noexcept : m_n(tc::bit_cast< typename boost::uint_t< CHAR_BIT*sizeof(T) >::exact >(n)) {} // print the bit pattern of anything we get

			template<typename Sink>
			auto operator()(Sink sink) const& MAYTHROW -> decltype(tc::continue_if_not_break(std::declval<Sink&>(), std::declval<tc::char_ascii>())) {
				static_assert( 0<nWidth );
				static_assert( nWidth<=(sizeof(m_n)*CHAR_BIT+3)/4 );
				using return_t = decltype(tc::continue_if_not_break(std::declval<Sink&>(), std::declval<tc::char_ascii>()));

				auto nShift=sizeof(m_n)*CHAR_BIT;
				do {
					nShift-=4;
				} while( nWidth*4<=nShift && 0==(m_n>>nShift) );
				for(;;) {
					auto const nDigit=(m_n>>nShift)&0xf;
					tc_yield(sink, nDigit<10 ? tc::char_ascii('0')+nDigit : tc::char_ascii(tc::lowercase==c ? 'a' : 'A')+(nDigit-10));
					if constexpr (!std::is_same<return_t, tc::constant<tc::break_>>::value) {
						if (0 == nShift) break;
						nShift -= 4;
					}
				}
				if constexpr (!std::is_same<return_t, tc::constant<tc::break_>>::value) {
					return tc::constant<tc::continue_>();
				}
			}
		};
	}
	using as_hex_adl::as_hex_impl;

    // The as_hex function takes an integer and returns a hexadecimal representation of it.
	template< unsigned int nWidth, tc::casing c=tc::uppercase, typename T >
	constexpr auto as_hex(T const& t) return_ctor_noexcept(
		TC_FWD(as_hex_impl<T, nWidth, c>),
		(t)
	)

	template< tc::casing c=tc::uppercase, typename T >
	constexpr auto as_padded_hex(T const& t) return_ctor_noexcept(
		TC_FWD(as_hex_impl<T, (sizeof(T)*CHAR_BIT+3)/4, c>),
		(t)
	)

	//Do not use in XML, because the standard wants hexBinary to be padded to an even length
	template< tc::casing c=tc::uppercase, typename T >
	constexpr auto as_unpadded_hex(T const& t) return_ctor_noexcept(
		TC_FWD(as_hex_impl<T, 1, c>),
		(t)
	)

	//////////////////////////////////////////////////
	// conversion from string to number

    // The following functions convert strings to numbers, checking for overflow or underflow and throwing exceptions when necessary.
	template< typename T, typename Rng >
	auto unsigned_integer_from_string_head(Rng&& rng) noexcept {
		auto pairnit=std::make_pair(tc::explicit_cast<T>(0),tc::begin(rng));
		auto const itEnd=tc::end(rng);
		while( pairnit.second!=itEnd ) {
			unsigned int const nDigit=*pairnit.second-tc::explicit_cast<tc::range_value_t<Rng&>>('0');
			if( 9<nDigit || (std::numeric_limits<T>::max()-static_cast<int>(nDigit))/10<pairnit.first ) break; // overflow
			pairnit.first*=10;
MODIFY_WARNINGS_BEGIN(((disable)(4244))) // conversion from 'const unsigned int' to 'uint16_t', possible loss of data
			pairnit.first+=nDigit;
MODIFY_WARNINGS_END
			++pairnit.second;
		}
		return pairnit;
	}

	template< typename T, typename Rng >
	auto signed_integer_from_string_head(Rng&& rng) noexcept {
		auto pairnit=std::make_pair(tc::explicit_cast<T>(0),tc::begin(rng));
		auto const itEnd=tc::end(rng);
		if( pairnit.second!=itEnd ) {
			if (tc::explicit_cast<tc::range_value_t<Rng&>>('-') == *pairnit.second) {
				++pairnit.second;
				while (pairnit.second != itEnd) {
					unsigned int const nDigit = *pairnit.second - tc::explicit_cast<tc::range_value_t<Rng&>>('0');
					if (9 < nDigit || pairnit.first < (std::numeric_limits<T>::lowest() + static_cast<int>(nDigit)) / 10) break; // underflow
					pairnit.first *= 10;
MODIFY_WARNINGS_BEGIN(((disable)(4244))) // conversion from 'const unsigned int' to 'uint16_t', possible loss of data
					pairnit.first -= nDigit;
MODIFY_WARNINGS_END
					++pairnit.second;
				}
			} else if (tc::explicit_cast<tc::range_value_t<Rng&>>('+') == *pairnit.second) {
				pairnit = unsigned_integer_from_string_head<T>(tc::begin_next<tc::return_drop>(rng));
			} else {
				pairnit = unsigned_integer_from_string_head<T>(rng);
			}
		}
		return pairnit;
	}

	struct integer_parse_exception final {};

	template< typename T, typename Rng >
	T signed_integer_from_string( Rng const& rng ) THROW(tc::integer_parse_exception) {
		if (tc::empty(rng)) throw tc::integer_parse_exception();
		auto pairnit=tc::signed_integer_from_string_head<T>(rng);
		if( pairnit.second!=tc::end(rng) ) throw tc::integer_parse_exception();
		return pairnit.first;
	}

	template< typename T, typename Rng >
	T unsigned_integer_from_string( Rng const& rng ) THROW(tc::integer_parse_exception) {
		if (tc::empty(rng)) throw tc::integer_parse_exception();
		auto pairnit=tc::unsigned_integer_from_string_head<T>(rng);
		if( pairnit.second!=tc::end(rng) ) throw tc::integer_parse_exception();
		return pairnit.first;
	}

	namespace no_adl {
		template<typename Rng>
		struct [[nodiscard]] size_prefixed_impl : private tc::range_adaptor_base_range<Rng> {
			friend auto range_output_t_impl(size_prefixed_impl const&) -> tc::type::list<unsigned char const&>; // declaration only
			using tc::range_adaptor_base_range<Rng>::range_adaptor_base_range;

			template<typename Sink>
			void operator()(Sink&& sink) const& MAYTHROW {
				tc::for_each(tc::concat(tc::as_blob(tc::implicit_cast<std::uint32_t>(tc::size_linear(this->base_range()))), tc::range_as_blob(this->base_range())), std::forward<Sink>(sink)); // THROW(tc::file_failure)
			}
		};
	}

	template< typename Rng >
	auto size_prefixed(Rng&& rng) return_ctor_noexcept(
		no_adl::size_prefixed_impl<Rng>,
		(aggregate_tag, std::forward<Rng>(rng))
	)

	inline auto size_prefixed(tc::empty_range) noexcept {
		static constexpr std::uint32_t nSize=0;
		return tc::as_blob(nSize);
	}

	namespace no_adl {
		template<typename T>
		struct [[nodiscard]] bool_prefixed_impl {
			friend auto range_output_t_impl(bool_prefixed_impl const&) -> tc::type::list<unsigned char const&>; // declaration only

			template<typename Rhs>
			bool_prefixed_impl(aggregate_tag_t, Rhs&& rhs) noexcept
				: m_ot(aggregate_tag, std::forward<Rhs>(rhs))
			{}

			template<typename Sink>
			void operator()(Sink&& sink) const& MAYTHROW {
				if(*m_ot) {
					tc::for_each(tc::concat(tc::as_blob(true), tc::as_blob(**m_ot)), std::forward<Sink>(sink)); // THROW(tc::file_failure)
				} else {
					tc::for_each(tc::as_blob(false), std::forward<Sink>(sink)); // THROW(tc::file_failure)
				}
			}
		private:
			tc::reference_or_value<T> m_ot;
		};
	}

	template< typename T > requires tc::instance<std::remove_reference_t<T>, std::optional>
	auto bool_prefixed(T&& t) return_ctor_noexcept(
		no_adl::bool_prefixed_impl<T>,
		(aggregate_tag, std::forward<T>(t))
	)
}
