﻿#include <iostream>
#include <tuple>
#include <stdexcept>
#include <assert.h>
#include <string_view>
#include <optional>
#include <utility> // For std::forward
#include <unordered_map>
#include <functional>
#include <memory>
#include <any>
#include <type_traits> // For std::is_invocable
#include <map>

#include <chrono>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace refl {

	// 这个宏用于创建字段信息
#define REFLECTABLE_PROPERTIES(TypeName, ...)  using CURRENT_TYPE_NAME = TypeName; \
    static constexpr auto properties_() { return std::make_tuple(__VA_ARGS__); }
#define REFLECTABLE_MENBER_FUNCS(TypeName, ...) using CURRENT_TYPE_NAME = TypeName; \
    static constexpr auto member_funcs() { return std::make_tuple(__VA_ARGS__); }

// 这个宏用于创建属性信息，并自动将字段名转换为字符串
#define REFLEC_PROPERTY(Name) refl::Property<decltype(&CURRENT_TYPE_NAME::Name), &CURRENT_TYPE_NAME::Name>(#Name)
#define REFLEC_FUNCTION(Func) refl::Function<decltype(&CURRENT_TYPE_NAME::Func), &CURRENT_TYPE_NAME::Func>(#Func)

// 定义一个属性结构体，存储字段名称和值的指针
	template <typename T, T Value>
	struct Property {
		const char* name;
		constexpr Property(const char* name) : name(name) {}
		constexpr T get_value() const { return Value; }
	};
	template <typename T, T Value>
	struct Function {
		const char* name;
		constexpr Function(const char* name) : name(name) {}
		constexpr T get_func() const { return Value; }
	};

	// 使用 std::any 来处理不同类型的字段值和函数返回值
	template <typename T, typename Tuple, size_t N = 0>
	std::any __get_field_value_impl(T& obj, const char* name, const Tuple& tp) {
		if constexpr (N >= std::tuple_size_v<Tuple>) {
			return std::any();// Not Found!
		}
		else {
			const auto& prop = std::get<N>(tp);
			if (std::string_view(prop.name) == name) {
				return std::any(obj.*(prop.get_value()));
			}
			else {
				return __get_field_value_impl<T, Tuple, N + 1>(obj, name, tp);
			}
		}
	}

	// 使用 std::any 来处理不同类型的字段值和函数返回值
	template <typename T, size_t N = 0>
	std::any get_field_value(T* obj, const char* name) {
		return obj ? __get_field_value_impl(*obj, name, T::properties_()) : std::any();
	}

	// 使用 std::any 来处理不同类型的字段值和函数返回值
	template <typename T, typename Tuple, typename Value, size_t N = 0>
	std::any __assign_field_value_impl(T& obj, const char* name, const Value& value, const Tuple& tp) {
		if constexpr (N >= std::tuple_size_v<Tuple>) {
			return std::any();// Not Found!
		}
		else {
			const auto& prop = std::get<N>(tp);
			if (std::string_view(prop.name) == name) {
				if constexpr (std::is_assignable_v<decltype(obj.*(prop.get_value())), Value>) {
					obj.*(prop.get_value()) = value;
					return std::any(obj.*(prop.get_value()));
				}
				else {
					assert(false);// 无法赋值 类型不匹配!!
					return std::any();
				}
			}
			else {
				return __assign_field_value_impl<T, Tuple, Value, N + 1>(obj, name, value, tp);
			}
		}
	}
	template <typename T, typename Value>
	std::any assign_field_value(T* obj, const char* name, const Value& value) {
		return obj ? __assign_field_value_impl(*obj, name, value, T::properties_()) : std::any();
	}

	// 成员函数调用相关:
	template <bool assert_when_error = true, typename T, typename FuncTuple, size_t N = 0, typename... Args>
	constexpr std::any __invoke_member_func_impl(T& obj, const char* name, const FuncTuple& tp, Args&&... args) {
		if constexpr (N >= std::tuple_size_v<FuncTuple>) {
			assert(!assert_when_error);// 没找到！
			return std::any();// Not Found!
		}
		else {
			const auto& func = std::get<N>(tp);
			if (std::string_view(func.name) == name) {
				if constexpr (std::is_invocable_v<decltype(func.get_func()), T&, Args...>) {
					if constexpr (std::is_void<decltype(std::invoke(func.get_func(), obj, std::forward<Args>(args)...))>::value) {
						// 如果函数返回空，那么兼容这种case
						std::invoke(func.get_func(), obj, std::forward<Args>(args)...);
						return std::any();
					}
					else {
						return std::invoke(func.get_func(), obj, std::forward<Args>(args)...);
					}
				}
				else {
					assert(!assert_when_error);// 调用参数不匹配
					return std::any();
				}
			}
			else {
				return __invoke_member_func_impl<assert_when_error, T, FuncTuple, N + 1>(obj, name, tp, std::forward<Args>(args)...);
			}
		}
	}

	template <typename T, typename... Args>
	constexpr std::any invoke_member_func(T* obj, const char* name, Args&&... args) {
		constexpr auto funcs = T::member_funcs();
		return obj ? __invoke_member_func_impl(obj, name, funcs, std::forward<Args>(args)...) : std::any();
	}

	template <typename T, typename... Args>
	constexpr std::any invoke_member_func_safe(T* obj, const char* name, Args&&... args) {
		constexpr auto funcs = T::member_funcs();
		return obj ? __invoke_member_func_impl<true>(obj, name, funcs, std::forward<Args>(args)...) : std::any();
	}

	template <typename T, typename FuncPtr, typename FuncTuple, size_t N = 0>
	constexpr const char* __get_member_func_name_impl(FuncPtr func_ptr, const FuncTuple& tp) {
		if constexpr (N >= std::tuple_size_v<FuncTuple>) {
			return nullptr; // Not Found!
		}
		else {
			const auto& func = std::get<N>(tp);
			if constexpr (std::is_same< decltype(func.get_func()), FuncPtr >::value) {
				return func.name;
			}
			else {
				return __get_member_func_name_impl<T, FuncPtr, FuncTuple, N + 1>(func_ptr, tp);
			}
		}
	}

	template <typename T, typename FuncPtr>
	constexpr const char* get_member_func_name(FuncPtr func_ptr) {
		constexpr auto funcs = T::member_funcs();
		return __get_member_func_name_impl<T, FuncPtr>(func_ptr, funcs);
	}


	// 定义一个类型特征模板，用于获取属性信息
	template <typename T>
	struct For {
		static_assert(std::is_class_v<T>, "Reflector requires a class type.");

		// 遍历所有字段名称
		template <typename Func>
		static void for_each_propertie_name(Func&& func) {
			constexpr auto props = T::properties_();
			std::apply([&](auto... x) {
				((func(x.name)), ...);
				}, props);
		}

		// 遍历所有字段值
		template <typename Func>
		static void for_each_propertie_value(T* obj, Func&& func) {
			constexpr auto props = T::properties_();
			std::apply([&](auto... x) {
				((func(x.name, obj->*(x.get_value()))), ...);
				}, props);
		}

		// 遍历所有函数名称
		template <typename Func>
		static void for_each_member_func_name(Func&& func) {
			constexpr auto props = T::member_funcs();
			std::apply([&](auto... x) {
				((func(x.name)), ...);
				}, props);
		}
	};

	// ===============================================================

	// 以下是动态反射机制的支持代码：
	namespace dynamic {
		// 反射基类
		class IReflectable : public std::enable_shared_from_this<IReflectable> {
		public:
			virtual ~IReflectable() = default;
			virtual std::string_view get_type_name() const = 0;

			virtual std::any get_field_value_by_name(const char* name) const = 0;

			virtual std::any invoke_member_func_by_name(const char* name) = 0;
			virtual std::any invoke_member_func_by_name(const char* name, std::any param1) = 0;
			virtual std::any invoke_member_func_by_name(const char* name, std::any param1, std::any param2) = 0;
			virtual std::any invoke_member_func_by_name(const char* name, std::any param1, std::any param2, std::any param3) = 0;
			virtual std::any invoke_member_func_by_name(const char* name, std::any param1, std::any param2, std::any param3, std::any param4) = 0;
			// 不能无限增加，会增加虚表大小。最多支持4个参数的调用。
		};

		// 类型注册工具
		class TypeRegistry {
		public:
			using CreatorFunc = std::function<std::shared_ptr<IReflectable>()>;

			static TypeRegistry& instance() {
				static TypeRegistry registry;
				return registry;
			}

			void register_type(const std::string_view type_name, CreatorFunc creator) {
				creators_[type_name] = std::move(creator);
			}

			std::shared_ptr<IReflectable> create(const std::string_view type_name) {
				if (auto it = creators_.find(type_name); it != creators_.end()) {
					return it->second();
				}
				return nullptr;
			}

		private:
			std::unordered_map<std::string_view, CreatorFunc> creators_;
		};

		// 用于注册类型信息的宏
#define DECL_DYNAMIC_REFLECTABLE(TypeName) \
    friend class refl::dynamic::TypeRegistryEntry<TypeName>; \
    static std::string_view static_type_name() { return #TypeName; } \
    virtual std::string_view get_type_name() const override { return static_type_name(); } \
    static std::shared_ptr<::refl::dynamic::IReflectable> create_instance() { return std::make_shared<TypeName>(); } \
    static const bool is_registered; \
    std::any get_field_value_by_name(const char* name) const override { \
        return refl::get_field_value(this, name); \
    } \
    std::any invoke_member_func_by_name(const char* name) override { \
        return refl::invoke_member_func(static_cast<TypeName*>(this), name); \
    }\
	std::any invoke_member_func_by_name(const char* name, std::any param1) override { \
		return refl::invoke_member_func(static_cast<TypeName*>(this), name, param1); \
	}\
	std::any invoke_member_func_by_name(const char* name, std::any param1, std::any param2) override { \
		return refl::invoke_member_func(static_cast<TypeName*>(this), name, param1, param2); \
	}\
	std::any invoke_member_func_by_name(const char* name, std::any param1, std::any param2, std::any param3) override { \
		return refl::invoke_member_func(static_cast<TypeName*>(this), name, param1, param2, param3); \
	}\
	std::any invoke_member_func_by_name(const char* name, std::any param1, std::any param2, std::any param3, std::any param4) override { \
		return refl::invoke_member_func(static_cast<TypeName*>(this), name, param1, param2, param3, param4); \
	}\

	// 用于在静态区域注册类型的辅助类
		template <typename T>
		class TypeRegistryEntry {
		public:
			TypeRegistryEntry() {
				::refl::dynamic::TypeRegistry::instance().register_type(T::static_type_name(), &T::create_instance);
			}
		};

		// 为每个类型定义注册变量，这段宏需要出现在cpp中。
#define REGEDIT_DYNAMIC_REFLECTABLE(TypeName) \
    const bool TypeName::is_registered = [] { \
        static ::refl::dynamic::TypeRegistryEntry<TypeName> entry; \
        return true; \
    }();

	}//namespace dynamic


	//宏用于类中声明信号，并提供一个同名的方法来触发信号。宏参数是函数参数列表。示例：
	/*	void x_value_modified(int param) {
		IMPL_SIGNAL(param);
	}*/
#define REFLEC_IMPL_SIGNAL(...) raw_emit_signal_impl(__func__ , __VA_ARGS__)


	class CObject :
		public refl::dynamic::IReflectable {
	private:
		// 信号与槽的映射，键是信号名称，值是一组槽函数的信息
		using connections_list_type = std::list<std::tuple< std::weak_ptr<IReflectable>, std::string>>;
		using connections_type = std::unordered_map<std::string, connections_list_type>;
		connections_type connections_;

	public:
		template<typename... Args>
		void raw_emit_signal_impl(const char* signal_name, Args&&... args) {
			auto it = connections_.find(signal_name);
			if (it != connections_.end()) {
				auto& slots = it->second; // 获取槽信息列表的引用
				bool has_invalid_slot = false;
				for (const auto& slot_info : slots) {
					auto ptr = std::get<0>(slot_info).lock(); // 锁定弱引用
					if (ptr) {
						ptr->invoke_member_func_by_name(std::get<1>(slot_info).c_str(), std::forward<Args>(args)...);
					}
					else {
						has_invalid_slot = true;
					}
				}
				if (has_invalid_slot) {
					//如果存在无效对象，则执行一轮移除操作
					auto remove_it = std::remove_if(slots.begin(), slots.end(),
						[](const auto& slot_info) {
							return std::get<0>(slot_info).expired(); // 检查弱引用是否失效
						});
					slots.erase(remove_it, slots.end());
				}
			}
			else {/*没找到这个信号，要不要assert？*/ }
		}

		auto connect(const char* signal_name, refl::CObject* slot_instance, const char* slot_member_func_name) {
			if (!slot_instance || !signal_name || !slot_member_func_name) {
				throw std::runtime_error("param is null!");
			}
			assert(slot_instance->weak_from_this().lock());//target必须通过make_share构造！！因为要弱引用它

			std::string str_signal_name(signal_name);
			auto itMap = connections_.find(str_signal_name);
			if (itMap != connections_.end()) {
				itMap->second.emplace_back(slot_instance->weak_from_this(), slot_member_func_name);//必须插入末尾，因为返回了--end()迭代器指示这个链接
				return std::make_optional(std::make_tuple(this, itMap, --itMap->second.end()));
			}
			else {
				// 如果没找到，插入新元素到map中，并获取迭代器
				auto emplace_result = connections_.emplace(std::make_pair(std::move(str_signal_name), connections_list_type()));
				itMap = emplace_result.first;
				itMap->second.emplace_back(slot_instance->weak_from_this(), slot_member_func_name);
				return std::make_optional(std::make_tuple(this, itMap, --itMap->second.end()));
			}
		}
		template <typename SlotClass>
		auto connect(const char* signal_name, std::shared_ptr<SlotClass> slot_instance, const char* slot_member_func_name) {
			return connect(signal_name, slot_instance.get(), slot_member_func_name);
		}

		template <typename SignalClass, typename SignalType, typename SlotClass, typename SlotType>
		auto connect(SignalType SignalClass::* signal, SlotClass* slot_instance, SlotType SlotClass::* slot) {
			const char* signal_name = get_member_func_name<SignalClass>(signal);
			const char* slot_name = get_member_func_name<SlotClass>(slot);
			if (signal_name && slot_name) {
				return connect(signal_name, static_cast<CObject*>(slot_instance), slot_name);
			}
			throw std::runtime_error("signal name or slot_name is not found!");
		}
		template <typename SignalClass, typename SignalType, typename SlotClass, typename SlotType>
		auto connect(SignalType SignalClass::* signal, std::shared_ptr<SlotClass>& slot_instance, SlotType SlotClass::* slot) {
			return connect(signal, slot_instance.get(), slot);
		}

		template <typename T>
		bool disconnect(T connection) {
			//T是个这个类型：std::make_optional(std::make_tuple(this, itMap, it)); 由于T过于复杂，就直接用模板算了
			if (!connection) {
				return false;
			}
			auto& tuple = connection.value();
			if (std::get<0>(tuple) != this) {
				return false;//不是我的connection呀
			}
			std::get<1>(tuple)->second.erase(std::get<2>(tuple));
			return true;
		}

	};

	class QObject : public CObject {
	private:
		std::string objectName_;
		std::weak_ptr<refl::dynamic::IReflectable> parent_;
		std::unordered_map<std::string, std::any> properties_;
		std::list<std::shared_ptr<refl::dynamic::IReflectable>> children_;
	public:
		void setObjectName(const char* name) {
			objectName_ = name;
		}
		const std::string& getObjectName() {
			return objectName_;
		}

		void setParent(QObject* newParent) {
			if (auto oldParent = dynamic_cast<QObject*>(parent_.lock().get())) {
				auto it = std::find_if(oldParent->children_.begin(), oldParent->children_.end(),
					[this](const auto& child) { return child.get() == this; });
				if (it != oldParent->children_.end()) {
					oldParent->children_.erase(it);
				}
			}
			if (newParent) {
				parent_ = newParent->weak_from_this();
				newParent->children_.push_back(shared_from_this());
			}
			else {
				parent_.reset();
			}
		}
		template <typename T>
		void setParent(std::shared_ptr<T> newParent) {
			setParent(newParent.get());
		}

		void removeChild(CObject* child) {
			auto ch = static_cast<refl::dynamic::IReflectable*>(child);
			auto it = std::find_if(children_.begin(), children_.end(),
				[this, ch](const auto& child) { return child.get() == ch; });
			if (it != children_.end()) {
				children_.erase(it);
			}
		}
		CObject* findChild(const char* name) {
			for (auto child : children_) {
				QObject* qChild = dynamic_cast<QObject*>(child.get());
				if (qChild && qChild->objectName_ == name) {
					return qChild;
				}
			}
			return nullptr;
		}
		CObject* findChildRecursively(const char* name) {
			for (auto child : children_) {
				QObject* qChild = dynamic_cast<QObject*>(child.get());
				if (qChild) {
					if (qChild->objectName_ == name) {
						return qChild;
					}
					CObject* found = qChild->findChildRecursively(name);
					if (found) {
						return found;
					}
				}
			}
			return nullptr;
		}

		const std::any& getProperty(const char* name) {
			return properties_[name];
		}
		template<typename T>
		const T* getProperty(const char* name) {
			try {
				return &std::any_cast<const T&>(properties_[name]);
			}
			catch (...) {
				return nullptr;
			}
		}
		void setProperty(const char* name, const std::any& value) {
			properties_[name] = value;
		}

	};




	class CEventLoop {
	public:
		using Clock = std::chrono::steady_clock;
		using TimePoint = Clock::time_point;
		using Duration = Clock::duration;
		using Handler = std::function<void()>;
		struct TimedHandler {
			TimePoint time;
			Handler handler;
			bool operator<(const TimedHandler& other) const {
				return time > other.time;
			}
		};
		class IEventLoopHost {
		public:
			virtual void onWaitForTask(std::condition_variable& cond, std::unique_lock<std::mutex>& locker) = 0;
			virtual void onEvent(TimedHandler& event) = 0;
			virtual void onWaitForRun(std::condition_variable& cond, std::unique_lock<std::mutex>& locker, const TimePoint& timePoint) = 0;
		};
	private:
		IEventLoopHost* host = nullptr;
		std::priority_queue<TimedHandler> tasks_;
		std::mutex mutex_;
		std::condition_variable cond_;
		std::atomic<bool> running_{ true };

	public:
		void setHost(IEventLoopHost* host) {
			this->host = host;
		}
		void post(Handler handler, Duration delay = Duration::zero()) {
			std::unique_lock<std::mutex> lock(mutex_);
			tasks_.push({ Clock::now() + delay, std::move(handler) });
			cond_.notify_one();
		}

		void run() {
			while (running_) {
				std::unique_lock<std::mutex> lock(mutex_);
				if (tasks_.empty()) {
					if (host) {
						host->onWaitForTask(cond_, lock);
					}
					else {
						cond_.wait(lock, [this] { return !tasks_.empty() || !running_; });
					}
				}

				while (!tasks_.empty() && tasks_.top().time <= Clock::now()) {
					auto task = tasks_.top();
					tasks_.pop();
					lock.unlock();
					if (host) {
						host->onEvent(task);
					}
					else {
						task.handler();
					}

					lock.lock();
				}

				if (!tasks_.empty()) {
					if (host) {
						host->onWaitForRun(cond_, lock, tasks_.top().time);
					}
					else {
						cond_.wait_until(lock, tasks_.top().time);
					}
				}
			}
		}

		void stop() {
			running_ = false;
			cond_.notify_all();
		}

	};


}// namespace refl


// =========================一下为使用示例代码====================================

// 用户自定义的结构体
class MyStruct :
	//public refl::dynamic::IReflectable 	// 如果不需要动态反射，可以不从public refl::dynamic::IReflectable派生
	public refl::QObject // 这里我们也测试信号槽等功能，因此从这个类派生
{

public:
	~MyStruct() {
		std::cout << getObjectName() << " destoryed " << std::endl;
	}
	int x{ 10 };
	double y{ 20.5f };
	int print() const {
		std::cout << "MyStruct::print called! " << "x: " << x << ", y: " << y << std::endl;
		return 666;
	}
	// 如果需要支持动态调用，参数必须是std::any，并且不能超过4个参数。
	int print_with_arg(std::any param) const {
		std::cout << "MyStruct::print called! " << " arg is: " << std::any_cast<int>(param) << std::endl;
		return 888;
	}
	// 定义一个方法，用作槽函数，必须在REFLECTABLE_MENBER_FUNCS列表中，不支持返回值，并且参数必须是std::any，不能超过4个参数。
	std::any on_x_value_modified(std::any& new_value) {
		int value = std::any_cast<int>(new_value);
		std::cout << "MyStruct::on_x_value_modified called! New value is: " << value << std::endl;
		return 0;
	}

	void x_value_modified(std::any param) {
		REFLEC_IMPL_SIGNAL(param);
	}

	REFLECTABLE_PROPERTIES(MyStruct,
		REFLEC_PROPERTY(x),
		REFLEC_PROPERTY(y)
	);
	REFLECTABLE_MENBER_FUNCS(MyStruct,
		REFLEC_FUNCTION(print),
		REFLEC_FUNCTION(print_with_arg),
		REFLEC_FUNCTION(on_x_value_modified),
		REFLEC_FUNCTION(x_value_modified)
	);

	DECL_DYNAMIC_REFLECTABLE(MyStruct)//动态反射的支持，如果不需要动态反射，可以去掉这行代码
};

//动态反射注册类，注册创建工厂
REGEDIT_DYNAMIC_REFLECTABLE(MyStruct)


int main() {

	// 静态反射部分：
	std::cout << "---------------------静态反射部分：" << std::endl;

	auto obj = std::make_shared<MyStruct>();
	// 打印所有字段名称
	refl::For<MyStruct>::for_each_propertie_name([](const char* name) {
		std::cout << "Field name: " << name << std::endl;
		});

	// 打印所有字段值
	refl::For<MyStruct>::for_each_propertie_value(obj.get(), [](const char* name, auto&& value) {
		std::cout << "Field " << name << " has value: " << value << std::endl;
		});

	// 打印所有函数名称
	refl::For<MyStruct>::for_each_member_func_name([](const char* name) {
		std::cout << "Member func name: " << name << std::endl;
		});

	// 获取特定成员的值，如果找不到成员，则返回默认值
	auto x_value = refl::get_field_value(obj.get(), "x");
	std::cout << "Field x has value: " << std::any_cast<int>(x_value) << std::endl;

	auto y_value = refl::get_field_value(obj.get(), "y");
	std::cout << "Field y has value: " << std::any_cast<double>(y_value) << std::endl;

	//修改值：
	refl::assign_field_value(obj.get(), "y", 33.33f);
	y_value = refl::get_field_value(obj.get(), "y");
	std::cout << "Field y has modifyed,new value is: " << std::any_cast<double>(y_value) << std::endl;

	auto z_value = refl::get_field_value(obj.get(), "z"); // "z" 不存在
	if (z_value.type().name() == std::string_view("int")) {
		std::cout << "Field z has value: " << std::any_cast<int>(z_value) << std::endl;
	}

	// 通过字符串调用成员函数 'print'
	auto print_ret = refl::invoke_member_func_safe(obj.get(), "print");
	std::cout << "print member return: " << std::any_cast<int>(print_ret) << std::endl;


	std::cout << "---------------------动态反射部分：" << std::endl;

	// 动态反射部分(动态反射完全不需要知道类型MyStruct的定义)：
	// 动态创建 MyStruct 实例并调用方法
	auto instance = refl::dynamic::TypeRegistry::instance().create("MyStruct");
	if (instance) {
		std::cout << "Dynamic instance type: " << instance->get_type_name() << std::endl;
		// 这里可以调用 MyStruct 的成员方法
		auto x_value2 = instance->get_field_value_by_name("x");
		std::cout << "Field x has value: " << std::any_cast<int>(x_value2) << std::endl;

		instance->invoke_member_func_by_name("print");
		instance->invoke_member_func_by_name("print_with_arg", 10);
		//instance->invoke_member_func_by_name("print_with_arg", 20, 222);//这个调用会失败，命中断言，因为print_with_arg只接受一个函数
	}

	// 信号槽部分：
	std::cout << "---------------------信号槽部分：" << std::endl;

	auto obj1 = std::make_shared<MyStruct>();
	obj1->setObjectName("obj1");
	auto obj2 = std::make_shared<MyStruct>();
	obj2->setObjectName("obj2");
	obj2->setParent(obj1);

	// 连接obj1的信号到obj2的槽函数
	auto connection_id = obj1->connect("x_value_modified", obj2.get(), "on_x_value_modified");
	if (!connection_id) {
		std::cout << "Signal x_value_modified from obj1 connected to on_x_value_modified slot in obj2." << std::endl;
	}

	obj1->x_value_modified(42);// 触发信号
	// 断开连接
	obj1->disconnect(connection_id);
	// 再次触发信号，应该没有任何输出，因为已经断开连接
	obj1->x_value_modified(84);

	// 使用成员函数指针版本的connect
	connection_id = obj1->connect(&MyStruct::x_value_modified, obj2, &MyStruct::on_x_value_modified);
	if (!connection_id) {
		std::cout << "Signal connected to slot." << std::endl;
	}
	obj1->x_value_modified(666);// 触发信号

	obj2.reset();
	obj1.reset();

	
	// 事件循环部分：
	std::cout << "---------------------事件循环部分：" << std::endl;
	refl::CEventLoop loop;
	loop.post([]() {
		std::cout << "Immediate task\n";
		});//马上执行

	loop.post([]() {
		std::cout << "Delayed task\n";
		}, std::chrono::seconds(1));//延时一秒

	std::thread loop_thread([&loop]() {
		loop.run();
		});

	std::this_thread::sleep_for(std::chrono::seconds(2));
	loop.stop();//停止消息循环
	loop_thread.join();


	std::cout << "====end=====" << std::endl;
	return 0;
}

