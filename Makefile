.PHONY: build
build:
	mkdir -p bin
	g++ -O2 -Wall -Wextra -Werror -obin/site -std=c++2a src/main.cpp -pthread -lcmark -lhttp_parser -lfmt -lctemplate static/static.s

.PHONY: clean
clean:
	rm -rf bin
