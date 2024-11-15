# SCNUCC

## 简介
全国大学生计算机系统能力大赛编译器实现赛作品，系统使用ANTLR做词法分析、语法分析，并在ANTLR的Visitor上遍历语法树生成LLVM风格的中间代码，最后生成RISCV架构的汇编代码。

 支持特定语言、面向 RISC-V 硬件平台的综合性编译系统。该系统基于 C++ 语言开发，能够在 Ubuntu 18.04（64 位）操作系统的 x86 评测服务器上编译。它可以将符合自定义程序设计语言 SysY2022 的测试程序编译为 RISC-V 汇编语言程序（64 位），并通过汇编和链接后，在 64 位 Debian64 GNU/Linux bookworm/sid 操作系统的昉·星光2（VisionFive 2）设备上运行。

开发的编译器应具备编译大赛提供的 SysY2022 语言编写的基准测试程序的能力，包括以下功能：

* 词法分析、语法分析、语义分析、目标代码生成。

* 对于正确编译通过的 SysY2022 基准测试程序，生成符合要求的汇编文件。

## 环境

* Ubuntu 18.04

* llvm-clang 14.0.6 

* C++17

## 文件
* `main`在`compiler.cpp`
* 输出RISCV汇编：`compiler.exe -S -o outputfile.s inputfile.sy`
* 输出LLVM IR：`compiler.exe -ll -o outputfile.ll inputfile.sy`
