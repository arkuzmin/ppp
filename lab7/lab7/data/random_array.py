import sys;
from random import randint

f = open('source','w')
n = int(sys.argv[1])
f.write(str(n))
for i in range(n):
    f.write('\n')
    f.write(str(randint(0,100)))
f.close()