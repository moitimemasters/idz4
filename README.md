### Отчет о выполнении задачи

#### Исполнитель

- Фамилия: Песня
- Имя: Иван
- Группа: БПИ-229
- Выполнено на оценку: 10

#### Номер варианта и условие задачи:

Вариант: 15

Условие задачи:

Создать клиент-серверное приложение, моделирующее состояния цветов на клумбе и действия садовников. Сервер используется для обмена информацией между садовниками и клумбой. Клумба — клиент, отслеживающий состояния всех цветов. Каждый садовник — отдельный клиент.

### Низкоуровневые классы для работы с сокетами

#### Класс `SocketManager`

Этот класс представляет низкоуровневую абстракцию для работы с UDP сокетами. Он инкапсулирует основные операции по созданию, привязке, отправке и приему данных через UDP сокет.

1. Создание и привязка сокета:
   - Метод `initializeSocket()` создает UDP сокет и привязывает его к определенному адресу и порту.

2. Отправка данных:
   - Метод `sendMessage()` отправляет данные на указанный адрес и порт через сокет.

3. Получение данных:
   - Метод `receiveMessage()` принимает данные на определенном адресе и порту.

#### Класс `UDPServer`

Этот класс представляет серверную сторону, которая использует UDP сокет для обработки входящих запросов от клиентов.

1. Инициализация сервера:
   - Конструктор класса инициализирует сокет и связывает его с определенным портом.

2. Цикл обработки сообщений:
   - Метод `run()` начинает основной цикл сервера, который постоянно ожидает входящие сообщения и обрабатывает их.

3. Обработка событий с использованием `epoll`:
   - `epoll` используется для эффективного мониторинга нескольких файловых дескрипторов одновременно.
   - Сервер добавляет свои сокеты в `epoll`, чтобы отслеживать события чтения.

   На текущий момент используется только один файловый дескриптор, поэтому `epoll` используется только для упрощения кода, однако он может быть использован для потенциального увеличения производительности (расширение на несколько сокетов).

#### Класс `UDPClient`

Этот класс представляет клиентскую сторону, которая использует UDP сокет для отправки запросов и получения ответов от сервера.

1. Инициализация клиента:
   - Конструктор класса инициализирует сокет.

2. Отправка запросов и прием ответов:
   - Метод `sendRequest()` отправляет запрос на сервер.
   - Метод `waitForResponse()` блокирует выполнение и ожидает ответа от сервера.

### Сегментация сообщений

#### Разделение сообщения на сегменты

Сообщения, превышающие максимальный размер UDP пакета, разбиваются на сегменты:

1. Определение количества сегментов:
   - Сообщение делится на несколько частей, каждая из которых имеет предопределенный максимальный размер.

2. Формирование сегментов:
   - Каждый сегмент снабжен метаданными, включая номер сегмента, общее количество сегментов и контрольную сумму для проверки целостности данных.

#### Сборка сегментов

Когда клиент или сервер получает сегменты, они собираются в исходное сообщение:

1. Хранение сегментов:
   - Сегменты хранятся во временной структуре, пока не будут получены все части сообщения.

2. Проверка целостности:
   - Контрольные суммы каждого сегмента проверяются для предотвращения ошибок при сборке.

3. Сборка сообщения:
   - Когда все сегменты получены и проверены, они объединяются в исходное сообщение.

### Подтверждения и обработка сообщений

#### Подтверждения (ACK и NACK)

Для гарантии доставки используются подтверждения:

1. Отправка `ACK`:
   - После успешного получения и проверки сегмента, отправляется подтверждение (ACK).

2. Отправка `NACK`:
   - В случае ошибки контрольной суммы отправляется отрицательное подтверждение (NACK). Клиент или сервер затем повторно отправляет сегмент.

#### Парсинг сообщений

Для интерпретации структуры сообщений используются регулярные выражения:

1. Определение структуры сообщения:
   - Сообщение имеет заранее определенную структуру, например,
   ```
   ID:<client_id>;SEQ:<seq_num>;TOT:<total_segments>;CS:<checksum>;DATA:<payload>.
   ```
    где:
    - ID - идентификатор клиента;
    - SEQ - номер сегмента;
    - TOT - общее количество сегментов;
    - CS - контрольная сумма;
    - DATA - данные.

2. Разбор сообщения с использованием регулярных выражений:
   - MessageParser использует регулярные выражения для разбора сообщений и извлечения полей.

    ```cpp
    std::regex message_regex("ID:(.*?);SEQ:(\\d+);TOT:(\\d+);CS:(\\d+);DATA:(.*)");
    std::smatch match;
    if (std::regex_match(message, match, message_regex)) {
        // получаем значения полей
    }
    ```
### Управление соединениями

#### Класс `ConnectionManager`

Этот класс отвечает за управление состоянием соединений и хранение сегментов сообщений до их полной сборки.

1. Хранение сегментов сообщений:
   - Хранит сегменты в структуре данных, ассоциированной с конкретным клиентом.

2. Проверка полноты сообщения:
   - Проверяет, все ли сегменты сообщения получены.

3. Сборка и проверка целостности:
   - Собирает сегменты в одно сообщение и проверяет его контрольную сумму.

4. Подтверждение получения:
   - После проверки целостности собранного сообщения отправляет подтверждение отправителю.

#### Класс `HandshakeManager`

Этот класс отвечает за выполнение инициализации соединения и обработку рукопожатий.

1. Отправка начального запроса:
   - Клиент отправляет на сервер запрос на инициацию соединения и ожидает ответа.

2. Обработка начального запроса:
   - Сервер принимает запрос и отправляет ответное рукопожатие для подтверждения установления соединения.

3. Подтверждение соединения:
   - Клиент получает ответное рукопожатие и завершает процесс инициализации соединения.

### Простой флоу отправки сообщения и получения ответа

1. Отправка сообщения клиентом:
   - Клиент разбивает сообщение на сегменты и отправляет их на сервер.

2. Получение сегментов сервером:
   - Сервер принимает сегменты, парсит их и хранятся сегменты в ConnectionManager.

3. Подтверждение (`ACK`/`NACK`):
   - Сервер проверяет контрольную сумму каждого сегмента и отправляет ACK или NACK.

4. Сборка сообщения сервером:
   - Когда сервер получает все сегменты сообщения и успешно их проверяет, он собирает их в исходное сообщение и обрабатывает его.

5. Отправка ответа сервером:
   - После успешной обработки сервер отправляет ответ клиенту, который также разбивается на сегменты.

6. Получение сегментов клиентом:
   - Клиент принимает сегменты ответа, парсит их, проверяет целостность и собирает в исходное сообщение.

### Описание работы сервера

#### Архитектура сервера

Сервер наследует функционал от базового класса UDPServer, который инкапсулирует основные операции работы с сокетами, такие как инициализация, отправка и получение данных. В дополнение к этому базовому функционалу, сервер реализует специфическую логику взаимодействия с клиентами-клумбой и клиентами-садовниками, а также управление состоянием системы.

##### Основные компоненты сервера:

1. `Server` - Класс, наследующийся от `UDPServer` и переопределяющий лишь метод `handleMessage`

2. `RouteManager`:
   - Утилитарный класс, который управляет маршрутизацией сообщений, направляя их к соответствующим обработчикам на основе префикса сообщения.

3. `FlowerBedStateManager`:
   - Класс, отвечающий за управление состоянием клумбы и садовников, отслеживание их активности, учет состояний цветов и обработку запросов на полив.

4. `Worker Thread`:
   - Отдельный поток, который периодически проверяет состояние подключений, удаляя неактивных клиентов и проверяя готовность сервера к обработке запросов.

#### Флоу обработки сообщений сервером

1. Получение сообщения:
   - Сервер ожидает новых сообщений от клиентов с помощью механизма epoll.
   - При получении сообщения, оно передается в метод `handleMessage`, который декодирует и обрабатывает его.

2. Маршрутизация сообщения:
   - В `handleMessage` используется RouteManager для определения, какой обработчик должен быть вызван на основе префикса сообщения (например, `/ping/gardener/`, `/getUpdates/`).

3. Обработка различных типов сообщений:

   - Пинг от садовника (`/ping/gardener/`):
     - Регистрирует нового садовника или обновляет таймстамп активности существующего.
     - Отправляет подтверждение (`OK`) клиенту.

   - Пинг от клумбы (`/ping/flowerbed/`):
     - Устанавливает статус подключения клумбы и обновляет таймстамп активности.
     - Отправляет подтверждение (`OK`) клиенту.

   - Запрос обновлений статусов цветов (`/getUpdates/`):
     - Проверяет готовность сервера к обработке запросов.
     - Отправляет клиенту список обновленных статусов цветов.

   - Запрос на полив (`/getFlower/`):
     - Проверяет готовность сервера к обработке запросов.
     - Отправляет клиенту идентификатор цветка, который нужно полить.

   - Уведомление о поливе (`/water/`):
     - Проверяет готовность сервера к обработке запросов.
     - Обновляет состояние цветка как политого и отправляет подтверждение (`OK`) клиенту.

   - Новые цветы для полива от клумбы (`/toWater/`):
     - Проверяет готовность сервера к обработке запросов.
     - Обновляет список цветов, которые нуждаются в поливе, и отправляет подтверждение (`OK`) клиенту.

4. Проверка состояния подключений:
   - В отдельном рабочем потоке сервер регулярно проверяет активность клиентов, используя `FlowerBedStateManager`.
   - Удаляет неактивных клиентов, если они не отправляли пинг-сообщения в течение заданного времени.

### Форматы ответов на различные сообщения

- Пинг от садовника:
  - `OK` — успешно зарегистрирован или обновлен.
  - `ERR` — ошибка, садовник уже подключен.

- Пинг от клумбы:
  -`OK` — успешно зарегистрирована или обновлена.
  - `HC` — клумба уже подключена.

- Запрос обновлений статусов цветов:
  - Формат: "flowerIndex:flowerState;" (например, "0:1;1:0;") — список обновлений состояния цветов.
  - `NOT_READY` — сервер не готов к обработке запроса.

- Запрос на полив:
  - Формат: индекс цветка (например, 3) или -1 — если цветов для полива нет.
  - `NOT_READY` — сервер не готов к обработке запроса.

- Уведомление о поливе:
  - `OK` — успешно.
  - `ERR` — ошибка, неверный индекс цветка.
  - `NOT_READY` — сервер не готов к обработке запроса.

- Новые цветы для полива от клумбы:
  - `OK` — успешно.
  - `NOT_READY` — сервер не готов к обработке запроса.

#### Мониторинг подключений и отказоустойчивость

1. Мониторинг активности клумбы и садовников:
   - Клумба и садовники регулярно отправляют пинг-сообщения для уведомления сервера о своем статусе.
   - Таймстампы пингов сохраняются в `FlowerBedStateManager`.

2. Удаление неактивных клиентов:
   - Сервер использует таймстампы для определения неактивных клиентов и удаления их из списка активных.

3. Потокобезопасность:
   - Все операции внутри `FlowerBedStateManager` выполняются под мьютексом для обеспечения потокобезопасности.


...### Описание работы клиента-клумбы

#### Класс FlowerBedClient

Этот класс реализует клиента-клумбы, который взаимодействует с сервером для отслеживания состояния цветов и передачи информации о цветах, которые нуждаются в поливе.

##### Основные компоненты клиента:

1. `UDPClient`:
   - Класс, предоставляющий функции для работы с UDP сокетом, включая отправку и прием сообщений от сервера.

2. Атомарный флаг остановки (`stop_flag`):
   - Переменная типа `std::atomic<bool>`, используемая для управления состоянием выполнения клиента (работает/остановлен).

3. Состояния цветов и счетчики полива:
   - Векторы, хранящие состояние каждого из 10 цветов и количество поливов для каждого цветка.

4. Мьютексы и условная переменная:
   - Для обеспечения потокобезопасности используются мьютексы (`mtx`, `socket_mtx`) и условная переменная (`cv`).

5. Потоки (`jthreads`):
   - Вектор, содержащий запущенные потоки (`jthreads`), выполняющие фоновые задачи клиента.

#### Функциональность клиента

1. Первый пинг (`firstPing`):
   - Отправка первого пинг-сообщения серверу для инициализации соединения.
   - Проверка ответа от сервера (`OK` для успешного соединения, `HC` если клумба уже подключена).

   Данный пинг необходим, так как в отличие от садовнико, клиент клумбы может быть один, соответственно сервер не хранит информацию о количестве подключенных клубм, а лишь информацию о том, есть клумба или нет. Поэтому первый клиент, который получит сообщение `OK`, сможет дальше интерпретировать `HC` ответы (ответы о том, что подключение уже существует) не как ошибку, а как успешное продолжение соединения.

2. Фоновый пинг (`pingServer`):
   - Потоковая функция, которая переодически отправляет пинг-сообщения серверу, уведомляя его об активности клиента.
   - Если сервер не отвечает, клиент завершает свою работу (устанавливается флаг `stop_flag`).

3. Обновление состояния цветов (`updateFlowerStates`):
   - Потоковая функция, которая случайным образом выбирает новые цветы для полива и отправляет эту информацию серверу.
   - Генерация нового состояния происходит с использованием случайных чисел.

4. Запрос обновлений от садовников (`start`):
   - Основной цикл, который выполняет периодические запросы к серверу на получение обновлений по состоянию цветов.
   - В случае ошибки или превышения количества попыток, клиент завершает свою работу.
   - В случае неудачной поливки/неполивки цветов (перелитые цветы, завядшие цветы), клиент завершает свою работу, сообщая об этом в стандартный поток ошибки.

5. Обработка ответов сервера (`handleServerResponse`):
   - Метод для обработки сообщений о состоянии цветов, полученных от сервера.
   - Если цветок был полит дважды или не был полит вовремя, клиент завершает свою работу.

6. Остановка клиента (`stop`):
   - Метод устанавливает флаг остановки (`stop_flag`), уведомляет условную переменную (`cv`) и завершает все запущенные потоки.

#### Потокобезопасность

1. Мьютексы (`std::mutex`):
   - Используются для синхронизации доступа к общим ресурсам (состояния цветов, общение с сервером) между потоками.
   - `socket_mtx` используется для последовательного доступа к UDP сокету.
   - `mtx` используется для синхронизации доступа к состояниям цветов.

2. Атомарный флаг (`std::atomic<bool>`):
   - Обеспечивает безопасное завершение выполнения всех потоков без необходимости использования дополнительных синхронизирующих примитивов.

3. Условная переменная (`std::condition_variable`):
   - Используется для уведомления потоков о необходимости завершить выполнение при остановке клиента.

#### Отключения и исключительные ситуации

1. Проверка ответов от сервера:
   - Клиент проверяет ответы от сервера на корректность и формат. В случае получения некорректного ответа или отсутствия ответа, клиент завершает свою работу.

2. Количество попыток подключения (`retryAttempts`):
   - Клиент делает до трех попыток подключиться к серверу или получить обновления. Если все попытки исчерпаны, клиент завершает свою работу и устанавливает флаг `stop_flag`.

3. **Обработка исключений:**
   - В каждом потоке и критических секциях кода используется обработка стандартных исключений для логирования и корректного завершения работы клиента в случае ошибок.

...### Описание работы клиента-садовника

#### Класс `GardenerClient`

Этот класс реализует клиента-садовника, который взаимодействует с сервером для получения информации о цветах, нуждающихся в поливе, и уведомления сервера о выполнении поливов.

##### Основные компоненты клиента:

1. `UDPClient`:
   - Класс, предоставляющий функции для работы с UDP сокетом, включая отправку и прием сообщений от сервера.

2. Атомарный флаг остановки (`stop_flag`):
   - Переменная типа `std::atomic<bool`>, используемая для управления состоянием выполнения клиента (работает/остановлен).

3. Мьютексы и условная переменная:
   - Для обеспечения потокобезопасности используются мьютексы (`mtx`, `socket_mtx`) и условная переменная (`cv`).

4. Потоки (`jthreads`):
   - Вектор, содержащий запущенные потоки (`jthreads`), выполняющие фоновые задачи клиента.

#### Функциональность клиента

1. Запуск клиента (`start`):
   - Основной метод, запускающий выполнение клиента.
   - Создает и запускает потоки для отправки пинг-сообщений серверу и выполнения поливов цветов.
   - В конце метода вызывается `stop()` для завершения всех потоков.

2. Фоновый пинг (`pingServer`):
   - Потоковая функция, которая периодически отправляет пинг-сообщения серверу, уведомляя его об активности клиента.
   - Если сервер не отвечает или отвечает неверным сообщением, клиент завершает свою работу (устанавливается флаг `stop_flag`).

3. Полив цветов (`waterFlowers`):
   - Потоковая функция, которая периодически запрашивает у сервера информацию о цветах, нуждающихся в поливе, затем выполняет полив и уведомляет сервер о выполнении.
   - Если сервер не отвечает или отвечает неверным сообщением, клиент завершает свою работу (устанавливается флаг `stop_flag`).

4. Остановка клиента (`stop`):
   - Метод устанавливает флаг остановки (`stop_flag`), уведомляет условную переменную (`cv`) и завершает все запущенные потоки.

#### Потокобезопасность

1. Мьютексы (`std::mutex`):
   - Используются для синхронизации доступа к общим ресурсам между потоками.
   - `socket_mtx` используется для последовательного доступа к UDP сокету.
   - `mtx` используется для синхронизации доступа к другим критическим разделам кода.

2. Атомарный флаг (`std::atomic<bool>`):
   - Обеспечивает безопасное завершение выполнения всех потоков без необходимости использования дополнительных синхронизирующих примитивов.

3. Условная переменная (`std::condition_variable`):
   - Используется для уведомления потоков о необходимости завершить выполнение при остановке клиента.

#### Стратегия обработки сбоев

1. Проверка ответов от сервера:
   - Клиент проверяет ответы от сервера на корректность и формат. В случае получения некорректного ответа или отсутствия ответа, клиент завершает свою работу.

2. Количество попыток подключения (`retry_attempts`):
   - При запросе цветка на полив клиент делает до трех попыток получить корректный ответ от сервера. Если все попытки исчерпаны, клиент завершает свою работу и устанавливает флаг `stop_flag`.

3. Обработка исключений:
   - В каждом потоке (пинг и полив) используются блоки `try-catch` для обработки стандартных исключений. В случае возникновения исключений клиент корректно завершает свою работу и логирует ошибку.

### Форматы ответов на различные сообщения

- Пинг от садовника (`/ping/gardener/`):
  - `OK` — Successfully registered or pinged.
  - Любой другой ответ ведет к завершению работы клиента.

- Запрос цветка на полив (`/getFlower/`):
  - Формат: индекс цветка (например, 3) или -1, если цветов для полива нет.
  - `NOT_READY` — Сервер не готов к обработке запроса.
  - Любой другой ответ ведет к завершению работы клиента.

- Уведомление о поливе (`/water/`):
  - `OK` — Successfully watered the flower.
  - Любой другой ответ ведет к завершению работы клиента.

...### Запуск, компиляция и тестирование клиентских программ

#### Сборка проекта

Для сборки проекта используется `CMake`. Команды для сборки:

1. Инициализация `CMake`:

```sh
   cmake -S . -B build
```
2. Компиляция проекта:
```sh
   cmake --build build --target all
   ```
Эти команды создают директорию `build` и компилируют все необходимые исполняемые файлы (сервер и клиенты).

#### Запуск проекта

Для автоматического запуска серверной и клиентских программ используется сценарий run.py. Этот сценарий компилирует проект, запускает сервер и клиентов, и сохраняет их логи в соответствующие файлы.

##### Команда для запуска сценария:

```sh
python3 run.py <run_case> --port <port>...где <run_case> — это одна из следующих папок:
```
- 4-5
- 6-7
- 8-9-10

а <port> — порт, на котором будет запущен сервер (по умолчанию 12345).

##### Пример команды:

```sh
python3 run.py 4-5 --port 12345
```

#### Процесс работы сценария

1. **Компиляция проекта**: Сценарий `run.py` использует `CMake` для сборки всех компонентов.
2. **Запуск сервера**: Запускается серверный процесс и сохраняются его выводы и ошибки в файлы `server.out` и `server.err`.
3. **Запуск садовников**: Запускаются два клиента-садовника и сохраняются их выводы и ошибки в файлы `gardener1.out`, `gardener1.err`, `gardener2.out` и `gardener2.err`.
4. **Запуск клиентов-клумбы**: Запускается клиент-клумба и сохраняются его выводы и ошибки в файлы `flowerbed.out` и `flowerbed.err`.
5. **Автоматическое завершение**: Через 40 секунд все процессы завершаются скриптом автоматически.

##### Пример логов вывода

**Server Output (server.out)**:
```
Server is running
Waiting for all clients to be connected...
[INFO] Recieved { /ping/gardener/ } from client_0
[INFO] sending {OK} to client_0
[INFO] Recieved { /ping/gardener/ } from client_1
[INFO] sending {OK} to client_1
...
```
**Gardener 1 Output (gardener1.out)**:
```
Sent INIT_REQUEST
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Requesting flower from server...
[INFO] Got flower number from server: 4
[INFO] Notifying server to water flower number 4
[INFO] Flower 4 was watered successfully.
...
```
**Gardener 2 Output (gardener2.out)**:
```
Sent INIT_REQUEST
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Requesting flower from server...
[INFO] Got flower number from server: 8
[INFO] Notifying server to water flower number 8
[INFO] Flower 8 was watered successfully.
...
```
**Flowerbed Output (flowerbed.out)**:
```
Sent INIT_REQUEST
[INFO] Sending first ping...
[INFO] First ping successful: OK
[INFO] Requesting updates on gardeners actions on the flowerbed...
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Telling server what flowers need to be watered...
[INFO] Server acknowledged new flowers.
...
```
### Значимые примеры ввода-вывода

#### Пример 1: Получение цветов для полива

- **Входное сообщение от gardener1 к серверу**:
  - "/getFlower/"

- **Ответ сервера**:
  - "4"

- **Действие gardener1**:
  - "/water/4"

- **Ответ сервера**:
  - "OK"

gardener1 получил цветок с индексом 4 и успешно его полил, сервер подтвердил действие ответом "OK".

#### Пример 2: Запрос на добавление новых цветов от клиента-клумбы

- **Входное сообщение от flowerbed к серверу**:
  - "/toWater/4;8;9;4;3;0;"

- **Ответ сервера**:
  - "OK"

flowerbed успешно уведомил сервер о новых цветах для полива, и сервер подтвердил действие ответом "OK".

### Ручной запуск
- Для начала необходимо запустить сервер, в данном случае его исполняемый файл хранится по пути `./build/4-5/server`. Параметр запуска сервера - порт, на котором сервер будет слушать запросы.
- После этого необходимо запустить двух клиентов садовников `./build/4-5/gardener 127.0.0.1 12345 # <-- ip, port`
- После этого необходимо запустить клиента садовника flowerbed `./build/4-5/flowerbed 127.0.0.1 12345 # <-- ip, port`
- **Важно** запустить клиенты достаточно быстро, чтобы предыдущие запустившиеся клиенты не успели израсходовать попытки переподключения.
- **При неудачном запуске клиентов**, необходимо подождать пол-минуты (если сервер при этом не отключался), и запустить всех клиентов. Сервер автоматически дожидается подключения всех необходимых клиентов, до запуска самого процесса обработки входящих сообщений.

### Расширение программы на оценку 6-7 баллов

#### Новое требование
В дополнение к программе на более низкую оценку необходимо разработать клиентскую программу, обеспечивающую отображение комплексной информации о выполнении приложения в целом. Этот клиент (монитор) должен адекватно отображать поведение моделируемой системы, позволяя не использовать отдельные виды, предоставляемые клиентами и серверами по отдельности.

### Добавленный клиент-монитор

##### Класс `MonitorClient`

Этот класс реализует клиента-монитора, который периодически запрашивает у сервера комплексную информацию о состоянии системы и отображает её в консоли.

##### Основные компоненты клиента-монитора:

1. **UDPClient:**
   - Класс, предоставляющий функции для работы с UDP-сокетом, включая отправку и прием сообщений от сервера.

2. **Основной метод (start):**
   - В бесконечном цикле отправляет запросы серверу (`/monitor/`) для получения информации о состоянии системы.
   - Обрабатывает и отображает полученные данные в консоли.

##### Значимые изменения в других частях программы:

1. **Сервер:**
   - Добавлен обработчик для пути `/monitor/`, который собирает и отправляет комплексную информацию о состоянии всей системы, включая состояния цветов и активность садовников.

### Пример логов выполнения программ
**Server Output (server.out)**:
```
[INFO] Recieved { /monitor/ } from client_0
[INFO] sending {Gardeners connected: 2
Flowerbed connected: 1
Flowers to water: 3 6
Updates from gardeners (watered flowers):
	9 4 0 2 7
	} to client_0
```
**Monitor Output (monitor.out)**:
```
----------------------------
Gardeners connected: 2
Flowerbed connected: 1
Flowers to water: 0 2 7 3 6
Updates from gardeners (watered flowers):
	9 4

----------------------------
Gardeners connected: 2
Flowerbed connected: 1
Flowers to water: 2 7 3 6
Updates from gardeners (watered flowers):
	9 4 0

----------------------------
Gardeners connected: 2
Flowerbed connected: 1
Flowers to water: 2 7 3 6
Updates from gardeners (watered flowers):
	9 4 0
```

### Запуск программы

**Запуск через run.py не меняется**, необходимо лишь укащать правильную директорию -- `8-9-10`

#### Ручной запуск клиента-монитора
```sh
./build/8-9-10/monitor 127.0.0.1 12345
```

### Расширение программы для оценки 8-10 баллов

#### Актуальное состояние программы

Благодаря изначально заложенной архитектуре классов `UDPServer` и `UDPClient`, наше приложение _"из коробки"_ предоставляет следующие способности:

1. Поддержка множества клиентов-мониторов (оценка 8 баллов):
   - Клиентские программы мониторинга могут быть запущены на нескольких независимых компьютерах без нарушения работы системы.
   - Сервер отслеживает и отображает список подключенных клиентов-мониторов.

2. Гибкость подключения и отключения клиентов (оценка 9 баллов):
   - Клиенты могут быть отключены и повторно подключены без нарушений в работе сервера.
   - Клиенты автоматически пытаются переподключиться при потере соединения.

3. Корректное завершение всех клиентов при завершении работы сервера (оценка 10 баллов):
   - При завершении работы сервера все подключенные клиенты корректно завершают свои процессы.

##### Модификации для отображения подключенных мониторов

Для реализации оценки 8 баллов был добавлен функционал отображения подключенных клиентов-мониторов. Это было реализовано в классе, управляющем состояниями клиентов-клумб и садовников (`FlowerbedStateManager`). Сервер отслеживает, когда в последний раз раз клиент-монитор делал запрос, и если заданный промежуток времени (10 секунд) прошел, то клиент считается отключенным.

### Примеры логов

**Лог клубмы**
```
[INFO] Requesting updates on gardeners actions on the flowerbed...
[WARNING] Server not ready, retrying...
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Requesting updates on gardeners actions on the flowerbed...
[WARNING] Server not ready, retrying...
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Telling server what flowers need to be watered...
[WARNING] Server is not ready, retrying...
[INFO] Pinging server...
[INFO] Ping successful: HC
[INFO] Pinging server...
[INFO] Ping successful: HC
[ERROR] Exceeded maximum retry attempts. Exiting...
```

**Лог сервера**
```
Waiting for all clients to be connected...
[INFO] Recieved { /toWater/2;7;2;4;1;8;4;6; } from client_0
[INFO] sending {NOT_READY} to client_0
[INFO] Recieved { /ping/flowerbed/ } from client_0
[INFO] sending {HC} to client_0
[INFO] Recieved { /ping/flowerbed/ } from client_0
[INFO] sending {HC} to client_0
Waiting for all clients to be connected...
Waiting for all clients to be connected...
Waiting for all clients to be connected...
Waiting for all clients to be connected...
```

**Лог садовника**
```
Sent INIT_REQUEST
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Requesting flower from server...
[WARNING] Server not ready, retrying...
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Requesting flower from server...
[WARNING] Server not ready, retrying...
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Requesting flower from server...
[WARNING] Server not ready, retrying...
[INFO] Pinging server...
[INFO] Ping successful: OK
[INFO] Pinging server...
[INFO] Ping successful: OK
[ERROR] Exceeded maximum retry attempts. Exiting...
```

**Лог одного из мониторов**
```
Updates from gardeners (watered flowers):
        7 1
Connected monitors:
        client_4         last update: 2024-06-18 21:14:38
        client_0         last update: 2024-06-18 21:14:37

----------------------------
Gardeners connected: 2
Flowerbed connected: 1
Flowers to water: 2
Updates from gardeners (watered flowers):
        7 1 6
Connected monitors:
        client_4         last update: 2024-06-18 21:14:39
        client_0         last update: 2024-06-18 21:14:38

----------------------------
Gardeners connected: 2
Flowerbed connected: 1
Flowers to water: 2
Updates from gardeners (watered flowers):
        7 1 6
Connected monitors:
        client_4         last update: 2024-06-18 21:14:40
        client_0         last update: 2024-06-18 21:14:39
```
