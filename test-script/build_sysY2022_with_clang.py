import os
import sys

from rich import print
import subprocess


def shell(command):
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(f"[bold red]Fail to run {command}: {result.stderr.decode()}[/bold red]")
        return False
    else:
        print(result.stdout.decode())
        return True


def test_frontend():
    test_files = os.listdir("../sysY2022/")
    for test_file in test_files:
        if test_file.endswith(".c"):
            test_file = test_file.split('.')[0]

            # 使用clang编译成可执行文件
            print(f"[bold yellow]Compile {test_file}.c to exe with clang[/bold yellow]")
            ret = shell(f"clang ../sysY2022/{test_file}.c -o ./clang_exes/{test_file}")
            if not ret:
                print("[bold red]Fail to build with clang. Skip this file[/bold red].")
                continue

            print("[bold green]Build successfully with SCNUcc and clang[/bold green]")


if __name__ == "__main__":
    test_frontend()
