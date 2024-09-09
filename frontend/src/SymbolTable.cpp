#include <utility>

#include "../include/SymbolTable.h"

namespace SCNUCC {

auto NewSymbol(const std::string &name, const BaseType &t) -> std::shared_ptr<Symbol> {
  Type type = Type(t, SymKind::VAR);
  std::shared_ptr<Symbol> sym = std::make_shared<VarSymbol>(type, name);
  return sym;
}

template<typename T>
auto NewSymbol(const std::string &name, const BaseType &t, T x) -> std::shared_ptr<Symbol> {
  Type type = Type(t, SymKind::VAR);
  std::shared_ptr<VarSymbol> sym = std::make_shared<VarSymbol>(type, name);
  sym->SetScalerValue(x);
  return sym;
}


auto NewFuncSymbol(const std::string &name, const BaseType &t) -> std::shared_ptr<FuncSymbol> {
  Type type = Type(t, SymKind::VAR);
  std::shared_ptr<FuncSymbol> fsym = std::make_shared<FuncSymbol>(type, name);
  return fsym;
}

//auto NewFuncSymbol(const std::string& name, const BaseType& t, const std::vector<BaseType>& params) -> std::shared_ptr<FuncSymbol>{
//    Type type = Type(FUNC,t);
//    std::shared_ptr<FuncSymbol> fsym = std::make_shared<FuncSymbol>(type,name);
//    fsym->SetAllParams(params);
//    return fsym;
//}

auto Typeid2Name(int i) -> const std::string {
  const std::string mp[6] = {"VOID", "INT", "FLOAT", "STRING", "INT_PTR", "FLOAT_PTR"};
  return mp[i];
}




// 各类的实现



/*  Scope 类实现  */

//用于全局作用域
Scope::Scope(std::shared_ptr<Scope> parent, int level) : parent_(std::move(parent)), level_(level), def_cnt_(0) {}

Scope::Scope(const std::shared_ptr<Scope> &parent) : parent_(parent), def_cnt_(0) {
  level_ = parent->level_ + 1;
}

Scope::Scope(const std::shared_ptr<Scope> &parent, const std::string &name) : parent_(parent), func_name_(name),
                                                                              def_cnt_(0) {
  level_ = parent->level_ + 1;
}

auto Scope::Insert(const std::shared_ptr<VarSymbol> &sym) -> bool {
  std::string name = sym->GetString();
  auto result = symbols_.emplace(name, sym);
  if (!result.second) {
    // 插入失败，符号重定义
    return false;
  }
  return true;
}

auto Scope::Insert(const std::shared_ptr<FuncSymbol> &fsym) -> bool {
  std::string name = fsym->GetString();
  func_name_ = name;
  auto result = funcsymbols_.emplace(name, fsym);
  if (!result.second) {
    // 插入失败，符号重定义
    return false;
  }
  return true;
}


bool Scope::Remove(const std::string &name) {
  return (symbols_.erase(name) > 0) || (funcsymbols_.erase(name) > 0);
}

auto Scope::VarLookUp(const std::string &name, SearchScope curScope) const -> const std::shared_ptr<VarSymbol> & {
  auto it = symbols_.find(name);
  if (it != symbols_.end()) {
    return it->second;
  }
  // 在父作用域中查找
  if (parent_ != nullptr && curScope == SearchScope::ALL_SCOPE) {
    return parent_->VarLookUp(name, curScope);
  }
  // 未找到符号
  static std::shared_ptr<VarSymbol> nullSymbol;
  return nullSymbol;
}

auto Scope::FuncLookUp(const std::string &name) const -> const std::shared_ptr<FuncSymbol> & {
  auto it = funcsymbols_.find(name);
  if (it != funcsymbols_.end()) {
    return it->second;
  }
  // 在父作用域中查找
  if (parent_ != nullptr) {
    return parent_->FuncLookUp(name);
  }
  // 未找到符号
  static std::shared_ptr<FuncSymbol> fnullSymbol;
  return fnullSymbol;
}


auto Scope::GetParent() const -> const std::shared_ptr<Scope> & {
  return parent_;
}

auto Scope::AddChild(const std::shared_ptr<Scope> &new_scope) -> bool {
  children_.push_back(new_scope);
  return true;
}

auto Scope::IsReDefinition(const std::string &name) const -> bool {
  auto it1 = funcsymbols_.find(name);
  if (it1 != funcsymbols_.end()) return true;
  auto it2 = symbols_.find(name);
  if (it2 != symbols_.end()) return true;
  return false;
}

auto Scope::GetLevel() const -> const unsigned int {
  return level_;
}

auto Scope::GetFuncName() const -> const std::string {
  if (func_name_.empty()) {
    // 从父作用域查找
    if (level_ != 0) {
      return GetParent()->GetFuncName();
    } else {
      std::cout << "ERROR: Currently not inside a function.";
      return "";
    }
  }
  return func_name_;
}


/* Symbol 类实现 */
Symbol::Symbol() { scope_ = SymbolTable::GetCurrentScope(); }
// Symbol::Symbol(const Type& t) : type_(t){scope_= SymbolTable::GetCurrentScope();}


Symbol::Symbol(const Type &t, const std::string &name) : type_(t), name_(name) {
  scope_ = SymbolTable::GetCurrentScope();
  type_.is_global_ = (SymbolTable::GetCurrentScope()->GetLevel() == 0); //是否为全局变量
}


auto Symbol::GetScope() const -> std::shared_ptr<Scope> {
  return scope_;
}


auto Symbol::GetType() const -> const Type & {
  return type_;
}


void Symbol::PrintType() {
  std::cout << Typeid2Name(GetType().base_type_) << std::endl;
}


auto Symbol::GetString() const -> const std::string {
  return name_;
}

/* VarSymbol 类实现 */
auto VarSymbol::Dim() const -> size_t {
  // 返回符号的维度信息
  return type_.array_shape_.size();
}

auto VarSymbol::IsGlobal() const -> const bool {
  return type_.is_global_;
}

auto VarSymbol::IsConst() const -> const bool {
  return type_.is_const_;
}

auto VarSymbol::IsPtr() const -> bool {
  return Dim() > 0;
}

auto VarSymbol::IsFunc() const -> bool {
  return false;
}

auto VarSymbol::SetArrayVal(std::any val) -> void {
  array_val_ = std::move(val);
}

auto VarSymbol::GetArrayVal() -> std::any {
  return array_val_;
}


/* FuncSymbol 类实现 */
auto FuncSymbol::ParamSize() const -> size_t {
  return func_params_.size();
}

void FuncSymbol::SetAllParams(const std::vector<Type> &params) {
  func_params_.resize(params.size());
  func_params_.assign(params.begin(), params.end());
}

auto FuncSymbol::GetAllParams() const -> const std::vector<Type> & {
  return func_params_;
}

void FuncSymbol::PrintAllParams() {
  for (auto p: func_params_) {
    std::cout << Typeid2Name(p.base_type_) << ' ';
  }
  std::cout << std::endl;
}


auto FuncSymbol::GetReturnType() const -> const BaseType & {
  return GetType().base_type_;
}

auto FuncSymbol::IsFunc() const -> bool {
  return true;
}

void FuncSymbol::PushPackParam(const Type& t){
  func_params_.push_back(t);
}

void FuncSymbol::ResetParams(){
  func_params_.clear();
}

/* SymbolTable 类实现 */

std::shared_ptr<Scope> SymbolTable::current_scope_ = nullptr;

SymbolTable::SymbolTable() {
  global_scope_ = std::make_shared<Scope>(nullptr, 0); // 创建全局作用域
  current_scope_ = global_scope_; // 当前作用域指向全局作用域
}

void SymbolTable::EnterScope() {
  std::shared_ptr<Scope> newScope = std::make_shared<Scope>(current_scope_);
  current_scope_->AddChild(newScope); // 将新作用域添加为当前作用域的子作用域
  current_scope_ = newScope; // 更新当前作用域为新作用域
}

void SymbolTable::ExitScope() {
  if (current_scope_->GetParent() != nullptr) {
    current_scope_ = current_scope_->GetParent(); // 切换到父作用域
  }
}

auto SymbolTable::Insert(const std::shared_ptr<VarSymbol> &sym) -> bool {
  std::string name = sym->GetString();
  // 当前作用域存在才是重定义
  if (VarIsDefined(name, SearchScope::CUR_SCOPE)) {  
    std::cout << "Symbol '" << name << "' redefinition error." << std::endl;
    return false; // 符号重定义
  } else {
    current_scope_->Insert(sym);
    return true;
  }
}

auto SymbolTable::Insert(const std::shared_ptr<FuncSymbol> &fsym) -> bool {
  std::string name = fsym->GetString();
  if (FuncIsDefined(name)) {
    std::cout << "Symbol '" << name << "' redefinition error." << std::endl;
    return false; // 符号重定义
  } else {
    current_scope_->Insert(fsym);
    return true;
  }
}


auto SymbolTable::Remove(const std::string &name) -> bool {
  return current_scope_->Remove(name);
}


auto SymbolTable::VarLookUp(const std::string &name, SearchScope curScope) const -> const std::shared_ptr<VarSymbol> & {
  return current_scope_->VarLookUp(name, curScope);
}

auto SymbolTable::FuncLookUp(const std::string &name) const -> const std::shared_ptr<FuncSymbol> & {
  return current_scope_->FuncLookUp(name);
}


auto SymbolTable::VarIsDefined(const std::string &name, SearchScope curScope) -> bool {
  return VarLookUp(name, curScope) != nullptr;
}

auto SymbolTable::FuncIsDefined(const std::string & name) -> bool {
  return FuncLookUp(name) != nullptr;
}

auto SymbolTable::GetCurrentScope() -> const std::shared_ptr<Scope> & {
  return current_scope_;
}

auto SymbolTable::GetFuncName() const -> const std::string {
  return GetCurrentScope()->GetFuncName();
}

auto SymbolTable::IsGlobalScope() const -> bool {
  return GetCurrentScope()->GetLevel() == 0;
}

auto toBaseType(const std::string &str) -> BaseType {
  if (str == "int") return BaseType::INT;
  else if (str == "float") return BaseType::FLOAT;
  else if (str == "void") return BaseType::VOID;
  else throw std::runtime_error("Unsupported type");
}

auto toIRType(BaseType base_type) -> IRTYPE::IRTYPE {
  switch (base_type) {
    case BaseType::INT:
      return IRTYPE::IR_I32;
    case BaseType::FLOAT:
      return IRTYPE::IR_FLOAT;
    case BaseType::VOID:
      return IRTYPE::IR_VOID;
    default:
      throw std::runtime_error("Unsupported type");
  }
}

auto toIRType(const Type &type) -> IRTYPE::IRTYPE {
  return toIRType(type.base_type_);
}

auto TOIRType(const std::shared_ptr<Symbol> &sym) -> IRTYPE::IRTYPE {
  return toIRType(sym->GetType());
}

auto toIRType(const std::string &str) -> IRTYPE::IRTYPE {
  if (str == "int") return IRTYPE::IR_I32;
  else if (str == "float") return IRTYPE::IR_FLOAT;
  else if (str == "void") return IRTYPE::IR_VOID;
  else throw std::runtime_error("Unsupported type");
}


} // SCNUCC
