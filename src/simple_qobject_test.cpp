
#include "simple_qobject.h"

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
	auto instance = refl::internal::TypeRegistry::instance().create("MyStruct");
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
	CWindowsEventLoopHost host;
	base::CEventLoop loop(&host);

	// 启动一个每秒触发一次的周期性定时器
	auto timerId = loop.startTimer([] {
		std::cout << "Periodic timer 500ms triggered." << std::endl;
		}, std::chrono::milliseconds(500));

	loop.post([]() {
		std::cout << "Immediate task\n";
		});//马上执行

	loop.post([]() {
		std::cout << "Delayed task\n";
		}, std::chrono::seconds(1));//延时一秒

	loop.post([]() {
		std::cout << "Delayed task in 3 second\n";
		}, std::chrono::seconds(3));//延时3秒

	loop.post([&]() {
		std::cout << "timer stoped!\n";
		loop.stopTimer(timerId);
		}, std::chrono::seconds(4));//延时4秒

	loop.post([&]() {
		std::cout << "stop msg loop in 8 second\n";
		loop.stop();
		}, std::chrono::seconds(8));//延时6秒

	if (true) {
		loop.run();//主线程直接run
	}
	else {
		// 或者起一个其他线程run:
		std::thread loop_thread([&loop]() {
			loop.run();
			});
		std::this_thread::sleep_for(std::chrono::seconds(10));
		loop.stop();//停止消息循环
		loop_thread.join();
	}
	std::cout << "====end=====" << std::endl;
	return 0;
}