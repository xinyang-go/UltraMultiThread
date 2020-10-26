# 将该文件和编译生成的libpy_example.so拷贝到同一个文件夹下
import libpy_example
import time
libpy_example.publisher()
libpy_example.subscriber()
libpy_example.subscriber()
libpy_example.subscriber()
time.sleep(2)