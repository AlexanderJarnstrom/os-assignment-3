GCC=g++
#GCC=g++-11

all: filesystem tests

filesystem: main.o shell.o fs.o disk.o
	$(GCC) -std=c++11 -o filesystem main.o shell.o disk.o fs.o

main.o: main.cpp shell.h disk.h
	$(GCC) -std=c++11 -O2 -c main.cpp

shell.o: shell.cpp shell.h fs.h disk.h
	$(GCC) -std=c++11 -O2 -c shell.cpp

fs.o: fs.cpp fs.h disk.h
	$(GCC) -std=c++11 -O2 -c fs.cpp

disk.o: disk.cpp disk.h
	$(GCC) -std=c++11 -O2 -c disk.cpp

test_script1.o: test_script1.cpp test_script.h fs.h disk.h
	$(GCC) -std=c++11 -O2 -c test_script1.cpp

test_script2.o: test_script2.cpp test_script.h fs.h disk.h
	$(GCC) -std=c++11 -O2 -c test_script2.cpp

test_script3.o: test_script3.cpp test_script.h fs.h disk.h
	$(GCC) -std=c++11 -O2 -c test_script3.cpp

test_script4.o: test_script4.cpp test_script.h fs.h disk.h
	$(GCC) -std=c++11 -O2 -c test_script4.cpp

test_script5.o: test_script5.cpp test_script.h fs.h disk.h
	$(GCC) -std=c++11 -O2 -c test_script5.cpp

test: main.o test_script.o fs.o disk.o
	$(GCC) -std=c++11 -o test_script main.o test_script.o disk.o fs.o

test1: main.o test_script1.o fs.o disk.o
	$(GCC) -std=c++11 -o test1 main.o test_script1.o disk.o fs.o

test2: main.o test_script2.o fs.o disk.o
	$(GCC) -std=c++11 -o test2 main.o test_script2.o disk.o fs.o

test3: main.o test_script3.o fs.o disk.o
	$(GCC) -std=c++11 -o test3 main.o test_script3.o disk.o fs.o

test4: main.o test_script4.o fs.o disk.o
	$(GCC) -std=c++11 -o test4 main.o test_script4.o disk.o fs.o

test5: main.o test_script5.o fs.o disk.o
	$(GCC) -std=c++11 -o test5 main.o test_script5.o disk.o fs.o

tests: test1 test2 test3 test4 test5

runtests: tests
	./test1; ./test2; ./test3; ./test4; ./test5

clean:
	rm filesystem test1 test2 test3 test4 test5 main.o shell.o fs.o disk.o test_script*.o diskfile.bin
