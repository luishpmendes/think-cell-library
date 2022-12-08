
// think-cell public library
//
// Copyright (C) 2016-2022 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once
#include "../base/assert_defs.h"
#include "index_range.h"
#include "iterator_facade.h"

#include <boost/iterator/detail/facade_iterator_category.hpp>

namespace tc {
	namespace no_adl {
		template<typename It>
		struct nullable_iterator {
			static_assert(tc::decayed<It>);
			template<typename Derived>
			struct base : It {
				using difference_type = typename std::iterator_traits<It>::difference_type;
				using value_type = typename std::iterator_traits<It>::value_type;
				using pointer = typename std::iterator_traits<It>::pointer;
				using reference = typename std::iterator_traits<It>::reference;
				using iterator_category = typename std::iterator_traits<It>::iterator_category;

			private:
				Derived& derived() noexcept {return tc::derived_cast<Derived>(*MSVC_WORKAROUND_THIS);}
			public:

				base() noexcept : m_bValid(false) {}

				// no implicit conversion from It; we must ensure that It has the semantics of element rather than border before allowing the construction
				explicit base(It const& it) noexcept : It(it), m_bValid(true) {}
				explicit base(It && it) noexcept : It(tc_move(it)), m_bValid(true) {}

				Derived& operator=(It const& it) & noexcept {
					It::operator=(it);
					m_bValid = true;
					return derived();
				}
				Derived& operator=(It && it) & noexcept {
					It::operator=(tc_move(it));
					m_bValid = true;
					return derived();
				}

				explicit operator bool() const& noexcept {
					return m_bValid;
				}

				Derived& operator++() & noexcept {
					_ASSERT(m_bValid);
					++tc::base_cast<It>(*this);
					return derived();
				}

				Derived& operator--() & noexcept {
					_ASSERT(m_bValid);
					--tc::base_cast<It>(*this);
					return derived();
				}

				reference operator*() const& noexcept {
					_ASSERT(m_bValid);
					return *tc::base_cast<It>(*this);
				}

				pointer operator->() const& noexcept {
					_ASSERT(m_bValid);
					return tc::base_cast<It>(*this).operator->();
				}

			protected:
				bool m_bValid;
			};

			struct element_type final : base<element_type> {
				using base_ = base<element_type>;
				using base_::base_;
				using base_::operator=;

				template <typename It_ = It>
				auto element_base() const& noexcept {
					using ItBase = typename nullable_iterator<decltype(tc::base_cast<It>(*this).element_base())>::element_type;
					return this->m_bValid ? ItBase{ tc::base_cast<It>(*this).element_base() } : ItBase{};
				}
			};

			struct border_type final : base<border_type> {
				using base_ = base<border_type>;
				using base_::base_;
				using base_::operator=;

				template <typename It_ = It>
				auto border_base() const& noexcept {
					using ItBase = typename nullable_iterator<decltype(tc::base_cast<It>(*this).border_base())>::border_type;
					return this->m_bValid ? ItBase{ tc::base_cast<It>(*this).border_base() } : ItBase{};
				}
			};
		};

		template<typename It> requires tc::is_explicit_castable<bool, It>::value
		struct nullable_iterator<It> {
			static_assert(tc::decayed<It>);
			using element_type = It;
			using border_type = It;
		};
	}

	template< typename It >
	using element_t = typename no_adl::nullable_iterator<It>::element_type;
	
	template< typename It >
	using border_t = typename no_adl::nullable_iterator<It>::border_type;

	template< typename It >
	constexpr auto make_element(It&& it) noexcept {
		return element_t<tc::decay_t<It>>(std::forward<It>(it));
	}

	// Regarding constness:
	// const_iterators point to const ranges; non-const iterators point to non-const ranges.
	// The index type, traversal, and difference_type must be the same regardless of whether the range is const.
	// The reference type, and pointer type may be different.
	// The value_type should be the same but there is nothing preventing it from being different.
		
	// Other than dereference_index, base_range and creating subranges, all range functions are called on const references.

	template< typename T, bool bConst >
	using conditional_const_t=std::conditional_t< bConst, T const, T >;

	namespace index_iterator_impl {
		template<typename IndexRange, bool bConst>
		struct index_iterator;
	}
	using index_iterator_impl::index_iterator;

	namespace index_iterator_traits_no_adl {
		template< typename IndexRange>
		struct difference_type_base {
			using difference_type = std::ptrdiff_t; // needed to compile interfaces relying on difference_type
		};

		template< typename IndexRange> requires has_mem_fn_distance_to_index<IndexRange>::value
		struct difference_type_base<IndexRange> {
			using difference_type = decltype(std::declval<IndexRange const&>().distance_to_index(std::declval<typename IndexRange::tc_index const&>(), std::declval<typename IndexRange::tc_index const&>()));
		};

		template< typename IndexRange, typename = void>
		struct value_type_base {
			using value_type = void;
		};

		template< typename IndexRange >
		struct value_type_base<IndexRange, tc::void_t<tc::range_value_t<IndexRange>>> {
			using value_type = tc::range_value_t<IndexRange>;
		};
	}
}

namespace std {
	template<typename IndexRange, bool bConst>
	struct iterator_traits<tc::index_iterator<IndexRange, bConst>>
		: tc::index_iterator_traits_no_adl::difference_type_base<IndexRange> // should not be different for const and non-const
		, tc::index_iterator_traits_no_adl::value_type_base<tc::conditional_const_t<IndexRange, bConst>&>
	{
		static_assert(tc::has_index<IndexRange>::value);

		// IndexRange::dereference_index does not take an argument of type Index. Did you write tc::transform(Rng, Fn) and Fn takes the wrong argument?
		using reference = tc::iter_reference_t<tc::index_iterator<IndexRange, bConst>>;

		// Since C++20, iterator_traits<It>::pointer is synthesized in this way by default, if typename It::pointer does not exist.
		using pointer = decltype(std::declval<tc::index_iterator<IndexRange, bConst>&>().operator->());

		// Some of our ranges, may not even conform to LecayInputIterator concept.
		// We ignore the requirements on value_type and reference, because library implementors do not rely on them anyway.
		using iterator_category = std::conditional_t<tc::has_mem_fn_decrement_index<IndexRange>::value,
			std::conditional_t<tc::has_mem_fn_distance_to_index<IndexRange>::value,
				std::random_access_iterator_tag,
				std::bidirectional_iterator_tag
			>,
			std::forward_iterator_tag
		>;
	};
}

namespace tc {
	namespace no_adl {
		struct end_sentinel final {};
	}
	using no_adl::end_sentinel;

	namespace index_iterator_impl {
		template<typename IndexRange, bool bConst>
		struct index_iterator : tc::iterator_facade<index_iterator<IndexRange, bConst>>
		{
			static_assert(tc::decayed<IndexRange>);

		private:
			friend class boost::iterator_core_access;
			friend struct index_iterator<IndexRange,!bConst>;

			conditional_const_t<IndexRange, bConst>* m_pidxrng;

		public: // TODO private
			using tc_index = index_t<IndexRange>;
			tc_index m_idx;

		private:
			using this_type = index_iterator<IndexRange, bConst>;

		public:
			using is_index_iterator = void;

			constexpr index_iterator() noexcept
				: m_pidxrng(nullptr)
				, m_idx()
			{}

			template<bool bConstOther> requires bConst || (!bConstOther)
			constexpr index_iterator(index_iterator<IndexRange,bConstOther> const& other) noexcept
				: m_pidxrng(other.m_pidxrng)
				, m_idx(other.m_idx)
			{}

			constexpr index_iterator(conditional_const_t<IndexRange, bConst>* pidxrng, tc_index idx) noexcept
				: m_pidxrng(pidxrng)
				, m_idx(tc_move(idx))
			{}

			constexpr tc_index const& get_index() const& noexcept {
				return m_idx;
			}

			template< ENABLE_SFINAE, std::enable_if_t<tc::has_mem_fn_middle_point<SFINAE_TYPE(IndexRange)>::value>* = nullptr >
			friend index_iterator middle_point( index_iterator const& itBegin, index_iterator const& itEnd ) noexcept {
				index_iterator it=itBegin;
				_ASSERTE(itBegin.m_pidxrng);
				_ASSERTE(itBegin.m_pidxrng == itEnd.m_pidxrng);
				tc::as_const(*itBegin.m_pidxrng).middle_point(it.m_idx,itEnd.m_idx);
				return it;
			}

			template< typename IndexRange_, bool bConst1, bool bConst2>
				requires tc::has_mem_fn_distance_to_index<IndexRange_>::value
			friend constexpr auto operator -(
				index_iterator<IndexRange_, bConst1> const& itLhs,
				index_iterator<IndexRange_, bConst2> const& itRhs) noexcept;

			template< ENABLE_SFINAE, std::enable_if_t<tc::has_mem_fn_base_range<SFINAE_TYPE(IndexRange)>::value && tc::has_mem_fn_border_base_index<IndexRange>::value>* = nullptr >
			constexpr auto border_base() const& noexcept {
				return tc::make_iterator( VERIFY(m_pidxrng)->base_range(), tc::as_const(*m_pidxrng).border_base_index(m_idx));
			}

			template< ENABLE_SFINAE, std::enable_if_t<tc::has_mem_fn_base_range<SFINAE_TYPE(IndexRange)>::value && tc::has_mem_fn_element_base_index<IndexRange>::value>* = nullptr >
			constexpr auto element_base() const& noexcept {
				using It = tc::element_t<tc::decay_t<decltype(tc::make_iterator(m_pidxrng->base_range(), tc::as_const(*m_pidxrng).element_base_index(m_idx)))>>;
				if(m_pidxrng) {
					return It(tc::make_iterator(m_pidxrng->base_range(), tc::as_const(*m_pidxrng).element_base_index(m_idx)));
				} else {
					return It();
				}
			}

			constexpr decltype(auto) operator*() const noexcept(noexcept(m_pidxrng->dereference_index(m_idx))) {
				return VERIFY(m_pidxrng)->dereference_index(m_idx);
			}

			constexpr this_type& operator++() noexcept(noexcept(m_pidxrng->increment_index(m_idx))) {
				tc::as_const(*VERIFY(m_pidxrng)).increment_index(m_idx);
				return *this;
			}

			template<ENABLE_SFINAE, std::enable_if_t<tc::has_mem_fn_decrement_index<SFINAE_TYPE(IndexRange)>::value>* = nullptr>
			constexpr this_type& operator--() noexcept(noexcept(m_pidxrng->decrement_index(m_idx))) {
				tc::as_const(*VERIFY(m_pidxrng)).decrement_index(m_idx);
				return *this;
			}

			// subrange from iterator pair
			template<typename IndexRange_, bool bConst_>
			friend constexpr tc::make_subrange_result_t< conditional_const_t<IndexRange_,bConst_> & > make_iterator_range_impl( index_iterator<IndexRange_, bConst_> itBegin, index_iterator<IndexRange_, bConst_> itEnd ) noexcept;

			explicit constexpr operator bool() const& noexcept {
				return tc::explicit_cast<bool>(m_pidxrng);
			}

			friend constexpr bool operator==(index_iterator const& it, end_sentinel) noexcept {
				return tc::as_const(*VERIFY(it.m_pidxrng)).at_end_index(it.m_idx);
			}

			template<typename N> requires tc::has_mem_fn_advance_index<IndexRange>::value
			constexpr this_type& operator+=(N&& n) noexcept(noexcept(m_pidxrng->advance_index(m_idx, n))) {
				tc::as_const(*VERIFY(m_pidxrng)).advance_index(m_idx, std::forward<N>(n));
				return *this;
			}

			template< typename IndexRange_, bool bConst1, bool bConst2> requires tc::is_equality_comparable<tc::index_t<IndexRange_>>::value
			friend constexpr bool operator==(index_iterator<IndexRange_, bConst1> const& itLhs, index_iterator<IndexRange_, bConst2> const& itRhs) noexcept;
		};

		template<typename IndexRange, bool bConst>
		constexpr tc::make_subrange_result_t< conditional_const_t<IndexRange,bConst> & > make_iterator_range_impl( index_iterator<IndexRange, bConst> itBegin, index_iterator<IndexRange, bConst> itEnd ) noexcept {
			return tc::make_subrange_result_t< conditional_const_t<IndexRange,bConst> & >( *VERIFYEQUALNOPRINT(VERIFY(itBegin.m_pidxrng),itEnd.m_pidxrng), tc_move(itBegin).m_idx, tc_move(itEnd).m_idx );
		}

		template< typename IndexRange_, bool bConst1, bool bConst2> requires tc::is_equality_comparable<tc::index_t<IndexRange_>>::value
		constexpr bool operator==(index_iterator<IndexRange_, bConst1> const& itLhs, index_iterator<IndexRange_, bConst2> const& itRhs) noexcept {
			_ASSERTE(itLhs.m_pidxrng == itRhs.m_pidxrng);
			_ASSERTDEBUG( itLhs.m_pidxrng || itLhs.m_idx == itRhs.m_idx );
			return itLhs.m_idx == itRhs.m_idx;
		}

		template< typename IndexRange_, bool bConst1, bool bConst2> requires tc::has_mem_fn_distance_to_index<IndexRange_>::value
		constexpr auto operator -(
			index_iterator<IndexRange_, bConst1> const& itLhs,
			index_iterator<IndexRange_, bConst2> const& itRhs) noexcept {
			_ASSERTE(itLhs.m_pidxrng);
			_ASSERTE(itLhs.m_pidxrng == itRhs.m_pidxrng);
			return tc::as_const(*itLhs.m_pidxrng).distance_to_index(itRhs.m_idx, itLhs.m_idx);
		}

	}
	using index_iterator_impl::index_iterator;

	namespace no_adl {
		template <typename IndexRange, bool b>
		struct is_stashing_element<tc::index_iterator<IndexRange,b>> : tc::constant<IndexRange::c_bHasStashingIndex> {};
	}

	namespace iterator2index_detail {
		BOOST_MPL_HAS_XXX_TRAIT_DEF(is_index_iterator)
	}

	template<typename It, std::enable_if_t<!iterator2index_detail::has_is_index_iterator<std::remove_reference_t<It>>::value>* = nullptr>
	constexpr decltype(auto) iterator2index(It&& it) noexcept {
		return std::forward<It>(it);
	}

	template<typename It, std::enable_if_t<iterator2index_detail::has_is_index_iterator<std::remove_reference_t<It>>::value>* = nullptr>
	constexpr auto iterator2index(It&& it) return_decltype_xvalue_by_ref_noexcept(
		std::forward<It>(it).m_idx
	)
}