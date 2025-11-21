# Forti




1. Ubuntu/Debian (и производные: Mint, Pop!_OS и др.)





sudo apt update
sudo apt install build-essential gdb cmake git
# Для GUI:
sudo apt install qt6-base-dev qt6-base-dev-tools libgtk-3-dev libwxgtk3.2-dev
# Для работы с графикой (OpenGL, Vulkan и др.):
sudo apt install libglu1-mesa-dev freeglut3-dev mesa-common-dev
# Для работы с мультимедиа (если нужно):
sudo apt install libsdl2-dev libsfml-dev


2. Fedora/RHEL/CentOS/AlmaLinux/Rocky Linux

sudo dnf groupinstall "Development Tools"
sudo dnf install gcc-c++ gdb cmake git
# Для GUI:
sudo dnf install qt6-devel gtk3-devel wxGTK-devel
# Для работы с графикой:
sudo dnf install mesa-libGL-devel mesa-libGLU-devel freeglut-devel
# Для работы с мультимедиа:
sudo dnf install SDL2-devel SFML-devel


3. Arch Linux/Manjaro

sudo pacman -S base-devel gcc gdb cmake git
# Для GUI:
sudo pacman -S qt6-base gtk3 wxgtk3
# Для работы с графикой:
sudo pacman -S mesa glu freeglut
# Для работы с мультимедиа:
sudo pacman -S sdl2 sfml


4. openSUSE

sudo zypper install -t pattern devel_C_C++
sudo zypper install gcc-c++ gdb cmake git
# Для GUI:
sudo zypper install libqt6-devel gtk3-devel wxWidgets-devel
# Для работы с графикой:
sudo zypper install Mesa-libGL-devel Mesa-libGLU-devel freeglut-devel
# Для работы с мультимедиа:
sudo zypper install libSDL2-devel libSFML-devel


Что устанавливается:

build-essential/devel_C_C++ — компиляторы (gcc, g++), make, автоинструменты.
gdb — отладчик.
cmake — система сборки.
git — для работы с репозиториями.
qt6-base-dev/qt6-devel — Qt6 для GUI.
libgtk-3-dev/gtk3-devel — GTK3 для GUI.
libwxgtk3.2-dev/wxGTK-devel — wxWidgets для GUI.
mesa-libGL-devel/Mesa-libGL-devel — OpenGL для графики.
libsdl2-dev/SDL2-devel — SDL2 для мультимедиа.
libsfml-dev/SFML-devel — SFML для мультимедиа и графики.
