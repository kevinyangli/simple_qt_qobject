#pragma once

#include <iostream>
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

	// ��������ڴ����ֶ���Ϣ
#define REFLECTABLE_PROPERTIES(TypeName, ...)  using CURRENT_TYPE_NAME = TypeName; \
    static constexpr auto properties_() { return std::make_tuple(__VA_ARGS__); }
#define REFLECTABLE_MENBER_FUNCS(TypeName, ...) using CURRENT_TYPE_NAME = TypeName; \
    static constexpr auto member_funcs() { return std::make_tuple(__VA_ARGS__); }

// ��������ڴ���������Ϣ�����Զ����ֶ���ת��Ϊ�ַ���
#define REFLEC_PROPERTY(Name) refl::internal::__Property<decltype(&CURRENT_TYPE_NAME::Name), &CURRENT_TYPE_NAME::Name>(#Name)
#define REFLEC_FUNCTION(Func) refl::internal::__Function<decltype(&CURRENT_TYPE_NAME::Func), &CURRENT_TYPE_NAME::Func>(#Func)

	namespace internal {
		// ����һ�����Խṹ�壬�洢�ֶ����ƺ�ֵ��ָ��
		template <typename T, T Value>
		struct __Property {
			const char* name;
			constexpr __Property(const char* name) : name(name) {}
			constexpr T get_value() const { return Value; }
		};
		template <typename T, T Value>
		struct __Function {
			const char* name;
			constexpr __Function(const char* name) : name(name) {}
			constexpr T get_func() const { return Value; }
		};

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
						assert(false);// �޷���ֵ ���Ͳ�ƥ��!!
						return std::any();
					}
				}
				else {
					return __assign_field_value_impl<T, Tuple, Value, N + 1>(obj, name, value, tp);
				}
			}
		}

		// ��Ա�����������:
		template <bool assert_when_error = true, typename T, typename FuncTuple, size_t N = 0, typename... Args>
		constexpr std::any __invoke_member_func_impl(T& obj, const char* name, const FuncTuple& tp, Args&&... args) {
			if constexpr (N >= std::tuple_size_v<FuncTuple>) {
				assert(!assert_when_error);// û�ҵ���
				return std::any();// Not Found!
			}
			else {
				const auto& func = std::get<N>(tp);
				if (std::string_view(func.name) == name) {
					if constexpr (std::is_invocable_v<decltype(func.get_func()), T&, Args...>) {
						if constexpr (std::is_void<decltype(std::invoke(func.get_func(), obj, std::forward<Args>(args)...))>::value) {
							// ����������ؿգ���ô��������case
							std::invoke(func.get_func(), obj, std::forward<Args>(args)...);
							return std::any();
						}
						else {
							return std::invoke(func.get_func(), obj, std::forward<Args>(args)...);
						}
					}
					else {
						assert(!assert_when_error);// ���ò�����ƥ��
						return std::any();
					}
				}
				else {
					return __invoke_member_func_impl<assert_when_error, T, FuncTuple, N + 1>(obj, name, tp, std::forward<Args>(args)...);
				}
			}
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
	}

	template <typename T, size_t N = 0>
	std::any get_field_value(T* obj, const char* name) {
		return obj ? __get_field_value_impl(*obj, name, T::properties_()) : std::any();
	}
	template <typename T, typename Value>
	std::any assign_field_value(T* obj, const char* name, const Value& value) {
		return obj ? __assign_field_value_impl(*obj, name, value, T::properties_()) : std::any();
	}

	template <typename T, typename... Args>
	constexpr std::any invoke_member_func(T* obj, const char* name, Args&&... args) {
		constexpr auto funcs = T::member_funcs();
		return obj ? __invoke_member_func_impl(obj, name, funcs, std::forward<Args>(args)...) : std::any();
	}

	template <typename T, typename... Args>
	constexpr std::any invoke_member_func_safe(T* obj, const char* name, Args&&... args) {
		constexpr auto funcs = T::member_funcs();
		return obj ? internal::__invoke_member_func_impl<true>(obj, name, funcs, std::forward<Args>(args)...) : std::any();
	}


	template <typename T, typename FuncPtr>
	constexpr const char* get_member_func_name(FuncPtr func_ptr) {
		constexpr auto funcs = T::member_funcs();
		return internal::__get_member_func_name_impl<T, FuncPtr>(func_ptr, funcs);
	}


	// �����������ģ�����ڱ�����ȡ������Ϣ
	template <typename T>
	struct For {
		static_assert(std::is_class_v<T>, "Reflector requires a class type.");

		// ���������ֶ�����
		template <typename Func>
		static void for_each_propertie_name(Func&& func) {
			constexpr auto props = T::properties_();
			std::apply([&](auto... x) {
				((func(x.name)), ...);
				}, props);
		}

		// ���������ֶ�ֵ
		template <typename Func>
		static void for_each_propertie_value(T* obj, Func&& func) {
			constexpr auto props = T::properties_();
			std::apply([&](auto... x) {
				((func(x.name, obj->*(x.get_value()))), ...);
				}, props);
		}

		// �������к�������
		template <typename Func>
		static void for_each_member_func_name(Func&& func) {
			constexpr auto props = T::member_funcs();
			std::apply([&](auto... x) {
				((func(x.name)), ...);
				}, props);
		}
	};

	// ���涼�Ǿ�̬���书�ܣ������Ƕ�̬������Ƶ�֧�ִ��룺
	
	// IReflectable�ṩ��̬���书�ܵ�֧��
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
		// �����������ӣ�����������С��������֧��4�������ĵ��á�
	};

	namespace internal {
		// ����ע�Ṥ��
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

		// ����ע��������Ϣ�ĺ�
#define DECL_DYNAMIC_REFLECTABLE(TypeName) \
    friend class refl::internal::TypeRegistryEntry<TypeName>; \
    static std::string_view static_type_name() { return #TypeName; } \
    virtual std::string_view get_type_name() const override { return static_type_name(); } \
    static std::shared_ptr<::refl::IReflectable> create_instance() { return std::make_shared<TypeName>(); } \
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

	// �����ھ�̬����ע�����͵ĸ�����
		template <typename T>
		class TypeRegistryEntry {
		public:
			TypeRegistryEntry() {
				::refl::internal::TypeRegistry::instance().register_type(T::static_type_name(), &T::create_instance);
			}
		};

		// Ϊÿ�����Ͷ���ע���������κ���Ҫ���������cpp�С�
#define REGEDIT_DYNAMIC_REFLECTABLE(TypeName) \
    const bool TypeName::is_registered = [] { \
        static ::refl::internal::TypeRegistryEntry<TypeName> entry; \
        return true; \
    }();

	}//namespace dynamic


	//���������������źţ����ṩһ��ͬ���ķ����������źš�������Ǻ��������б�ʾ����
	/*	void x_value_modified(int param) {
		IMPL_SIGNAL(param);
	}*/
#define REFLEC_IMPL_SIGNAL(...) raw_emit_signal_impl(__func__ , __VA_ARGS__)

	// CObject��IReflectable�Ļ����ϣ������ṩ�źŲ۹��ܵ�֧��
	class CObject :
		public refl::IReflectable {
	private:
		// �ź���۵�ӳ�䣬�����ź����ƣ�ֵ��һ��ۺ�������Ϣ
		using connections_list_type = std::list<std::tuple< std::weak_ptr<IReflectable>, std::string>>;
		using connections_type = std::unordered_map<std::string, connections_list_type>;
		connections_type connections_;

	public:
		template<typename... Args>
		void raw_emit_signal_impl(const char* signal_name, Args&&... args) {
			auto it = connections_.find(signal_name);
			if (it != connections_.end()) {
				auto& slots = it->second; // ��ȡ����Ϣ�б������
				bool has_invalid_slot = false;
				for (const auto& slot_info : slots) {
					auto ptr = std::get<0>(slot_info).lock(); // ����������
					if (ptr) {
						ptr->invoke_member_func_by_name(std::get<1>(slot_info).c_str(), std::forward<Args>(args)...);
					}
					else {
						has_invalid_slot = true;
					}
				}
				if (has_invalid_slot) {
					//���������Ч������ִ��һ���Ƴ�����
					auto remove_it = std::remove_if(slots.begin(), slots.end(),
						[](const auto& slot_info) {
							return std::get<0>(slot_info).expired(); // ����������Ƿ�ʧЧ
						});
					slots.erase(remove_it, slots.end());
				}
			}
			else {/*û�ҵ�����źţ�Ҫ��Ҫassert��*/ }
		}

		auto connect(const char* signal_name, refl::CObject* slot_instance, const char* slot_member_func_name) {
			if (!slot_instance || !signal_name || !slot_member_func_name) {
				throw std::runtime_error("param is null!");
			}
			assert(slot_instance->weak_from_this().lock());//target����ͨ��make_share���죡����ΪҪ��������

			std::string str_signal_name(signal_name);
			auto itMap = connections_.find(str_signal_name);
			if (itMap != connections_.end()) {
				itMap->second.emplace_back(slot_instance->weak_from_this(), slot_member_func_name);//�������ĩβ����Ϊ������--end()������ָʾ�������
				return std::make_optional(std::make_tuple(this, itMap, --itMap->second.end()));
			}
			else {
				// ���û�ҵ���������Ԫ�ص�map�У�����ȡ������
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
			//T�Ǹ�������ͣ�std::make_optional(std::make_tuple(this, itMap, it)); ����T���ڸ��ӣ���ֱ����ģ����
			if (!connection) {
				return false;
			}
			auto& tuple = connection.value();
			if (std::get<0>(tuple) != this) {
				return false;//�����ҵ�connectionѽ
			}
			std::get<1>(tuple)->second.erase(std::get<2>(tuple));
			return true;
		}
	};

	// QObject��CObject�Ļ����ϣ��ṩ���ӹ�ϵ����̬���Ե�֧��
	class QObject : public CObject {
	private:
		std::string objectName_;
		std::weak_ptr<refl::IReflectable> parent_;
		std::unordered_map<std::string, std::any> properties_;
		std::list<std::shared_ptr<refl::IReflectable>> children_;
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
			auto ch = static_cast<refl::IReflectable*>(child);
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

}// namespace refl


namespace base {

	// CEventLoop�ṩ�¼�ѭ������֧��
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
			CEventLoop* eventLoop = nullptr;
			virtual ~IEventLoopHost() = default;
			virtual void onPostTask() = 0;
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
		CEventLoop(IEventLoopHost* host = nullptr) {
			this->host = host;
			host->eventLoop = this;
		}
		void post(Handler handler, Duration delay = Duration::zero()) {
			std::unique_lock<std::mutex> lock(mutex_);
			tasks_.push({ Clock::now() + delay, std::move(handler) });
			cond_.notify_one();
			if (host) {
				host->onPostTask();
			}
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
				if (!running_) {
					break;
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

}// namespace base


#ifdef _WIN32
#include <Windows.h>
class CWindowsEventLoopHost : public base::CEventLoop::IEventLoopHost {
	static constexpr UINT WM_WAKEUP = WM_USER + 15151515;
	UINT_PTR timerID_ = 0;

public:
	~CWindowsEventLoopHost() {
		if (timerID_) {
			KillTimer(NULL, timerID_);
		}
	}

	void onPostTask() override {
		PostThreadMessage(GetCurrentThreadId(), WM_WAKEUP, 0, 0);
	}

	void onWaitForTask(std::condition_variable& cond, std::unique_lock<std::mutex>& locker) override {
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (handleWindowsMessage(msg)) {
				break;
			}
			if (cond.wait_for(locker, std::chrono::milliseconds(0), [] { return true; })) {
				break;
			}
		}
	}

	void onEvent(base::CEventLoop::TimedHandler& event) override {
		event.handler();
	}

	void onWaitForRun(std::condition_variable& cond, std::unique_lock<std::mutex>& locker, const std::chrono::steady_clock::time_point& timePoint) override {
		auto now = std::chrono::steady_clock::now();
		if (now < timePoint) {
			auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint - now).count();
			timerID_ = SetTimer(NULL, 0, (UINT)delay_ms, NULL);//ͨ��timer�¼�����
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0)) {
				if (handleWindowsMessage(msg)) {
					break;
				}
			}
		}
	}

	bool handleWindowsMessage(MSG& msg) {
		if (msg.message == WM_QUIT) {
			if (this->eventLoop) {
				this->eventLoop->stop();//�˳���Ϣѭ��
			}
			return true;
		}
		if (msg.message == WM_TIMER && msg.wParam == timerID_) {
			KillTimer(NULL, timerID_);
			timerID_ = 0;
			return true; // Timer event occurred
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return false;
	}
};
#endif // _WIN32