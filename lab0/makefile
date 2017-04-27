
	
	
default: lab0.c
	gcc lab0.c -o lab0
	
			
check: test1 test2 test3 test4 test5
	echo "SUCCESS"
	
test1:
	./lab0 --input=examples/input.txt --output=examples/output.txt;diff examples/input.txt examples/output.txt ;status=$$PIPESTATUS; test $$status -eq 0

test2:	
	./lab0 --input=examples/null.txt;status=$$PIPESTATUS; test $$status -eq 1

test3:	
	./lab0 --input=examples/input.txt ;status=$$PIPESTATUS; test $$status -eq 2
	
test4:
	./lab0 --segfault --catch;status=$$PIPESTATUS;test $$status -eq 3 

test5:
	./lab0 --catch --input=examples/input.txt --output=examples/output.txt;status=$$PIPESTATUS;test $$status -eq 0
	
	
clean:
	rm  examples/output.txt lab0
	
dist:
	tar -czvf lab0-704534585.tar.gz lab0.c makefile README.txt screenshot
