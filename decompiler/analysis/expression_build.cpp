#include "expression_build.h"
#include "decompiler/Function/Function.h"
#include "decompiler/IR2/Form.h"
#include "decompiler/IR2/FormStack.h"
#include "decompiler/util/DecompilerTypeSystem.h"
#include "common/goos/PrettyPrinter.h"

namespace decompiler {

// TODO - remove all these and put them in the analysis methods instead.
void clean_up_ifs(Form* top_level_form) {
  bool changed = true;
  while (changed) {
    changed = false;
    top_level_form->apply([&](FormElement* elt) {
      auto as_cne = dynamic_cast<CondNoElseElement*>(elt);
      if (!as_cne) {
        return;
      }

      auto top_condition = as_cne->entries.front().condition;
      if (!top_condition->is_single_element() && elt->parent_form) {
        auto real_condition = top_condition->back();
        top_condition->pop_back();

        auto& parent_vector = elt->parent_form->elts();
        // find us in the parent vector
        auto me = std::find_if(parent_vector.begin(), parent_vector.end(),
                               [&](FormElement* x) { return x == elt; });
        assert(me != parent_vector.end());

        // now insert the fake condition
        parent_vector.insert(me, top_condition->elts().begin(), top_condition->elts().end());
        top_condition->elts() = {real_condition};
        changed = true;
      }
    });

    top_level_form->apply([&](FormElement* elt) {
      auto as_sc = dynamic_cast<ShortCircuitElement*>(elt);
      if (!as_sc) {
        return;
      }

      auto top_condition = as_sc->entries.front().condition;
      if (!top_condition->is_single_element() && elt->parent_form) {
        auto real_condition = top_condition->back();
        top_condition->pop_back();

        auto& parent_vector = elt->parent_form->elts();
        // find us in the parent vector
        auto me = std::find_if(parent_vector.begin(), parent_vector.end(),
                               [&](FormElement* x) { return x == elt; });
        assert(me != parent_vector.end());

        // now insert the fake condition
        parent_vector.insert(me, top_condition->elts().begin(), top_condition->elts().end());
        top_condition->elts() = {real_condition};
        changed = true;
      }
    });

    top_level_form->apply([&](FormElement* elt) {
      auto as_ge = dynamic_cast<GenericElement*>(elt);
      if (!as_ge) {
        return;
      }

      if (as_ge->op().kind() == GenericOperator::Kind::CONDITION_OPERATOR) {
        if (as_ge->op().condition_kind() == IR2_Condition::Kind::TRUTHY) {
          assert(as_ge->elts().size() == 1);
          auto top_condition = as_ge->elts().front();
          if (!top_condition->is_single_element() && elt->parent_form) {
            auto real_condition = top_condition->back();
            top_condition->pop_back();

            auto& parent_vector = elt->parent_form->elts();
            // find us in the parent vector
            auto me = std::find_if(parent_vector.begin(), parent_vector.end(),
                                   [&](FormElement* x) { return x == elt; });
            assert(me != parent_vector.end());

            // now insert the fake condition
            parent_vector.insert(me, top_condition->elts().begin(), top_condition->elts().end());
            top_condition->elts() = {real_condition};
            changed = true;
          }
        }
      }
    });
  }
}

bool convert_to_expressions(Form* top_level_form,
                            FormPool& pool,
                            Function& f,
                            const DecompilerTypeSystem& dts) {
  assert(top_level_form);

  //  fmt::print("Before anything:\n{}\n",
  //  pretty_print::to_string(top_level_form->to_form(f.ir2.env)));
  try {
    //    top_level_form->apply_form([&](Form* form) {
    //      if (form == top_level_form || !form->is_single_element()) {
    //        FormStack stack;
    //        for (auto& entry : form->elts()) {
    //          fmt::print("push {} to stack\n", entry->to_form(f.ir2.env).print());
    //          entry->push_to_stack(f.ir2.env, pool, stack);
    //        }
    //        std::vector<FormElement*> new_entries;
    //        if (form == top_level_form && f.type.last_arg() != TypeSpec("none")) {
    //          new_entries = stack.rewrite_to_get_reg(pool, Register(Reg::GPR, Reg::V0));
    //        } else {
    //          new_entries = stack.rewrite(pool);
    //        }
    //        assert(!new_entries.empty());
    //        form->clear();
    //        for (auto x : new_entries) {
    //          form->push_back(x);
    //        }
    //      }
    //    });

    FormStack stack;
    for (auto& entry : top_level_form->elts()) {
      //      fmt::print("push {} to stack\n", entry->to_form(f.ir2.env).print());
      entry->push_to_stack(f.ir2.env, pool, stack);
      //      fmt::print("Stack is now:\n{}\n", stack.print(f.ir2.env));
    }
    std::vector<FormElement*> new_entries;
    if (f.type.last_arg() != TypeSpec("none")) {
      auto return_var = f.ir2.atomic_ops->end_op().return_var();
      new_entries = stack.rewrite_to_get_var(pool, return_var, f.ir2.env);
      auto reg_return_type =
          f.ir2.env.get_types_after_op(f.ir2.atomic_ops->ops.size() - 1).get(return_var.reg());
      if (!dts.ts.typecheck(f.type.last_arg(), reg_return_type.typespec(), "", false, false)) {
        // we need to cast the final value.
        auto to_cast = new_entries.back();
        new_entries.pop_back();
        auto cast = pool.alloc_element<CastElement>(f.type.last_arg(),
                                                    pool.alloc_single_form(nullptr, to_cast));
        new_entries.push_back(cast);
      }
    } else {
      new_entries = stack.rewrite(pool);
    }
    assert(!new_entries.empty());
    top_level_form->clear();
    for (auto x : new_entries) {
      top_level_form->push_back(x);
    }

    //    fmt::print("Before clean:\n{}\n",
    //    pretty_print::to_string(top_level_form->to_form(f.ir2.env)));
    // fix up stuff
    clean_up_ifs(top_level_form);

  } catch (std::exception& e) {
    std::string warning = fmt::format("Expression building failed: {}", e.what());
    lg::warn(warning);
    f.warnings.append(";; " + warning);
    return false;
  }

  return true;
}
}  // namespace decompiler
