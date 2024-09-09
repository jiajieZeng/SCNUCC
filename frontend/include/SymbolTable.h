#ifndef SymbolTable_H
#define SymbolTable_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <utility>
#include <any>
#include <map>
#include <typeinfo>
#include "./LLVMIR.h"


namespace SCNUCC {

typedef enum {
    VAR, FUNC
} SymKind;

typedef enum {
    VOID, INT, FLOAT ,STRING
} BaseType;

typedef enum {
  CUR_SCOPE, ALL_SCOPE
} SearchScope;

struct Type {
    SymKind kind_;

    BaseType base_type_;

    bool is_const_ = false;

    bool is_global_ = false;

    std::vector<size_t> array_shape_;

    Type() = default;

    explicit Type(BaseType t, SymKind k = SymKind::VAR, bool is_const = false, bool is_global = false) : kind_(k), base_type_(t),
                                                                                                is_const_(is_const),
                                                                                is_global_(is_global) {}

    // 针对数组的构造函数
    explicit Type(
      BaseType t, 
      SymKind k, 
      bool is_const, 
      bool is_global,
      const std::vector<size_t>& shape
    ) 
    : base_type_(t), 
      kind_(k), 
      is_const_(is_const), 
      is_global_(is_global), 
      array_shape_(shape) 
    {}

};


class Symbol;

class VarSymbol;

class FuncSymbol;

class Scope;

class SymbolTable;


/* 为了便于调用与测试，添加一些创建对象的函数 */
auto NewSymbol(const std::string &, const BaseType &) -> std::shared_ptr<Symbol>;

template<typename T>
auto NewSymbol(const std::string &, const BaseType &, T) -> std::shared_ptr<Symbol>;

auto NewFuncSymbol(const std::string &, const BaseType &) -> std::shared_ptr<FuncSymbol>;

auto NewFuncSymbol(const std::string &, const BaseType &, const std::vector<BaseType> &) -> std::shared_ptr<FuncSymbol>;

auto Typeid2Name(int) -> const std::string;


class Scope {
private:
  std::shared_ptr<Scope> parent_;

  std::vector<std::shared_ptr<Scope> > children_;

  std::unordered_map<std::string, std::shared_ptr<VarSymbol> > symbols_;

  std::unordered_map<std::string, std::shared_ptr<FuncSymbol> > funcsymbols_;

  unsigned int def_cnt_;

  unsigned int level_;

  std::string func_name_; //针对函数作用域


public:
  Scope(const std::shared_ptr<Scope> &parent);

  Scope(std::shared_ptr<Scope> parent, int);

  Scope(const std::shared_ptr<Scope> &parent, const std::string &);

  auto Insert(const std::shared_ptr<VarSymbol> &) -> bool;

  auto Insert(const std::shared_ptr<FuncSymbol> &) -> bool;

  auto Remove(const std::string &) -> bool;

  // 新加一个参数表示仅在当前作用域查找
  auto VarLookUp(const std::string &, SearchScope) const -> const std::shared_ptr<VarSymbol> &;

  auto FuncLookUp(const std::string &) const -> const std::shared_ptr<FuncSymbol> &;

  auto GetParent() const -> const std::shared_ptr<Scope> &;

  auto AddChild(const std::shared_ptr<Scope> &new_scope) -> bool;

  auto IsReDefinition(const std::string &) const -> bool;

  auto GetLevel() const -> const unsigned int;

  auto GetFuncName() const -> const std::string;

};


class Symbol {
protected:
  Type type_;

  std::shared_ptr<Scope> scope_;

  std::string name_; //符号名称

public:
  Symbol();

  Symbol(const Type &, const std::string &);

  virtual bool IsFunc() const = 0;

  virtual auto GetScope() const -> std::shared_ptr<Scope>;

  auto GetType() const -> const Type &;

  void PrintType(); //打印实际类型

  auto GetString() const -> const std::string;

};

class VarSymbol : public Symbol {
protected:
  std::map<int, int> int_value_;
  std::map<int, float> float_value_;
  std::any array_val_;

public:
  VarSymbol(const Type &t, const std::string &n) : Symbol(t, n) {}

  auto IsPtr() const -> bool;

  auto IsFunc() const -> bool;

  auto Dim() const -> size_t;

  auto IsGlobal() const -> const bool; //是否为全局变量

  auto IsConst() const -> const bool; //是否为常量

  auto SetArrayVal(std::any val) -> void;

  auto GetArrayVal() -> std::any;


  /**
   * 通过indices数组来找到这个位置的常量, 转换太费时间.
  */
  template<typename T>
  auto GetArrayConst(const std::vector<size_t> &indices) -> T {
    int pos = 0;
    int stride = 1;
    std::vector<size_t> shape = type_.array_shape_;
    for (int i = (int)indices.size() - 1; i >= 0; i--) {
      pos += indices[i] * stride;
      stride *= shape[i];
    }
    if (type_.base_type_ == BaseType::INT) {
      return int_value_[pos];
    } else {
      return float_value_[pos];
    }
  }

  /**
   * @brief 设置一个数组变量的值。
   * 需要保证vec.size() == array_shape[0]*..*array_shape[n]
   * @tparam T
   */
  template<typename T>
  auto SetValue(const std::map<int, T> &x) -> void {
    // 如果不是数组，直接设置值
    if (type_.base_type_ == BaseType::INT || type_.base_type_ == BaseType::FLOAT) {
      if (type_.array_shape_.empty()) {
        if (x.size() != 1) {
          throw std::runtime_error("Array size mismatch");
        }
        if (type_.base_type_ == BaseType::INT) {
          int_value_[0] = x.at(0);
        } else if (type_.base_type_ == BaseType::FLOAT) {
          float_value_[0] = x.at(0);
        }
        return;
      }
    }
    if (type_.base_type_ == BaseType::INT) {
      // int_value_ = x;
      for (auto &[key, value]: x) {
        int_value_[key] = x.at(key);
      }
    } else if (type_.base_type_ == BaseType::FLOAT) {
      for (auto &[key, value]: x) {
        int_value_[key] = x.at(key);
      }
    }
  }

  /**
   * @brief 获取一个数组变量的值
   * @tparam T
   * @return
   */
  template<typename T>
  auto GetValue() const -> const std::vector<T> & {
    if (type_.base_type_ == BaseType::INT) {
      return int_value_;
    } else if (type_.base_type_ == BaseType::FLOAT) {
      return float_value_;
    } else {
      throw std::runtime_error("Unsupported type");
    }
  }

  /**
   * @brief 设置一个标量变量的值
   * @tparam T
   */
  template<typename T>
  void SetScalerValue(const T & x) {
    if (type_.base_type_ == BaseType::INT) {
      int_value_[0] = x;
    } else if (type_.base_type_ == BaseType::FLOAT) {
      float_value_[0] = x;
    } else {
      throw std::runtime_error("Unsupported type");
    }
  }

  /**
   * @brief 获取一个标量变量的值
   * @tparam T
   * @return
   */
  template<typename T>
  auto GetScalerValue() const -> T {
    if (type_.base_type_ == BaseType::INT) {
      return int_value_.at(0);
    } else if (type_.base_type_ == BaseType::FLOAT) {
      return float_value_.at(0);
    } else {
      throw std::runtime_error("Unsupported type");
    }

  }
};

class FuncSymbol : public Symbol {
private:
  std::vector<Type> func_params_;

public:
  // FuncSymbol();

  FuncSymbol(const Type &t, const std::string &n) : Symbol(t, n) {}

  FuncSymbol(const Type &t, const std::string &n, const std::vector<Type> &fp) : Symbol(t, n),func_params_(std::move(fp)) {}

  void SetAllParams(const std::vector<Type> &);

  void PushPackParam(const Type &);

  void ResetParams();

  void PrintAllParams();

  auto ParamSize() const -> size_t;

  auto GetAllParams() const -> const std::vector<Type> &;

  auto GetReturnType() const -> const BaseType &;

  auto IsFunc() const -> bool override;


};

class SymbolTable {
private:
  std::shared_ptr<Scope> global_scope_;

  std::unordered_map<std::string, std::shared_ptr<Scope> > func_scope_; //这个在实际场景中要如何调用？

  static std::shared_ptr<Scope> current_scope_;

public:
  SymbolTable();

  void EnterScope(); //进入作用域

  void ExitScope(); //退出作用域

  auto Insert(const std::shared_ptr<VarSymbol> &) -> bool;

  auto Insert(const std::shared_ptr<FuncSymbol> &) -> bool;

  auto Remove(const std::string &) -> bool;

  // 新加一个参数表示是否只在当前作用域寻找
  auto VarLookUp(const std::string &, SearchScope) const -> const std::shared_ptr<VarSymbol> &;

  auto FuncLookUp(const std::string &) const -> const std::shared_ptr<FuncSymbol> &;

  // 新加一个参数表示是否只在当前作用域寻找
  auto VarIsDefined(const std::string &, SearchScope) -> bool;

  auto FuncIsDefined(const std::string &) -> bool;

  static auto GetCurrentScope() -> const std::shared_ptr<Scope> &;

  auto GetFuncName() const -> const std::string; //当前所处函数作用域的名称

  auto IsGlobalScope() const -> bool;
};

auto toBaseType(const std::string &) -> BaseType;

auto toIRType(BaseType) -> IRTYPE::IRTYPE;

auto toIRType(const Type &) -> IRTYPE::IRTYPE;

auto toIRType(const std::shared_ptr<Symbol> &) -> IRTYPE::IRTYPE;

auto toIRType(const std::string &) -> IRTYPE::IRTYPE;

} // SCNUCC

#endif  //SymbolTable_H