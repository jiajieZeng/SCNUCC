import os
import sys

from rich import print

# 定义目录
clang_output_dir = '../test_cases/passed_cases/'


our_output_dir = './our_output/'
error_file_path = './log/compair_log.log'

def get_suffix(s):
    return s.split('.', 1)[1]

def get_prefix(s):
    return int(s.split('_', 1)[0])

def compair():
    # 创建存储错误文件的列表
    error_messages = []
    cnt = 0
    # 获取所有文件名
    clang_files = os.listdir(clang_output_dir)
    our_files = os.listdir(our_output_dir)
    our_files.sort()
    allfilenames = []
    for filename in clang_files:
        if get_suffix(filename) == "sy" or get_suffix(filename) == "in":
            continue
        else:
            allfilenames.append((get_prefix(filename),filename))
    allfilenames.sort()
    sorted_filenames = [filename for _, filename in allfilenames]
    # 比较两个目录下同名文件的内容
    for filename in sorted_filenames:
        if filename in our_files:
            with open(os.path.join(clang_output_dir, filename), 'r') as clang_file, open(os.path.join(our_output_dir, filename), 'r') as our_file:
                clang_lines = clang_file.read().splitlines()
                clang_content = ''
                for lines in clang_lines:
                    lines.rstrip()
                    while len(lines) != 0:
                        if lines[-1] == ' ' or lines[-1] == '\n':
                            lines = lines[:-1]
                        else:
                            break
                    if len(lines) == 0:
                        continue
                    if len(clang_content) == 0:
                        clang_content += lines
                    else:
                        clang_content += '\n' + lines
                our_lines = our_file.read().splitlines()
                our_content = ''
                for lines in our_lines:
                    lines.rstrip()
                    while len(lines) != 0:
                        if lines[-1] == ' ' or lines[-1] == '\n':
                            lines = lines[:-1]
                        else:
                            break
                    if len(lines) == 0:
                        continue
                    if len(our_content) == 0:
                        our_content += lines
                    else:
                        our_content += '\n' + lines
                if clang_content == our_content:
                    print(f"[bold green]Success: Content of {filename} matches.[/bold green]")
                    cnt += 1
                else:
                    print(f"[bold red]Failed:  Content of {filename} does not match.[/bold red]")
                    error_messages.append(f"Failed: Content of {filename} does not match.\nour output: \n{our_content} \nstd output: \n{clang_content}")
        else:
            print(f"[bold red]Failed: {filename} does not exist in directory.[/bold red]")
            error_messages.append(f"Failed: {filename} does not exist in directory.")

    # 将失败文件列表写入错误文件
    with open(error_file_path, 'w') as error_file:
        for msg in error_messages:
            error_file.write(msg)
            error_file.write('\n')
    return cnt

if __name__ == '__main__':
    print("[bold yellow]Comparison start.[/bold yellow]")
    cnt = compair()
    print("[bold yellow]Comparison completed.[/bold yellow]")
    print(f"[bold green]AC: {cnt}/{99}")
    print(f"[bold yellow]Message saved to {error_file_path}.[/bold yellow]")
    # sys.exit(exit_code)