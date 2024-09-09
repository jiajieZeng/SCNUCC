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
    print("[bold green]Building Project[/bold green]")

    # 如果找不到build目录，就报错
    if not os.path.exists("build"):
        print("[bold red]Please run this script in the root directory of the project[/bold red]")
        sys.exit(1)
    os.chdir("build")
    print("[bold yellow]Building SCNUcc[/bold yellow]")
    shell("cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS=\"-gdwarf-4\"")
    shell("make -j 2")
    shell("mkdir our_irs")
    shell("mkdir our_asms")
    shell("mkdir our_exes")
    shell("mkdir clang_exes")
    shell("mkdir our_output")
    shell("mkdir clang_output")
    shell("mkdir log")


if __name__ == "__main__":
    test_frontend()
