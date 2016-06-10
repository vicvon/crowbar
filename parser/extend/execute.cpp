#include "execute.h"
#include "string"
#include "CRB.h"
using namespace std;

CRB_TYPE::Value* value_copy(CRB_TYPE::Value* value) {
  CRB_TYPE::Value* result;
  auto Iheap = CRB::Interpreter::getInstance()->get_heap();
  switch(value->type) {
    case CRB_TYPE::BOOLEAN_VALUE: {
      return new CRB_TYPE::BooleanValue(dynamic_cast<CRB_TYPE::BooleanValue*>(value)->boolean_value);
    } 
    case CRB_TYPE::INT_VALUE: {
      return new CRB_TYPE::IntValue(dynamic_cast<CRB_TYPE::IntValue*>(value)->int_value);
    } 
    case CRB_TYPE::DOUBLE_VALUE: {
      return new CRB_TYPE::DoubleValue(dynamic_cast<CRB_TYPE::DoubleValue*>(value)->double_value);
    } 
    case CRB_TYPE::NULL_VALUE: {
      return new CRB_TYPE::Value(); // default NULL
    } 
    case CRB_TYPE::ARRAY_VALUE: {
      // we don't copy the array 
      // because we just use the the copy value to the stack
      // and stack will use stack_value_delete() to delete the copy value
      // howerer stack_value_delete just delete the non-object
      auto cast_array_value = dynamic_cast<CRB_TYPE::Array*>(value);
      cast_array_value->ref_cnt++;
      return cast_array_value;
      // auto old_array_value = dynamic_cast<CRB_TYPE::Array*>(value);
      // auto new_array_value = dynamic_cast<CRB_TYPE::Array*>(Iheap->alloc(CRB_TYPE::ARRAY_OBJECT));
      // new_array_value->vec.resize(old_array_value->vec.size());
      // for (int i = 0; i < old_array_value->vec.size(); i++) {
      //   new_array_value->vec[i] = value_copy(old_array_value->vec[i]);
      // }
      // cout << "copy array" << endl;
      // return new_array_value; 
    } 
    case CRB_TYPE::STRING_VALUE: {
      string* tem_string_ptr = dynamic_cast<CRB_TYPE::String*>(value)->string_value;
      result = Iheap->alloc(tem_string_ptr, false);
      return result;
    } 
    case CRB_TYPE::ASSOC_VALUE: {
      // return new 
    } 
    case CRB_TYPE::CLOSURE_VALUE: {
      auto function_definition = dynamic_cast<CRB_TYPE::Closure*>(value)->function_definition;
      auto scope_chain =  dynamic_cast<CRB_TYPE::Closure*>(value)->scope_chain;
      return new CRB_TYPE::Closure(function_definition, scope_chain);
    } 
    case CRB_TYPE::FAKE_METHOD_VALUE: {
      // return new 
    } 
    case CRB_TYPE::SCOPE_CHAIN_VALUE: {
      // return new 
    } 
  }
}

