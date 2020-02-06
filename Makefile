.PHONY: build
build:
	mkdir -p bin
	g++ -O2 -s -Wall -Wextra -Werror -obin/site -std=c++2a src/main.cpp -ljinja2cpp -lpthread -lboost_filesystem -lcmark static/static.s

.PHONY: clean
clean:
	rm -rf bin
