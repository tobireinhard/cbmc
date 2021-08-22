/*******************************************************************\

Module: Helper functions for k-induction and loop invariants

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Helper functions for k-induction and loop invariants

#include "loop_utils.h"

#include <analyses/local_may_alias.h>

#include <goto-programs/pointer_arithmetic.h>

#include <util/pointer_expr.h>
#include <util/std_expr.h>

goto_programt::targett get_loop_exit(const loopt &loop)
{
  assert(!loop.empty());

  // find the last instruction in the loop
  std::map<unsigned, goto_programt::targett> loop_map;

  for(loopt::const_iterator l_it=loop.begin();
      l_it!=loop.end();
      l_it++)
    loop_map[(*l_it)->location_number]=*l_it;

  // get the one with the highest number
  goto_programt::targett last=(--loop_map.end())->second;

  return ++last;
}


void get_modifies_lhs(
  const local_may_aliast &local_may_alias,
  goto_programt::const_targett t,
  const exprt &lhs,
  modifiest &modifies)
{
  if(lhs.id() == ID_symbol || lhs.id() == ID_member || lhs.id() == ID_index)
    modifies.insert(lhs);
  else if(lhs.id()==ID_dereference)
  {
    const pointer_arithmetict ptr(to_dereference_expr(lhs).pointer());
    for(const auto &mod : local_may_alias.get(t, ptr.pointer))
    {
      if(ptr.offset.is_nil())
        modifies.insert(dereference_exprt{mod});
      else
        modifies.insert(dereference_exprt{plus_exprt{mod, ptr.offset}});
    }
  }
  else if(lhs.id()==ID_if)
  {
    const if_exprt &if_expr=to_if_expr(lhs);

    get_modifies_lhs(local_may_alias, t, if_expr.true_case(), modifies);
    get_modifies_lhs(local_may_alias, t, if_expr.false_case(), modifies);
  }
}

void get_modifies(
  const local_may_aliast &local_may_alias,
  const loopt &loop,
  modifiest &modifies)
{
  for(loopt::const_iterator
      i_it=loop.begin(); i_it!=loop.end(); i_it++)
  {
    const goto_programt::instructiont &instruction=**i_it;

    if(instruction.is_assign())
    {
      const exprt &lhs = instruction.get_assign().lhs();
      get_modifies_lhs(local_may_alias, *i_it, lhs, modifies);
    }
    else if(instruction.is_function_call())
    {
      const exprt &lhs = instruction.call_lhs();
      get_modifies_lhs(local_may_alias, *i_it, lhs, modifies);
    }
  }
}
