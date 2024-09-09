import os
import subprocess

# 定义路径
test_case_dir = "../test_cases/passed_cases/"
gen_code_executable = "./compiler"
lib_path = "../test_cases/lib/"
sylib = "-lsylib"
output_dir = "./our_asms/"
log_file = os.path.join('log/', "asm_log.log")
timeout_seconds = 10  # 超时时间（秒）

# 创建输出目录和日志文件（如果不存在）
os.makedirs(output_dir, exist_ok=True)

def get_prefix_before_underscore(s):
    return int(s.split('_', 1)[0])

result_list = []

# 遍历test_case目录下的所有.sy文件
filelist = []
for filename in os.listdir(test_case_dir):
    filelist.append((get_prefix_before_underscore(filename), filename))
filelist.sort()
sortedfilenames = [filename for _, filename in filelist]
all = 0
succ = 0
for filename in sortedfilenames:
    if filename.endswith(".sy"):
        all += 1
        base_name = filename[:-3]  # 去掉.sy扩展名
        sy_file = os.path.join(test_case_dir, filename)
        s_file = os.path.join(output_dir, base_name + ".s")
        
        try:
            # Generate .S file from .sy
            subprocess.run([gen_code_executable, "-S", "-o", s_file, sy_file], stderr=subprocess.PIPE, check=True, timeout=timeout_seconds)
            ret = f"[SUCCESS] {filename} generated {s_file} successfully."
            result_list.append(ret)
            print(ret)
            succ += 1
        except subprocess.CalledProcessError as e:
            ret = f"[ERROR] Failed to generate {s_file}. Return code: {e.returncode}"
            cmd = f"Command: {' '.join(e.cmd)}"
            errcode = f"Error message: {e.stderr.decode('utf-8')}"
            print(ret)
            result_list.append(ret)
            print(cmd)
            result_list.append(red)
            print(errcode)
            result_list.append(errcode)
        except subprocess.TimeoutExpired:
            ret = f"[ERROR] Generation of {s_file} timed out."
            print(ret)
            result_list.append(ret)
        except Exception as e:
            ret = f"[ERROR] Unexpected error occurred while processing {filename}: {str(e)}"
            print(ret)
            result_list.append(ret)
print(f"[SUSCESS/ALL]{succ}/{all}")
# 打开文件并将列表的内容写入
with open(log_file, "w") as file:
    for item in result_list:
        file.write(f"{item}\n")  # 将每个元素写入文件并换行
    file.write(f"[SUSCESS/ALL]{succ}/{all}")