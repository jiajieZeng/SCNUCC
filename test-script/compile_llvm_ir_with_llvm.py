'''
Author: jiajieZeng
Date: 2024-05-21 11:09:18
LastEditors: Please set LastEditors
LastEditTime: 2024-05-21 14:20:48
Description: 请填写简介
'''
import os
import subprocess
import sys
from rich import print

# 定义目录
ir_dir = './our_irs/'
asm_dir = './our_asms/'
exe_dir = './our_exes/'

test_case_dir = '../test_cases/passed_cases/'

output_dir = './our_output/'
error_file_path = './log/compile_llvm_ir_log.log'

def compileIRAndExecute():
    # 确保目标目录存在
    os.makedirs(asm_dir, exist_ok=True)
    os.makedirs(exe_dir, exist_ok=True)
    os.makedirs(output_dir, exist_ok=True)
    
    error_messages = []
    # 遍历 our_irs 文件夹中的所有文件
    file_list = os.listdir(ir_dir)
    file_list.sort()
    for filename in file_list:
        if filename.endswith('.ll'):
            # 获取文件的基础名
            base_name = os.path.splitext(filename)[0]
            ir_file_path = os.path.join(ir_dir, filename)
            asm_file_path = os.path.join(asm_dir, f"{base_name}.s")
            exe_file_path = os.path.join(exe_dir, base_name)
            output_file_path = os.path.join(output_dir, f"{base_name}.out")

            # 使用 llc 编译 IR 文件为 ASM 文件
            llc_command = ['llc', ir_file_path, '-o', asm_file_path, '-opaque-pointers']
            # llc_command = ['llc', ir_file_path, '-o', asm_file_path]
            llc_result = subprocess.run(llc_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=10)

            if llc_result.returncode != 0:
                msg = f"Compilation of {filename} with llc failed:\n{llc_result.stderr}\n\n"
                error_messages.append(msg)
                print(f"[bold red]Compilation of {filename} with llc failed:\n{llc_result.stderr}[/bold red]")
                continue

            # 使用 clang 编译 ASM 文件为可执行文件
            clang_command = ['clang', asm_file_path, '-o', exe_file_path, '../dependencies/lib/libsysy64.a','-no-pie']
            clang_result = subprocess.run(clang_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=10)

            if clang_result.returncode != 0:
                msg = f"Compilation of {filename} with clang failed:\n{clang_result.stderr} \n\n"
                error_messages.append(msg)  
                print(f"[bold red]Compilation of {filename} with clang failed:\n{clang_result.stderr}[/bold red]")
                continue

            # 检查是否有 .in 文件
            in_file_path = os.path.join(test_case_dir, f"{base_name}.in")
            
            
            if os.path.exists(in_file_path):
                with open(in_file_path, 'r') as infile, open(output_file_path, 'w') as outfile:
                    # 运行可执行文件并重定向输出
                    run_command = [exe_file_path]
                    run_result = subprocess.run(run_command, stdin=infile, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=10)
                    res = run_result.stdout
                    if (len(res) == 0):
                        res = str(run_result.returncode)
                    else:
                        if res[-1] == '\n':
                            res = res + str(run_result.returncode)
                        else:
                            res = res + "\n" + str(run_result.returncode)
                    # print(f"{filename} out,{run_result.stdout},{run_result.returncode}")
                    outfile.write(res+"\n")
            else:
                with open(output_file_path, 'w') as outfile:
                    # 运行可执行文件并重定向输出
                    run_command = [exe_file_path]
                    run_result = subprocess.run(run_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=10)
                    res = run_result.stdout
                    
                    if (len(res) == 0):
                        res = str(run_result.returncode)
                    else:
                        if res[-1] == '\n':
                            res = res + str(run_result.returncode)
                        else:
                            res = res + "\n" + str(run_result.returncode)
                    # print(f"{filename} out,{run_result.stdout},{run_result.returncode}")
                    outfile.write(res+"\n")
                    
    # 将错误信息写入文件
    with open(error_file_path, 'w') as error_file:
        for error_message in error_messages:
            error_file.write(error_message)
            error_file.write('\n\n')
    if len(error_messages) == 0:
        return 0
    else:
        return 1

if __name__ == '__main__':
    print("[bold yellow]Start compile LLVM IR with LLVM.[/bold yellow]")
    exit_code = compileIRAndExecute()
    print("[bold yellow]End compile LLVM IR with LLVM.[/bold yellow]")
    print(f"[bold yellow]Message saved to {error_file_path}.[/bold yellow]")
    sys.exit(exit_code)