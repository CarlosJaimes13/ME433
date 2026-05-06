import csv
from python_fft import plot_signal_fft

##for i in range(len(t)):
    # print the data to verify it was read
   # print(str(t[i]) + ", " + str(data1[i]) )


#Making into function: 
def read_csv_file(filename):
    t = []
    data = []

    with open(filename) as f:
           # open the csv file
        reader = csv.reader(f)
        for row in reader:
        # read the rows 1 one by one
            t.append(float(row[0])) # leftmost column
            data.append(float(row[1])) # second column
    return t, data

file_names = ["sigA.csv","sigB.csv","sigC.csv","sigD.csv" ]

for file in file_names: 
    t, data = read_csv_file("sigA.csv")
    data_points = len(t)
    total_time = t[-1] - t[0] #Find time difference 
    Fs =data_points/total_time 
    print(file, "Sample rate:", Fs, "Hz")
