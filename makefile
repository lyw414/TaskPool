INC_DIR = -I ./comm
INC_DIR += -I ./taskPool
INC_DIR += -I ./serialTask

SRC_FILE = ./taskPool/TaskPool.cpp

FLAGS = -lpthread

all:
	g++ -O2 -o Test test.cpp ${SRC_FILE} ${INC_DIR} ${FLAGS}
