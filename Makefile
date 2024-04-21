all:
	g++ \
	-Wall -pedantic \
	-o freska \
	-I./deps/include \
	./src/main.cpp \
	-L./deps/lib/linux/ \
	-lraylib -limgui -limgui-node-editor -pthread -ldl



