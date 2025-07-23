#include "StaticBinding.hpp"

class MyClass
{
public:
	static inline int AVar = 42;
	static inline void HelloWorld()
	{
		UE_LOG(LogTemp, Display, TEXT("Hello World"));
	}
};

void test()
{
	// => 在ts中生存cpp.d.ts声明 并且MyClass.HelloWorld可以直接调用
	AvidRegister::Instance()
	.AvidClass<MyClass>("MyClass")
	.StaticVar<&MyClass::AVar>("AVar")
	.StaticFunc<&MyClass::HelloWorld>("HelloWorld");
}