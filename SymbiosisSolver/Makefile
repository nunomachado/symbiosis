CC=g++
CFLAGS=-std=c++11 -c
LDFLAGS=
SOURCES=ConstraintModelGenerator.cpp GraphvizGenerator.cpp JPFParser.cpp KQueryParser.cpp main.cpp Operations.cpp Parameters.cpp Types.cpp Util.cpp Z3Solver.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=symbiosisSolver


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
        $(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
        $(CC) $(CFLAGS) $< -o $@

clean:
        rm *.o $(EXECUTABLE)