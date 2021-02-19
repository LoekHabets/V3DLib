#ifndef _V3DLIB_SOURCE_COMPLEX_H_
#define _V3DLIB_SOURCE_COMPLEX_H_
#include "Common/SharedArray.h"
#include "Ptr.h"
#include "Float.h"

namespace V3DLib {

class Complex;

class ComplexExpr {
public:
  ComplexExpr();
  ComplexExpr(Complex const &rhs);
  ComplexExpr(Expr::Ptr re, Expr::Ptr im) : re_e(re), im_e(im) {}

  Expr::Ptr re() { return re_e; }
  Expr::Ptr im() { return im_e; }

private:
  Expr::Ptr re_e = nullptr;   // Abstract syntax tree
  Expr::Ptr im_e = nullptr;   // TODO prob necessary to combine in single expr
};


/**
 * CPU-side complex definition
 */
class complex {
public:
  complex(float re, float im) : m_re(re), m_im(im) {}

  float re() const { return m_re; }
  float im() const { return m_im; }
  std::string dump() const;

private:
  float m_re;
  float m_im;
};


/**
 * QPU-side complex definition
 */
class Complex {
public:
  enum {
    size = 2  // Size of instance in 32-bit values
  };


  /**
   * Encapsulates two disticnt shared float arrays for real and imaginary values
   */
  class Array {

    class ref {
    public:
      ref(float &re_ref, float &im_ref);

      ref &operator=(complex const &rhs);
      bool operator==(complex const &rhs) const;
      std::string dump() const;

    private:
      float &m_re_ref;
      float &m_im_ref;
    };

  public:
    Array(int size);

    void fill(complex const &rhs);
    std::string dump() const;

    SharedArray<float> &re() { return  m_re; }
    SharedArray<float> &im() { return  m_im; }

    ref operator[] (int i);

  private:
    SharedArray<float> m_re;
    SharedArray<float> m_im;
  };


  class Ptr {
  public:
    class Deref {
    public:
      Deref(Expr::Ptr re, Expr::Ptr im);

      Deref &operator=(Complex const &rhs);

      V3DLib::Deref<Float> m_re;
      V3DLib::Deref<Float> m_im;
    };

    Ptr(ComplexExpr rhs);

    Deref operator*();

    static Ptr mkArg();
    static bool passParam(Seq<int32_t> *uniforms, Complex::Array *p);

  private:
    Float::Ptr re;
    Float::Ptr im;
  };


  Complex() {}
  Complex(FloatExpr const &e_re, Float const &e_im);
  Complex(Complex const &rhs);
  Complex(ComplexExpr input);
  Complex(Ptr::Deref d);

  Float const &re() const { return m_re; }
  Float const &im() const { return m_im; }
  void re(FloatExpr const &e) { m_re = e; }
  void im(FloatExpr const &e) { m_im = e; }

  Float mag_square() const;

  Complex operator+(Complex rhs) const;
  Complex operator*(Complex rhs) const;
  Complex &operator*=(Complex rhs);
  void operator=(Complex const &rhs);

private:
  Float m_re;
  Float m_im;

  Complex &self();
};

}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_COMPLEX_H_