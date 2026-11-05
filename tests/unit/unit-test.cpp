#include "DriverUtils.hpp"
#include <RISCVModel/RVM.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::HasSubstr;
using ::testing::Not;
TEST(RISCVModelUnitTest, CreateIsaStringExtBasic) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_I] = true;
  auto rv64 =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ false);
  EXPECT_STREQ("RV64I", rv64.data());
  auto rv32 =
      rvm::create_isa_string(Ext, /* IsRV64 */ false, /* Lowercase */ false);
  EXPECT_STREQ("RV32I", rv32.data());

  rv64 = rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_STREQ("rv64i", rv64.data());
  rv32 = rvm::create_isa_string(Ext, /* IsRV64 */ false, /* Lowercase */ true);
  EXPECT_STREQ("rv32i", rv32.data());
}

TEST(RISCVModelUnitTest, CreateIsaStringExtG) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_G] = true;
  auto rv64 =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ false);
  EXPECT_STREQ("RV64IMAFDZicsr_Zifencei", rv64.data());
}

TEST(RISCVModelUnitTest, CreateIsaStringExtBits) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_G] = true;
  Ext.ZExt[RVM_ZEXT_BA] = true;
  Ext.ZExt[RVM_ZEXT_BB] = true;
  Ext.ZExt[RVM_ZEXT_BC] = true;
  Ext.ZExt[RVM_ZEXT_BS] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ false);
  EXPECT_STREQ("RV64IMAFDZicsr_Zifencei_Zba_Zbb_Zbc_Zbs", Isa.data());
}

TEST(RISCVModelUnitTest, CreateIsaStringExtBitmanip) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_G] = true;
  Ext.ZExt[RVM_ZEXT_BITMANIP] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ false);
  EXPECT_STREQ("RV64IMAFDZicsr_Zifencei_Zba_Zbb_Zbc_Zbs", Isa.data());
}

TEST(RISCVModelUnitTest, CreateIsaStringExtVkn) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_I] = true;
  Ext.ZExt[RVM_ZEXT_VKN] = true;
  Ext.ZExt[RVM_ZEXT_VKN] = true;
  auto rv64 =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_STREQ("rv64izvkb_zvkned_zvknhb_zvkt", rv64.data());
}

TEST(RISCVModelUnitTest, NormalizeExtensions) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_G] = true;
  Ext.MisaExt[RVM_MISA_J];
  Ext.ZExt[RVM_ZEXT_KN] = true;
  Ext.ZExt[RVM_ZEXT_KS] = true;
  Ext.ZExt[RVM_ZEXT_K] = true;
  Ext.ZExt[RVM_ZEXT_BITMANIP] = true;
  Ext.ZExt[RVM_ZEXT_VKN] = true;
  Ext.ZExt[RVM_ZEXT_VKNC] = true;
  Ext.ZExt[RVM_ZEXT_VKNG] = true;
  Ext.ZExt[RVM_ZEXT_VKS] = true;
  Ext.ZExt[RVM_ZEXT_VKSC] = true;
  Ext.ZExt[RVM_ZEXT_VKSG] = true;

  auto Norm = rvm::detail::normalize_extensions(Ext);
  EXPECT_TRUE(Norm.ZExtSize == Ext.ZExtSize);
  EXPECT_TRUE(Norm.XExtSize == Ext.XExtSize);
  EXPECT_TRUE(Norm.MisaExt[RVM_MISA_M]);
  EXPECT_TRUE(Norm.MisaExt[RVM_MISA_A]);
  EXPECT_TRUE(Norm.MisaExt[RVM_MISA_F]);
  EXPECT_TRUE(Norm.MisaExt[RVM_MISA_D]);

  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BKB]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BKC]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BKX]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KNE]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KND]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KNH]);

  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BA]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BB]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BC]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_BS]);

  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KSED]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KSH]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KR]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_KT]);

  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKB]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VBC]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKNED]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKNHB]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKG]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKSED]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKSH]);
  EXPECT_TRUE(Norm.ZExt[RVM_ZEXT_VKT]);
}

#define SET_MISA_TRUE(Name, name) MisaExt[Name] = true;
TEST(RISCVModelUnitTest, FOR_EACH_ENUM_MISA) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &MisaExt = Ext.MisaExt;
  RVM_FOR_EACH_MISA_EXT(SET_MISA_TRUE);
  EXPECT_TRUE(std::all_of(MisaExt, MisaExt + RVM_MISA_NUMBER,
                          [](auto E) -> bool { return E; }));
}

#define SET_ZEXT_TRUE(Name, name) ZExt[Name] = true;
TEST(RISCVModelUnitTest, FOR_EACH_ENUM_ZEXT) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &ZExt = Ext.ZExt;
  RVM_FOR_EACH_ZEXT(SET_ZEXT_TRUE);
  EXPECT_TRUE(std::all_of(ZExt, ZExt + RVM_ZEXT_NUMBER,
                          [](auto E) -> bool { return E; }));
}

#define SET_XEXT_TRUE(Name, name) XExt[Name] = true;
TEST(RISCVModelUnitTest, FOR_EACH_ENUM_XEXT) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &XExt = Ext.XExt;
  RVM_FOR_EACH_XEXT(SET_XEXT_TRUE);
  EXPECT_TRUE(std::all_of(XExt, XExt + RVM_XEXT_NUMBER,
                          [](auto E) -> bool { return E; }));
}

#define SET_SEXT_TRUE(Name, name) SExt[Name] = true;
TEST(RISCVModelUnitTest, FOR_EACH_ENUM_SEXT) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &SExt = Ext.SExt;
  RVM_FOR_EACH_SEXT(SET_SEXT_TRUE);
  EXPECT_TRUE(std::all_of(SExt, SExt + RVM_SEXT_NUMBER,
                          [](auto E) -> bool { return E; }));
}

TEST(RISCVModelUnitTest, ZIExtensions) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &ZExt = Ext.ZExt;
  ZExt[RVM_ZEXT_ICOND] = true;
  ZExt[RVM_ZEXT_ICBOM] = true;
  ZExt[RVM_ZEXT_ICBOZ] = true;
  ZExt[RVM_ZEXT_ICNTR] = true;
  ZExt[RVM_ZEXT_ICBOP] = true;
  ZExt[RVM_ZEXT_IMOP] = true;
  ZExt[RVM_ZEXT_ILSD] = true;
  ZExt[RVM_ZEXT_IHPM] = true;
  ZExt[RVM_ZEXT_IHINTNTL] = true;
  ZExt[RVM_ZEXT_IHINTPAUSE] = true;
  ZExt[RVM_ZEXT_ICFISS] = true;
  ZExt[RVM_ZEXT_ICFILP] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_THAT(Isa, HasSubstr("zicond"));
  EXPECT_THAT(Isa, HasSubstr("zicbom"));
  EXPECT_THAT(Isa, HasSubstr("zicboz"));
  EXPECT_THAT(Isa, HasSubstr("zicntr"));
  EXPECT_THAT(Isa, HasSubstr("zicbop"));
  EXPECT_THAT(Isa, HasSubstr("zimop"));
  EXPECT_THAT(Isa, HasSubstr("zilsd"));
  EXPECT_THAT(Isa, HasSubstr("zihpm"));
  EXPECT_THAT(Isa, HasSubstr("zihintntl"));
  EXPECT_THAT(Isa, HasSubstr("zihintpause"));
  EXPECT_THAT(Isa, HasSubstr("zicfiss"));
  EXPECT_THAT(Isa, HasSubstr("zicfilp"));
}

TEST(RISCVModelUnitTest, AtomicExtensions) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &ZExt = Ext.ZExt;
  ZExt[RVM_ZEXT_AAMO] = true;
  ZExt[RVM_ZEXT_ABHA] = true;
  ZExt[RVM_ZEXT_ACAS] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_THAT(Isa, HasSubstr("zaamo"));
  EXPECT_THAT(Isa, HasSubstr("zabha"));
  EXPECT_THAT(Isa, HasSubstr("zacas"));
}

TEST(RISCVModelUnitTest, FloatExtensions) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &ZExt = Ext.ZExt;
  ZExt[RVM_ZEXT_FA] = true;
  ZExt[RVM_ZEXT_FBFMIN] = true;
  ZExt[RVM_ZEXT_FH] = true;
  ZExt[RVM_ZEXT_FHMIN] = true;
  ZExt[RVM_ZEXT_DINX] = true;
  ZExt[RVM_ZEXT_HINX] = true;
  ZExt[RVM_ZEXT_HINXMIN] = true;
  ZExt[RVM_ZEXT_VFBFMIN] = true;
  ZExt[RVM_ZEXT_VFBFWMA] = true;
  ZExt[RVM_ZEXT_VFH] = true;
  ZExt[RVM_ZEXT_VFHMIN] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_THAT(Isa, HasSubstr("zfa"));
  EXPECT_THAT(Isa, HasSubstr("zfbfmin"));
  EXPECT_THAT(Isa, HasSubstr("zfh"));
  EXPECT_THAT(Isa, HasSubstr("zfhmin"));
  EXPECT_THAT(Isa, HasSubstr("zdinx"));
  EXPECT_THAT(Isa, HasSubstr("zhinx"));
  EXPECT_THAT(Isa, HasSubstr("zhinxmin"));
  EXPECT_THAT(Isa, HasSubstr("zvfbfmin"));
  EXPECT_THAT(Isa, HasSubstr("zvfbfwma"));
  EXPECT_THAT(Isa, HasSubstr("zvfh"));
  EXPECT_THAT(Isa, HasSubstr("zvfhmin"));
}

TEST(RISCVModelUnitTest, CompressedExtensions) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &ZExt = Ext.ZExt;
  ZExt[RVM_ZEXT_CMP] = true;
  ZExt[RVM_ZEXT_CMT] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_THAT(Isa, HasSubstr("zcmp"));
  EXPECT_THAT(Isa, HasSubstr("zcmt"));
}

TEST(RISCVModelUnitTest, Cmop) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &ZExt = Ext.ZExt;
  ZExt[RVM_ZEXT_CMOP] = true;
  ZExt[RVM_ZEXT_ICFISS] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_THAT(Isa, HasSubstr("zcmop"));
  EXPECT_THAT(Isa, HasSubstr("zicfiss"));
}

TEST(RISCVModelUnitTest, Rv64_Xempty) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  auto &XExt = Ext.XExt;
  XExt[RVM_XEXT_EMPTY] = true;
  auto Isa =
      rvm::create_isa_string(Ext, /* IsRV64 */ true, /* Lowercase */ true);
  EXPECT_THAT(Isa, HasSubstr("xempty"));
}

TEST(RISCVModelUnitTest, CompressedSubset) {
  RVMExtDescriptor Ext = RVM_INIT_EXT_DESCRIPTOR;
  Ext.MisaExt[RVM_MISA_I] = true;
  // Zc* v1.0.4-2 spec 1.5
  // MISA.C is set if the following extensions are selected:
  // - Zca and not F
  auto &ZExt = Ext.ZExt;
  ZExt[RVM_ZEXT_CA] = true;
  auto Isa = rvm::create_isa_string(Ext, /*IsRV64*/ true, /*Lowercase*/ true);
  EXPECT_THAT(Isa, HasSubstr("rv64ic"));
  // Zca, Zcf and F is specified (RV32 only)
  ZExt[RVM_ZEXT_CA] = true;
  ZExt[RVM_ZEXT_CF] = true;
  Ext.MisaExt[RVM_MISA_F] = true;
  Isa = rvm::create_isa_string(Ext, /*IsRV64*/ false, /*Lowercase*/ true);
  EXPECT_THAT(Isa, HasSubstr("rv32ifc"));
  // - Zca, Zcf and Zcd if D is specified (RV32 only)
  ZExt[RVM_ZEXT_CA] = true;
  ZExt[RVM_ZEXT_CF] = true;
  ZExt[RVM_ZEXT_CD] = true;
  Ext.MisaExt[RVM_MISA_F] = true;
  Ext.MisaExt[RVM_MISA_D] = true;
  Isa = rvm::create_isa_string(Ext, /*IsRV64*/ false, /*Lowercase*/ true);
  EXPECT_THAT(Isa, HasSubstr("rv32ifdc"));
  // - Zca, Zcd if D is specified (RV64 only)
  ZExt[RVM_ZEXT_CA] = true;
  ZExt[RVM_ZEXT_CD] = true;
  Ext.MisaExt[RVM_MISA_F] = true;
  Ext.MisaExt[RVM_MISA_D] = true;
  Isa = rvm::create_isa_string(Ext, /*IsRV64*/ true, /*Lowercase*/ true);
  EXPECT_THAT(Isa, HasSubstr("rv64ifdc"));
}

struct ParseIsaParam final {
  std::string_view ISA;
  std::vector<RVMMisaExt> Misa;
  std::vector<RVMZExt> ZExt;
  std::vector<RVMSExt> SExt;
  std::vector<RVMXExt> XExt;
  bool IsRV64;
};

struct ParseIsaTestNameGenerator {
  template <typename ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &Info) const {
    return std::string(Info.param.ISA);
  }
};

class ParseIsaTest : public testing::TestWithParam<ParseIsaParam> {};

#define CHECK_MISA(NAME, name)                                                 \
  EXPECT_EQ(Res.MisaExt[NAME], Ref.MisaExt[NAME])                              \
      << "Mismatch in MISA extension \"" #name "\"";
#define CHECK_ZEXT(NAME, name)                                                 \
  EXPECT_EQ(Res.ZExt[NAME], Ref.ZExt[NAME])                                    \
      << "Mismatch in standard extension \"z" #name "\"";
#define CHECK_XEXT(NAME, name)                                                 \
  EXPECT_EQ(Res.XExt[NAME], Ref.XExt[NAME])                                    \
      << "Mismatch in custom extension \"x" #name "\"";
#define CHECK_SEXT(NAME, name)                                                 \
  EXPECT_EQ(Res.SExt[NAME], Ref.SExt[NAME])                                    \
      << "Mismatch in standard extension \"s" #name "\"";

INSTANTIATE_TEST_SUITE_P(
    IsaStrings, ParseIsaTest,
    testing::Values(
        ParseIsaParam{"rv64i", {RVM_MISA_I}, {}, {}, {}, /*IsRV64*/ true},
        ParseIsaParam{"rv32i", {RVM_MISA_I}, {}, {}, {}, /*IsRV64*/ false},
        ParseIsaParam{"rv32e", {RVM_MISA_E}, {}, {}, {}, /*IsRV64*/ false},
        ParseIsaParam{
            "rv64imafd",
            {RVM_MISA_I, RVM_MISA_M, RVM_MISA_A, RVM_MISA_F, RVM_MISA_D},
            {},
            {},
            {},
            /*IsRV64*/ true},
        ParseIsaParam{"rv64gc",
                      {RVM_MISA_I, RVM_MISA_M, RVM_MISA_A, RVM_MISA_F,
                       RVM_MISA_D, RVM_MISA_C},
                      {RVM_ZEXT_ICSR, RVM_ZEXT_IFENCEI},
                      {},
                      {},
                      /*IsRV64*/ true},
        ParseIsaParam{"rv64imzba_zbb",
                      {RVM_MISA_I, RVM_MISA_M},
                      {RVM_ZEXT_BB, RVM_ZEXT_BA},
                      {},
                      {},
                      /*IsRV64*/ true},
        ParseIsaParam{"rv64im_zba_zbb",
                      {RVM_MISA_I, RVM_MISA_M},
                      {RVM_ZEXT_BB, RVM_ZEXT_BA},
                      {},
                      {},
                      /*IsRV64*/ true},
        ParseIsaParam{"rv64imzbb_xempty",
                      {RVM_MISA_I, RVM_MISA_M},
                      {RVM_ZEXT_BB},
                      {},
                      {RVM_XEXT_EMPTY},
                      /*IsRV64*/ true},
        ParseIsaParam{"RV64IMAZbb_Zicsr_Xempty",
                      {RVM_MISA_I, RVM_MISA_M, RVM_MISA_A},
                      {RVM_ZEXT_BB, RVM_ZEXT_ICSR},
                      {},
                      {RVM_XEXT_EMPTY},
                      /*IsRV64*/ true},
        ParseIsaParam{
            "rv32imfdvxempty",
            {RVM_MISA_I, RVM_MISA_M, RVM_MISA_F, RVM_MISA_D, RVM_MISA_V},
            {},
            {},
            {RVM_XEXT_EMPTY},
            /*IsRV64*/ false},
        ParseIsaParam{"rv64imafdc_sstc_svade",
                      {RVM_MISA_I, RVM_MISA_M, RVM_MISA_A, RVM_MISA_F,
                       RVM_MISA_D, RVM_MISA_C},
                      {},
                      {RVM_SEXT_STC, RVM_SEXT_VADE},
                      {},
                      /*IsRV64*/ true},
        ParseIsaParam{"rv64im_sstc_svpbmt",
                      {RVM_MISA_I, RVM_MISA_M},
                      {},
                      {RVM_SEXT_STC, RVM_SEXT_VPBMT},
                      {},
                      /*IsRV64*/ true}

        ),
    ParseIsaTestNameGenerator());

TEST_P(ParseIsaTest, SimplyParse) {
  auto &&Param = GetParam();
  auto [Res, IsRV64] = parse_isa_string(Param.ISA);
  EXPECT_EQ(IsRV64, Param.IsRV64);
  RVMExtDescriptor Ref = RVM_INIT_EXT_DESCRIPTOR;
  for (auto E : Param.Misa)
    Ref.MisaExt[E] = 1;
  for (auto E : Param.ZExt)
    Ref.ZExt[E] = 1;
  for (auto E : Param.SExt)
    Ref.SExt[E] = 1;
  for (auto E : Param.XExt)
    Ref.XExt[E] = 1;
  RVM_FOR_EACH_MISA_EXT(CHECK_MISA)
  RVM_FOR_EACH_ZEXT(CHECK_ZEXT)
  RVM_FOR_EACH_XEXT(CHECK_XEXT)
  RVM_FOR_EACH_SEXT(CHECK_SEXT)
}

TEST_P(ParseIsaTest, ParseAndSerialize) {
  auto &&Param = GetParam();
  RVMExtDescriptor Ref = RVM_INIT_EXT_DESCRIPTOR;
  for (auto E : Param.Misa)
    Ref.MisaExt[E] = 1;
  for (auto E : Param.ZExt)
    Ref.ZExt[E] = 1;
  for (auto E : Param.SExt)
    Ref.SExt[E] = 1;
  for (auto E : Param.XExt)
    Ref.XExt[E] = 1;
  auto Isa1 = rvm::create_isa_string(Ref, Param.IsRV64, /*lowercase*/ true);
  auto [Res, IsRV64] = parse_isa_string(Isa1);
  auto Isa2 = rvm::create_isa_string(Ref, IsRV64, /*lowercase*/ true);
  EXPECT_STREQ(Isa1.c_str(), Isa2.c_str());
}

struct InvalidISAParam final {
  std::string_view ISA;
  std::string_view Error;
};

class InvalidIsaTest : public testing::TestWithParam<InvalidISAParam> {};

INSTANTIATE_TEST_SUITE_P(
    InvalidIsaStrings, InvalidIsaTest,
    testing::Values(
        InvalidISAParam{"hello", "isa string should start with \"rv\" prefix"},
        InvalidISAParam{
            "rvrr",
            "Invalid ISA string \"rvrr\": string should contain bitness"},
        InvalidISAParam{"rv123",
                        "only 32 and 64-bit architectures are supported"},
        InvalidISAParam{"rv256",
                        "only 32 and 64-bit architectures are supported"},
        InvalidISAParam{"rv32_",
                        "extension should have 's', 'z' or 'x' after an "
                        "underscore"},
        InvalidISAParam{"rv32m", "either E or I should be available"},
        InvalidISAParam{
            "rv32imznonexisting",
            "unknown standard extension \"znonexisting\" specified"},
        InvalidISAParam{"rv32im_zbb_xnonexisting",
                        "unknown custom extension \"xnonexisting\" specified"}),
    ParseIsaTestNameGenerator());

TEST_P(InvalidIsaTest, Throws) {
  auto &&Param = GetParam();
  EXPECT_THAT([&] { parse_isa_string(Param.ISA); },
              ::testing::ThrowsMessage<invalid_isa_string>(
                  ::testing::HasSubstr(Param.Error)));
}
