default:lab4b.c lab4a.c lcd.h lcd.c
	gcc -g -lmraa lcd.c -c -lm
	gcc -g -lmraa lab4a.c -c -lm
	gcc -g -lmraa lab4a.o lcd.o -o lab4a -lm

	gcc -g -pthread -lmraa -o lab4b lab4b.c -lm

clean:
	rm lab4b lab4a lcd.o lab4a.o

dist:
	tar -czvf lab4-704534585.tar.gz lcd.h lcd.c lab4a.c lab4b.c lab4a.log lab4b.log README makefile




