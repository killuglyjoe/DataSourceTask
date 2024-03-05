# DataSourceTask
Тестове завдання

# Вимоги до збирання проекту:
 - cmake 3.28
 - minGW 8.1.0 

в командному рядку ввести

```bash
set PATH=C:\path\to\mingw810_64\bin;C:\path\to\mingw810_64\lib;%PATH%
cmake -S . -B build -G "MinGW Makefiles"
cd build
cmake --build .
DataSouceExample.exe
```
