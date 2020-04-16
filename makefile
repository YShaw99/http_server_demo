src=$(wildcard ./src/*.cc)
obj=$(patsubst %.cc ,%.o ,%(src))

myserver:
	g++ $(src) -o ./bin/myserver

clean:
	-rm -rf ./bin/*
	