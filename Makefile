.PHONY: build
build:
	mkdir -p bin
	g++ -O2 -g \
		-Wall -Wextra -Wconversion -Wextra-semi -Wold-style-cast -Wnon-virtual-dtor -pedantic -pedantic-errors \
		-fvisibility=hidden -std=c++2a -fwrapv -fstrict-enums \
		-obin/site src/main.cpp src/blog.cpp -pthread -lcmark -lhttp_parser -lfmt -lctemplate static/static.s

.PHONY: clean
clean:
	rm -rf bin
