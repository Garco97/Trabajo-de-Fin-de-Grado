import numpy as np
with open("prueba/chunk_values.txt") as f:
    lines = f.readlines()
for i, line in enumerate(lines):
    lines[i] = line.replace("\n","");
    lines[i] = float(lines[i])
x = np.array(lines)
y = np.fft.fft(x)

f = open("prueba/true_fft.txt","w")
for i in y:
    f.write(str(i) + "\n")
    print(i)
f.close()