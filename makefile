C_FLAGS 	:= -std=c2x -g -masm=intel -msse3 -mavx -masm=intel
CPP_FLAGS 	:= -std=c++11 -g -masm=intel -fno-threadsafe-statics -fno-exceptions -msse3 -mavx -masm=intel

C_SRC 		:= $(wildcard *.c) $(wildcard */*.c)
ASM_SRC 	:= $(wildcard *.asm)
INC 		:= $(wildcard *.s) $(wildcard *.h) $(wildcard */*.h)

LIB_PATH 	:= $(wildcard libs/*)
LIB_CPP_SRC := $(wildcard libs/*/src/*.cpp) $(wildcard libs/*/src/*/*.cpp)
LIB_C_SRC 	:= $(wildcard libs/*/src/*.c) $(wildcard libs/*/src/*/*.c)
LIB_INC 	:= $(wildcard libs/*/include)

TARGET		:= linux

OBJ := $(C_SRC:.c=.o) $(ASM_SRC:.asm=.o) $(LIB_CPP_SRC:.cpp=.o) $(LIB_C_SRC:.c=.o)

%.o: %.c
	gcc -c $< $(foreach path, $(LIB_PATH), -I$(path)/include) $(C_FLAGS) -o $@

%.o: %.cpp
	gcc -c $< $(foreach path, $(LIB_PATH), -I$(path)/include) $(CPP_FLAGS) -o $@

%.o: %.asm
	as $< -o $@

all: $(OBJ) $(INC)
	gcc $(OBJ) -Wl,--copy-dt-needed-entries -g $(foreach path, $(LIB_PATH),-L$(path)/lib/$(TARGET) -Wl,-rpath=$(path)/lib/$(TARGET)) -lSDL2main -lSDL2 -lGLEW -lstdc++ -o simoslator.out

test:
	echo $(OBJ)

clean:
	rm $(OBJ)