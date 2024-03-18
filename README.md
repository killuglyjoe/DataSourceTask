# DataSourceTask
Тестове завдання

# Вимоги до збирання проекту:
 - cmake > 3.6.2 
 - minGW > 7.3.0, MSVC, GCC 

в командному рядку ввести

```bash
set PATH=C:\path\to\mingw810_64\bin;C:\path\to\mingw810_64\x86_64-w64-mingw32\lib;%PATH%
cmake -S . -B build -G "MinGW Makefiles"
cd build
cmake --build .
DataSouceExample.exe
```
