#ifndef _V3DLIB_TARGET_INSTR_IMM_H_
#define _V3DLIB_TARGET_INSTR_IMM_H_
#include <string>

namespace V3DLib {

struct Imm {

  // Different kinds of immediate
  enum ImmTag {
    IMM_INT32,    // 32-bit word
    IMM_FLOAT32,  // 32-bit float
    IMM_MASK     // 1 bit per vector element (0 to 0xffff)
  };

  Imm(int i);
  Imm(float f);

  ImmTag tag() const { return m_tag; }
  int    intVal() const;
  int    mask() const;
  float  floatVal() const;

  bool is_zero() const;
  std::string pretty() const;

private:
  ImmTag m_tag      = IMM_INT32;
  int    m_intVal   = 0;
  float  m_floatVal = -1.0f;
};

}  // namespace V3DLib

#endif  // _V3DLIB_TARGET_INSTR_IMM_H_