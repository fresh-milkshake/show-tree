
## Что это?

**show-tree** — это кроссплатформенное CLI-приложение на C++, предназначенное для отображения древовидной структуры файлов и папок. 

```
./show-tree .
├─ build/
│   └─ show-tree.exe
├─ src/
│   └─ main.cpp
├─ build.py
└─ README.md
```

## Компиляция

Либо с помощью `g++`

```bash
g++ -std=c++17 -o show-tree src/main.cpp
```

Либо то же самое, но с Python и [pymake](https://github.com/fresh-milkshake/pymake)

```bash
pip install mypymake
python build.py
```

## Опции

- `-d`, `--depth <число>`  
  Задает максимальную глубину рекурсии. Значение `-1` означает отсутствие ограничения (по умолчанию).

- `-f`, `--filter <расширения>`  
  Фильтрация по расширениям файлов. Расширения указываются через запятую без пробелов (например, `cpp,h,txt`).

- `-s`, `--sort <тип>`  
  Выбор типа сортировки:
  - `name` — по имени (по умолчанию)
  - `size` — по размеру
  - `date` — по дате последнего изменения

- `-h`, `--hidden`  
  Показывает скрытые файлы и папки.

## Примеры использования

```bash
./show-tree /path/to/directory
./show-tree /path/to/directory -d 2
./show-tree /path/to/directory -f cpp,h
./show-tree /path/to/directory -s size
./show-tree /path/to/directory -h
./show-tree /path/to/directory -d 3 -f cpp,h,txt -s date -h
```