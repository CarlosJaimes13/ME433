import matplotlib.pyplot as plt
import numpy as np
from python_csv import read_csv_file


#
def mov_avg_filter(data, X):
    filtered = []
    for i in range(X, len(data)):
        average = sum(data[i-X:i]) / X
        filtered.append(average)
    return filtered

def iir_filter(data, A, B):
    filtered = []
    filtered.append(data[0])

    for i in range(1, len(data)):
        new_value = A * filtered[i-1] + B*data[i]
        filtered.append(new_value)
    return filtered

def fir_lowpass(fc, Fs, N):
    #fc = cutoff freq
    #Fs = sample rate
    # N = number of taps
    h = []
    M = N-1
    for n in range(N):
        if n == M//2:
            value = 2*fc/Fs
        else: 
            value = np.sin(2*np.pi*fc*(n-M//2)/Fs)/ (np.pi * (n-M/2))
        h.append(value)
    return np.array(h)

def apply_hamming(h):
    N = len(h)
    w = np.hamming(N)
    return h*w

def apply_fir(data,h):
    filtered = []
    for i in range(len(data)):
        acc = 0
        for k in range(len(h)):
            if i-k >= 0:
                acc += h[k] * data[i-k]
        filtered.append(acc)
    return filtered

#Made Code into Function 
def plot_signal_fft(filename):
    t, y = read_csv_file(filename)
    X = 10 
    #filtered_y = mov_avg_filter(y,X)
    #filtered_t = t[X:]

    Fs = len(t) / (t[-1] - t[0])
    Ts = 1.0/Fs #Sampling interval

    fc = 5
    N = 51

    h= fir_lowpass(fc,Fs,N)
    h = apply_hamming(h)
    h = h / np.sum(h)

    n = len(y)
    k = np.arange(n)
    T = n/Fs
    frq = k/T
    frq = frq[range(int(n/2))]


    Y = np.fft.fft(y)/n
    Y = Y[range(int(n/2))]

    A = 0.95
    B = 0.05
    filtered_y = apply_fir(y,h)
    filtered_t = t

    fig, (ax1, ax2) = plt.subplots(2, 1)
    ax1.plot(t,y, 'k', label = 'Unfiltered')
    ax1.plot(filtered_t, filtered_y, 'r', label = 'FIR Filtered')
    ax1.set_xlabel('Time')
    ax1.set_ylabel('Amplitude')
    ax1.set_title(filename + " FIR (fc =" + str(fc) + ") Hz, N =" + str(N) + ", Hamming)")
    ax1.legend()


    #Second half
    n2 = len(filtered_y)
    k2 = np.arange(n2)
    T2 = n2 / Fs
    frq2 = k2/T2
    frq2 = frq2[:n2//2]
    Y2 = np.fft.fft(filtered_y)/n2
    Y2 = Y2[:n2//2]

   # ax2.loglog(frq,abs(Y)) # plotting the fft
    ax2.plot(frq, abs(Y), 'k', label = "Unfiltered FIR")
    ax2.plot(frq2, abs(Y2), 'r', label = "FIR FFT")
    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')
    ax2.set_title("FIR")
    ax2.legend()
    plt.show()

file_names = ["sigA.csv","sigB.csv","sigC.csv","sigD.csv" ]
for file in file_names:
    plot_signal_fft(file)