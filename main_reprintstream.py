import os

FIFO = 'myfifo'
os.mkfifo(FIFO)
while True:
    with open(FIFO) as fifo:
        for line in fifo:
            print(line)