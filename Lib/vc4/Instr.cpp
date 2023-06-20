#include "Instr.h"
#include "Support/basics.h"

namespace V3DLib {
namespace vc4 {
namespace {


/**
 * @brief Determine the regfile and index combination to use for writes, for the passed 
 *        register definition 'reg'.
 *
 * This function deals exclusively with write values of the regfile registers.
 *
 * See also NOTES in header comment for `encodeSrcReg()`.
 *
 * @param reg   register definition for which to determine output value
 * @param file  out-parameter; the regfile to use (either REG_A or REG_B)
 *
 * @return index into regfile (A, B or both) of the passed register
 *
 * ----------------------------------------------------------------------------------------------
 * ## NOTES
 *
 * * The regfile location for `ACC4` is called `TMP_NOSWAP` in the doc. This is because
 *   special register `r4` (== ACC4) is read-only.
 *   TODO: check code if ACC4 is actually used. Better to name it TMP_NOSWAP
 *
 * * ACC5 has parentheses with extra function descriptions.
 *   This implies that the handling of ACC5 differs from the others (at least, for ACC[0123])
 *   TODO: Check code for this; is there special handling of ACC5? 
 */
uint32_t encodeDestReg(Reg reg, RegTag* file) {
  // Selection of regfile for the cases where using A or B doesn't matter
  RegTag AorB = REG_A;  // Select A as default
  if (reg.tag == REG_A || reg.tag == REG_B) {
    AorB = reg.tag;     // If the regfile is preselected in `reg`, use that.
  }

  switch (reg.tag) {
    case REG_A:
      assert(reg.regId >= 0 && reg.regId < 32);
      *file = REG_A; return reg.regId;
    case REG_B:
      assert(reg.regId >= 0 && reg.regId < 32);
      *file = REG_B; return reg.regId;
    case ACC:
      // See NOTES in header comment
      assert(reg.regId >= 0 && reg.regId <= 5); // !! ACC4 is TMP_NOSWAP, *not* r4
      *file = reg.regId == 5 ? REG_B : AorB; 
      return 32 + reg.regId;
    case SPECIAL:
      switch (reg.regId) {
        case SPECIAL_RD_SETUP:      *file = REG_A; return 49;
        case SPECIAL_WR_SETUP:      *file = REG_B; return 49;
        case SPECIAL_DMA_LD_ADDR:   *file = REG_A; return 50;
        case SPECIAL_DMA_ST_ADDR:   *file = REG_B; return 50;
        case SPECIAL_VPM_WRITE:     *file = AorB;  return 48;
        case SPECIAL_HOST_INT:      *file = AorB;  return 38;
        case SPECIAL_TMU0_S:        *file = AorB;  return 56;
        case SPECIAL_SFU_RECIP:     *file = AorB;  return 52;
        case SPECIAL_SFU_RECIPSQRT: *file = AorB;  return 53;
        case SPECIAL_SFU_EXP:       *file = AorB;  return 54;
        case SPECIAL_SFU_LOG:       *file = AorB;  return 55;
        default:                    break;
      }
    case NONE:
      // NONE maps to 'NOP' in regfile.
      *file = AorB; return 39;

    default:
      fatal("V3DLib: missing case in encodeDestReg");
      return 0;
  }
}


/**
 * @brief Determine regfile index and the read field encoding for alu-instructions, for the passed 
 *        register 'reg'.
 *
 * The read field encoding, output parameter `mux` is a bitfield in instructions `alu` and 
 * 'alu small imm'. It specifies the register(s) to use as input.
 *
 * This function deals exclusively with 'read' values.
 *
 * @param reg   register definition for which to determine output value
 * @param file  regfile to use. This is used mainly to validate the `regid` field in param `reg`. In specific
 *              cases where both regfile A and B are valid (e.g. NONE), it is used to select the regfile.
 * @param mux   out-parameter; value in ALU instruction encoding for fields `add_a`, `add_b`, `mul_a` and `mul_b`.
 *
 * @return index into regfile (A, B or both) of the passed register.
 *
 * ----------------------------------------------------------------------------------------------
 * ## NOTES
 *
 * * There are four combinations of access to regfiles:
 *   - read A
 *   - read B
 *   - write A
 *   - write B
 *
 * This is significant, because SPECIAL registers may only be accessible through a specific combination
 * of A/B and read/write.
 *
 * * References in VideoCore IV Reference document:
 *
 *   - Fields `add_a`, `add_b`, `mul_a` and `mul_b`: "Figure 4: ALU Instruction Encoding", page 26
 *   - mux value: "Table 3: ALU Input Mux Encoding", page 28
 *   - Index regfile: "Table 14: 'QPU Register Addess Map'", page 37.
 *
 * ----------------------------------------------------------------------------------------------
 * ## TODO
 *
 * * NONE/NOP - There are four distinct versions for `NOP`, A/B and no read/no write.
 *              Verify if those distinctions are important at least for A/B.
 *              They might be the same thing.
 */
uint32_t encodeSrcReg(Reg reg, RegTag file, uint32_t* mux) {
  assert (file == REG_A || file == REG_B);

  const uint32_t NO_REGFILE_INDEX = 0;  // Return value to use when there is no regfile mapping for the register

  // Selection of regfile for the cases that both A and B are possible.
  // Note that param `file` has precedence here.
  uint32_t AorB = (file == REG_A)? 6 : 7;

  switch (reg.tag) {
    case REG_A:
      assert(reg.regId >= 0 && reg.regId < 32 && file == REG_A);
      *mux = 6; return reg.regId;
    case REG_B:
      assert(reg.regId >= 0 && reg.regId < 32 && file == REG_B);
      *mux = 7; return reg.regId;
    case ACC:
      // ACC does not map onto a regfile for 'read'
      assert(reg.regId >= 0 && reg.regId <= 4);  // TODO index 5 missing, is this correct??
      *mux = reg.regId; return NO_REGFILE_INDEX;
    case NONE:
      // NONE maps to `NOP` in the regfile
      *mux = AorB; return 39;
    case SPECIAL:
      switch (reg.regId) {
        case SPECIAL_UNIFORM:     *mux = AorB;                     return 32;
        case SPECIAL_ELEM_NUM:    assert(file == REG_A); *mux = 6; return 38;
        case SPECIAL_QPU_NUM:     assert(file == REG_B); *mux = 7; return 38;
        case SPECIAL_VPM_READ:    *mux = AorB;                     return 48;
        case SPECIAL_DMA_LD_WAIT: assert(file == REG_A); *mux = 6; return 50;
        case SPECIAL_DMA_ST_WAIT: assert(file == REG_B); *mux = 7; return 50;
      }

    default:
      fatal("V3DLib: missing case in encodeSrcReg");
      return 0;
  }
}

}  // anon namespace


void Instr::tag(Tag in_tag, bool imm) {
  m_tag = in_tag;

  switch(m_tag) {
    case NOP:  // default
    case LI:
      break;  // as is

    case ROT:
      m_sig = 13;
      break;

    case ALU:
      m_sig = imm?13:1;
      break;

    case BR:
      m_sig = 15;
      break;

    case END:
      m_sig = 3;
      m_raddrb = 39;
      break;

    case LDTMU:
      m_sig = 10;
      m_raddrb = 39;
      break;

    case SINC:
    case SDEC:
      m_sig = 0xe;
      m_sem_flag = 8;
      break;
  };
}


/**
 * Handle case where there are two source operands
 *
 * This is fairly convoluted stuff; apparently there are rules with regfile A/B usage
 * which I am not aware of.
 */
void Instr::encode_operands(RegOrImm const &srcA, RegOrImm const &srcB) {
  uint32_t muxa, muxb;
  uint32_t raddra = 0, raddrb;

  if (srcA.is_reg() && srcB.is_reg()) { // Both operands are registers
    RegTag aFile = srcA.reg().regfile();
    RegTag aTag  = srcA.reg().tag;

    RegTag bFile = srcB.reg().regfile();

    // If operands are the same register
    if (aTag != NONE && srcA == srcB) {
      if (aFile == REG_A) {
        raddra = encodeSrcReg(srcA.reg(), REG_A, &muxa);
        raddrb = 39;
        muxb = muxa;
      } else {
        raddra = 39;
        raddrb = encodeSrcReg(srcA.reg(), REG_B, &muxa);
        muxb = muxa;
      }
    } else {
      // Operands are different registers
      assert(aFile == NONE || bFile == NONE || aFile != bFile);  // TODO examine why aFile == bFile is disallowed here
      if (aFile == REG_A || bFile == REG_B) {
        raddra = encodeSrcReg(srcA.reg(), REG_A, &muxa);
        raddrb = encodeSrcReg(srcB.reg(), REG_B, &muxb);
      } else {
        raddra = encodeSrcReg(srcB.reg(), REG_A, &muxb);
        raddrb = encodeSrcReg(srcA.reg(), REG_B, &muxa);
      }
    }
  } else if (srcA.is_imm() || srcB.is_imm()) {
    if (srcA.is_imm() && srcB.is_imm()) {
      assertq(srcA.imm().val == srcB.imm().val,
        "srcA and srcB can not both be immediates with different values", true);

      raddrb = (uint32_t) srcA.imm().val;  // srcB is the same
      muxa   = 7;
      muxb   = 7;
    } else if (srcB.is_imm()) {
      // Second operand is a small immediate
      raddra = encodeSrcReg(srcA.reg(), REG_A, &muxa);
      raddrb = (uint32_t) srcB.imm().val;
      muxb   = 7;
    } else if (srcA.is_imm()) {
      // First operand is a small immediate
      raddra = encodeSrcReg(srcB.reg(), REG_A, &muxb);
      raddrb = (uint32_t) srcA.imm().val;
      muxa   = 7;
    } else {
      assert(false);  // Not expecting this
    }
  } else {
    assert(false);  // Not expecting this
  }

  m_raddra  = raddra;
  m_raddrb  = raddrb;
  m_muxa  = muxa;
  m_muxb  = muxb;
}


uint32_t Instr::high() const {
  uint32_t ret = (m_sig << 28) | (m_sem_flag << 24) | (waddr_add << 6) | waddr_mul;

  if (m_tag == BR) {
    ret |= (cond_add << 20)
        |  (m_rel?(1 << 19):0);
  } else if (m_tag != END && m_tag != LDTMU) {
    ret |= (cond_add << 17) | (cond_mul << 14) 
        |  (m_sf?(1 << 13):0)
        |  (m_ws?(1 << 12):0);
  }

  return ret;
}


uint32_t Instr::low() const {
  switch (m_tag) {
    case NOP:
      return  0;
    case ROT:
      return (mulOp << 29) | (m_raddra << 18) | (m_raddrb << 12);
    case ALU:
      return (mulOp << 29) | (addOp << 24) | (m_raddra << 18) | (m_raddrb << 12)
           | (m_muxa << 9) | (m_muxb << 6)
           | (m_muxa << 3) | m_muxb;
    case END:
    case LDTMU:
      return (m_raddra << 18) | (m_raddrb << 12);
    case LI:
    case BR:
      return li_imm;
    case SINC:
      return sema_id;
    case SDEC:
      return (1 << 4) | sema_id;
  };

  assert(false);
  return 0;
}


void Instr::encode(V3DLib::Instr const &instr) {
  switch (instr.tag) {
    case InstrTag::NO_OP:
      break; // Use default value for instr, which is a full NOP

    case InstrTag::LI: {        // Load immediate
      auto &li = instr.LI;
      RegTag file;

      tag(Instr::LI);
      cond_add  = instr.assign_cond().encode();
      waddr_add = encodeDestReg(instr.dest(), &file);
      ws(file != REG_A);
      li_imm = li.imm.encode();
      sf(instr.set_cond().flags_set());
    }
    break;

    case InstrTag::BR:  // Branch
      assertq(!instr.branch_target().useRegOffset, "Register offset not supported");

      tag(Instr::BR);
      cond_add = instr.branch_cond().encode();
      rel(instr.branch_target().relative);
      li_imm = 8*instr.branch_target().immOffset;
    break;

    case InstrTag::ALU: {
      auto &alu = instr.ALU;

      RegTag file;
      uint32_t dest  = encodeDestReg(instr.dest(), &file);

      if (alu.op.isMul()) {
        cond_mul  = instr.assign_cond().encode();
        waddr_mul = dest;
        ws(file != REG_B);
      } else {
        cond_add  = instr.assign_cond().encode();
        waddr_add = dest;
        ws(file != REG_A);
      }

      sf(instr.set_cond().flags_set());

      if (alu.op.isRot()) {
        assert(alu.srcA.is_reg() && alu.srcA.reg().tag == ACC && alu.srcA.reg().regId == 0);
        assert(!alu.srcB.is_reg() || (alu.srcB.reg().tag == ACC && alu.srcB.reg().regId == 5));
        uint32_t raddrb = 48;

        if (!alu.srcB.is_reg()) {  // i.e. value is an imm
          uint32_t n = (uint32_t) alu.srcB.imm().val;
          assert(n >= 1 || n <= 15);
          raddrb += n;
        }

        tag(Instr::ROT);
        mulOp  = ALUOp(ALUOp::M_V8MIN).vc4_encodeMulOp();
        m_raddrb = raddrb;
      } else {
        tag(Instr::ALU, instr.hasImm());
        mulOp = (alu.op.isMul() ? alu.op.vc4_encodeMulOp() : 0);
        addOp = (alu.op.isMul() ? 0 : alu.op.vc4_encodeAddOp());
        encode_operands(alu.srcA, alu.srcB);
      }
    }
    break;

    // Halt
    case InstrTag::END:
      tag(Instr::END);
      break;

    case InstrTag::RECV: {
      assert(instr.dest() == Reg(ACC,4));  // ACC4 is the only value allowed as dest
      tag(Instr::LDTMU);
    }
    break;

    // Semaphore increment/decrement
    case InstrTag::SINC:
      tag(Instr::SINC);
      sema_id = instr.semaId;
      break;

    case InstrTag::SDEC:
      tag(Instr::SDEC);
      sema_id = instr.semaId;
      break;

    default:
      assertq(false, "vc4::Instr::encode(): Target lang instruction tag not handled");
      break;
  }
}

}  // namespace vc4
}  // namespace V3DLib
