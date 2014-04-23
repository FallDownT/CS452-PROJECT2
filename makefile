run: project2.cpp
	g++ project2.cpp InitShader.cpp -std=c++11 -lGL -lGLU -lGLEW -lm -lSDL2 -g

clean:
	rm -f *.out *~
