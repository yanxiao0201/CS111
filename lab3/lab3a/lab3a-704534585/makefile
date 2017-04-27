default:lab3a.c
	gcc -o lab3a lab3a.c
test:lab3a disk-image
	./lab3a disk-image
diff:super.csv group.csv bitmap.csv inode.csv directory.csv indirect.csv
	diff super.csv super2.csv |wc
	diff group.csv group2.csv |wc
	diff bitmap.csv bitmap2.csv |wc
	diff inode.csv inode2.csv |wc
	diff directory.csv directory2.csv |wc
	diff indirect.csv indirect2.csv |wc
dist:
	tar -czvf lab3a-704534585.tar.gz lab3a.c README makefile

clean:
	rm lab3a super.csv group.csv bitmap.csv inode.csv directory.csv indirec.csv


