# 使用和设计文档

## 概述

提供的代码实现了一个简单的反射机制，允许在运行时动态查询和操作对象的属性和成员函数。同时，它还包括一个事件循环系统和信号槽机制，用于处理异步事件和对象间通信。

## 核心功能

1. **反射机制**：允许动态获取和设置对象的属性，调用对象的成员函数。
2. **事件循环**：提供了基于时间的调度，允许安排在特定时刻运行的任务。
3. **信号槽机制**：实现了对象间的事件通知和处理，允许连接和断开信号和槽。

## 设计思路

1. **属性和成员函数的元信息声明**：使用宏来声明类的属性和成员函数，以便在编译时自动创建对应的元数据。
2. **属性和函数的封装**：通过 `Property` 和 `Function` 模板结构体封装属性和函数，以便在运行时通过名字访问。
3. **动态类型注册和创建**：使用 `TypeRegistry` 类和相关宏来注册类型和提供一个通过类名创建实例的工厂方法。
4. **信号槽的实现**：通过 `CObject` 类实现信号发射和槽函数的连接，支持跨对象的事件处理。

## 使用方法

### 属性和函数的反射声明

在自定义类中，使用以下宏来声明属性和函数：

```cpp
class MyClass {
public:
    int my_property;
    void my_function(int arg) { /* ... */ }

    // 反射属性和成员函数的声明
    REFLECTABLE_PROPERTIES(MyClass,
        REFLEC_PROPERTY(my_property)
    );
    REFLECTABLE_MENBER_FUNCS(MyClass,
        REFLEC_FUNCTION(my_function)
    );
};
```

### 动态反射支持

如果需要动态反射支持，则在类定义中添加 `DECL_DYNAMIC_REFLECTABLE` 宏，且需要在相应的源文件中使用 `REGEDIT_DYNAMIC_REFLECTABLE` 宏进行类型注册。

```cpp
class MyClass {
    // ...
    DECL_DYNAMIC_REFLECTABLE(MyClass)
};

// 在 .cpp 文件中
REGEDIT_DYNAMIC_REFLECTABLE(MyClass)
```

### 创建和使用对象

创建自定义类的实例，并使用反射机制获取和设置属性，调用成员函数：

```cpp
MyClass obj;
std::any value = refl::get_field_value(&obj, "my_property"); // 获取属性
refl::assign_field_value(&obj, "my_property", 42); // 设置属性
std::any result = refl::invoke_member_func(&obj, "my_function", 123); // 调用函数
```

### 事件循环的使用

创建一个 `CEventLoop` 对象，安排任务并启动事件循环：

```cpp
base::CEventLoop event_loop;
event_loop.post([]{ std::cout << "Immediate task\n"; });
event_loop.run();
```

支持Windows主消息循环的事件循环
```cpp
base::CEventLoop event_loop;
CWindowsEventLoopHost host;
loop.setHost(&host);
event_loop.run();
```

### 信号和槽的连接

创建对象并连接信号和槽：

```cpp
MyClass sender, receiver;
auto connection = sender.connect("signal_name", &receiver, "slot_function");
sender.emit_signal("signal_name", arg1, arg2); // 触发信号
sender.disconnect(connection); // 断开连接
```

## 注意事项

1. 类型安全：由于使用 `std::any`，需要确保类型转换正确，否则可能抛出异常。
2. 异常处理：代码中未详细处理所有潜在异常，应谨慎处理可能的异常情况。
3. 线程安全：信号槽机制未明确考虑线程安全问题，需要在多线程环境下额外注意同步和互斥。

## 性能和限制

1. 运行时性能：反射机制在运行时需要遍历元组，可能导致性能损耗。
2. 编译时性能：大量使用模板和元编程可能增加编译时间和编译器的资源消耗。
3. 功能限制：信号支持固定数量的参数，并且槽函数的参数类型必须为 `std::any`。