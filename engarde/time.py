import time
import os
import sys

time1 = time.time()
os.system(' '.join(sys.argv[1:]))
time2 = time.time()
print time2-time1