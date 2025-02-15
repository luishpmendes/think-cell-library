
// think-cell public library
//
// Copyright (C) 2016-2023 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../array.h"
#include "../algorithm/accumulate.h"

namespace tc {
	namespace cartesian_product_adaptor_adl {
		template<bool HasIterator, typename... Rng>
		struct cartesian_product_adaptor;
	}

	template<typename... Rng>
	using cartesian_product_adaptor = cartesian_product_adaptor_adl::cartesian_product_adaptor<(0 < sizeof...(Rng)) && (tc::range_with_iterators<Rng> && ...), Rng...>;

	namespace cartesian_product_adaptor_detail {
		template<typename T>
		constexpr static T&& select_element(T&& val, tc::constant<true>) noexcept {
			return std::forward<T>(val);
		}

		template<typename T>
		constexpr static T const&& select_element(T&& val, tc::constant<false>) noexcept {
			return tc::const_forward<T>(val);
		}
	}

	namespace cartesian_product_adaptor_adl {
		template<typename... Rng>
		struct [[nodiscard]] cartesian_product_adaptor</*HasIterator*/false, Rng...> {
		protected:
			tc::tuple<tc::range_adaptor_base_range<Rng>...> m_tupleadaptbaserng;

		public:
			constexpr cartesian_product_adaptor() = default;
			template<typename... Rhs>
			constexpr cartesian_product_adaptor(aggregate_tag_t, Rhs&&... rhs) noexcept
				: m_tupleadaptbaserng{{ {{aggregate_tag, std::forward<Rhs>(rhs)}}... }}
			{}

			static bool constexpr c_bIsCartesianProductAdaptor = true;

		private:
			template<typename Self, typename Sink, typename... Ts>
			struct cartesian_product_sink {
				using guaranteed_break_or_continue = std::conditional_t<
					std::is_same<tc::constant<tc::continue_>, tc::guaranteed_break_or_continue_t<Sink>>::value,
					tc::constant<tc::continue_>,
					tc::break_or_continue
				>;

				Self& m_self;
				Sink const& m_sink;
				tc::tuple<Ts...>& m_ts;

				template<typename T>
				constexpr auto operator()(T&& val) const& return_decltype_MAYTHROW(
					std::remove_reference_t<Self>::internal_for_each_impl( // recursive MAYTHROW
						std::forward<Self>(m_self),
						m_sink,
						tc::tuple_cat(
							/*cast to const rvalue*/std::move(tc::as_const(m_ts)),
							tc::forward_as_tuple(
								cartesian_product_adaptor_detail::select_element(
									std::forward<T>(val),
									tc::constant<sizeof...(Rng) == sizeof...(Ts) + 1>()
								)
							)
						)
					)
				)
			};

			template<typename Self, typename Sink, typename... Ts, std::enable_if_t<sizeof...(Ts) == sizeof...(Rng)>* = nullptr>
			static constexpr auto internal_for_each_impl(Self const&, Sink const& sink, tc::tuple<Ts...> ts) return_decltype_MAYTHROW(
				tc::continue_if_not_break(sink, tc_move(ts)) // MAYTHROW
			)

			template<typename Self, typename Sink, typename... Ts, std::enable_if_t<sizeof...(Ts) < sizeof...(Rng)>* = nullptr>
			static constexpr auto internal_for_each_impl(Self&& self, Sink const& sink, tc::tuple<Ts...> ts) return_decltype_MAYTHROW(
				tc::for_each(
					tc::get<sizeof...(Ts)>(std::forward<Self>(self).m_tupleadaptbaserng).base_range(),
					cartesian_product_sink<Self, Sink, Ts...>{self, sink, ts}
				) // recursive MAYTHROW
			)

		public:
			template<typename Self, typename Sink, std::enable_if_t<tc::decayed_derived_from<Self, cartesian_product_adaptor>>* = nullptr> // use terse syntax when Xcode supports https://cplusplus.github.io/CWG/issues/2369.html
			friend constexpr auto for_each_impl(Self&& self, Sink const sink) return_decltype_MAYTHROW(
				std::remove_reference_t<Self>::internal_for_each_impl(std::forward<Self>(self), sink, tc::make_tuple()) // MAYTHROW
			)

			template<typename Self, std::enable_if_t<tc::decayed_derived_from<Self, cartesian_product_adaptor>>* = nullptr> // use terse syntax when Xcode supports https://cplusplus.github.io/CWG/issues/2369.html
			friend auto range_output_t_impl(Self&&) -> tc::type::list<tc::tuple<
				tc::type::apply_t<
					tc::common_reference_xvalue_as_ref_t,
					tc::type::transform_t<
						tc::range_output_t<decltype(*std::declval<tc::apply_cvref_t<tc::reference_or_value<Rng>, Self>>())>,
						std::add_rvalue_reference_t
					>
				>...
			>> {} // unevaluated

			// Use variadic tc::mul and boost::multiprecision::number?
			constexpr std::size_t size() const& noexcept requires (... && requires { {tc::size_raw(std::declval<Rng const&>())} -> std::convertible_to<std::size_t>; }) {
				return tc::accumulate(
					m_tupleadaptbaserng,
					tc::explicit_cast<std::size_t>(1),
					[](std::size_t& nAccu, auto const& adaptbaserng) noexcept {
						tc::assign_mul(nAccu, tc::explicit_cast<std::size_t>(tc::size_raw(adaptbaserng.base_range())));
					}
				);
			}
		};

		template <typename RangeReturn, IF_TC_CHECKS(typename CheckUnique,) typename CartesianProductAdaptor, typename... Ts> requires std::remove_reference_t<CartesianProductAdaptor>::c_bIsCartesianProductAdaptor
		[[nodiscard]] constexpr decltype(auto) find_first_or_unique_impl(tc::type::identity<RangeReturn>, IF_TC_CHECKS(CheckUnique bCheckUnique,) CartesianProductAdaptor&& rngtpl, tc::tuple<Ts...> const& tpl) MAYTHROW {
			if constexpr( RangeReturn::requires_iterator ) {
				typename std::remove_reference_t<CartesianProductAdaptor>::tc_index idx;
				if (tc::continue_ == tc::for_each(
					tc::zip(std::remove_reference_t<CartesianProductAdaptor>::base_ranges(std::forward<CartesianProductAdaptor>(rngtpl)), tpl, idx),
					[&](auto&& baserng, auto const& t, auto& baseidx) MAYTHROW {
						if( auto it = tc::find_first_or_unique(tc::type::identity<tc::return_element_or_null>(), IF_TC_CHECKS(bCheckUnique, ) tc_move_if_owned(baserng), t) ) { // MAYTHROW
							baseidx = tc::iterator2index<decltype(baserng)>(tc_move(it));
							return tc::continue_;
						} else {
							return tc::break_;
						}
					}
				) ) {
					return RangeReturn::pack_element(rngtpl.make_iterator(tc_move(idx)), std::forward<CartesianProductAdaptor>(rngtpl)); // MAYTHROW
				} else {
					return RangeReturn::template pack_no_element(std::forward<CartesianProductAdaptor>(rngtpl));
				}
			} else if constexpr( std::same_as<RangeReturn, tc::return_bool> ) {
				return tc::all_of(
					tc::zip(std::remove_reference_t<CartesianProductAdaptor>::base_ranges(std::forward<CartesianProductAdaptor>(rngtpl)), tpl),
					[&](auto&& baserng, auto const& t) MAYTHROW {
						return tc::find_first_or_unique(tc::type::identity<tc::return_bool>(), IF_TC_CHECKS(bCheckUnique, ) tc_move_if_owned(baserng), t); // MAYTHROW
					}
				);
			} else {
				tc::range_value_t<CartesianProductAdaptor> tplFound; // TODO Support non-default constructible and non-assignable types.
				if (tc::continue_ == tc::for_each(
					tc::zip(std::remove_reference_t<CartesianProductAdaptor>::base_ranges(std::forward<CartesianProductAdaptor>(rngtpl)), tpl, tplFound),
					[&](auto&& baserng, auto const& t, auto& tFound) MAYTHROW {
						if( auto ot = tc::find_first_or_unique(tc::type::identity<tc::return_value_or_none>(), IF_TC_CHECKS(bCheckUnique, ) tc_move_if_owned(baserng), t) ) { // MAYTHROW
							tFound = *tc_move(ot);
							return tc::continue_;
						} else {
							return tc::break_;
						}
					}
				)) {
					return RangeReturn::template pack_element<CartesianProductAdaptor>(tc_move_if_owned(tplFound)); // MAYTHROW
				} else {
					return RangeReturn::template pack_no_element<CartesianProductAdaptor>();
				}
			}
		}

		template<typename Rng0, typename... Rng>
		struct [[nodiscard]] cartesian_product_adaptor</*HasIterator*/true, Rng0, Rng...>
			: product_index_range_adaptor<cartesian_product_adaptor, /*IndexTemplate*/tc::tuple, Rng0, Rng...> {
		private:
			using this_type = cartesian_product_adaptor;
			using base_ = typename cartesian_product_adaptor::product_index_range_adaptor;
		public:
			constexpr cartesian_product_adaptor() = default;
			using base_::base_;

			using typename base_::tc_index;
			using difference_type = std::ptrdiff_t; // Like .size(), which returns size_t.

		private:
			static bool constexpr c_bCommonRange = tc::has_end_index<std::remove_reference_t<Rng0>>;

			constexpr bool internal_at_end_index(tc_index const& idx) const& MAYTHROW {
				return tc::any_of(tc::zip(this->m_tupleadaptbaserng, idx), [](auto const& adaptbaserng, auto const& baseidx) MAYTHROW {
					return tc::at_end_index(adaptbaserng.base_range(), baseidx); // MAYTHROW
				});
			}

		private:
			STATIC_FINAL_MOD(constexpr, begin_index)() const& MAYTHROW -> tc_index {
				auto idx = tc::tuple_transform(this->m_tupleadaptbaserng, tc_mem_fn(.base_begin_index)); // MAYTHROW
				if constexpr( c_bCommonRange ) {
					if( internal_at_end_index(idx) ) { // MAYTHROW
						tc::get<0>(idx) = tc::get<0>(this->m_tupleadaptbaserng).base_end_index(); // MAYTHROW
					}
				}
				return idx;
			}

			STATIC_FINAL_MOD(constexpr, end_index)() const& MAYTHROW requires c_bCommonRange {
				return tc::tuple_transform(tc::enumerate(this->m_tupleadaptbaserng), [](auto const nconst, auto const& adaptbaserng) MAYTHROW {
					if constexpr( 0 == nconst() ) {
						return adaptbaserng.base_end_index(); // MAYTHROW
					} else {
						return adaptbaserng.base_begin_index(); // MAYTHROW
					}
				});
			}

			STATIC_FINAL_MOD(constexpr, at_end_index)(tc_index const& idx) const& MAYTHROW -> bool {
				if constexpr( c_bCommonRange ) {
					return tc::at_end_index(tc::get<0>(this->m_tupleadaptbaserng).base_range(), tc::get<0>(idx)); // MAYTHROW
				} else {
					// TODO This is suboptimal.
					// We should represent end by setting the end iterator at the first common range or add a bool to the index, if no factor is a common range.
					// On may also argue that the index should always contain the bool because increment_index, decrement_index and advance_index usually know
					// whether the resulting index is at end.
					return internal_at_end_index(idx); // MAYTHROW
				}
			}

			STATIC_FINAL_MOD(constexpr, increment_index)(tc_index& idx) const& MAYTHROW -> void {
				tc::for_each(tc::reverse(tc::enumerate(tc::zip(this->m_tupleadaptbaserng, idx))), [](auto const nconst, auto const& adaptbaserng, auto& baseidx) MAYTHROW {
					tc::increment_index(adaptbaserng.base_range(), baseidx); // MAYTHROW
					if constexpr( 0 != nconst() ) {
						if( tc::at_end_index(adaptbaserng.base_range(), baseidx) ) { // MAYTHROW
							baseidx = adaptbaserng.base_begin_index(); // MAYTHROW
							return tc::continue_;
						} else {
							return tc::break_;
						}
					}
				});
			}

			STATIC_FINAL_MOD(constexpr, decrement_index)(tc_index& idx) const& MAYTHROW -> void
				requires
					(tc::has_decrement_index<std::remove_reference_t<Rng0>> && ... && tc::has_decrement_index<std::remove_reference_t<Rng>>) &&
					(... && tc::has_end_index<std::remove_reference_t<Rng>>)
			{
				tc::for_each(tc::reverse(tc::enumerate(tc::zip(this->m_tupleadaptbaserng, idx))), [](auto const nconst, auto const& adaptbaserng, auto& baseidx) noexcept {
					if constexpr( 0 != nconst() ) {
						if( adaptbaserng.base_begin_index() == baseidx ) { // MAYTHROW
							baseidx = adaptbaserng.base_end_index(); // MAYTHROW
							tc::decrement_index(adaptbaserng.base_range(), baseidx); // MAYTHROW
							return tc::continue_;
						}
					}
					tc::decrement_index(adaptbaserng.base_range(), baseidx); // MAYTHROW
					return tc::break_;
				});
			}

			STATIC_FINAL_MOD(constexpr, advance_index)(tc_index& idx, difference_type d) const& MAYTHROW -> void
				requires
					(tc::has_advance_index<std::remove_reference_t<Rng0>> && ... && tc::has_advance_index<std::remove_reference_t<Rng>>) &&
					(... && tc::has_distance_to_index<std::remove_reference_t<Rng>>) &&
					(... && tc::has_size<std::remove_reference_t<Rng>>)
			{
				if( d != 0 ) {
					tc::for_each(
						tc::reverse(tc::enumerate(tc::zip(this->m_tupleadaptbaserng, idx))),
						[&](auto const nconst, auto&& adaptbaserng, auto& baseidx) MAYTHROW {
							if constexpr( 0 == nconst() ) {
								tc::advance_index(adaptbaserng.base_range(), baseidx, tc::explicit_cast<typename boost::range_difference<decltype(adaptbaserng.base_range())>::type>(d)); // MAYTHROW
							} else {
								auto const nSize = tc::explicit_cast<difference_type>(tc::size_raw(adaptbaserng.base_range()));
								auto const nOldIdx = tc::distance_to_index(adaptbaserng.base_range(), adaptbaserng.base_begin_index(), baseidx);
								auto nNewIdx = nOldIdx + d;
								d = nNewIdx / nSize;
								nNewIdx %= nSize;
								if(nNewIdx < 0) {
									nNewIdx+=nSize;
									--d;
								}
								tc::advance_index(adaptbaserng.base_range(), baseidx, tc::explicit_cast<typename boost::range_difference<decltype(adaptbaserng.base_range())>::type>(nNewIdx - nOldIdx)); // MAYTHROW
								return tc::continue_if(d != 0);
							}
						}
					);
				}
			}

			STATIC_FINAL_MOD(constexpr, distance_to_index)(tc_index const& idxLhs, tc_index const& idxRhs) const& MAYTHROW -> difference_type
				requires
					(tc::has_distance_to_index<std::remove_reference_t<Rng0>> && ... && tc::has_distance_to_index<std::remove_reference_t<Rng>>) &&
					(... && tc::has_size<std::remove_reference_t<Rng>>)
			{
				return tc::accumulate(
					tc::enumerate(tc::zip(this->m_tupleadaptbaserng, idxLhs, idxRhs)),
					tc::explicit_cast<difference_type>(0),
					[](auto& nAccu, auto const nconst, auto const& adaptbaserng, auto const& baseidxLhs, auto const& baseidxRhs) MAYTHROW {
						if constexpr( 0 < nconst() ) {
							tc::assign_mul(nAccu, tc::explicit_cast<difference_type>(tc::size_raw(adaptbaserng.base_range()))); // MAYTHROW
						}
						tc::assign_add(nAccu, tc::explicit_cast<difference_type>(tc::distance_to_index(adaptbaserng.base_range(), baseidxLhs, baseidxRhs))); // MAYTHROW
					}
				);
			}

			STATIC_FINAL_MOD(constexpr, middle_point)(tc_index& idx, tc_index const& idxEnd) const& MAYTHROW -> void
				requires
					(tc::has_middle_point<std::remove_reference_t<Rng0>> && ... && tc::has_middle_point<std::remove_reference_t<Rng>>)
			{
				bool const bAdvanced = false;
				tc::for_each(
					tc::zip(this->m_tupleadaptbaserng, idx, idxEnd),
					[&](auto const& adaptbaserng, auto& baseidx, auto const& baseidxEnd) MAYTHROW {
						if( bAdvanced ) {
							baseidx = adaptbaserng.base_begin_index(); // MAYTHROW
						} else if( baseidx != baseidxEnd ) {
							auto const baseidxBegin = baseidx;
							tc::middle_point(adaptbaserng.base_range(), baseidx, baseidxEnd); // MAYTHROW
							bAdvanced = baseidxBegin != baseidx;
						}
					}
				);
			}
		};
	}

	namespace no_adl {
		template<bool HasIterator, tc::has_constexpr_size... Rng>
		struct constexpr_size_impl<cartesian_product_adaptor_adl::cartesian_product_adaptor<HasIterator, Rng...>>
			: tc::constant<(... * tc::constexpr_size<Rng>::value)>
		{};

		template<typename... Rng>
		struct is_index_valid_for_move_constructed_range<cartesian_product_adaptor_adl::cartesian_product_adaptor</*HasIterator*/true, Rng...>>
			: std::conjunction<tc::is_index_valid_for_move_constructed_range<Rng>...>
		{};
	}

	template<typename... Rng>
	constexpr decltype(auto) cartesian_product(Rng&&... rng) noexcept {
		if constexpr( 0 == sizeof...(Rng) ) {
			return tc::single(tc::tuple<>());
		} else {
			return tc::cartesian_product_adaptor<Rng...>(tc::aggregate_tag, std::forward<Rng>(rng)...);
		}
	}
}
