#ifndef UTIL_H
#define UTIL_H

#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>



namespace Util {

/** 
 *
 * 返回浮点数的64位16进制IEEE 754编码
 * @param double num
 * @return std::string
 */
auto GetIEEE754(double num) -> std::string;

auto GetIEEE75416(float num) -> std::string;



/* 管理全局字符串以实现库函数putf */
// id 从 1 开始计数
inline std::vector<std::string> gstrings={""}; //与调用编号一一对应
inline int CUR_POOLID = 1; // 字符串池编号 
inline int CUR_CALLID = 1; // 语句调用编号（次）
inline int CALLIR_STRING_ID = 1; //与调用编号一一对应 

inline std::unordered_map<int,int> Callid2Poolid; // 调用编号 -> 字符串池编号
inline std::unordered_map<int,int> Poolid2Callid; // 字符串池编号 -> 调用编号
inline std::unordered_map<std::string,int> String2Poolid; // 字符串 -> 字符串池编号

void PushBackgstring(const std::string);


} //namespace Util


#endif // UTIL_H