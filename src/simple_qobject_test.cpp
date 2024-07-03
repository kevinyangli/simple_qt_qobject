
#include "simple_qobject.h"

// �û��Զ���Ľṹ��
class MyStruct :
	//public refl::dynamic::IReflectable 	// �������Ҫ��̬���䣬���Բ���public refl::dynamic::IReflectable����
	public refl::QObject // ��������Ҳ�����źŲ۵ȹ��ܣ���˴����������
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
	// �����Ҫ֧�ֶ�̬���ã�����������std::any�����Ҳ��ܳ���4��������
	int print_with_arg(std::any param) const {
		std::cout << "MyStruct::print called! " << " arg is: " << std::any_cast<int>(param) << std::endl;
		return 888;
	}
	// ����һ�������������ۺ�����������REFLECTABLE_MENBER_FUNCS�б��У���֧�ַ���ֵ�����Ҳ���������std::any�����ܳ���4��������
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

	DECL_DYNAMIC_REFLECTABLE(MyStruct)//��̬�����֧�֣��������Ҫ��̬���䣬����ȥ�����д���
};

//��̬����ע���࣬ע�ᴴ������
REGEDIT_DYNAMIC_REFLECTABLE(MyStruct)


int main() {

	// ��̬���䲿�֣�
	std::cout << "---------------------��̬���䲿�֣�" << std::endl;

	auto obj = std::make_shared<MyStruct>();
	// ��ӡ�����ֶ�����
	refl::For<MyStruct>::for_each_propertie_name([](const char* name) {
		std::cout << "Field name: " << name << std::endl;
		});

	// ��ӡ�����ֶ�ֵ
	refl::For<MyStruct>::for_each_propertie_value(obj.get(), [](const char* name, auto&& value) {
		std::cout << "Field " << name << " has value: " << value << std::endl;
		});

	// ��ӡ���к�������
	refl::For<MyStruct>::for_each_member_func_name([](const char* name) {
		std::cout << "Member func name: " << name << std::endl;
		});

	// ��ȡ�ض���Ա��ֵ������Ҳ�����Ա���򷵻�Ĭ��ֵ
	auto x_value = refl::get_field_value(obj.get(), "x");
	std::cout << "Field x has value: " << std::any_cast<int>(x_value) << std::endl;

	auto y_value = refl::get_field_value(obj.get(), "y");
	std::cout << "Field y has value: " << std::any_cast<double>(y_value) << std::endl;

	//�޸�ֵ��
	refl::assign_field_value(obj.get(), "y", 33.33f);
	y_value = refl::get_field_value(obj.get(), "y");
	std::cout << "Field y has modifyed,new value is: " << std::any_cast<double>(y_value) << std::endl;

	auto z_value = refl::get_field_value(obj.get(), "z"); // "z" ������
	if (z_value.type().name() == std::string_view("int")) {
		std::cout << "Field z has value: " << std::any_cast<int>(z_value) << std::endl;
	}

	// ͨ���ַ������ó�Ա���� 'print'
	auto print_ret = refl::invoke_member_func_safe(obj.get(), "print");
	std::cout << "print member return: " << std::any_cast<int>(print_ret) << std::endl;


	std::cout << "---------------------��̬���䲿�֣�" << std::endl;

	// ��̬���䲿��(��̬������ȫ����Ҫ֪������MyStruct�Ķ���)��
	// ��̬���� MyStruct ʵ�������÷���
	auto instance = refl::internal::TypeRegistry::instance().create("MyStruct");
	if (instance) {
		std::cout << "Dynamic instance type: " << instance->get_type_name() << std::endl;
		// ������Ե��� MyStruct �ĳ�Ա����
		auto x_value2 = instance->get_field_value_by_name("x");
		std::cout << "Field x has value: " << std::any_cast<int>(x_value2) << std::endl;

		instance->invoke_member_func_by_name("print");
		instance->invoke_member_func_by_name("print_with_arg", 10);
		//instance->invoke_member_func_by_name("print_with_arg", 20, 222);//������û�ʧ�ܣ����ж��ԣ���Ϊprint_with_argֻ����һ������
	}

	// �źŲ۲��֣�
	std::cout << "---------------------�źŲ۲��֣�" << std::endl;

	auto obj1 = std::make_shared<MyStruct>();
	obj1->setObjectName("obj1");
	auto obj2 = std::make_shared<MyStruct>();
	obj2->setObjectName("obj2");
	obj2->setParent(obj1);

	// ����obj1���źŵ�obj2�Ĳۺ���
	auto connection_id = obj1->connect("x_value_modified", obj2.get(), "on_x_value_modified");
	if (!connection_id) {
		std::cout << "Signal x_value_modified from obj1 connected to on_x_value_modified slot in obj2." << std::endl;
	}

	obj1->x_value_modified(42);// �����ź�
	// �Ͽ�����
	obj1->disconnect(connection_id);
	// �ٴδ����źţ�Ӧ��û���κ��������Ϊ�Ѿ��Ͽ�����
	obj1->x_value_modified(84);

	// ʹ�ó�Ա����ָ��汾��connect
	connection_id = obj1->connect(&MyStruct::x_value_modified, obj2, &MyStruct::on_x_value_modified);
	if (!connection_id) {
		std::cout << "Signal connected to slot." << std::endl;
	}
	obj1->x_value_modified(666);// �����ź�

	obj2.reset();
	obj1.reset();


	// �¼�ѭ�����֣�
	std::cout << "---------------------�¼�ѭ�����֣�" << std::endl;
	CWindowsEventLoopHost host;
	base::CEventLoop loop(&host);

	// ����һ��ÿ�봥��һ�ε������Զ�ʱ��
	auto timerId = loop.startTimer([] {
		std::cout << "Periodic timer 500ms triggered." << std::endl;
		}, std::chrono::milliseconds(500));

	loop.post([]() {
		std::cout << "Immediate task\n";
		});//����ִ��

	loop.post([]() {
		std::cout << "Delayed task\n";
		}, std::chrono::seconds(1));//��ʱһ��

	loop.post([]() {
		std::cout << "Delayed task in 3 second\n";
		}, std::chrono::seconds(3));//��ʱ3��

	loop.post([&]() {
		std::cout << "timer stoped!\n";
		loop.stopTimer(timerId);
		}, std::chrono::seconds(4));//��ʱ4��

	loop.post([&]() {
		std::cout << "stop msg loop in 8 second\n";
		loop.stop();
		}, std::chrono::seconds(8));//��ʱ6��

	if (true) {
		loop.run();//���߳�ֱ��run
	}
	else {
		// ������һ�������߳�run:
		std::thread loop_thread([&loop]() {
			loop.run();
			});
		std::this_thread::sleep_for(std::chrono::seconds(10));
		loop.stop();//ֹͣ��Ϣѭ��
		loop_thread.join();
	}
	std::cout << "====end=====" << std::endl;
	return 0;
}