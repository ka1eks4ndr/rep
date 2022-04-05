spreadsheet

spreadsheet - упрощённый аналог существующих решений: лист таблицы Microsoft Excel. В ячейках таблицы могут быть текст или формулы. Формулы, как и в существующих решениях, могут содержать индексы ячеек.

Работа с ANTLR — это специальная программа, которая генерирует код лексического и синтаксического анализаторов, а также код для обхода дерева разбора на С++.


Сборка spreadsheet

Pre-requisites:
C/C++ Compiler
проект использует с++17, ANTLR, CMAKE

Скачать ANTLR можно с https://www.antlr.org/download.html. Собственно нужна утилита ANTLR https://www.antlr.org/download/antlr-4.9.3-complete.jar и библиотека ANTLR 4 C++ Runtime https://www.antlr.org/download/antlr4-cpp-runtime-4.9.3-source.zip 

в папке с исходным кодом необходимо создать папку antlr4_runtime и распаковать в нее содержимое библиотеки ANTLR 4 C++ Runtime. jar файл утилиты ANTLR необходимо сложить в папку с исходным кодом. 

в файле CMakeLists.txt в строке 17, где объявляется переменная "ANTLR_EXECUTABLE", указать необходимую версию antlr: 
    set(ANTLR_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/antlr-4.7.2-complete.jar)


Команды сборки могут немного различаться в зависимости от того, какая у вас операционная система и чем вы собираете программы. Если это Windows, то в качестве консоли используйте консоль MinGW с компилятором GCC.

далее в папке с исходным кодом выполнить комманды
md build
cd build
cmake ../
cmake --build .

в результате сборки в каталоге build появиться исполняемый файл spreadsheet или spreadsheet.exe

