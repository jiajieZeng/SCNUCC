#include <iomanip>

#include "../include/Util.h"



namespace Util {
auto GetIEEE754(double num) -> std::string{
  double x = num;
  unsigned long long *int_val = reinterpret_cast<unsigned long long*>(&x);
  std::stringstream ss; 
  ss << std::hex << std::setfill('0') << std::setw(16) << *int_val;
  return "0x" + ss.str();
}

auto GetIEEE75416(float num) -> std::string {
  float x = num;
  unsigned long *int_val = reinterpret_cast<unsigned long*>(&x);
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(8) << *int_val;
  return "0x" + ss.str();
}


/* 字符串 */


void PushBackgstring(const std::string s) {
  gstrings.push_back(s);
  // 字符串池不存在
  if( String2Poolid[s] == 0){
    Callid2Poolid[CUR_CALLID] = CUR_POOLID;
    Poolid2Callid[CUR_POOLID] = CUR_CALLID++;
    String2Poolid[s] = CUR_POOLID++;
  }
  // 已经有了
  else{
    Callid2Poolid[CUR_CALLID++] = String2Poolid[s];
  }
  
}

}

