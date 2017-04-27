default:
	gcc -pthread lab2_add.c -o lab2_add
	gcc -g -pthread lab2_list.c -c
	gcc -g -pthread SortedList.c -c
	gcc -pthread -o lab2_list lab2_list.o SortedList.o

tests:lab2_add lab2_list
	for thread in 1 2 4 8 12;\
	do\
	  ./lab2_add --threads=$$thread --iterations=10000 --sync=m >> lab_2b_list0.csv;\
	  ./lab2_add --threads=$$thread --iterations=10000 --sync=s >> lab_2b_list0.csv;\
	done
	
	for thread in 1 2 4 8 12 16 24;\
	do\
	  ./lab2_list --threads=$$thread --iterations=1000 --sync=m >> lab_2b_list0.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --sync=s >> lab_2b_list0.csv;\
	done
	
	-for thread in 1 4 8 12 16;\
	do \
	./lab2_list --threads=$$thread --iterations=1 --yield=id --lists=4 >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=2 --yield=id --lists=4 >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=4 --yield=id --lists=4 >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=8 --yield=id --lists=4 >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=16 --yield=id --lists=4 >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=10 --yield=id --lists=4 --sync=m >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=20 --yield=id --lists=4 --sync=m >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=40 --yield=id --lists=4 --sync=m >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=80 --yield=id --lists=4 --sync=m >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=10 --yield=id --lists=4 --sync=s >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=20 --yield=id --lists=4 --sync=s >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=40 --yield=id --lists=4 --sync=s >> lab_2b_list.csv;\
	./lab2_list --threads=$$thread --iterations=80 --yield=id --lists=4 --sync=s >> lab_2b_list.csv;\
	done
	for thread in 1 2 4 8 12;\
	do \
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=1 --sync=m >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=4 --sync=m >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=8 --sync=m >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=16 --sync=m >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=1 --sync=s >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=4 --sync=s >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=8 --sync=s >> lab_2b_list.csv;\
	  ./lab2_list --threads=$$thread --iterations=1000 --lists=16 --sync=s >> lab_2b_list.csv;\
	  done


profile:lab2_list makefile
	-rm -f ./raw.gerf
	LD_PRELOAD=/u/eng/class/classxya/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --thread=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=list_spin_lock ./lab2_list ./raw.gperf >> profile.gperf
	rm -f ./raw.gperf

graphs:lab_2b_list0.csv lab_2b_list.csv
	gnuplot lab2b.gp

tarball:
	tar -czvf lab2b-704534585.tar.gz lab2_list.c lab2_add.c SortedList.c SortedList.h makefile lab_2b_list.csv lab_2b_list0.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png README.txt profile.gperf
clean:
	rm lab2_add lab2_list lab2_list.o SortedList.o lab_2b_list.csv lab_2b_list0.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png profile.gperf

