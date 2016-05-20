#include "dfa.h"
#include "util.h"
using namespace std;

class Digraph;

DFA::DFA(NFA* nfa) {
  _nfa = nfa;
  _nfa_to_dfa();
}
DFA::DFA(TreeNode* root) {
  _root = root;
  _re_tree_to_dfa();
}
int DFA::match(string txt, string& result) {
  for (int i = 0; i < txt.length(); i++) {
    for (int j = txt.length(); j >=i+1 ; j--) {
      if (simulate(txt.substr(i,j-i))) {
        result = txt.substr(i,j-i);
        return i;
      }
    }
  }
  return -1;
}
bool DFA::simulate(string txt) {
  // only match the txt exactly
  Digraph::DNode *cur_dnode = _start;
  for (int i = 0; i < txt.length(); i++) {
    set<Digraph::DNode*> next_set = cur_dnode->jump(txt[i]);
    if (next_set.size() == 0) {
      return false;
    }
    assert(next_set.size() == 1, "the size of next_set must be 1");
    for (auto it = next_set.begin(); it != next_set.end(); it++) {
      cur_dnode = *it;
      // we omit the break, for only one item in the next_set
    }
  }
  // find the final state at last, in order to know whether exactly match
  if (cur_dnode->accept) return true;
  else return false;
}
void DFA::minimize() {
  cout << "before minimize size: " << _Dstates.size() << endl;
  _partition();
  _connect();
  cout << "after  minimize size: " << _group.size() << endl;
}
void DFA::_partition() {
  // init _group
  _group = set<Group*>();
  Group* accept_group = new Group();
  Group* nonaccept_group = new Group();
  for (auto it : _Dstates) {
    if (it.second->dnode->accept) {
      accept_group->dfa_node_set.insert(it.second->dnode);
    } else {
      nonaccept_group->dfa_node_set.insert(it.second->dnode);
    }
  }
  _group.insert(accept_group);
  _group.insert(nonaccept_group);
  // cout << "accept size " <<  accept_group->dfa_node_set.size() << endl;
  // cout << "nonaccept size " << nonaccept_group->dfa_node_set.size() << endl;
  // init _group_map
  _group_map.clear();
  for (auto it_group : _group) {
    for (auto it_dfa_node : it_group->dfa_node_set) {
      _group_map[it_dfa_node] = it_group;
    }
  }

  set<Group*> next_group = set<Group*>();
  bool first = true;
  while(true) {
    if (!first) {
      if (Group::group_set_deep_equal(_group, next_group)) {
        _group = next_group;
        break;
      } else {
        _group = next_group;
      }
    }
    cout << "_group.size = " << _group.size() << endl;
    first = false;
    next_group.clear();
    for (auto it_group : _group) { // for each group
      set<char> input_set;
      for (auto it_dfa_node: it_group->dfa_node_set) { // get input
        for (auto it_edge : it_dfa_node->out) {
          input_set.insert(it_edge->symbol);
        }
      }

      set<Group*> splited_group = it_group->split(input_set, _group_map);
      for (auto it: splited_group) {
        next_group.insert(it);
      }
    }
    // update _group_map
    _group_map.clear();
    for (auto it_group : next_group) {
      for (auto it_dfa_node : it_group->dfa_node_set) {
        _group_map[it_dfa_node] = it_group;
        // cout << it_dfa_node << " => " << it_group << endl;
      }
    }
  }
}
void DFA::_connect() {
  // setup representative for each group
  map< Digraph::DNode*,Digraph::DNode* > representative_map; // old => new
  for (auto it_group : _group) {
    for (auto it_dfa_node : it_group->dfa_node_set) {
      it_group->representative = it_dfa_node;
      representative_map[it_dfa_node] = new Digraph::DNode();
      break;
    }
  }
  auto mini_start = representative_map[_group_map[_start]->representative];
  // // set accept state
  for (auto it_group : _group) {
    for (auto it_dfa_node : it_group->dfa_node_set) {
      if (it_dfa_node->accept) {
        representative_map[_group_map[it_dfa_node]->representative]->accept = true;
      }
    }
  }
  // // set transition
  for (auto it_group : _group) {
    auto old_s = it_group->representative;
    auto new_s = representative_map[old_s];
    for (auto it_edge : old_s->out) {
      auto old_t = it_edge->to;
      auto new_t = representative_map[_group_map[old_t]->representative];
      Digraph::addEdge(new_s, it_edge->symbol, new_t);
    }
  }
  _start = mini_start;
}
bool DFA::Group::group_set_deep_equal(set< Group* > & gs1, set< Group* > &gs2) {
  if (gs1.size() != gs2.size()) {
    return false;
  }
  for (auto g1 : gs1) {
    bool has_equal = false;
    for (auto g2 : gs2) {
      if (g1->dfa_node_set == g2->dfa_node_set) {
        has_equal = true;
        break;
      }
    }
    if (!has_equal) return false;
  }
  return true;
}

set<DFA::Group*> DFA::Group::split(set<char> input_set, map< Digraph::DNode*,Group* > &group_map) {
  // for each s:(it1),t:(it2) dfa state pair
  set<DFA::Group*> result;
  map< Digraph::DNode*,bool > has_group;
  for (auto it: this->dfa_node_set) {
    has_group[it] = false;
  }
  for (auto it1 = this->dfa_node_set.begin(); it1 != this->dfa_node_set.end(); it1++) {
    if (has_group[*it1]) {
      continue;
    }
    Group* outer_group = new Group();
    outer_group->dfa_node_set.insert(*it1);
    has_group[*it1] = true;
    auto it2 = it1;
    it2++;
    for (; it2 != this->dfa_node_set.end(); it2++) {
      if (has_group[*it2]) {
        continue;
      }
      bool can_group = true;
      for (auto input: input_set) {
        auto jump_set1 = (*it1)->jump(input);
        auto jump_set2 = (*it2)->jump(input);
        assert(jump_set1.size() <= 1 && jump_set2.size() <= 1, "jump_set1 and jump_set2 should be less than 1");
        if (jump_set1.size() == 1 && jump_set2.size() == 1) {
          Group* group_flag1, *group_flag2;
          for (auto next_state1: jump_set1) group_flag1 = group_map[next_state1];
          for (auto next_state2: jump_set2) group_flag2 = group_map[next_state2];
          if (group_flag1 != group_flag2) {
            can_group = false;
            break;
          }
        } else {
          can_group = false;
          break;
        }
      }
      if (can_group) {
        has_group[*it2] = true;
        outer_group->dfa_node_set.insert(*it2);
      }
    }
    result.insert(outer_group);
  }
  return result;
}
void DFA::_nfa_to_dfa() {
  set<Digraph::DNode*> nfa_start_set;
  nfa_start_set.insert(_nfa->_start);
  set<Digraph::DNode*> e_closure_s0 = Digraph::e_closure(nfa_start_set);
  _start = new Digraph::DNode();
  _Dstates[e_closure_s0] = new MNode(false, _start);
  for (auto it = e_closure_s0.begin(); it != e_closure_s0.end(); it++) {
    if ((*it)->accept) {
      _Dstates[e_closure_s0]->dnode->accept = true;
      break;
    }
  }
  set<Digraph::DNode*> unmarked_state;
  unmarked_state = _find_unmarked_state();
  while (unmarked_state.size() != 0) {
    _Dstates[unmarked_state]->mark = true;
    set<char> input_symbol_set;

    // get all the input symbol
    for (auto it = unmarked_state.begin(); it != unmarked_state.end(); it++) {
      for (int i = 0; i < (*it)->out.size(); i++) {
        if ((*it)->out[i]->symbol != EPS) {
          input_symbol_set.insert((*it)->out[i]->symbol);
        }
      }
    }

    // jump and e-closure
    // some tricks here: for example we have input_set { a, b, k, ANY }
    // then for each input we can get a new DFA node  U = e-closure(jump(T,input));
    // if U not in _Dstates then add U to _Dstates
    // and then let transition table [T input] = U
    // the problem is how to deal with the ANY input
    // we know that we should let the DFA be deterministic
    // so ANY should be split as { a, b, k, OTHER} => input_set = { a, b, k, { a, b, k, OTHER} }
    // for the jump(T,input) , more specific, take input as a
    // jump(T, a) we should make use of the ANY out nodes. 
    // (maybe serval ANY out nodes, but the input_set will keep only one ANY, same as the { a, b, k })
    // then, the most tricky point come. 
    // when we get the new DFA node, the out edge should be => { a, b, k, OTHER}
    // remember "deterministic" we can only use OTHER insteal of ANY.
    // O(∩_∩)O~~ we solve the problem. (all the other input are replaced with OTHER)
    for (auto it1 = input_symbol_set.begin(); it1 != input_symbol_set.end(); it1++) {
      set<Digraph::DNode*> total_jump_set;
      for (auto it2 = unmarked_state.begin(); it2 != unmarked_state.end(); it2++) {
        set<Digraph::DNode*> jump_set = (*it2)->jump(*it1);
        for (auto it3 = jump_set.begin(); it3 != jump_set.end(); it3++) {
          total_jump_set.insert(*it3);
        }
      }
      set<Digraph::DNode*> new_group = Digraph::e_closure(total_jump_set);
      if (_Dstates.count(new_group) == 0) {
        bool accept = false;
        for (auto it = new_group.begin(); it != new_group.end(); it++) {
          if ((*it)->accept) {
            accept = true;
            break;
          }
        }
        _Dstates[new_group] = new MNode(false, new Digraph::DNode());
        _Dstates[new_group]->dnode->accept = accept;
      }
      // because the *it1 are distinct, so only one OTHER edge for a new dfa node
      Digraph::addEdge(_Dstates[unmarked_state]->dnode, 
                       (*it1==ANY)?OTHER:*it1,
                       _Dstates[new_group]->dnode);
    } // end jump and e-closure
    unmarked_state = _find_unmarked_state();
  } // end while
}
void DFA::_re_tree_to_dfa() {

}
set<Digraph::DNode*> DFA::_find_unmarked_state() {
  for (map< set<Digraph::DNode*>,MNode* >::iterator it = _Dstates.begin(); it != _Dstates.end(); it++) {
    if (it->second->mark == false) {
      return it->first;
    }
  }
  set<Digraph::DNode*> s;
  return s;
}