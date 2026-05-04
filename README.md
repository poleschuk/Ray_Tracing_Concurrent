# Ray_Tracing_Concurrent

Для компиляции и запуска программы необходимо:

1. Компилятор с поддержкой C++11 (или новее) и OpenMP.
   Рекомендуется GCC (g++) версии 5+ или Clang с поддержкой OpenMP.

2. Библиотека "raylib" (версия 3.0 или выше).
   Инструкции по установке:
   - Ubuntu/Debian:  sudo apt-get install libraylib-dev
   - Fedora:         sudo dnf install raylib-devel
   - macOS (Homebrew): brew install raylib
   - Windows: скачайте с https://www.raylib.com/ или используйте vcpkg.
   - Сборка из исходников: git clone https://github.com/raysan5/raylib.git
                           cd raylib/src && make PLATFORM=PLATFORM_DESKTOP

3. Запуск:
   cmake ..
   cmake --build .
   ./raytracer   (или ./raytracer.exe на Windows)

   Откроется окно, отображающее отрендеренное изображение.
   Нажмите ESC для выхода.

ПРИМЕЧАНИЯ:
- Разрешение и количество семплов (nx, ny, ns) можно изменить в main.cc.
  Чем больше ns, тем выше качество, но дольше рендер.
- Время рендера сохранится в файл raytracing_parallel.csv.
- Если изображение слишком тёмное, увеличьте яркость источников света
  (параметры в diffuse_light) или добавьте ещё источники.
