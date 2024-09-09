import os
import subprocess

# Define paths
test_case_dir = "../test_cases/passed_cases/"
asm_dir = "./our_asms/"
lib_path = "../dependencies/lib/"
sylib = "-lsysy"
exe_dir = "./our_exes/"
log_file = os.path.join('log/', "asm_compile_log.log")
timeout_seconds = 10  # Timeout in seconds
output_dir = './our_output/'

# Create output directory and log file (if not exists)
os.makedirs(exe_dir, exist_ok=True)

def get_prefix_before_underscore(s):
    return int(s.split('_', 1)[0])

all = 0
succ = 0

filelist = []
for filename in os.listdir(test_case_dir):
    if filename.endswith(".sy"):
        filelist.append((get_prefix_before_underscore(filename), filename.replace('.sy', '')))
filelist.sort()
sorted_filenames = [filename for _, filename in filelist]

# Open the log file, ready to write log information
with open(log_file, "w") as log:
    log.write("COMPILE:\n")
    # Traverse all .s files in the asm_dir directory


    for filename in sorted_filenames:
        asm_file = os.path.join(asm_dir, filename) + '.s'
        out_file = os.path.join(exe_dir, filename)

        # Assemble and link the .s file to create an executable
        link_command = ["riscv64-unknown-elf-gcc", "-o", out_file, asm_file, "-L", lib_path, sylib]
        if os.path.exists(asm_file):
            try:
                subprocess.run(link_command, timeout=timeout_seconds, check=True, stderr=subprocess.PIPE)
                msg = f"[SUCCESS] Linked {asm_file} to {out_file}"
                log.write(msg + "\n")
                print(msg)
                succ += 1
            except subprocess.TimeoutExpired:
                msg = f"[ERROR] Timeout when linking {asm_file}"
                log.write(msg + "\n")
                log.write(msg)
            except subprocess.CalledProcessError as e:
                msg = f"[ERROR] Error during linking of {asm_file}. Return code: {e.returncode}"
                log.write(msg)
                print(msg)
                msg = f"Command: {' '.join(e.cmd)}"
                log.write(msg + "\n")
                print(msg)
                msg = f"Error message: {e.stderr.decode('utf-8')}"
                log.write(msg + "\n")
                print(msg)
        else:
            msg = f"[ERROR] File {asm_file} not found"
            print(msg)
            log.write(msg + "\n")
    
    msg = f"[COMPILE SUCCESS/ALL]{succ}/{len(sorted_filenames)}\n"
    print(msg)
    log.write(msg)
    log.write("EXCUTE:\n")
    succ = 0
    for filename in sorted_filenames:
        exe_file = os.path.join(exe_dir, filename)
        if not os.path.exists(exe_file):
            msg = f"[ERROR] File {exe_file} not found"
            print(msg)
            log.write(msg + "\n")
        else:
            # 检查是否有 .in 文件
            in_file_path = os.path.join(test_case_dir, f"{filename}.in")
            output_file_path = os.path.join(output_dir, f"{filename}.out")
            if os.path.exists(in_file_path):
                with open(in_file_path, 'r') as infile, open(output_file_path, 'w') as outfile:
                    # 运行可执行文件并重定向输出
                    run_command = ["qemu-riscv64", exe_file]
                    try:
                        run_result = subprocess.run(run_command, stdin=infile, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=20)
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
                    except subprocess.TimeoutExpired:
                        outfile.write("-1\ntimeOutError" +"\n")
            else:
                with open(output_file_path, 'w') as outfile:
                    # 运行可执行文件并重定向输出
                    run_command = ["qemu-riscv64", exe_file]
                    try:
                        run_result = subprocess.run(run_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=20)
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
                    except subprocess.TimeoutExpired:
                        outfile.write("-1\ntimeOutError" +"\n")
