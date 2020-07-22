
out_csv = open("data.csv","w")
out_csv.write("tamaño,dispositivo,fichero,tiempo_total\n")

for size in [1024,8192]:
    for length in [5,20]:
        for device in ["GPU","CPU"]:
            for i in range(1,11):
                start = open("data/S"+str(i)+"-"+str(size)+"-"+str(length)+"-"+device+".txt","r")
                end = open("data/E"+str(i)+"-"+str(size)+"-"+str(length)+"-"+device+".txt","r")
                start = start.readline()
                if(start):
                    start = float(start[1:])
                    end = float(end.readline()[1:])
                    out_csv.write(str(size)+","+device+","+str(length)+","+str(round((end-start)/1000000,3))+"\n")
                else:
                    print("S"+str(i)+"-"+str(size)+"-"+str(length)+"-"+device+".txt")

out_csv.close()
 