sudo rm -rf build
mkdir build && cd build
cmake .. \
    -DIMGUI_DIR=/home/tasman/Coding/ImGui/imgui \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1
make
sudo make install # sudo may be needed
cd ..

    # -BOX2D_LIBRARY=/home/tasman/Coding/Box2d/box2d \
    # -BOX2D_INCLUDE_DIR=/home/tasman/Coding/Box2d/box2d