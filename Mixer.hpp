#ifndef PAQ8PX_MIXER_HPP
#define PAQ8PX_MIXER_HPP

#include "IPredictor.hpp"
#include "Shared.hpp"
#include "utils.hpp"
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#elif defined(__ARM_FEATURE_SIMD32) || defined(__ARM_NEON)
#include <arm_neon.h>
typedef int32x4_t __m128i;
#define vreinterpretq_m128i_s8(x) \
	    vreinterpretq_s32_s8(x)
#define vreinterpretq_m128i_s16(x) \
	    vreinterpretq_s32_s16(x)
#define vreinterpretq_m128i_s32(x) \
	    (x)
#define vreinterpretq_s8_m128i(x) \
        vreinterpretq_s8_s32(x)
#define vreinterpretq_s16_m128i(x) \
	    vreinterpretq_s16_s32(x)
#define vreinterpretq_s32_m128i(x) \
	    (x)
#define vreinterpretq_m128i_u16(x) \
	    vreinterpretq_s32_u16(x)

#endif

#if (defined(__GNUC__) || defined(__clang__)) && defined(__AVX2__)
__attribute__((target("avx2")))
#endif
static auto dotProductSimdAvx2(const short *const t, const short *const w, int n) -> int {
#ifndef __AVX2__
  return 0;
#else
  __m256i sum = _mm256_setzero_si256();

  while((n -= 16) >= 0 ) {
    __m256i tmp = _mm256_madd_epi16(*(__m256i * ) & t[n], *(__m256i * ) & w[n]);
    tmp = _mm256_srai_epi32(tmp, 8);
    sum = _mm256_add_epi32(sum, tmp);
  }

  __m128i lo = _mm256_extractf128_si256(sum, 0);
  __m128i hi = _mm256_extractf128_si256(sum, 1);

  __m128i newSum = _mm_hadd_epi32(lo, hi);
  newSum = _mm_add_epi32(newSum, _mm_srli_si128(newSum, 8));
  newSum = _mm_add_epi32(newSum, _mm_srli_si128(newSum, 4));
  return _mm_cvtsi128_si32(newSum);
#endif
}

#if (defined(__GNUC__) || defined(__clang__)) && defined(__AVX2__)

__attribute__((target("avx2")))
#endif
static void trainSimdAvx2(const short *const t, short *const w, int n, const int e) {
#ifndef __AVX2__
  return;
#else
  const __m256i one = _mm256_set1_epi16(1);
  const __m256i err = _mm256_set1_epi16(short(e));

  while((n -= 16) >= 0 ) {
    __m256i tmp = _mm256_adds_epi16(*(__m256i * ) & t[n], *(__m256i * ) & t[n]);
    tmp = _mm256_mulhi_epi16(tmp, err);
    tmp = _mm256_adds_epi16(tmp, one);
    tmp = _mm256_srai_epi16(tmp, 1);
    tmp = _mm256_adds_epi16(tmp, *reinterpret_cast<__m256i *>(&w[n]));
    *reinterpret_cast<__m256i *>(&w[n]) = tmp;
  }
#endif
}

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__ARM_FEATURE_SIMD32) || defined(__ARM_NEON))
static inline __m128i _mm_mulhi_epi16(__m128i a, __m128i b){
  int16x4_t a1 = vget_low_s16(vreinterpretq_s16_m128i(a));
  int16x4_t b1 = vget_low_s16(vreinterpretq_s16_m128i(b));
  int32x4_t ab1 = vmull_s16(a1, b1);
  int16x4_t a2 = vget_high_s16(vreinterpretq_s16_m128i(a));
  int16x4_t b2 = vget_high_s16(vreinterpretq_s16_m128i(b));
  int32x4_t ab2 = vmull_s16(a2, b2); 
  uint16x8x2_t r = vuzpq_u16(vreinterpretq_u16_s32(ab1), vreinterpretq_u16_s32(ab2));
  return vreinterpretq_m128i_u16(r.val[1]);
}

static inline __m128i _mm_madd_epi16(__m128i a, __m128i b) {
    int32x4_t pl = vmull_s16(vget_low_s16(vreinterpretq_s16_m128i(a)), vget_low_s16(vreinterpretq_s16_m128i(b)));
    int32x4_t ph = vmull_s16(vget_high_s16(vreinterpretq_s16_m128i(a)), vget_high_s16(vreinterpretq_s16_m128i(b)));
    int32x2_t rl = vpadd_s32(vget_low_s32(pl), vget_high_s32(pl));
    int32x2_t rh = vpadd_s32(vget_low_s32(ph), vget_high_s32(ph));
    return vreinterpretq_m128i_s32(vcombine_s32(rl, rh));
}

#endif

static auto dotProductSimdNeon(const short* const t, const short* const w, int n) -> int {
#if (!defined(__ARM_FEATURE_SIMD32) && !defined(__ARM_NEON))
    return 0;
#else
    __m128i sum = vreinterpretq_m128i_s32(vdupq_n_s32(0));

    while ((n -= 8) >= 0) {
        __m128i tmp = _mm_madd_epi16(*(__m128i*) & t[n], *(__m128i*) & w[n]);
        tmp = vreinterpretq_m128i_s32(vshrq_n_s32(vreinterpretq_s32_m128i(tmp), 8));
        sum = vreinterpretq_m128i_s32(vaddq_s32(vreinterpretq_s32_m128i(sum), vreinterpretq_s32_m128i(tmp)));
    }

    sum = vreinterpretq_m128i_s32(vaddq_s32(vreinterpretq_s32_m128i(sum), vreinterpretq_s32_m128i(vreinterpretq_m128i_s8(vextq_s8(vreinterpretq_s8_m128i(sum), vdupq_n_s8(0), 8)))));
    sum = vreinterpretq_m128i_s32(vaddq_s32(vreinterpretq_s32_m128i(sum), vreinterpretq_s32_m128i(vreinterpretq_m128i_s8(vextq_s8(vreinterpretq_s8_m128i(sum), vdupq_n_s8(0), 4)))));
    return vgetq_lane_s32(vreinterpretq_s32_m128i(sum), 0);
#endif
}

static void trainSimdNeon(const short* const t, short* const w, int n, const int e) {
#if (!defined(__ARM_FEATURE_SIMD32) && !defined(__ARM_NEON))
  return;
#else
  const __m128i one = vreinterpretq_m128i_s16(vdupq_n_s16(1));
  const __m128i err = vreinterpretq_m128i_s16(vdupq_n_s16(short(e)));

  while ((n -= 8) >= 0) {
    __m128i tmp = vreinterpretq_m128i_s16(vqaddq_s16(vreinterpretq_s16_m128i(*(__m128i*) & t[n]), vreinterpretq_s16_m128i(*(__m128i*) & t[n])));
    tmp = _mm_mulhi_epi16(tmp, err);
    tmp = vreinterpretq_m128i_s16(vqaddq_s16(vreinterpretq_s16_m128i(tmp), vreinterpretq_s16_m128i(one)));
    tmp = vreinterpretq_m128i_s16(vshrq_n_s16(vreinterpretq_s16_m128i(tmp), (1)));
    tmp = vreinterpretq_m128i_s16(vqaddq_s16(vreinterpretq_s16_m128i(tmp), vreinterpretq_s16_m128i(*reinterpret_cast<__m128i*>(&w[n]))));
    *reinterpret_cast<__m128i*>(&w[n]) = tmp;
  }
#endif
}

#if (defined(__GNUC__) || defined(__clang__)) && defined(__SSE2__)

__attribute__((target("sse2")))
#endif
static auto dotProductSimdSse2(const short *const t, const short *const w, int n) -> int {
#ifndef __SSE2__
  return 0;
#else
  __m128i sum = _mm_setzero_si128();

  while((n -= 8) >= 0 ) {
    __m128i tmp = _mm_madd_epi16(*(__m128i * ) & t[n], *(__m128i * ) & w[n]);
    tmp = _mm_srai_epi32(tmp, 8);
    sum = _mm_add_epi32(sum, tmp);
  }

  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
  return _mm_cvtsi128_si32(sum);
#endif
}


#if (defined(__GNUC__) || defined(__clang__)) && defined(__SSE2__)

__attribute__((target("sse2")))
#endif
static void trainSimdSse2(const short *const t, short *const w, int n, const int e) {
#ifndef __SSE2__
  return;
#else
  const __m128i one = _mm_set1_epi16(1);
  const __m128i err = _mm_set1_epi16(short(e));

  while((n -= 8) >= 0 ) {
    __m128i tmp = _mm_adds_epi16(*(__m128i * ) & t[n], *(__m128i * ) & t[n]);
    tmp = _mm_mulhi_epi16(tmp, err);
    tmp = _mm_adds_epi16(tmp, one);
    tmp = _mm_srai_epi16(tmp, 1);
    tmp = _mm_adds_epi16(tmp, *reinterpret_cast<__m128i *>(&w[n]));
    *reinterpret_cast<__m128i *>(&w[n]) = tmp;
  }
#endif
}


static auto dotProductSimdNone(const short *const t, const short *const w, int n) -> int {
  int sum = 0;
  while((n -= 2) >= 0 ) {
    sum += (t[n] * w[n] + t[n + 1] * w[n + 1]) >> 8U;
  }
  return sum;
}

static void trainSimdNone(const short *const t, short *const w, int n, const int err) {
  while((n -= 1) >= 0 ) {
    int wt = w[n] + ((((t[n] * err * 2) >> 16U) + 1) >> 1U);
    if( wt < -32768 ) {
      wt = -32768;
    } else if( wt > 32767 ) {
      wt = 32767;
    }
    *reinterpret_cast<short*>(&w[n]) = wt;
  }
}

class Mixer : protected IPredictor {
protected:
    Shared *shared = Shared::getInstance();
    const uint32_t n; /**< max inputs */
    const uint32_t m; /**< max contexts */
    const uint32_t s; /**< max context sets */
    int scaleFactor; /**< scale factor for dot product */
    Array<short, 32> tx; /**< n inputs from add() */
    Array<short, 32> wx; /**< n*m weights */
    Array<uint32_t> cxt; /**< s contexts */
    Array<ErrorInfo> info; /**< stats for the adaptive learning rates  */
    Array<int> rates; /**< learning rates */
    uint32_t numContexts {}; /**< number of contexts (0 to s)  */
    uint32_t base {}; /**< offset of next context */
    uint32_t nx {}; /**< number of inputs in tx, 0 to n */
    Array<int> pr; /**< last result (scaled 12 bits) */
public:
    /**
     * Mixer m(n, m, s) combines models using @ref m neural networks with
     * @ref n inputs each, of which up to @ref s may be selected.  If s > 1 then
     * the outputs of these neural networks are combined using another
     * neural network (with arguments s, 1, 1). If s = 1 then the
     * output is direct. The weights are initially w (+-32K).
     * @param n
     * @param m
     * @param s
     */
    Mixer(int n, int m, int s);

    ~Mixer() override = default;
    /**
     * Returns the output prediction that the next bit is 1 as a 12 bit number (0 to 4095).
     * @return the prediction
     */
    virtual auto p() -> int = 0;
    virtual void setScaleFactor(int sf0, int sf1) = 0;

    /**
     * Input x (call up to n times)
     * m.add(stretch(p)) inputs a prediction from one of n models.  The
     * prediction should be positive to predict a 1 bit, negative for 0,
     * nominally +-256 to +-2K.  The maximum allowed value is +-32K but
     * using such large values may cause overflow if n is large.
     * @param x
     */
    void add(int x);

    /**
     *  Selects @ref cx as one of @ref range neural networks to
     *  use. 0 <= cx < range. Should be called up to @ref s times such
     *  that the total of the ranges is <= @ref m.
     * @param cx
     * @param range
     * @param rate
     */
    void set(uint32_t cx, uint32_t range, int rate = DEFAULT_LEARNING_RATE);
    void reset();
};

#endif //PAQ8PX_MIXER_HPP
