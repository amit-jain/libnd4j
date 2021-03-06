/*

 Half-precision data type, based on NVIDIA code: https://github.com/NVIDIA/caffe/tree/experimental/fp16

 */

#ifndef CAFFE_UTIL_FP16_H_
#define CAFFE_UTIL_FP16_H_

#include <cfloat>
#include <iosfwd>



#ifdef __CUDACC__
#include <fp16_conversion.hpp>
#else
typedef struct {
    unsigned short x;
} __half;



typedef __half half;
#endif

#ifdef __CUDACC__
#define op_def inline __host__ __device__
#elif _MSC_VER
#define op_def inline
#elif __clang__
#define op_def inline
#elif __GNUC__
#define op_def inline
#endif

#include <fp16_emu.h>

op_def float cpu_half2float(half h) {
    unsigned sign = ((h.x >> 15) & 1);
    unsigned exponent = ((h.x >> 10) & 0x1f);
    unsigned mantissa = ((h.x & 0x3ff) << 13);

    if (exponent == 0x1f) {  /* NaN or Inf */
        mantissa = (mantissa ? (sign = 0, 0x7fffff) : 0);
        exponent = 0xff;
    } else if (!exponent) {  /* Denorm or Zero */
        if (mantissa) {
            unsigned int msb;
            exponent = 0x71;
            do {
                msb = (mantissa & 0x400000);
                mantissa <<= 1;  /* normalize */
                --exponent;
            } while (!msb);
            mantissa &= 0x7fffff;  /* 1.mantissa is implicit */
        }
    } else {
        exponent += 0x70;
    }

    int temp = ((sign << 31) | (exponent << 23) | mantissa);

    return *((float*)((void*)&temp));
}


op_def half cpu_float2half_rn(float f)
{
    half ret;

    unsigned x = *((int*)(void*)(&f));
    unsigned u = (x & 0x7fffffff), remainder, shift, lsb, lsb_s1, lsb_m1;
    unsigned sign, exponent, mantissa;

    // Get rid of +NaN/-NaN case first.
    if (u > 0x7f800000) {
        ret.x = 0x7fffU;
        return ret;
    }

    sign = ((x >> 16) & 0x8000);

    // Get rid of +Inf/-Inf, +0/-0.
    if (u > 0x477fefff) {
        ret.x = sign | 0x7c00U;
        return ret;
    }
    if (u < 0x33000001) {
        ret.x = (sign | 0x0000);
        return ret;
    }

    exponent = ((u >> 23) & 0xff);
    mantissa = (u & 0x7fffff);

    if (exponent > 0x70) {
        shift = 13;
        exponent -= 0x70;
    } else {
        shift = 0x7e - exponent;
        exponent = 0;
        mantissa |= 0x800000;
    }
    lsb = (1 << shift);
    lsb_s1 = (lsb >> 1);
    lsb_m1 = (lsb - 1);

    // Round to nearest even.
    remainder = (mantissa & lsb_m1);
    mantissa >>= shift;
    if (remainder > lsb_s1 || (remainder == lsb_s1 && (mantissa & 0x1))) {
        ++mantissa;
        if (!(mantissa & 0x3ff)) {
            ++exponent;
            mantissa = 0;
        }
    }

    ret.x = (sign | (exponent << 10) | mantissa);

    return ret;
}

namespace nd4j
{

  struct float16
  {
    /* constexpr */ op_def float16() { data.x = 0; }
    
    template <class T>
    op_def /*explicit*/ float16(const T& rhs) {
      assign(rhs);
    }
      
//    op_def float16(float rhs) {
//      assign(rhs);
//    }
//
//    op_def float16(double rhs) {
//      assign(rhs);
//    }

    op_def operator float() const {
#ifdef __CUDA_ARCH__
      return __half2float(data);
#else
      return cpu_half2float(data);
#endif
    }

    //    op_def operator double() const { return (float)*this; } 
       
    op_def operator half() const { return data; }

    op_def unsigned short getx() const { return data.x; }
    op_def float16& setx(unsigned short x) { data.x = x; return *this; }

    template <class T>
    op_def float16& operator=(const T& rhs) { assign(rhs); return *this; }

    op_def void assign(unsigned int rhs) {
      // may be a better way ?
      assign((float)rhs);
    }

    op_def void assign(int rhs) {
      // may be a better way ?
      assign((float)rhs);
    }

    op_def void assign(double rhs) {
      assign((float)rhs);
    }
   
    op_def void assign(float rhs) {
#ifdef __CUDA_ARCH__
      data.x = __float2half_rn(rhs);
#else
  #if defined(DEBUG) && defined (CPU_ONLY)
      if (rhs > HLF_MAX || rhs < -HLF_MAX) {
        LOG(WARNING) << "Overflow: " << rhs;
      } else if (rhs != 0.F && rhs < HLF_MIN && rhs > -HLF_MIN) {
        LOG(WARNING) << "Underflow: " << rhs;
      }
  #endif
      data = cpu_float2half_rn(rhs);
#endif
    }

    op_def void assign(const half& rhs) {
      data = rhs;
    }

    op_def void assign(const float16& rhs) {
      data = rhs.data;
    }

    op_def float16& operator+=(float16 rhs) { assign((float)*this + rhs); return *this; }  

    op_def float16& operator-=(float16 rhs) { assign((float)*this - rhs); return *this; }  

    op_def float16& operator*=(float16 rhs) { assign((float)*this * rhs); return *this; }   

    op_def float16& operator/=(float16 rhs) { assign((float)*this / rhs); return *this; }  

    op_def float16& operator+=(float rhs) { assign((float)*this + rhs); return *this; }  

    op_def float16& operator-=(float rhs) { assign((float)*this - rhs); return *this; }  

    op_def float16& operator*=(float rhs) { assign((float)*this * rhs); return *this; }  

    op_def float16& operator/=(float rhs) { assign((float)*this / rhs); return *this; }  

    op_def float16& operator++() { assign(*this + 1.f); return *this; }  

    op_def float16& operator--() { assign(*this - 1.f); return *this; }  

    op_def float16 operator++(int i) { assign(*this + (float)i); return *this; }  

    op_def float16 operator--(int i) { assign(*this - (float)i); return *this; }  


    half data;

    // Utility contants
    static const float16 zero;
    static const float16 one;
    static const float16 minus_one;
  };

//  op_def bool  operator==(const float16& a, const float16& b) { return ishequ(a.data, b.data); }
//
//  op_def bool  operator!=(const float16& a, const float16& b) { return !(a == b); }
//
//  op_def bool  operator<(const float16& a, const float16& b) { return (float)a < (float)b; }
//
//  op_def bool  operator>(const float16& a, const float16& b) { return (float)a > (float)b; }
//
//  op_def bool  operator<=(const float16& a, const float16& b) { return (float)a <= (float)b; }
//
//  op_def bool  operator>=(const float16& a, const float16& b) { return (float)a >= (float)b; }
//
//  template <class T>
//  op_def float16 operator+(const float16& a, const T& b) { return float16((float)a + (float)b); }
//
//  template <class T>
//  op_def float16 operator-(const float16& a, const T& b) { return float16((float)a - (float)b); }
//
//  template <class T>
//  op_def float16 operator*(const float16& a, const T& b) { return float16((float)a * (float)b); }
//
//  template <class T>
//  op_def float16 operator/(const float16& a, const T& b) { return float16((float)a / (float)b); }
  

  op_def float16 /* constexpr */ operator+(const float16& h) { return h; }
  
  op_def float16 operator - (const float16& h) { return float16(hneg(h.data)); }

#ifdef __CUDACC__
  op_def int isnan(const float16& h)  { return ishnan(h.data); }
  
  op_def int isinf(const float16& h) { return ishinf(h.data); }
#endif

  std::ostream& operator << (std::ostream& s, const float16&);

}   // namespace caffe

#endif
