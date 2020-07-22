
out_csv = open("data.csv","w")
out_csv.write("tamaño,dispositivo,fichero,work_groups,work_items,tiempo_total\n")

for size in [8192,16384]:
    for length in [5]:
        for group in [1,2,4,8,16,32,64,128,256]:
            for device in ["GPU","CPU"]:
                for i in range(1,11):
                    start = open("data/S"+str(i)+"-"+str(size)+"-"+str(length)+"-"+str(group)+"-"+device+".txt","r")
                    end = open("data/E"+str(i)+"-"+str(size)+"-"+str(length)+"-"+str(group)+"-"+device+".txt","r")
                    start = start.readline()
                    if(start):
                        start = float(start[1:])
                        end = float(end.readline()[1:])
                        out_csv.write(str(size)+","+device+","+str(length)+","+str(size/group)+","+str(group)+","+str(round((end-start)/1000000,3))+"\n")
                    else:
                        print("S"+str(i)+"-"+str(size)+"-"+str(length)+"-"+str(group)+"-"+device+".txt")

out_csv.close()
 