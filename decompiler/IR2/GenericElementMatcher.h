/*!
 * @file GenericElementMatcher.h
 *
 * The Matcher is supposed to match up forms to templates, and extract the variables actually used.
 */

#pragma once
#include "Form.h"

namespace decompiler {
class DerefTokenMatcher;
class GenericOpMatcher;

struct MatchResult {
  bool matched = false;
  struct Maps {
    std::vector<std::optional<Variable>> regs;
    std::unordered_map<int, std::string> strings;
    std::unordered_map<int, Form*> forms;
  } maps;
};

class Matcher {
 public:
  static Matcher any_reg(int match_id = -1);
  static Matcher op(const GenericOpMatcher& op, const std::vector<Matcher>& args);
  static Matcher op_with_rest(const GenericOpMatcher& op, const std::vector<Matcher>& args);
  static Matcher fixed_op(FixedOperatorKind op, const std::vector<Matcher>& args);
  static Matcher match_or(const std::vector<Matcher>& args);
  static Matcher cast(const std::string& type, Matcher value);
  static Matcher any(int match_id = -1);
  static Matcher integer(std::optional<int> value);
  static Matcher any_reg_cast_to_int_or_uint(int match_id = -1);
  static Matcher any_quoted_symbol(int match_id = -1);
  static Matcher any_symbol(int match_id = -1);
  static Matcher deref(const Matcher& root,
                       bool is_addr_of,
                       const std::vector<DerefTokenMatcher>& tokens);

  enum class Kind {
    ANY_REG,     // matching any register
    GENERIC_OP,  // matching
    GENERIC_OP_WITH_REST,
    OR,
    CAST,
    ANY,
    INT,
    ANY_QUOTED_SYMBOL,
    ANY_SYMBOL,
    DEREF_OP,
    INVALID
  };

  bool do_match(Form* input, MatchResult::Maps* maps_out) const;

 private:
  std::vector<Matcher> m_sub_matchers;
  std::vector<DerefTokenMatcher> m_token_matchers;
  std::shared_ptr<GenericOpMatcher> m_gen_op_matcher;
  bool m_deref_is_addr_of = false;
  Kind m_kind = Kind::INVALID;
  int m_reg_out_id = -1;
  int m_string_out_id = -1;
  int m_form_match = -1;
  std::optional<int> m_int_match;
  std::string m_str;
};

MatchResult match(const Matcher& spec, Form* input);

class DerefTokenMatcher {
 public:
  static DerefTokenMatcher string(const std::string& str);
  static DerefTokenMatcher any_string(int match_id = -1);

  enum class Kind { STRING, ANY_STRING, INVALID };

  bool do_match(const DerefToken& input, MatchResult::Maps* maps_out) const;

 private:
  Kind m_kind = Kind::INVALID;
  std::string m_str;
  int m_str_out_id = -1;
};

class GenericOpMatcher {
 public:
  static GenericOpMatcher fixed(FixedOperatorKind kind);
  static GenericOpMatcher func(const Matcher& func_matcher);

  enum class Kind { FIXED, FUNC, INVALID };

  bool do_match(GenericOperator& input, MatchResult::Maps* maps_out) const;

 private:
  Kind m_kind = Kind::INVALID;
  FixedOperatorKind m_fixed_kind = FixedOperatorKind::INVALID;
  Matcher m_func_matcher;
};

}  // namespace decompiler