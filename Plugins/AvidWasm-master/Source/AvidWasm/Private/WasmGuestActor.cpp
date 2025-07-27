#include "WasmGuestActor.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_WASM3
const void* NativeUnrealLog(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, void* _mem);

const void* wasmAbort(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, void* _mem)
{
	uint32_t messagePtr = (uint32_t) *(_sp + 0);
	uint32_t filename = (uint32_t) *(_sp + 1);  
	uint32_t line = (uint32_t) *(_sp + 2);
	uint32_t col = (uint32_t) *(_sp + 3);
    
	UE_LOG(LogTemp, Error, TEXT("WASM Abort called at line %d, col %d"), line, col);
	return m3Err_none;
}
// C-Style callback for Wasm3. extern "C" is crucial for correct linking.
extern "C" const void* Native_CallUEFunction(IM3Runtime runtime, M3ImportContext* _ctx, uint64* _sp, void* _userData)
{
	uint32_t messagePtr = *(_sp);
	uint32_t memorySize = 0;
	uint8_t* wasmMemory = m3_GetMemory(runtime, &memorySize, 0);

	if (wasmMemory && messagePtr < memorySize)
	{
		const char* messageFromWasm = (const char*)(wasmMemory + messagePtr);
		FString ueMessage(messageFromWasm);

		UE_LOG(LogTemp, Warning, TEXT("SUCCESS! Native_CallUEFunction was executed!"));
		UKismetSystemLibrary::PrintString(nullptr, ueMessage, true, true, FLinearColor::Red, 30.0f);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Native_CallUEFunction failed: could not read wasm memory."));
	}

	return m3Err_none;
}

#endif // WITH_WASM3

AWasmGuestActor::AWasmGuestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 【最佳实践】在构造函数中初始化所有指针成员为nullptr
#if WITH_WASM3
	M3Environment = nullptr;
	M3Runtime = nullptr;
	M3Module = nullptr;
	WasmStartFunction = nullptr;
#endif
}

void AWasmGuestActor::BeginPlay()
{
	Super::BeginPlay();
	InitializeWasm();
}

void AWasmGuestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
#if WITH_WASM3
	// 【关键修正】现在只释放Environment即可，它会管理所有相关资源，不会再崩�����
	if (M3Environment)
	{
		m3_FreeEnvironment(M3Environment);
		M3Environment = nullptr; // good practice to null the pointer after freeing
		UE_LOG(LogTemp, Log, TEXT("WasmGuestActor: Wasm3 Environment released."));
	}
#endif
}

void AWasmGuestActor::InitializeWasm()
{
#if WITH_WASMTIME
	UE_LOG(LogTemp, Warning, TEXT("Initializing Wasmtime..."));
    
	// 1. 创建一个Engine，这是Wasmtime的JIT编译器核心
	// 对于整个应用程序，通常只需要一个Engine
	Engine = wasm_engine_new();
	if (!Engine)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Wasmtime engine!"));
		return;
	}

	// 2. 创建一个Store，Wasm实例和对象都存在于Store中
	Store = wasmtime_store_new(Engine, nullptr, nullptr);
	if (!Store)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Wasmtime store!"));
		return;
	}
#endif
	
#if WITH_WASM3
    // ---- 1-3步骤保持不变 ----
    M3Environment = m3_NewEnvironment();
    if (!M3Environment) {
        UE_LOG(LogTemp, Error, TEXT("Wasm3: Failed to create environment."));
        return;
    }

    M3Runtime = m3_NewRuntime(M3Environment, 65536, nullptr);
    if (!M3Runtime) {
        UE_LOG(LogTemp, Error, TEXT("Wasm3: Failed to create runtime."));
        return;
    }

    TArray<uint8> WasmFileBytes;
    FString PathToWasm = FPaths::ProjectContentDir() + TEXT("release.wasm");
    if (!FFileHelper::LoadFileToArray(WasmFileBytes, *PathToWasm))
    {
        UE_LOG(LogTemp, Error, TEXT("Wasm3: Failed to load .wasm file from %s"), *PathToWasm);
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Wasm3: Loaded %d bytes from wasm file."), WasmFileBytes.Num());

    M3Result parseResult = m3_ParseModule(M3Environment, &M3Module, WasmFileBytes.GetData(), WasmFileBytes.Num());
    if (parseResult != m3Err_none)
    {
        UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_ParseModule failed with error: %s"), ANSI_TO_TCHAR(parseResult));
        return;
    }

    M3Result loadResult = m3_LoadModule(M3Runtime, M3Module);
    if (loadResult != m3Err_none)
    {
        UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_LoadModule failed with error: %s"), ANSI_TO_TCHAR(loadResult));
        return;
    }

    // ---- 尝试多种可能的导入函数链接 ----
    M3Result linkResult;
    bool linkSuccess = false;

    // 尝试1: hostLog
    linkResult = m3_LinkRawFunction(M3Module, "env", "hostLog", "v(ii)", &NativeUnrealLog);
    if (linkResult == m3Err_none) {
        UE_LOG(LogTemp, Log, TEXT("Successfully linked hostLog function"));
        linkSuccess = true;
    }

	// 链接AssemblyScript运行时需要的函数（这些可能是必需的）
	linkResult = m3_LinkRawFunction(M3Module, "env", "abort", "v(iiii)", &wasmAbort);
	if (linkResult == m3Err_none) {
		UE_LOG(LogTemp, Log, TEXT("Successfully linked abort function"));
	}
	
    if (!linkSuccess) {
        linkResult = m3_LinkRawFunction(M3Module, "env", "log", "v(ii)", &NativeUnrealLog);
        if (linkResult == m3Err_none) {
            UE_LOG(LogTemp, Log, TEXT("Successfully linked log function"));
            linkSuccess = true;
        }
    }

    // 尝试3: 尝试console.log风格
    if (!linkSuccess) {
        linkResult = m3_LinkRawFunction(M3Module, "env", "console.log", "v(i)", &NativeUnrealLog);
        if (linkResult == m3Err_none) {
            UE_LOG(LogTemp, Log, TEXT("Successfully linked console.log function"));
            linkSuccess = true;
        }
    }

    // 尝试4: 尝试Native_CallUEFunction
    if (!linkSuccess) {
        linkResult = m3_LinkRawFunction(M3Module, "env", "callUEFunction", "v(i)", &Native_CallUEFunction);
        if (linkResult == m3Err_none) {
            UE_LOG(LogTemp, Log, TEXT("Successfully linked callUEFunction function"));
            linkSuccess = true;
        }
    }

    // 如果所有尝试都失败，跳过链接继续（某些WASM文件可能不需要导入）
    if (!linkSuccess) {
        UE_LOG(LogTemp, Warning, TEXT("Warning: Failed to link any import functions, continuing anyway"));
    }

    // ---- 编译模块 ----
    M3Result compileResult = m3_CompileModule(M3Module);
    if (compileResult != m3Err_none)
    {
        UE_LOG(LogTemp, Error, TEXT("Wasm3: m3_CompileModule failed with error: %s"), ANSI_TO_TCHAR(compileResult));
        return;
    }

    // ---- 查找导出函数 ----
    M3Result findResult = m3_FindFunction(&WasmStartFunction, M3Runtime, "add");
    if (findResult != m3Err_none)
    {
        UE_LOG(LogTemp, Warning, TEXT("'add' 函数未找到，尝试查找 'logHello'"));
        findResult = m3_FindFunction(&WasmStartFunction, M3Runtime, "logHello");
        if (findResult != m3Err_none)
        {
            // 尝试其他可能的函数名
            findResult = m3_FindFunction(&WasmStartFunction, M3Runtime, "main");
            if (findResult != m3Err_none)
            {
                findResult = m3_FindFunction(&WasmStartFunction, M3Runtime, "_start");
                if (findResult != m3Err_none)
                {
                    UE_LOG(LogTemp, Error, TEXT("未找到任何可用的导出函数"));
                    return;
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("找到 '_start' 函数"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("找到 'main' ���数"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("找到 'logHello' 函数"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("找到 'add' 函数"));
    }

    UE_LOG(LogTemp, Log, TEXT("WasmGuestActor: 初始化成功，函数已缓存"));
#endif
}

void AWasmGuestActor::TriggerWasmLogic()
{
#if WITH_WASM3
	if (WasmStartFunction)
	{
		UE_LOG(LogTemp, Log, TEXT("UE -> Calling cached Wasm function 'startMyLogic'..."));
		M3Result result = m3_CallV(WasmStartFunction);
		if (result != m3Err_none)
		{
			UE_LOG(LogTemp, Error, TEXT("Wasm3: Error calling 'startMyLogic': %s"), ANSI_TO_TCHAR(result));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WasmGuestActor: Trigger failed, 'startMyLogic' function pointer is not valid. Initialization might have failed."));
	}
#endif
}

int32 AWasmGuestActor::CallWasmAdd(int32 a, int32 b)
{
#if WITH_WASM3
	if (!WasmStartFunction)
	{
		UE_LOG(LogTemp, Error, TEXT("WasmGuestActor: add function not available"));
		return 0;
	}

	// 调用 wasm 函数并传入参数
	M3Result result = m3_CallV(WasmStartFunction, a, b);
	if (result != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: Error calling 'add': %s"), ANSI_TO_TCHAR(result));
		return 0;
	}

	// 获取返回值
	uint64_t returnValue = 0;
	result = m3_GetResultsV(WasmStartFunction, &returnValue);
	if (result != m3Err_none)
	{
		UE_LOG(LogTemp, Error, TEXT("Wasm3: Error getting return value: %s"), ANSI_TO_TCHAR(result));
		return 0;
	}

	int32 wasmResult = static_cast<int32>(returnValue);
	UE_LOG(LogTemp, Log, TEXT("Wasm add(%d, %d) = %d"), a, b, wasmResult);
    
	return wasmResult;
#elif WITH_WASMTIME
	 if (!Store) return -1;
	wasmtime_context_t* Context = wasmtime_store_context(Store);
	const FString WasmFilePath = TEXT("release.wasm");
    // --- 1. 加载 .wasm 文件字节码 ---
    FString FullPath = FPaths::ProjectContentDir() + WasmFilePath;
    TArray<uint8> WasmFileContent;
    if (!FFileHelper::LoadFileToArray(WasmFileContent, *FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Wasm file: %s"), *FullPath);
        return -1;
    }
    UE_LOG(LogTemp, Log, TEXT("Loaded %d bytes from %s"), WasmFileContent.Num(), *FullPath);

    // --- 2. 编译模块 ---
    // 将UE的TArray转换为Wasmtime的wasm_byte_vec_t
    wasm_byte_vec_t WasmBytes;
    wasm_byte_vec_new(&WasmBytes, WasmFileContent.Num(), (const wasm_byte_t*)WasmFileContent.GetData());

	wasmtime_error_t* Error = nullptr;
	wasmtime_module_t* Module;
    Error = wasmtime_module_new(Engine, WasmFileContent.GetData(), WasmFileContent.Num(), &Module);
    wasm_byte_vec_delete(&WasmBytes); // 字节码已拷贝，可以释放
    
    if (Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to compile Wasm module."));
        return -1;
    }

    // --- 3. 实例化模块 ---
	wasmtime_instance_t Instance;
	wasm_trap_t* Trap = nullptr;
    Error = wasmtime_instance_new(Context, Module, nullptr, 0, &Instance, &Trap);
    if (Error || Trap)
    {
	    // 如果发生错误或陷阱，打印错误信息
    	if (Error)
    	{
    		UE_LOG(LogTemp, Error, TEXT("Failed to instantiate Wasm module"));
    		wasmtime_error_delete(Error);
    	}
    	if (Trap)
    	{
    		wasm_message_t TrapMessage;
    		wasm_trap_message(Trap, &TrapMessage);
    		UE_LOG(LogTemp, Error, TEXT("Wasm trap occurred: %s"), *FString(TrapMessage.size, TrapMessage.data));
    		wasm_trap_delete(Trap);
    	}
    }
	// --- 4. 查找导出的 "add" 函数 ---
	wasmtime_extern_t AddExtern;
	FString Name = TEXT("Add");
	int32 FuncLength = Name.Len();
	bool success = wasmtime_instance_export_get(Context, &Instance, TCHAR_TO_ANSI(*Name), FuncLength, &AddExtern);
	if (!success || AddExtern.kind != WASMTIME_EXTERN_FUNC)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get exported function"));
		return false;
	}
	wasmtime_val_t args[2];
	args[0].kind = WASMTIME_I32;
	args[0].of.i32 = a;
	args[1].kind = WASMTIME_I32;
	args[1].of.i32 = b;
	wasmtime_val_t results[1];
    
	Error = wasmtime_func_call(Context, &AddExtern.of.func, args, 2, results, 1, &Trap);
    
	if (Error || Trap)
	{
		if (Error)
		{
			wasm_name_t error_name;
			wasmtime_error_message(Error, &error_name);
			UE_LOG(LogTemp, Error, TEXT("Error calling function: %s"), 
				   UTF8_TO_TCHAR(error_name.data));
			wasm_name_delete(&error_name);
			wasmtime_error_delete(Error);
		}
		if (Trap)
		{
			UE_LOG(LogTemp, Error, TEXT("Wasm trap occurred"));
		}
		return -1;
	}
    
	int32 result = results[0].of.i32;
	UE_LOG(LogTemp, Log, TEXT("Wasm add(%d, %d) = %d"), a, b, result);
    
	return result;
#endif
}

// 1. 定义 C++ 函数，它将作为 Wasm 导入函数的实现。
// 这是 wasm3 中用于链接原始 C 函数的标准签名。
#if WITH_WASM3
const void* NativeUnrealLog(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, void* _mem)
{
    // 从 wasm3 的调用栈中获取参数。
    // 第一个参数是字符串指针 (offset)，第二个是长度 (length)。
    // 注意：这里的类型转换和指针算术需要非常小心。
    uint32_t offset = (uint32_t) *(_sp + 0);
    uint32_t length = (uint32_t) *(_sp + 1);

    // 获取 Wasm 模块的内存指针
    uint32_t mem_size = 0;
    uint8_t* wasm_memory = m3_GetMemory(runtime, &mem_size, 0);

    // 安全检查：确保要读取的内存范围在 Wasm 内存的边界内。
    if (offset + length > mem_size) {
        UE_LOG(LogTemp, Error, TEXT("WasmLog: Memory access out of bounds!"));
        return m3Err_trapOutOfBoundsMemoryAccess;
    }

    // 从 Wasm 内存中读取 UTF-8 字节并创建 FString。
    // FUTF8ToTCHAR 是一个很好的转换工具。
    FString message = FUTF8ToTCHAR(reinterpret_cast<const char*>(wasm_memory + offset), length).Get();

    // 使用 UE_LOG 打印日志！
    UE_LOG(LogTemp, Warning, TEXT("Log from Wasm: %s"), *message);

	return (const void*)m3Err_none;
}
#endif


// 在你的 Wasm 初始化和执行函数中
void AWasmGuestActor::TestWasm3CallUE()
{
#if WITH_WASM3
    // ... 原有的 wasm3 环境、运行时和模块加载代码 ...
    // IM3Environment env = m3_NewEnvironment();
    // IM3Runtime runtime = m3_NewRuntime(env, 1024, nullptr);
    // uint8_t* wasm_bytecode = ... // 加载 build/optimized.wasm 文件内容
    // IM3Module module = nullptr;
    // m3_ParseModule(env, &module, wasm_bytecode, wasm_size);
    // m3_LoadModule(runtime, module);

    // 3. 查找并调用 Wasm 的导出函数
    IM3Function f = nullptr;
    M3Result findResult = m3_FindFunction(&f, M3Runtime, "logHello");

    if (findResult != m3Err_none) {
        UE_LOG(LogTemp, Error, TEXT("Failed to find exported function 'logHello'."));
        // ... 清理并返回 ...
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("Calling Wasm function 'logHello'..."));

    // 调用函数
    M3Result callResult = m3_CallV(f); // logHello 没有参数，所以用 m3_CallV

    if (callResult != m3Err_none) {
        UE_LOG(LogTemp, Error, TEXT("Error calling 'logHello': %s"), *FString(callResult));
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("'logHello' executed successfully. Check logs for message from Wasm."));
    }

    // ... 清理 wasm3 环境 ...
#endif
}