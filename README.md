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

3. Все .h файлы из данного проекта должны находиться в одной папке с main.cc:
   - vec3.h, ray.h, aabb.h, hitable.h, sphere.h, hitable_list.h,
   - camera.h, material.h, box.h, aarect.h, texture.h, pdf.h,
   - onb.h, perlin.h, cylinder.h

4. Команда для компиляции (Linux/macOS):
   g++ -std=c++11 -O3 -fopenmp -o raytracer main.cc -lraylib -lm -lpthread -ldl

   Для macOS может потребоваться фреймворк:
   g++ -std=c++11 -O3 -fopenmp -o raytracer main.cc -framework raylib -lm -lpthread

   Для Windows с MinGW (в терминале MSYS2):
   g++ -std=c++11 -O3 -fopenmp -o raytracer.exe main.cc -lraylib -lm -lwinmm -lgdi32 -lopengl32 -lws2_32

   Если OpenMP не поддерживается вашим компилятором, удалите флаг -fopenmp и
   #pragma omp parallel for в коде (программа станет однопоточной).

5. Запуск:
   ./raytracer   (или ./raytracer.exe на Windows)

   Откроется окно, отображающее отрендеренное изображение.
   Нажмите ESC для выхода.

ПРИМЕЧАНИЯ:
- Разрешение и количество семплов (nx, ny, ns) можно изменить в main.cc.
  Чем больше ns, тем выше качество, но дольше рендер.
- Время рендера сохранится в файл raytracing_parallel.csv.
- Если изображение слишком тёмное, увеличьте яркость источников света
  (параметры в diffuse_light) или добавьте ещё источники.
