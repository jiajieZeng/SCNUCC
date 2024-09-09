# check_mylog.py

import os

def main():
    log_file = './log/compair_log.log'
    if os.path.exists(log_file) and os.path.getsize(log_file) > 0:
        print("Error: {} is not empty.".format(log_file))
        exit(1)
    else:
        print("No errors found in {}.".format(log_file))

if __name__ == "__main__":
    main()
