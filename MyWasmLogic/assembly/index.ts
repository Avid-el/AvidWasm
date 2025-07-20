/*
 * @author: Avidel
 * @LastEditors: Avidel
 */
// assembly/index.ts

// 导入函数：从宿主环境（UE）导入一个名为 "env.ue_log" 的函数。
// "env" 是通常约定的模块名。
// 该函数接受一个 i32 类型的参数（代表一个指向内存的指针）。
// AssemblyScript 中的 string 会被处理为指针，我们需要在 C++ 端解码。
// 为了简化，我们先从简单的数字日志开始。
@external("env", "ue_log_int")
declare function ue_log_int(value: i32): void;

@external("env", "ue_log_string")
declare function ue_log_string(ptr: i32, len: i32): void;


// 一个内部辅助函数，用于调用宿主的日志函数
function log(message: string): void {
	// 将 AssemblyScript 的 string 编码为 UTF8
	let buffer = String.UTF8.encode(message);
	// 获取指向内存缓冲区的指针和长度
	let ptr = changetype<i32>(buffer);
	let len = buffer.byteLength;
	// 调用导入的宿主函数
	ue_log_string(ptr, len);
}

// 导出一个函数，供宿主环境（UE）调用。
// 它接收两个整数，返回它们的和，并通过宿主函数打印一条日志。
export function add(a: i32, b: i32): i32 {
	const result = a + b;
	// // 使用导入的函数在UE中打印日志
	// ue_log_int(result);
	//
	// // 也可以打印字符串
	// log("Hello from Wasm! The calculation is complete.");

	return result;
}