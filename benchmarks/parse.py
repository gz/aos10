#
# Parses the IO Benchmark values and
# computes mean and Std. Deviation
#
from math import sqrt

f = open('uod_625_write.txt')

read = False
read_values = {}
write_values = {}

for line in f:
    if line.find("READ") > -1:
        read = True
        print "READ"
    elif line.find("WRITE") > -1:
        read = False
        print "WRITE"
    elif len(line.split()) == 2:
        rs, bs = line.split()
        request_size = int(rs)
        bandwith = float(bs)
        if read:
            if not read_values.has_key(request_size):
                read_values[request_size] = []
            read_values[request_size].append(bandwith)        
        else:
            if not write_values.has_key(request_size):
                write_values[request_size] = []
            write_values[request_size].append(bandwith)

#print read_values
#print write_values

print "read"
for key, values in read_values.items():
    mean = sum(values) / len(values)
    deviation = sqrt( sum(map(lambda x: (x-mean)**2, values)) / (len(values)-1) )
    print key, mean, deviation

print "write"
for key, values in write_values.items():
    mean = sum(values) / len(values)
    deviation = sqrt( sum(map(lambda x: (x-mean)**2, values)) / (len(values)-1) )
    print key, mean, deviation
