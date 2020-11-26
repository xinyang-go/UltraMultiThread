import time

# 通过导出的ObjManager类，查找相应的命名共享对象
f = ObjManager_foo.find("obj-foo-0")
# 打印当前foo对象值
print(f"f={{.x={f.x}, .y={f.y}}}")
# 修改foo对象值
f.x, f.y = 12, 34

# 遍历ObjManager_foo下的所有命名共享对象
for name in ObjManager_foo.names():
    f = ObjManager_foo.find(name)
    if f is not None:
        print(f"{name}: {{.x={f.x}, .y={f.y}}}")

# 通过导出的Sync类，得出对应的同步对象
sync = Sync_int("sync-0")
# 修改sync对象值为77，此时cpp中的sync_print函数对应的线程应该被唤醒
sync.set(77)
# 当前线程休眠5s
time.sleep(2)
# 修改sync对象值为0，此时cpp中的sync_print函数对应的线程应该进入休眠
sync.set(0)

# 通过导出的Publisher类，得出对应的发布器对象
pub = Publisher_foo("msg-foo-0")
# 发布一条消息，此时cpp中的subscribe函数对应的线程应该会收到这条消息，并打印消息内容
pub.push(foo(77, 88))
pub.push(foo(33, 44))
pub.push(foo(55, 66))
# 当前线程休眠1s
time.sleep(1)