import time

# 通过导出的ObjManager类，查找相应的命名共享对象
# p_foo对应std::shared_ptr<foo>对象
p_foo = ObjManager_foo.find("obj-foo-0")
# obj()函数对应从std::shared_ptr<foo>中得到对应的foo对象
f = p_foo.obj()
# 打印当前foo对象值
print(f"f={{.x={f.x}, .y={f.y}}}")
# 修改foo对象值
f.x, f.y = 12, 34

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