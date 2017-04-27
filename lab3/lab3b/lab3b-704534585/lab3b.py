import csv

class Inode(object):
    def __init__(self, number, linksNum):
        self.number = number
        self.refList = []
        self.linksNum = linksNum
        self.ptrs = []

class Block(object):
    def __init__(self, number):
        self.number = number
        self.refList = []

class ReadData(object):
    def __init__(self):
        self.inodeNum = 0
        self.blockNum = 0
        self.blocksize = 0
        self.blockpergroup = 0
        self.inodepergroup = 0

        self.InodeBitmaps = []
        self.BlockBitmaps = []

        self.Inodefree = []
        self.Blockfree = []

        self.alloInodes = {}
        self.alloBlocks = {}

        #self.incorret = []
        #self.entryone = []


        self.indirectMap = {}
        self.directoryMap = {}

    def readSuperblock(self, fileName = "super.csv"):
        with open('super.csv', 'r') as csvfile:
            data = csv.reader(csvfile, delimiter=',')
            for row in data:
                self.inodeNum = int(row[1])
                self.blockNum = int(row[2])
                self.blocksize = int(row[3])
                self.blockpergroup = int(row[5])
                self.inodepergroup = int(row[6])

    def readGroup(self):
        with open('group.csv', 'r') as csvfile:
            data = csv.reader(csvfile, delimiter =',')
            for row in data:
                blockbitmap = int(row[5],16)
                self.BlockBitmaps.append(blockbitmap)
                inodebitmap = int(row[4],16)
                self.InodeBitmaps.append(inodebitmap)

    def readBitmap(self):
        with open('bitmap.csv', 'r') as csvfile:
            data = csv.reader(csvfile, delimiter =',')
            for row in data:
                blocknum = int(row[0],16)
                free = int(row[1])

                if blocknum in self.InodeBitmaps:
                    self.Inodefree.append(free)
                elif blocknum in self.BlockBitmaps:
                    self.Blockfree.append(free)

    def readIndirect(self):
        with open('indirect.csv','r') as csvfile:
            data = csv.reader(csvfile, delimiter = ',')
            for row in data:
                containblockNum = int(row[0],16)
                entryNum = int(row[1])
                blockptr = int(row[2],16)
                if containblockNum in self.indirectMap:
                    self.indirectMap[containblockNum] += [(entryNum,blockptr)]
                else:
                    self.indirectMap[containblockNum] = [(entryNum,blockptr)]

    def readDirectory(self):
        with open('directory.csv','r') as csvfile:
            data = csv.reader(csvfile,delimiter = ',')
            for row in data:
                parentinode = int(row[0])
                childinode = int(row[4])
                entrynum = int(row[1])

                if childinode not in self.directoryMap:
                    self.directoryMap[childinode] = [(parentinode, entrynum)]
                else:
                    self.directoryMap[childinode].append((parentinode,entrynum))

                if childinode in self.alloInodes:
                    self.alloInodes[childinode].refList.append((parentinode,entrynum))

                #if entrynum == 0:
                    #if childinode != parentinode:
                        #self.incorret.append((childinode,parentinode,entrynum))


    def readInode(self):
        with open('inode.csv','r') as csvfile:
            data = csv.reader(csvfile,delimiter = ',')
            for row in data:
                inodeNum = int(row[0])
                linksNum = int(row[5])
                newinode = Inode(inodeNum, linksNum)
                self.alloInodes[inodeNum] = newinode

                blockNum = int(row[10])
                for i in range(11, min(12, blockNum) + 11):
                    block = int(row[i],16)
                    self.updateblockref(block,inodeNum,0,i-11)

                if blockNum > 12:
                    indirectblock = int(row[23],16)
                    self.updateblockref(indirectblock,inodeNum,0,12)

                    if indirectblock in self.indirectMap:
                        for i in self.indirectMap[indirectblock]:
                            self.updateblockref(i[1],inodeNum,indirectblock,i[0])

                if blockNum > 268:
                    doubleblock = int(row[24],16)
                    self.updateblockref(doubleblock,inodeNum,0,13)
                    if doubleblock in self.indirectMap:
                        for j in self.indirectMap[doubleblock]:
                            self.updateblockref(j[1],inodeNum,doubleblock,j[0])
                            if j[1] in self.indirectMap:
                                for k in self.indirectMap[j[1]]:
                                    self.updateblockref(k[1],inodeNum,j[1],k[0])

                if blockNum > 65804:
                    triplyblock = int(row[25],16)
                    self.updateblockref(triplyblock,inodeNum,0,14)
                    if triplyblock in self.indirectMap:
                        for i in self.indirectMap[triplyblock]:
                            self.updateblockref(i[1],inodeNum,triplyblock,i[0])
                            if i[1] in self.indirectMap:
                                for j in self.indirectMap[i[1]]:
                                    self.updateblockref(j[1],inodeNum,i[1],j[0])
                                    if j[1] in self.indirectMap:
                                        for k in self.indirectMap[j[1]]:
                                            self.updateblockref(k[1],inodeNum,j[1],k[0])


    def updateblockref(self,block,inodeNum,indirectBlock,entryNum):
        if block not in self.alloBlocks:
            newblock = Block(block)
            self.alloBlocks[block] = newblock
        self.alloBlocks[block].refList.append((inodeNum, indirectBlock, entryNum))

    def write(self):
        output = open("lab3b_check.txt",'w')

        for inode in self.alloInodes:
            # I think there are problems on the notes
            if inode > 10 and self.alloInodes[inode].linksNum == 0:
                freelistnum = self.InodeBitmaps[inode/self.inodepergroup]
                message = "MISSING INODE < " + str(inode) + " > SHOULD BE IN FREE LIST < " + str(freelistnum) + " > "
                output.write(message.strip() + "\n")

            elif self.alloInodes[inode].linksNum != len(self.alloInodes[inode].refList):
                message = "LINKCOUNT < " + str(inode) + " > IS < " + str(self.alloInodes[inode].linksNum) + " > SHOULD BE < " + str(len(self.alloInodes[inode].refList)) + " > "
                output.write(message.strip() + "\n")

        for childinode in self.directoryMap:
            for data in self.directoryMap[childinode]:
                if data[1] == 0: #entry == 0
                    if childinode != data[0]:
                        message = "INCORRECT ENTRY IN < " + str(data[0]) + " > NAME < . > LINK TO < " + str(childinode) + " > SHOULD BE < " + str(data[0]) + " > "
                        output.write(message.strip() + "\n")

                elif data[1] == 1:
                    parentinode = data[0]
                    directorydata = self.directoryMap[parentinode]
                    if childinode !=2:
                        isincorrect = True
                        for result in directorydata:
                            if result[1] >= 2:
                                tmp = result[0]
                                if result[0] == childinode:
                                    isincorrect = False
                        if isincorrect:
                            message = "INCORRECT ENTRY IN < " + str(parentinode) + " > NAME < .. > LINK TO < " + str(childinode) + " > SHOULD BE < " + str(tmp) + " > "
                            output.write(message.strip() + "\n")


        for i in range(11, self.inodeNum + 1):
            if i not in self.alloInodes and i not in self.Inodefree:
                freelistnum = self.InodeBitmaps[i/self.inodepergroup]
                message = "MISSING INODE < " + str(i) + " > SHOULD BE IN FREE LIST < " + str(freelistnum) + " > "
                output.write(message.strip() + "\n")

        for inode in self.directoryMap:
            if inode not in self.alloInodes:
                message = "UNALLOCATED INODE < " + str(inode) + " > REFERENCED BY "
                for msn in sorted(self.directoryMap[inode]):
                    message += "DIRECTORY < " + str(msn[0]) + " > ENTRY < " + str(msn[1]) + " > "
                output.write(message.strip() + "\n")

        for block in self.alloBlocks:

            if block == 0 or block >= self.blockNum:
                for msn in sorted(self.alloBlocks[block].refList):
                    message = "INVALID BLOCK < " + str(block) + " > IN INODE < " + str(msn[0]) + " > "
                    if msn[1] == 0:
                        message += "ENTRY < " + str(msn[2]) + " > "
                    else:
                        message += "INDIRECT BLOCK < " + str(msn[1]) + " > ENTRY < " + str(msn[2]) + " > "
                    output.write(message.strip() + "\n")
            else:
                if len(self.alloBlocks[block].refList) > 1:
                    message = "MULTIPLY REFERENCED BLOCK < " + str(block) + " > BY "
                    for msn in sorted(self.alloBlocks[block].refList):
                        if msn[1] == 0:
                            message += "INODE < " + str(msn[0]) + " > ENTRY < " + str(msn[2]) + " > "
                        else:
                            message += "INODE < " + str(msn[0]) + " > INDIRECT BLOCK < " + str(msn[1]) + " > ENTRY < " + str(msn[2]) + " > "
                    output.write(message.strip() + "\n")

                if block in self.Blockfree:
                    message = "UNALLOCATED BLOCK < " + str(block) + " > REFERENCED BY "
                    for msn in sorted(self.alloBlocks[block].refList):
                        if msn[1] == 0:
                            message += "INODE < " + str(msn[0]) + " > ENTRY < " + str(msn[2]) + " > "
                        else:
                            message += "INODE < " + str(msn[0]) + " > INDIRECT BLOCK < " + str(msn[1]) + " > ENTRY < " + str(msn[2]) + " > "
                    output.write(message.strip() + "\n")

        output.close()



if __name__ == "__main__":
    data = ReadData()
    data.readSuperblock()
    data.readGroup()
    data.readBitmap()
    data.readIndirect()
    data.readInode()
    data.readDirectory()
    data.write()
