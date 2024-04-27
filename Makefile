all:
	g++ \
	-std=c++17 \
	-Wall -pedantic \
	-o freska \
	-I./deps/include \
	./src/main.cpp \
	./src/graph.cpp \
	./src/app.cpp \
	-L./deps/lib/linux/ \
	-lraylib -limgui -limgui-node-editor -pthread -ldl



