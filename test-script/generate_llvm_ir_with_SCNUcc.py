import os
import subprocess
import sys

from rich import print

# 定义目录
gen_ir_path = './compiler'


test_case_dir = '../test_cases/passed_cases/'
# test_case_dir = '../test_cases/public_cases/'
# test_case_dir = '../test_cases/public_cases/functional'
# test_case_dir = '../test_cases/public_cases/hidden_functional'
# test_case_dir = '../test_cases/public_cases/performance'
# test_case_dir = '../test_cases/public_cases/final_performance'

our_irs_dir = './our_irs/'
log_dir = './log/'
error_file_path = './log/gen_ir_log.log'

def generate():

    # 确保目标目录存在
    os.makedirs(our_irs_dir, exist_ok=True)
    os.makedirs(log_dir, exist_ok=True)

    # 存储错误信息的列表
    error_messages = []


    # 递归遍历 公开样例 文件夹中的所有文件
    for root,dirs,files in os.walk(test_case_dir):
        # print(root,dirs,files)
        for filename in files:
            
            if filename.endswith('.sy'):
                # 获取文件的基础名
                base_name = os.path.splitext(filename)[0]
                c_file_path = os.path.join(test_case_dir, filename)
                
                # 生成输出文件路径
                output_ir_path = os.path.join(our_irs_dir, f"{base_name}.ll")

                # 运行 gen_ir 可执行文件，将输出重定向到文件
                gen_ir_command = [gen_ir_path, "-ll", '-o', output_ir_path, c_file_path]
                process = subprocess.run(gen_ir_command, universal_newlines=True)
                        
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
    print("[bold yellow]Start generate LLVM IR with SCNUCC.[/bold yellow]")
    exit_code = generate()
    print("[bold yellow]End generate LLVM IR with SCNUCC.[/bold yellow]")
    print(f"[bold yellow]Message saved to {error_file_path}.[/bold yellow]")
    sys.exit(exit_code)