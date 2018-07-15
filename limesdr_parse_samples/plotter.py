import random
import matplotlib.pyplot as plt

samples = []

with open('samples.txt', 'r') as f:
    for line in f:
        if line.startswith('--'):
            samples.append(float(line[2:]))

plt.figure(figsize=(15,9))
plt.plot(list(range(len(samples))), samples, linewidth=1.0)
plt.ylabel('Amplitude')
plt.xlabel('Samples')
plt.show()