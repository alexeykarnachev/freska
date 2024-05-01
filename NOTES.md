### Build OpenCV
In this project I use a very restricted subset of OpenCV functionality.
It could be built using these flags:

```bash
cmake \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_opencv_apps=OFF \
    -DWITH_ITT=OFF \
    -DWITH_TIFF=OFF \
    -DWITH_WEBP=OFF \
    -DWITH_PNG=OFF \
    -DWITH_OPENEXR=OFF \
    -DWITH_GTK=OFF \
    -DWITH_QT=OFF \
    -DWITH_IPP=OFF \
    -DBUILD_ZLIB=ON \
    -DBUILD_JPEG=ON \
    -DBUILD_OPENJPEG=ON \
    -DBUILD_LIST=videoio \
    .. && cmake --build .
```

### Build ImGui
```bash
g++ -c \
./*.cpp ./backends/imgui_impl_opengl3.cpp ./backends/imgui_impl_glfw.cpp \
-I. -I../glfw-3.4/include \
&& ar rcs libimgui.a ./*.o \
&& rm ./*.o \
&& mv ./libimgui.a ../../../deps/lib/linux/libimgui.a
```

### Build ImGui-node-editor
```bash
g++ -c ./*.cpp -I. -I../imgui/
ar rcs libimgui-node-editor.a ./*.o
rm ./*.o
mv libimgui-node-editor.a ../../lib/linux/
cp ./*.h ../../include/imgui-node-editor/
```

