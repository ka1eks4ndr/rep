simple_vector

Студенческий проект. Упрощенная вариация std::vector, представляет из себя обертку над массивом в динамической памяти. Цель получить опыт работы с объектами в динамической памяти. RAII. Понимание работы стандартных контейнеров.
сам контейнер реализован в файлах simple_vector.h и array_ptr.h. main.cpp не несет какой-либо полезной нагрузки, является набором тестов. 

Сборка simple_vector
Pre-requisites:
C/C++ Compiler
проект использует с++17, не требует ни каких зависимых библиотек. Для сборки достаточно запустить в папке с исходными файлами

g++.exe -g -std=c++17 *.cpp -o simple_vector.exe