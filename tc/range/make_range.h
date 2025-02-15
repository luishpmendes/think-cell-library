
// think-cell public library
//
// Copyright (C) 2016-2023 think-cell Software GmbH
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../array.h"

namespace tc {
	template<typename T0, typename... T>
	constexpr decltype(auto) make_range(T0&& t0, T&&... t) noexcept {
		if constexpr( (std::is_same<T0, T>::value && ...) ) {
			return tc::make_array<std::conditional_t<std::is_reference<T0>::value, T0, std::remove_cv_t<T0>>>(tc::aggregate_tag,
				std::forward<T0>(t0), std::forward<T>(t)...
			);
		} else {
			return tc::transform(
				tc::make_tuple(
					tc::make_reference_or_value(std::forward<T0>(t0)),
					tc::make_reference_or_value(std::forward<T>(t))...
				),
				tc::fn_indirection()
			);
		}
	}
}
