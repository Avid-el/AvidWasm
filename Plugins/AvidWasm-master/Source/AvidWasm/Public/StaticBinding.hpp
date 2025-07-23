#pragma once
#include <any>

class Any
{
	private:
	void* Data = nullptr;
};
class AvidUtils
{
	template <typename T>
	struct FunctionTraits
	{
		static_assert(std::is_function_v<T> || std::is_member_function_pointer_v<T> || std::is_function_v<std::remove_pointer_t<T>>, 
					 "T must be a function type");
	};

	template <typename Ret, typename... Args>
	struct FunctionTraits<Ret (*)(Args...)>
	{
		using ReturnType = Ret;
		using Arguments = std::tuple<Args...>;
	};

	template <typename C, typename Ret, typename... Args>
	struct FunctionTraits<Ret (C::*)(Args...)> : FunctionTraits<Ret (*)(Args...)>
	{
		using ClassType = C;
	};
};

class RegisterType
{
protected:
	FString TypeName;
};

class VariableType: public RegisterType
{
public:
	std::any Get();
	void Set(void* value);
};
class ClassType: public RegisterType
{
	public:
	struct ConstructorInfo
	{
		FString Name;
	};
	static TMap<FString, ConstructorInfo> Constructors;
	static TMap<FString, VariableType> StaticVariables;
};

template <typename C>
class ClassRegister
{
public:
	static ClassRegister<C>& Constructor();

	template<auto Ptr>
	ClassRegister<C>& StaticFunc(const FString& Name);

	template<auto Ptr>
	ClassRegister<C>& StaticVar(const FString& Name);
};

template <typename C>
template<auto Ptr>
ClassRegister<C>& ClassRegister<C>::StaticFunc(const FString& Name)
{
	// static_assert(AvidUtils::FunctionTraits<Ptr>::ReturnType, "Ptr must be a function pointer");
	ClassType::ConstructorInfo Constructor{
		.Name = Name
	};
	ClassType::Constructors.Add(Constructor.Name, Constructor);
	return *this;
}

template <typename C>
template<auto Ptr>
ClassRegister<C>& ClassRegister<C>::StaticVar(const FString& Name)
{
	// static_assert(AvidUtils::FunctionTraits<Ptr>::ReturnType, "Ptr must be a function pointer");
	VariableType Variable;
	ClassType::StaticVariables.Add(Name, Variable);
	return *this;
}

class AvidRegister
{
public:
	static inline AvidRegister& Instance()
	{
		static AvidRegister instance;
		return instance;
	}

	template <typename C>
	ClassRegister<C> AvidClass(const FString& Name)
	{
		ClassType::ConstructorInfo Constructor{
			.Name = Name
		};
		ClassType::Constructors.Add(Constructor.Name, Constructor);
		// ToDo
		return ClassRegister<C>();
	}
};

