# Контракт исполняемого файла Нерпы

## Общий формат (beta)

Тело исполняемого файла представляет собой набор секций, каждая секция начинается с `*`, строки в каждой секции разделены символом перевода строки.

1. `*config` - [параметры конфигурации](#параметры-конфигурации). `<ID_ПАРАМЕТРА>:<ЗНАЧЕНИЕ>`
2. `*variables` - список переменных. `<ID_ПЕРЕМЕННОЙ>:<type>`([enum](#type---типы-значений))
3. `*strings` - список значений типа строка. `<ID_СТРОКИ>:"<СТРОКА>"`
4. `*code` - [вызовы инструкций](#список-инструкций-beta). `<НОМЕР_СТРОКИ>:<ИНСТРУКЦИЯ> <АРГУМЕНТ1> <АРГУМЕНТ2> ...`

## Список инструкций (beta)

`data_stack` слева направо, вершина справа. Аргументы снимаются с вершины, результат кладётся обратно.

### Стек

- `CONST type value` - на `data_stack` `value` типа `type`([enum](#type---типы-значений))
- `POP` - снять вершину с `data_stack`
- `DUP` - на `data_stack` копию вершины

### Арифметика

- `MATH operation_id` - операция `operation_id`([enum](#operation_id---id-математической-операции)), снять и положить на `data_stack` по ней

### Сравнение

- `EQ` - `a == b` [*]
- `LT` - `a < b`
- `GT` - `a > b`

### Поток исполнения

- `JMP line` - перейти к `line`
- `JMPF line` - если false, перейти к `line`
- `CALL line` - на `call_stack` место вызова, перейти к `line`
- `RET` - с `call_stack` место вызова, вернуться к нему

### Переменные

- `GET variable_id` - на `data_stack` значение переменной `variable_id`
- `SET variable_id` - установить переменной `variable_id` значение с `data_stack`

### Ввод/вывод

- `HOST function_id` - функция `function_id`([enum](#function_id---id-функции-хоста-beta)), снять и положить на `data_stack` по ней

## Параметры конфигурации

Тип значения из `type`([enum](#type---типы-значений)).

0. Номер строки функции `setup` (`number`, default: 0)
1. Номер строки функции `loop` (`number`, required)
2. Является ли Лёня пидорасом (`bool`, default: true)
3. Используется ли I2C (`bool`, default: false)

## Enum-ы

### `type` - Типы значений

0. `number` - число
1. `bool` - логическое, в `value` 0 - false, 1 - true
2. `string` - строка, в `value` id строки в `*strings`
3. `null` - пустое значение, в `value` 0

### `function_id` - Id функции хоста (beta)

0. `debug_print(str: string)[]` - строка в консоль
1. `pin_mode(pin_num: number, pin_mode: number)[]` - номер пина, режим (`pin_mode`(enum))
2. `digital_write(pin_num: number, state: bool)[]` - bool в состояние пина
3. `digital_read(pin_num: number)[state: bool]` - состояние пина -> bool
4. `analog_write(pin_num: number, state: number)[]` - number 0-1 в состояние пина
5. `analog_read(pin_num: number)[state: number]` - состояние пина -> number 0-1
6. `time_delay(timeout: number)[]` - заблокировать программу на `timeout` милисекунд
7. `time_millis()[num: number]` - вывести сколько милисекунд прошла с момента начала работы программы
8. `i2c_begin_transmission(address: number)[]`
8. `i2c_write(str: string)[]`
9. `i2c_end_transmission()[]`
10. `i2c_request(address: number, quantity: number)[]`
11. `i2c_read_available()[result: string]` - прочитать все доступные байты и записать их в `result`

### `operation_id` - Id математической операции

0. `number_plus` - `a + b` [*]
1. `number_minus` - `a - b`
2. `number_mult` - `a * b`
3. `number_div` - `a / b`
4. `string_plus` - `a` с `b` на конце
5. `bool_and` - оба true
6. `bool_or` - хотя бы один true
7. `bool_not` - не `a`
8. `number_to_string` - число с `data_stack`, в строку, на `data_stack`

[*]: `a` - глубже (левое, первое), `b` - вершина (правое, второе).

## Стеки

1. `data_stack` - стек данных
2. `call_stack` - стек вызовов функций (тип элемента: число, то же которым описаны номера строк)

## Важное

1. Секции идут в порядке, описанном в контракте.
2. Функции, вызываемые через `CALL`, всегда возвращают одно значение. Если функции нечего вернуть, она возвращает `null`([enum](#type---типы-значений)).
3. Формулировки "на `стек`" / "с `стек`" означают положить на стек и снять со стека, если не указано иного.
