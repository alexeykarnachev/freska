all:
	g++ \
	-std=c++2a \
	-Wall -pedantic \
	-o freska \
	-I./deps/include \
	./src/main.cpp \
	./src/graph.cpp \
	./src/app.cpp \
	-L./deps/lib/linux/ \
	-lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -limgui -lraylib -limgui -limgui-node-editor -llibopenjp2 -llibjpeg-turbo -lzlib -lpthread -ldl



