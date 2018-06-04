/*  This file is part of the Vc library. {{{
Copyright © 2016-2017 Matthias Kretz <kretz@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of contributing organizations nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_SIMD_AVX512_H_
#define VC_SIMD_AVX512_H_

#include "macros.h"
#ifdef Vc_HAVE_AVX512_ABI
#include "avx.h"
#include "storage.h"
#include "simd_tuple.h"
#include "x86/intrinsics.h"
#include "x86/convert.h"

// clang 3.9 doesn't have the _MM_CMPINT_XX constants defined {{{
#ifndef _MM_CMPINT_EQ
#define _MM_CMPINT_EQ 0x0
#endif
#ifndef _MM_CMPINT_LT
#define _MM_CMPINT_LT 0x1
#endif
#ifndef _MM_CMPINT_LE
#define _MM_CMPINT_LE 0x2
#endif
#ifndef _MM_CMPINT_UNUSED
#define _MM_CMPINT_UNUSED 0x3
#endif
#ifndef _MM_CMPINT_NE
#define _MM_CMPINT_NE 0x4
#endif
#ifndef _MM_CMPINT_NLT
#define _MM_CMPINT_NLT 0x5
#endif
#ifndef _MM_CMPINT_GE
#define _MM_CMPINT_GE 0x5
#endif
#ifndef _MM_CMPINT_NLE
#define _MM_CMPINT_NLE 0x6
#endif
#ifndef _MM_CMPINT_GT
#define _MM_CMPINT_GT 0x6
#endif /*}}}*/

Vc_VERSIONED_NAMESPACE_BEGIN
namespace detail
{
// simd impl {{{1
struct avx512_simd_impl : public generic_simd_impl<avx512_simd_impl, simd_abi::__avx512> {
    // member types {{{2
    using abi = simd_abi::__avx512;
    template <class T> static constexpr size_t size() { return simd_size_v<T, abi>; }
    template <class T> using simd_member_type = avx512_simd_member_type<T>;
    template <class T> using intrinsic_type = intrinsic_type_t<T, 64 / sizeof(T)>;
    template <class T> using mask_member_type = avx512_mask_member_type<T>;
    template <class T> using simd = Vc::simd<T, abi>;
    template <class T> using simd_mask = Vc::simd_mask<T, abi>;
    template <size_t N> using size_tag = size_constant<N>;
    template <class T> using type_tag = T *;

    // make_simd {{{2
    template <class T>
    static Vc_INTRINSIC simd<T> make_simd(simd_member_type<T> x)
    {
        return {detail::private_init, x};
    }

    // load {{{2
    // from long double has no vector implementation{{{3
    template <class T, class F>
    static Vc_INTRINSIC simd_member_type<T> load(const long double *mem, F,
                                                    type_tag<T>) Vc_NOEXCEPT_OR_IN_TEST
    {
        return generate_storage<T, size<T>()>(
            [&](auto i) { return static_cast<T>(mem[i]); });
    }

    // load without conversion{{{3
    template <class T, class F>
    static Vc_INTRINSIC simd_member_type<T> load(const T *mem, F f, type_tag<T>) Vc_NOEXCEPT_OR_IN_TEST
    {
        return detail::load64(mem, f);
    }

    // convert from an AVX512 load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) == sizeof(U)> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, simd_member_type<U>>(load64(mem, f));
    }

    // convert from an AVX load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) == sizeof(U) * 2> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, avx_simd_member_type<U>>(load32(mem, f));
    }

    // convert from an SSE load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) == sizeof(U) * 4> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, sse_simd_member_type<U>>(load16(mem, f));
    }

    // convert from a half SSE load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) == sizeof(U) * 8> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, sse_simd_member_type<U>>(load8(mem, f));
    }

    // convert from a 2-AVX512 load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) * 2 == sizeof(U)> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, simd_member_type<U>>(
            load64(mem, f), load64(mem + size<U>(), f));
    }

    // convert from a 4-AVX512 load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) * 4 == sizeof(U)> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, simd_member_type<U>>(
            load64(mem, f), load64(mem + size<U>(), f), load64(mem + 2 * size<U>(), f),
            load64(mem + 3 * size<U>(), f));
    }

    // convert from a 8-AVX512 load{{{3
    template <class T, class U, class F>
    static Vc_INTRINSIC simd_member_type<T> load(
        const U *mem, F f, type_tag<T>,
        enable_if<sizeof(T) * 8 == sizeof(U)> = nullarg) Vc_NOEXCEPT_OR_IN_TEST
    {
        return convert<simd_member_type<T>, simd_member_type<U>>(
            load64(mem, f), load64(mem + size<U>(), f), load64(mem + 2 * size<U>(), f),
            load64(mem + 3 * size<U>(), f), load64(mem + 4 * size<U>(), f),
            load64(mem + 5 * size<U>(), f), load64(mem + 6 * size<U>(), f),
            load64(mem + 7 * size<U>(), f));
    }

    // masked load {{{2
    template <class T, class U, class... Abis, size_t... Indexes>
    static Vc_INTRINSIC simd_member_type<T> convert_helper(
        const simd_tuple<U, Abis...> &uncvted, std::index_sequence<Indexes...>)
    {
        return x86::convert<simd_member_type<T>>(detail::get<Indexes>(uncvted)...);
    }
    template <class T, class U, class F>
    static Vc_INTRINSIC void masked_load(simd_member_type<T> &merge, mask_member_type<T> k,
                                         const U *mem, F) Vc_NOEXCEPT_OR_IN_TEST
    {
        if constexpr(sizeof(U) > 8) {
            // no SIMD support, use a scalar loop
            bit_iteration(k.d, [&](auto i) { merge.set(i, static_cast<T>(mem[i])); });
        } else {
            static_assert(!std::is_same<T, U>::value, "");
            using fixed_traits = detail::traits<U, simd_abi::fixed_size<size<T>()>>;
            using fixed_impl = typename fixed_traits::simd_impl_type;
            typename fixed_traits::simd_member_type uncvted{};
            fixed_impl::masked_load(uncvted, static_cast<ullong>(k), mem, F());
            masked_assign(k, merge,
                          convert_helper<T>(
                              uncvted, std::make_index_sequence<uncvted.tuple_size>()));
        }
    }

    // non-converting masked loads
    template <class T, class F>
    static Vc_INTRINSIC void masked_load(simd_member_type<T> &merge, mask_member_type<T> k,
                                         const T *mem, F) Vc_NOEXCEPT_OR_IN_TEST
    {
        if constexpr (have_avx512bw && sizeof(T) == 1) {
            merge = _mm512_mask_loadu_epi8(merge, k, mem);
        } else if constexpr (have_avx512bw && sizeof(T) == 2) {
            merge = _mm512_mask_loadu_epi16(merge, k, mem);
        } else if constexpr (sizeof(T) == 4 && std::is_integral_v<T>) {
            merge = _mm512_mask_loadu_epi32(merge, k, mem);
        } else if constexpr (sizeof(T) == 4 && std::is_floating_point_v<T>) {
            merge = _mm512_mask_loadu_ps(merge, k, mem);
        } else if constexpr (sizeof(T) == 8 && std::is_integral_v<T>) {
            merge = _mm512_mask_loadu_epi64(merge, k, mem);
        } else if constexpr (sizeof(T) == 8 && std::is_floating_point_v<T>) {
            merge = _mm512_mask_loadu_pd(merge, k, mem);
        } else {
            execute_n_times<size<T>()>([&](auto i) {
                if (k[i]) {
                    merge.set(i, static_cast<T>(mem[i]));
                }
            });
        }
    }

    // store {{{2
    template <class T, class U, class F>
    static Vc_INTRINSIC void store(simd_member_type<T> v, U *mem, F,
                                   type_tag<T>) Vc_NOEXCEPT_OR_IN_TEST
    {
        if constexpr (std::is_same_v<T, U>) {
            builtin_store(v, mem, F());
        } else if constexpr (sizeof(U) <= 8) {  // make sure to skip long double
            if constexpr (sizeof(T) == sizeof(U) * 8) {
                builtin_store<8>(convert<sse_simd_member_type<U>>(v), mem, F());
            } else if constexpr (sizeof(T) == sizeof(U) * 4) {
                builtin_store(convert<sse_simd_member_type<U>>(v), mem, F());
            } else if constexpr (sizeof(T) == sizeof(U) * 2) {
                builtin_store(convert<avx_simd_member_type<U>>(v), mem, F());
            } else if constexpr (sizeof(T) == sizeof(U)) {
                builtin_store(convert<simd_member_type<U>>(v), mem, F());
            } else if constexpr (sizeof(T) * 2 == sizeof(U)) {
                builtin_store(convert<simd_member_type<U>>(lo256(v)), mem, F());
                builtin_store(convert<simd_member_type<U>>(hi256(v)), mem + size<U>(), F());
            } else if constexpr (sizeof(T) * 4 == sizeof(U)) {
                builtin_store(convert<simd_member_type<U>>(lo128(v)), mem, F());
                builtin_store(convert<simd_member_type<U>>(extract128<1>(v)), mem + size<U>(),
                        F());
                builtin_store(convert<simd_member_type<U>>(extract128<2>(v)),
                        mem + 2 * size<U>(), F());
                builtin_store(convert<simd_member_type<U>>(extract128<3>(v)),
                        mem + 3 * size<U>(), F());
            } else if constexpr (sizeof(T) * 8 == sizeof(U)) {
                const std::array<simd_member_type<U>, 8> converted = x86::convert_all<simd_member_type<U>>(v);
                builtin_store(converted[0], mem + 0 * size<U>(), F());
                builtin_store(converted[1], mem + 1 * size<U>(), F());
                builtin_store(converted[2], mem + 2 * size<U>(), F());
                builtin_store(converted[3], mem + 3 * size<U>(), F());
                builtin_store(converted[4], mem + 4 * size<U>(), F());
                builtin_store(converted[5], mem + 5 * size<U>(), F());
                builtin_store(converted[6], mem + 6 * size<U>(), F());
                builtin_store(converted[7], mem + 7 * size<U>(), F());
            } else {
                detail::assert_unreachable<T>();
            }
        } else {
            execute_n_times<size<T>()>([&](auto i) { mem[i] = v[i]; });
        }
    }

    // masked store {{{2
    template <class T, class F>
    static Vc_INTRINSIC void masked_store(simd_member_type<T> v, long double *mem, F,
                                          mask_member_type<T> k) Vc_NOEXCEPT_OR_IN_TEST
    {
        // no support for long double
        execute_n_times<size<T>()>([&](auto i) {
            if (k[i]) {
                mem[i] = v[i];
            }
        });
    }

    template <class T, class U, class F>
    static Vc_INTRINSIC void masked_store(simd_member_type<T> v, U *mem, F,
                                          mask_member_type<T> k) Vc_NOEXCEPT_OR_IN_TEST
    {
        constexpr bool truncate = std::is_integral_v<T> && std::is_integral_v<U> && sizeof(T) > sizeof(U);
        using V = simd_member_type<U>;
        using M = mask_member_type<U>;

        if constexpr (std::is_same_v<T, U> ||
                      (std::is_integral_v<T> && std::is_integral_v<U> &&
                       sizeof(T) == sizeof(U))) {
            // bitwise or no conversion, reinterpret:
            x86::maskstore(storage_bitcast<U>(v), mem, F(), k);
        } else if constexpr(truncate && sizeof(T) == 8) {
            if constexpr (sizeof(U) == 4) {
                _mm512_mask_cvtepi64_storeu_epi32(mem, k, v);
            } else if constexpr (sizeof(U) == 2) {
                _mm512_mask_cvtepi64_storeu_epi16(mem, k, v);
            } else if constexpr (sizeof(U) == 1) {
                _mm512_mask_cvtepi64_storeu_epi8(mem, k, v);
            }
        } else if constexpr (truncate && sizeof(T) == 4) {
            if constexpr (sizeof(U) == 2) {
                _mm512_mask_cvtepi32_storeu_epi16(mem, k, v);
            } else if constexpr (sizeof(U) == 1) {
                _mm512_mask_cvtepi32_storeu_epi8(mem, k, v);
            }
        } else if constexpr (truncate && have_avx512bw && sizeof(T) == 2) {
            _mm512_mask_cvtepi16_storeu_epi8(mem, k, v);
        } else if constexpr (sizeof(T) == sizeof(U)) {
            x86::maskstore(convert<V>(v), mem, F(), M(k.d));
        } else if constexpr (sizeof(T) * 2 == sizeof(U)) {
            const std::array<V, 2> converted = convert_all<V>(v);
            x86::maskstore(converted[0], mem, F(), M(k >> 0));
            x86::maskstore(converted[1], mem + V::width, F(), M(k >> V::width));
        } else if constexpr (sizeof(T) * 4 == sizeof(U)) {
            const std::array<V, 4> converted = convert_all<V>(v);
            x86::maskstore(converted[0], mem, F(), M(k >> 0));
            x86::maskstore(converted[1], mem + 1 * V::width, F(), M(k >> 1 * V::width));
            x86::maskstore(converted[2], mem + 2 * V::width, F(), M(k >> 2 * V::width));
            x86::maskstore(converted[3], mem + 3 * V::width, F(), M(k >> 3 * V::width));
        } else if constexpr (sizeof(T) * 8 == sizeof(U)) {
            const std::array<V, 8> converted = convert_all<V>(v);
            x86::maskstore(converted[0], mem, F(), M(k >> 0));
            x86::maskstore(converted[1], mem + 1 * V::width, F(), M(k >> 1 * V::width));
            x86::maskstore(converted[2], mem + 2 * V::width, F(), M(k >> 2 * V::width));
            x86::maskstore(converted[3], mem + 3 * V::width, F(), M(k >> 3 * V::width));
            x86::maskstore(converted[4], mem + 4 * V::width, F(), M(k >> 4 * V::width));
            x86::maskstore(converted[5], mem + 5 * V::width, F(), M(k >> 5 * V::width));
            x86::maskstore(converted[6], mem + 6 * V::width, F(), M(k >> 6 * V::width));
            x86::maskstore(converted[7], mem + 7 * V::width, F(), M(k >> 7 * V::width));
        } else if constexpr (sizeof(T) > sizeof(U)) {
            x86::maskstore(convert<V>(v), mem, F(), k.d);
        } else {
            static_assert(!std::is_same_v<T, T>, "this should be unreachable");
            execute_n_times<size<T>()>([&](auto i) {
                if (k[i]) {
                    mem[i] = static_cast<T>(v[i]);
                }
            });
        }
    }

    // negation {{{2
    // override because of __mmask return type
    template <class T>
    static Vc_INTRINSIC mask_member_type<T> negate(simd_member_type<T> x) noexcept
    {
        return equal_to(x, simd_member_type<T>());
    }

    // }}}2
};

// simd_mask impl {{{1
struct avx512_mask_impl : public generic_mask_impl<simd_abi::__avx512> {
    // member types {{{2
    using abi = simd_abi::__avx512;
    template <class T> static constexpr size_t size() { return simd_size_v<T, abi>; }
    template <size_t N> using mask_member_type = avx512_mask_member_type_n<N>;
    template <class T> using simd_mask = Vc::simd_mask<T, abi>;
    template <size_t N> using size_tag = size_constant<N>;
    template <class T> using type_tag = T *;

    // to_bitset {{{2
    template <size_t N>
    static Vc_INTRINSIC std::bitset<N> to_bitset(mask_member_type<N> v) noexcept
    {
        return v.intrin();
    }

    // from_bitset{{{2
    template <size_t N, class T>
    static Vc_INTRINSIC mask_member_type<N> from_bitset(std::bitset<N> bits, type_tag<T>)
    {
        return bits.to_ullong();
    }

    // broadcast {{{2
    static Vc_INTRINSIC __mmask8 broadcast_impl(bool x, size_tag<8>) noexcept
    {
        return static_cast<__mmask8>(x) * ~__mmask8();
    }
    static Vc_INTRINSIC __mmask16 broadcast_impl(bool x, size_tag<16>) noexcept
    {
        return static_cast<__mmask16>(x) * ~__mmask16();
    }
    static Vc_INTRINSIC __mmask32 broadcast_impl(bool x, size_tag<32>) noexcept
    {
        return static_cast<__mmask32>(x) * ~__mmask32();
    }
    static Vc_INTRINSIC __mmask64 broadcast_impl(bool x, size_tag<64>) noexcept
    {
        return static_cast<__mmask64>(x) * ~__mmask64();
    }
    template <typename T> static Vc_INTRINSIC auto broadcast(bool x, type_tag<T>) noexcept
    {
        return broadcast_impl(x, size_tag<size<T>()>());
    }

    // load {{{2
    template <class F>
    static Vc_INTRINSIC __mmask8 load(const bool *mem, F, size_tag<8>) noexcept
    {
        const auto a = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(mem));
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
        return _mm_test_epi8_mask(a, a);
#else
        const auto b = _mm512_cvtepi8_epi64(a);
        return _mm512_test_epi64_mask(b, b);
#endif  // Vc_HAVE_AVX512BW
    }
    template <class F>
    static Vc_INTRINSIC __mmask16 load(const bool *mem, F f, size_tag<16>) noexcept
    {
        const auto a = load16(mem, f);
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
        return _mm_test_epi8_mask(a, a);
#else
        const auto b = _mm512_cvtepi8_epi32(a);
        return _mm512_test_epi32_mask(b, b);
#endif  // Vc_HAVE_AVX512BW
    }
    template <class F>
    static Vc_INTRINSIC __mmask32 load(const bool *mem, F f, size_tag<32>) noexcept
    {
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
        const auto a = load32(mem, f);
        return _mm256_test_epi8_mask(a, a);
#else
        const auto a = _mm512_cvtepi8_epi32(load16(mem, f));
        const auto b = _mm512_cvtepi8_epi32(load16(mem + 16, f));
        return _mm512_test_epi32_mask(a, a) | (_mm512_test_epi32_mask(b, b) << 16);
#endif  // Vc_HAVE_AVX512BW
    }
    template <class F>
    static Vc_INTRINSIC __mmask64 load(const bool *mem, F f, size_tag<64>) noexcept
    {
#ifdef Vc_HAVE_AVX512BW
        const auto a = load64(mem, f);
        return _mm512_test_epi8_mask(a, a);
#else
        const auto a = _mm512_cvtepi8_epi32(load16(mem, f));
        const auto b = _mm512_cvtepi8_epi32(load16(mem + 16, f));
        const auto c = _mm512_cvtepi8_epi32(load16(mem + 32, f));
        const auto d = _mm512_cvtepi8_epi32(load16(mem + 48, f));
        return _mm512_test_epi32_mask(a, a) | (_mm512_test_epi32_mask(b, b) << 16) |
               (_mm512_test_epi32_mask(b, b) << 32) | (_mm512_test_epi32_mask(b, b) << 48);
#endif  // Vc_HAVE_AVX512BW
    }

    // masked load {{{2
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
    template <class F>
    static Vc_INTRINSIC void masked_load(mask_member_type<8> &merge,
                                         mask_member_type<8> mask, const bool *mem,
                                         F) noexcept
    {
        const auto a = _mm_mask_loadu_epi8(zero<__m128i>(), mask, mem);
        merge = (merge & ~mask) | _mm_test_epi8_mask(a, a);
    }

    template <class F>
    static Vc_INTRINSIC void masked_load(mask_member_type<16> &merge,
                                         mask_member_type<16> mask, const bool *mem,
                                         F) noexcept
    {
        const auto a = _mm_mask_loadu_epi8(zero<__m128i>(), mask, mem);
        merge = (merge & ~mask) | _mm_test_epi8_mask(a, a);
    }

    template <class F>
    static Vc_INTRINSIC void masked_load(mask_member_type<32> &merge,
                                         mask_member_type<32> mask, const bool *mem,
                                         F) noexcept
    {
        const auto a = _mm256_mask_loadu_epi8(zero<__m256i>(), mask, mem);
        merge = (merge & ~mask) | _mm256_test_epi8_mask(a, a);
    }

    template <class F>
    static Vc_INTRINSIC void masked_load(mask_member_type<64> &merge,
                                         mask_member_type<64> mask, const bool *mem,
                                         F) noexcept
    {
        const auto a = _mm512_mask_loadu_epi8(zero<__m512i>(), mask, mem);
        merge = (merge & ~mask) | _mm512_test_epi8_mask(a, a);
    }

#else
    template <size_t N, class F>
    static Vc_INTRINSIC void masked_load(mask_member_type<N> &merge,
                                         const mask_member_type<N> mask, const bool *mem,
                                         F) noexcept
    {
        detail::execute_n_times<N>([&](auto i) {
            if (mask[i]) {
                merge.set(i, mem[i]);
            }
        });
    }
#endif

    // store {{{2
    template <class F>
    static Vc_INTRINSIC void store(mask_member_type<8> v, bool *mem, F f,
                                   size_tag<8>) noexcept
    {
        builtin_store<8>(
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
            _mm_maskz_set1_epi8(v, 1),
#elif defined __x86_64__
            make_storage<ullong>(_pdep_u64(v, 0x0101010101010101ULL), 0ull),
#else
            make_storage<uint>(_pdep_u32(v, 0x01010101U), _pdep_u32(v >> 4, 0x01010101U)),
#endif
            mem, f);
    }
    template <class F>
    static Vc_INTRINSIC void store(mask_member_type<16> v, bool *mem, F f,
                                   size_tag<16>) noexcept
    {
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
        builtin_store(_mm_maskz_set1_epi8(v, 1), mem, f);
#else
        _mm512_mask_cvtepi32_storeu_epi8(mem, ~__mmask16(),
                                         _mm512_maskz_set1_epi32(v, 1));
        unused(f);
#endif
    }
#ifdef Vc_HAVE_AVX512BW
    template <class F>
    static Vc_INTRINSIC void store(mask_member_type<32> v, bool *mem, F f,
                                   size_tag<32>) noexcept
    {
#if defined Vc_HAVE_AVX512VL
        builtin_store(_mm256_maskz_set1_epi8(v, 1), mem, f);
#else
        builtin_store(lo256(_mm512_maskz_set1_epi8(v, 1)), mem, f);
#endif
    }
    template <class F>
    static Vc_INTRINSIC void store(mask_member_type<64> v, bool *mem, F f,
                                   size_tag<64>) noexcept
    {
        builtin_store(_mm512_maskz_set1_epi8(v, 1), mem, f);
    }
#endif  // Vc_HAVE_AVX512BW

    // masked store {{{2
    template <class F>
    static Vc_INTRINSIC void masked_store(mask_member_type<8> v, bool *mem, F,
                                          mask_member_type<8> k) noexcept
    {
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
        _mm_mask_cvtepi16_storeu_epi8(mem, k, _mm_maskz_set1_epi16(v, 1));
#elif defined Vc_HAVE_AVX512VL
        _mm256_mask_cvtepi32_storeu_epi8(mem, k, _mm256_maskz_set1_epi32(v, 1));
#else
        // we rely on k < 0x100:
        _mm512_mask_cvtepi32_storeu_epi8(mem, k, _mm512_maskz_set1_epi32(v, 1));
#endif
    }

    template <class F>
    static Vc_INTRINSIC void masked_store(mask_member_type<16> v, bool *mem, F,
                                          mask_member_type<16> k) noexcept
    {
#if defined Vc_HAVE_AVX512VL && defined Vc_HAVE_AVX512BW
        _mm_mask_storeu_epi8(mem, k, _mm_maskz_set1_epi8(v, 1));
#else
        _mm512_mask_cvtepi32_storeu_epi8(mem, k, _mm512_maskz_set1_epi32(v, 1));
#endif
    }

#ifdef Vc_HAVE_AVX512BW
    template <class F>
    static Vc_INTRINSIC void masked_store(mask_member_type<32> v, bool *mem, F,
                                          mask_member_type<32> k) noexcept
    {
#if defined Vc_HAVE_AVX512VL
        _mm256_mask_storeu_epi8(mem, k, _mm256_maskz_set1_epi8(v, 1));
#else
        _mm256_mask_storeu_epi8(mem, k, lo256(_mm512_maskz_set1_epi8(v, 1)));
#endif
    }

    template <class F>
    static Vc_INTRINSIC void masked_store(mask_member_type<64> v, bool *mem, F,
                                          mask_member_type<64> k) noexcept
    {
        _mm512_mask_storeu_epi8(mem, k, _mm512_maskz_set1_epi8(v, 1));
    }
#endif  // Vc_HAVE_AVX512BW

    // negation {{{2
    template <class T, class SizeTag>
    static Vc_INTRINSIC T negate(const T &x, SizeTag) noexcept
    {
        return ~x.d;
    }

    // smart_reference access {{{2
    template <size_t N>
    static Vc_INTRINSIC void set(mask_member_type<N> &k, int i, bool x) noexcept
    {
        k.set(i, x);
    }
    // }}}2
};

// simd_converter __avx512 -> scalar {{{1
template <class From, class To>
struct simd_converter<From, simd_abi::__avx512, To, simd_abi::scalar> {
    using Arg = avx512_simd_member_type<From>;

    Vc_INTRINSIC std::array<To, Arg::width> operator()(Arg a)
    {
        return impl(std::make_index_sequence<Arg::width>(), a);
    }

    template <size_t... Indexes>
    Vc_INTRINSIC std::array<To, Arg::width> impl(std::index_sequence<Indexes...>, Arg a)
    {
        return {static_cast<To>(a[Indexes])...};
    }
};

// }}}1
// simd_converter scalar -> __avx512 {{{1
template <class From, class To>
struct simd_converter<From, simd_abi::scalar, To, simd_abi::__avx512> {
    using R = avx512_simd_member_type<To>;

    Vc_INTRINSIC R operator()(From a)
    {
        R r{};
        r.set(0, static_cast<To>(a));
        return r;
    }
    Vc_INTRINSIC R operator()(From a, From b)
    {
        R r{};
        r.set(0, static_cast<To>(a));
        r.set(1, static_cast<To>(b));
        return r;
    }
    Vc_INTRINSIC R operator()(From a, From b, From c, From d)
    {
        R r{};
        r.set(0, static_cast<To>(a));
        r.set(1, static_cast<To>(b));
        r.set(2, static_cast<To>(c));
        r.set(3, static_cast<To>(d));
        return r;
    }
    Vc_INTRINSIC R operator()(From a, From b, From c, From d, From e, From f, From g,
                              From h)
    {
        R r{};
        r.set(0, static_cast<To>(a));
        r.set(1, static_cast<To>(b));
        r.set(2, static_cast<To>(c));
        r.set(3, static_cast<To>(d));
        r.set(4, static_cast<To>(e));
        r.set(5, static_cast<To>(f));
        r.set(6, static_cast<To>(g));
        r.set(7, static_cast<To>(h));
        return r;
    }
    Vc_INTRINSIC R operator()(From x0, From x1, From x2, From x3, From x4, From x5,
                              From x6, From x7, From x8, From x9, From x10, From x11,
                              From x12, From x13, From x14, From x15)
    {
        R r{};
        r.set(0, static_cast<To>(x0));
        r.set(1, static_cast<To>(x1));
        r.set(2, static_cast<To>(x2));
        r.set(3, static_cast<To>(x3));
        r.set(4, static_cast<To>(x4));
        r.set(5, static_cast<To>(x5));
        r.set(6, static_cast<To>(x6));
        r.set(7, static_cast<To>(x7));
        r.set(8, static_cast<To>(x8));
        r.set(9, static_cast<To>(x9));
        r.set(10, static_cast<To>(x10));
        r.set(11, static_cast<To>(x11));
        r.set(12, static_cast<To>(x12));
        r.set(13, static_cast<To>(x13));
        r.set(14, static_cast<To>(x14));
        r.set(15, static_cast<To>(x15));
        return r;
    }
    Vc_INTRINSIC R operator()(From x0, From x1, From x2, From x3, From x4, From x5,
                              From x6, From x7, From x8, From x9, From x10, From x11,
                              From x12, From x13, From x14, From x15, From x16, From x17,
                              From x18, From x19, From x20, From x21, From x22, From x23,
                              From x24, From x25, From x26, From x27, From x28, From x29,
                              From x30, From x31)
    {
        R r{};
        r.set(0, static_cast<To>(x0));
        r.set(1, static_cast<To>(x1));
        r.set(2, static_cast<To>(x2));
        r.set(3, static_cast<To>(x3));
        r.set(4, static_cast<To>(x4));
        r.set(5, static_cast<To>(x5));
        r.set(6, static_cast<To>(x6));
        r.set(7, static_cast<To>(x7));
        r.set(8, static_cast<To>(x8));
        r.set(9, static_cast<To>(x9));
        r.set(10, static_cast<To>(x10));
        r.set(11, static_cast<To>(x11));
        r.set(12, static_cast<To>(x12));
        r.set(13, static_cast<To>(x13));
        r.set(14, static_cast<To>(x14));
        r.set(15, static_cast<To>(x15));
        r.set(16, static_cast<To>(x16));
        r.set(17, static_cast<To>(x17));
        r.set(18, static_cast<To>(x18));
        r.set(19, static_cast<To>(x19));
        r.set(20, static_cast<To>(x20));
        r.set(21, static_cast<To>(x21));
        r.set(22, static_cast<To>(x22));
        r.set(23, static_cast<To>(x23));
        r.set(24, static_cast<To>(x24));
        r.set(25, static_cast<To>(x25));
        r.set(26, static_cast<To>(x26));
        r.set(27, static_cast<To>(x27));
        r.set(28, static_cast<To>(x28));
        r.set(29, static_cast<To>(x29));
        r.set(30, static_cast<To>(x30));
        r.set(31, static_cast<To>(x31));
        return r;
    }
    Vc_INTRINSIC R operator()(From x0, From x1, From x2, From x3, From x4, From x5,
                              From x6, From x7, From x8, From x9, From x10, From x11,
                              From x12, From x13, From x14, From x15, From x16, From x17,
                              From x18, From x19, From x20, From x21, From x22, From x23,
                              From x24, From x25, From x26, From x27, From x28, From x29,
                              From x30, From x31, From x32, From x33, From x34, From x35,
                              From x36, From x37, From x38, From x39, From x40, From x41,
                              From x42, From x43, From x44, From x45, From x46, From x47,
                              From x48, From x49, From x50, From x51, From x52, From x53,
                              From x54, From x55, From x56, From x57, From x58, From x59,
                              From x60, From x61, From x62, From x63)
    {
        return R(static_cast<To>(x0), static_cast<To>(x1), static_cast<To>(x2),
                 static_cast<To>(x3), static_cast<To>(x4), static_cast<To>(x5),
                 static_cast<To>(x6), static_cast<To>(x7), static_cast<To>(x8),
                 static_cast<To>(x9), static_cast<To>(x10), static_cast<To>(x11),
                 static_cast<To>(x12), static_cast<To>(x13), static_cast<To>(x14),
                 static_cast<To>(x15), static_cast<To>(x16), static_cast<To>(x17),
                 static_cast<To>(x18), static_cast<To>(x19), static_cast<To>(x20),
                 static_cast<To>(x21), static_cast<To>(x22), static_cast<To>(x23),
                 static_cast<To>(x24), static_cast<To>(x25), static_cast<To>(x26),
                 static_cast<To>(x27), static_cast<To>(x28), static_cast<To>(x29),
                 static_cast<To>(x30), static_cast<To>(x31), static_cast<To>(x32),
                 static_cast<To>(x33), static_cast<To>(x34), static_cast<To>(x35),
                 static_cast<To>(x36), static_cast<To>(x37), static_cast<To>(x38),
                 static_cast<To>(x39), static_cast<To>(x40), static_cast<To>(x41),
                 static_cast<To>(x42), static_cast<To>(x43), static_cast<To>(x44),
                 static_cast<To>(x45), static_cast<To>(x46), static_cast<To>(x47),
                 static_cast<To>(x48), static_cast<To>(x49), static_cast<To>(x50),
                 static_cast<To>(x51), static_cast<To>(x52), static_cast<To>(x53),
                 static_cast<To>(x54), static_cast<To>(x55), static_cast<To>(x56),
                 static_cast<To>(x57), static_cast<To>(x58), static_cast<To>(x59),
                 static_cast<To>(x60), static_cast<To>(x61), static_cast<To>(x62),
                 static_cast<To>(x63));
    }
};

// }}}1
// simd_converter __sse -> __avx512 {{{1
template <class From, class To>
struct simd_converter<From, simd_abi::__sse, To, simd_abi::__avx512> {
    using Arg = sse_simd_member_type<From>;

    Vc_INTRINSIC auto operator()(Arg a)
    {
        return x86::convert_all<avx512_simd_member_type<To>>(a);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b)
    {
        static_assert(2 * sizeof(From) >= sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b, Arg c, Arg d)
    {
        static_assert(sizeof(From) >= 1 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b, c, d);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg x0, Arg x1, Arg x2, Arg x3,
                                                     Arg x4, Arg x5, Arg x6, Arg x7)
    {
        static_assert(sizeof(From) >= 2 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(x0, x1, x2, x3, x4, x5, x6,
                                                           x7);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg x0, Arg x1, Arg x2, Arg x3,
                                                     Arg x4, Arg x5, Arg x6, Arg x7,
                                                     Arg x8, Arg x9, Arg x10, Arg x11,
                                                     Arg x12, Arg x13, Arg x14, Arg x15)
    {
        static_assert(sizeof(From) >= 4 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(
            x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(
        Arg x0, Arg x1, Arg x2, Arg x3, Arg x4, Arg x5, Arg x6, Arg x7, Arg x8, Arg x9,
        Arg x10, Arg x11, Arg x12, Arg x13, Arg x14, Arg x15, Arg x16, Arg x17, Arg x18,
        Arg x19, Arg x20, Arg x21, Arg x22, Arg x23, Arg x24, Arg x25, Arg x26, Arg x27,
        Arg x28, Arg x29, Arg x30, Arg x31)
    {
        static_assert(sizeof(From) >= 8 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(
            x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16,
            x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30, x31);
    }
};

// }}}1
// simd_converter __avx512 -> __sse {{{1
template <class From, class To>
struct simd_converter<From, simd_abi::__avx512, To, simd_abi::__sse> {
    using Arg = avx512_simd_member_type<From>;

    Vc_INTRINSIC auto operator()(Arg a)
    {
        return x86::convert_all<sse_simd_member_type<To>>(a);
    }
    Vc_INTRINSIC sse_simd_member_type<To> operator()(Arg a, Arg b)
    {
        static_assert(sizeof(From) >= 8 * sizeof(To), "");
        return x86::convert<sse_simd_member_type<To>>(a, b);
    }
};

// }}}1
// simd_converter __avx -> __avx512 {{{1
template <class From, class To>
struct simd_converter<From, simd_abi::__avx, To, simd_abi::__avx512> {
    using Arg = avx_simd_member_type<From>;

    Vc_INTRINSIC auto operator()(Arg a)
    {
        return x86::convert_all<avx512_simd_member_type<To>>(a);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b)
    {
        static_assert(sizeof(From) >= 1 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b, Arg c, Arg d)
    {
        static_assert(sizeof(From) >= 2 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b, c, d);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg x0, Arg x1, Arg x2, Arg x3,
                                                     Arg x4, Arg x5, Arg x6, Arg x7)
    {
        static_assert(sizeof(From) >= 4 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(x0, x1, x2, x3, x4, x5, x6,
                                                           x7);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg x0, Arg x1, Arg x2, Arg x3,
                                                     Arg x4, Arg x5, Arg x6, Arg x7,
                                                     Arg x8, Arg x9, Arg x10, Arg x11,
                                                     Arg x12, Arg x13, Arg x14, Arg x15)
    {
        static_assert(sizeof(From) >= 8 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(
            x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15);
    }
};

// }}}1
// simd_converter __avx512 -> __avx {{{1
template <class From, class To>
struct simd_converter<From, simd_abi::__avx512, To, simd_abi::__avx> {
    using Arg = avx512_simd_member_type<From>;

    Vc_INTRINSIC auto operator()(Arg a)
    {
        return x86::convert_all<avx_simd_member_type<To>>(a);
    }
    Vc_INTRINSIC avx_simd_member_type<To> operator()(Arg a, Arg b)
    {
        static_assert(sizeof(From) >= 4 * sizeof(To), "");
        return x86::convert<avx_simd_member_type<To>>(a, b);
    }
    Vc_INTRINSIC avx_simd_member_type<To> operator()(Arg a, Arg b, Arg c, Arg d)
    {
        static_assert(sizeof(From) >= 8 * sizeof(To), "");
        return x86::convert<avx_simd_member_type<To>>(a, b, c, d);
    }
};

// }}}1
// simd_converter __avx512 -> __avx512 {{{1
template <class T> struct simd_converter<T, simd_abi::__avx512, T, simd_abi::__avx512> {
    using Arg = avx512_simd_member_type<T>;
    Vc_INTRINSIC const Arg &operator()(const Arg &x) { return x; }
};

template <class From, class To>
struct simd_converter<From, simd_abi::__avx512, To, simd_abi::__avx512> {
    using Arg = avx512_simd_member_type<From>;

    Vc_INTRINSIC auto operator()(Arg a)
    {
        return x86::convert_all<avx512_simd_member_type<To>>(a);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b)
    {
        static_assert(sizeof(From) >= 2 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b, Arg c, Arg d)
    {
        static_assert(sizeof(From) >= 4 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b, c, d);
    }
    Vc_INTRINSIC avx512_simd_member_type<To> operator()(Arg a, Arg b, Arg c, Arg d, Arg e,
                                                     Arg f, Arg g, Arg h)
    {
        static_assert(sizeof(From) >= 8 * sizeof(To), "");
        return x86::convert<avx512_simd_member_type<To>>(a, b, c, d, e, f, g, h);
    }
};

// }}}1
// generic_simd_impl::masked_cassign specializations {{{1
#define Vc_MASKED_CASSIGN_SPECIALIZATION(TYPE_, TYPE_SUFFIX_, OP_, OP_NAME_)             \
    template <>                                                                          \
    template <>                                                                          \
    Vc_INTRINSIC void Vc_VDECL                                                           \
    generic_simd_impl<avx512_simd_impl, simd_abi::__avx512>::masked_cassign<             \
        OP_, TYPE_, bool, 64 / sizeof(TYPE_)>(                                           \
        const Storage<bool, 64 / sizeof(TYPE_)> k,                                       \
        Storage<TYPE_, 64 / sizeof(TYPE_)> &lhs,                                         \
        const detail::id<Storage<TYPE_, 64 / sizeof(TYPE_)>> rhs)                        \
    {                                                                                    \
        lhs = _mm512_mask_##OP_NAME_##_##TYPE_SUFFIX_(lhs, k, lhs, rhs);                 \
    }                                                                                    \
    Vc_NOTHING_EXPECTING_SEMICOLON

    Vc_MASKED_CASSIGN_SPECIALIZATION(double, pd, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(float, ps, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(detail::llong, epi64, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(detail::ullong, epi64, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(long, epi64, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(detail::ulong, epi64, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(int, epi32, std::plus, add);
    Vc_MASKED_CASSIGN_SPECIALIZATION(detail::uint, epi32, std::plus, add);
#ifdef Vc_HAVE_FULL_AVX512_ABI
Vc_MASKED_CASSIGN_SPECIALIZATION(         short, epi16, std::plus, add);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail::ushort, epi16, std::plus, add);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail:: schar, epi8 , std::plus, add);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail:: uchar, epi8 , std::plus, add);
#endif  // Vc_HAVE_FULL_AVX512_ABI

Vc_MASKED_CASSIGN_SPECIALIZATION(        double,  pd  , std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(         float,  ps  , std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail:: llong, epi64, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail::ullong, epi64, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(          long, epi64, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail:: ulong, epi64, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(           int, epi32, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail::  uint, epi32, std::minus, sub);
#ifdef Vc_HAVE_FULL_AVX512_ABI
Vc_MASKED_CASSIGN_SPECIALIZATION(         short, epi16, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail::ushort, epi16, std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail:: schar, epi8 , std::minus, sub);
Vc_MASKED_CASSIGN_SPECIALIZATION(detail:: uchar, epi8 , std::minus, sub);
#endif  // Vc_HAVE_FULL_AVX512_ABI
#undef Vc_MASKED_CASSIGN_SPECIALIZATION

// }}}1
}  // namespace detail

// [simd_mask.reductions] {{{
template <class T> Vc_ALWAYS_INLINE bool all_of(simd_mask<T, simd_abi::__avx512> k)
{
    const auto v = detail::data(k);
    return detail::x86::testallset(v);
}

template <class T> Vc_ALWAYS_INLINE bool any_of(simd_mask<T, simd_abi::__avx512> k)
{
    const auto v = detail::data(k);
    return v != 0U;
}

template <class T> Vc_ALWAYS_INLINE bool none_of(simd_mask<T, simd_abi::__avx512> k)
{
    const auto v = detail::data(k);
    return v == 0U;
}

template <class T> Vc_ALWAYS_INLINE bool some_of(simd_mask<T, simd_abi::__avx512> k)
{
    const auto v = detail::data(k);
    return v != 0 && !all_of(k);
}

template <class T> Vc_ALWAYS_INLINE int popcount(simd_mask<T, simd_abi::__avx512> k)
{
    const auto v = detail::data(k);
    switch (k.size()) {
    case  8: return detail::popcnt8(v);
    case 16: return detail::popcnt16(v);
    case 32: return detail::popcnt32(v);
    case 64: return detail::popcnt64(v);
    default: Vc_UNREACHABLE();
    }
}

template <class T> Vc_ALWAYS_INLINE int find_first_set(simd_mask<T, simd_abi::__avx512> k)
{
    if constexpr (k.size() <= 32) {
        const auto v = detail::data(k);
        return _tzcnt_u32(v);
    } else {
        const __mmask64 v = detail::data(k);
        return detail::firstbit(v);
    }
}

template <class T> Vc_ALWAYS_INLINE int find_last_set(simd_mask<T, simd_abi::__avx512> k)
{
    if constexpr (k.size() <= 32) {
        return 31 - _lzcnt_u32(detail::data(k));
    } else {
        const __mmask64 v = detail::data(k);
        return detail::lastbit(v);
    }
}

// }}}
Vc_VERSIONED_NAMESPACE_END

#endif  // Vc_HAVE_AVX512_ABI
#endif  // VC_SIMD_AVX512_H_

// vim: foldmethod=marker
