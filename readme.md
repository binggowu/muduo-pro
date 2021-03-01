
## muduo库学习

问题1: cc1plus: all warnings being treated as errors
解决: 删除 muduo/CMakeLists.txt 中的 -Werror

```
# 默认是debug模式,
./build.sh -j4
./build.sh install
```
