# TTaskScheduler

C++23 библиотека для выполнения задач с зависимостями (DAG) в пуле потоков плюс приложение: подсказывает, куда сходить в городе, с учётом погоды.

## Архитектура

Планировщик строит граф задач и сам определяет порядок выполнения.

- **Узел графа** - `TaskImpl<T>` (наследник `ITaskBase`). Хранит:
  - лямбду (`ICallable` / `CallableImpl<F>`)
  - список зависимостей
  - счётчик невыполненных зависимостей
  - результат (`TaskWithResult<T>`)
  - `exception_ptr` для проброса исключений.

- **`TTask`** - лёгкий хэндл задачи. Умеет:
  - `GetFutureResult<T>()` — получить `TFuture<T>`,
  - `GetResultSync<T>()` — выполнить поддерево синхронно и вернуть результат,
  - `Apply(f)` — добавить зависимую задачу.

- **`TFuture<T>`** - обещание результата. `Get()` блокируется на condition_variable, пока задача не выполнится, затем возвращает результат.

- **`TTaskScheduler`**:
  - `Add(f, args...)` - создать задачу. Если среди аргументов есть `TFuture<T>`, он становится зависимостью, а `T` - аргументом лямбды.
  - `ExecuteAll()` - проверить циклы (DFS по цветам), запустить воркеры, раздать готовые задачи, дождаться завершения всех.
  - `ExecuteTaskBranch(target)` - выполняет всё поддерево зависимостей.

- **Потоки**: пул из `hardware_concurrency()` воркеров, общая очередь готовых задач, счётчик завершённых задач и `condition_variable` для ожидания.

- **Готовность**: после выполнения узел уменьшает счётчики у потомков, если зависимостей не осталось - кладем поток в очередь.

Приложение строит такой граф:

```
Coordinates ──┬──> Weather ──┐
              ├──> Places ───┼──> Distances ──> FilterByWeather ──> Print
              └──────────────┘
```

## Файловая архитектура

```
.
├── app/                    # приложение
│   ├── App.hpp             # граф задач и вывод
│   ├── api.hpp             # HTTP + парсинг внешних API
│   ├── types.hpp           # Coordinates, Weather, Place + from_json
│   └── config.hpp          # ключ WeatherAPI, лимиты
│
├── lib/                    # планировщик задач
│   ├── ttask/
│   │   ├── ITaskBase.hpp   # базовый интерфейс узла
│   │   ├── ITaskBase.tpp   # Execute() + проброс готовности потомкам
│   │   ├── TaskWithResult.hpp
│   │   ├── TaskImpl.hpp             # узел с callable и зависимостями
│   │   ├── TTask.hpp/.tpp           # хэндл задачи
│   │   ├── TFuture.hpp              # ожидание результата
│   │   └── TTaskScheduler.hpp/.tpp  # Add, ExecuteAll, воркеры, DFS
│   └── callable/
│       ├── ICallable.hpp
│       └── CallableImpl.hpp
│
├── bin/main.cpp            # точка входа
├── tests/
│   ├── ts_test.cpp         # базовые сценарии планировщика
│   ├── mt_test.cpp         # многопоточность, параллелизм, цепочки
│   └── cl_test.cpp         # циклы, diamond DAG, повторный ExecuteAll
└── CMakeLists.txt
```

## Сборка

Требования:

- компилятор с поддержкой **C++23**
- **CMake** 3.16
- **libcurl**
- **nlohmann_json**
- **GoogleTest** (`FetchContent`)

Сборка:

```bash
cmake -S . -B build
cmake --build build -j
```

Запуск демо:

```bash
./build/TaskSchedulerApp
```

Запуск тестов:

```bash
cd build
ctest --output-on-failure
```

## Используемые API

Демо-приложение ходит в 4 API (все по HTTPS, без авторизации, кроме WeatherAPI):

| API | Назначение | Endpoint |
|-----|------------|----------|
| **2ip.ua** | Геолокация: широта, долгота, город | `https://api.2ip.ua/geo.json` |
| **WeatherAPI** | Текущая погода: температура, условие, ветер, осдаки | `https://api.weatherapi.com/v1/current.json` |
| **Overpass API** | Объекты рядом: музеи, парки, кафе, театры и т.д. радиусе 2 км | `https://overpass-api.de/api/interpreter` |
| **OSRM** | Матрица расстояний пешком между точками | `https://router.project-osrm.org/table/v1/walking/...` |

## Мини-пример

```cpp
#include "TTaskScheduler.hpp"
#include "TTask.hpp"
#include "TFuture.hpp"

TTaskScheduler scheduler;

auto a = scheduler.Add([]{ return 2; });
auto b = scheduler.Add([]{ return 3; });
auto c = scheduler.Add([](int x, int y){ return x + y; },
                       a.GetFutureResult<int>(),
                       b.GetFutureResult<int>());

scheduler.ExecuteAll();
std::cout << c.GetFutureResult<int>().Get() << "\n";  // 5
```
