/*
 * @author: Avidel
 * @LastEditors: Avidel
 */
@external("env", "hostLog")
declare function hostLog(ptr: u32, len: u32): void;

// 1. 先定义函数，并应用 @unmanaged 装饰器
export function add(a: i32, b: i32): i32 {
  return a + b;
}

export function logHello(): void {
  let message = "Hello from an UPDATED compiler! It works!";
  let messageBuffer = String.UTF8.encode(message);
  let ptr = changetype<i32>(messageBuffer);
  let len = messageBuffer.byteLength;
  hostLog(ptr, len);
}