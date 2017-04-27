default:lab3b.py
	python lab3b.py

run:
	python lab3b.py

test:lab3b_check.txt new_lab3b_check.txt
	sort lab3b_check.txt > lab3b_xy.txt
	sort new_lab3b_check.txt > lab3b.txt
	diff lab3b_xy.txt lab3b.txt | wc

clean:
	rm lab3b_check.txt lab3b_xy.txt lab3b.txt

dist:
	tar -czvf lab3b-704534585.tar.gz lab3b.py README makefile
